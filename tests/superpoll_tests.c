#include <assert.h>
#define NDEBUG 1

#include "minunit.h"
#include <adt/heap.h>
#include <mem/halloc.h>

#include <superpoll.h>
#include <zmq.h>

FILE *LOG_FILE = NULL;
SuperPoll *TEST_POLL = NULL;

/* Code for this unit test taken from:  pipetest.c
 *	Scalability test of async poll functionality on pipes.
 *	Copyright 2002, 2006 Red Hat, Inc.
 *	Portions Copyright 2001 Davide Libenzi <davidel@xmailserver.org>.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <dbg.h>

int done = 0;

#define FD_SLOP	10
#define MAX_FDS 64*1024	

#define READ	0
#define WRITE	1

struct {
	int fds[2];
} pipefds[MAX_FDS/2];

int nr_pipes;

long	nr_token_passes;
long	max_generation;
int	max_threads = 1;
int	threads_complete = 0;

struct token {
	long	generation;
	int	tofd;
	int	thread;
};

int BUFSIZE = sizeof(struct token);

struct fdinfo {
	int		zero;
	//struct iocb	iocb2;
	int		one;
	int		fd;
	int		pipe_idx;
	int		active:1;
	int		rw:1;
	char		*buf;
	//char		buf2[BUFSIZE];
} fdinfo[MAX_FDS];

struct token pending_tokens[MAX_FDS];
int nr_pending_tokens;

void pexit(char *msg)
{
	perror(msg);
	exit(1);
}

void really_send_toke(struct token *toke)
{
	struct fdinfo *inf;
	int *fds;
	int fd;
	int pipe_idx = nr_pipes - toke->thread - 1;

	fds = pipefds[pipe_idx].fds;
	fd = fds[WRITE];
	inf = &fdinfo[fd];

	debug("sending on pipe index %u, fd %d", pipe_idx, fd);

    int res = write(fds[WRITE], toke, BUFSIZE);
    debug("write[%ld %d %d]", toke->generation,
        toke->tofd, toke->thread);
    if (res != BUFSIZE) {
        printf("write = %d (%s)", res, strerror(errno));
        exit(1);
    }
}

void send_pending_tokes(void)
{
	int i;
	for (i=0; i<nr_pending_tokens; i++)
		really_send_toke(&pending_tokens[i]);
	nr_pending_tokens = 0;
}

void send_toke(struct token *toke)
{
	unsigned pipe_idx;
	int *fds;

	pipe_idx = nr_pipes - toke->thread - 1;

	fds = pipefds[pipe_idx].fds;

	toke->tofd = fds[READ];
	really_send_toke(toke);
}

void process_token(int fd, struct token *toke, int nr)
{
	if (nr != BUFSIZE)
		fprintf(stderr, "process_token: nr == %d (vs %d)\n",
			nr, BUFSIZE);
	assert(nr == BUFSIZE);
	assert(toke->tofd == fd);
	nr_token_passes++;
	toke->generation++;

	debug("passed %ld, gen: %ld, token_gen: %ld", nr_token_passes, max_generation,
            toke->generation);

	if (toke->generation < max_generation)
		send_toke(toke);
	else {
		debug("thread %d complete, %ld passes", toke->thread, nr_token_passes);
		threads_complete++;
		if (threads_complete >= max_threads)
			done = 1;
	}
}

void read_and_process_token(int fd)
{
	struct token *toke;
	struct fdinfo *inf = &fdinfo[fd];
	char *buf = inf->buf;
	int nr;
	nr = read(fd, buf, BUFSIZE);
	if (-1 == nr)
		pexit("read");

	toke = (struct token *)buf;

	while (nr >= BUFSIZE) {
		process_token(fd, toke, BUFSIZE);
		toke++;
		nr -= BUFSIZE;
	}
	assert(nr == 0);
}


void makeapipe(int idx)
{
	int *fds = pipefds[idx].fds;
	struct fdinfo *inf;
	int i;
    int rc = 0;

	if (pipe(fds))
		pexit("pipe");

	for (i=0; i<2; i++) {
		int fl = fcntl(fds[i], F_GETFL);
		fl |= O_NONBLOCK;
		fcntl(fds[i], F_SETFL, fl);
	}

	inf = &fdinfo[fds[READ]];
	inf->buf = calloc(1, BUFSIZE);
	assert(inf->buf != NULL);
	assert(inf->active == 0);
	inf->active = 1;
	inf->rw = READ;
	inf->fd = fds[READ];
	inf->pipe_idx = idx;

    rc = SuperPoll_add(TEST_POLL, NULL, NULL, fds[READ], 'r', 1);
    check(rc != -1, "Failed to add read side to superpoll.");

	inf = &fdinfo[fds[WRITE]];
	assert(inf->active == 0);
	inf->active = 1;
	inf->rw = WRITE;
	inf->fd = fds[WRITE];
	inf->pipe_idx = idx;

    return;

error:
    return;
}

void makepipes(int nr)
{
	int i;
	for (i=0; i<nr; i++)
		makeapipe(i);
	nr_pipes = nr;
}


int run_main_loop(void)
{
    int i = 0;
	int res = 0;
    PollResult result;
    
    res = PollResult_init(TEST_POLL, &result);
    check(res == 0, "Failed to initialize the poll result.");

	while (!done) {
		send_pending_tokes();

		res = SuperPoll_poll(TEST_POLL, &result, -1);

		if (res <= 0)
			pexit("poll");

		for (i = 0; i < res; i++) {
			if (result.hits[i].ev.revents & ZMQ_POLLIN) {
				read_and_process_token(result.hits[i].ev.fd);

			}
		}

        for(i = 0; i < nr_pipes; i++) {
            res = SuperPoll_add(TEST_POLL, NULL, NULL, pipefds[i].fds[READ], 'r', 1);
            check(res != -1, "Failed to add read side to superpoll.");
        }
	}

    PollResult_clean(&result);
    return 0;

error:
    PollResult_clean(&result);
    return -1;
}

void seedthreads(int nr)
{
	struct token toke;
	int i;

	for (i=0; i<nr; i++) {
		toke.generation = 0;
		toke.thread = i;
		toke.tofd = -1;
		send_toke(&toke);
	}
}

char *run_test(int nr, int threads, int gens, char *fail_msg)
{
	struct timeval stv, etv;
	long long usecs, passes_per_sec;
    int rc = 0;

    check(nr >= threads, "You can't have the nr less than threads.");

	max_threads = threads;
	max_generation = gens;

    printf("using %d pipe pairs, %d message threads, %ld generations, %d bufsize\n",
           nr, max_threads, max_generation, BUFSIZE);

    TEST_POLL = SuperPoll_create();

	makepipes(nr);

	send_pending_tokes();

	gettimeofday(&stv, NULL);

	seedthreads(max_threads);

    rc = run_main_loop();
    mu_assert(rc == 0, "Main loop failed.");

	gettimeofday(&etv, NULL);

	etv.tv_sec -= stv.tv_sec;
	etv.tv_usec -= stv.tv_usec;

	if (etv.tv_usec < 0) {
		etv.tv_usec += 1000000;
		etv.tv_sec -= 1;
	}

    printf("%ld passes in %ld.%06ld seconds\n", nr_token_passes,
           etv.tv_sec, etv.tv_usec);

	usecs = etv.tv_usec + etv.tv_sec * 1000000LL;
	passes_per_sec = nr_token_passes * 1000000LL * 100;
	passes_per_sec /= usecs;

    printf("passes_per_sec: %Ld.%02Ld\n",
           passes_per_sec / 100,
           passes_per_sec % 100);

    SuperPoll_destroy(TEST_POLL);


	return NULL;

error:
    return fail_msg;
}

char *test_maxed_pipes()
{
    return run_test(50, 50, 5, "max pipes 1 failed");
    return run_test(1, 1, 5, "max pipes 2 failed");
}


char *test_sparse_pipes()
{
    return run_test(10, 1, 5, "sparse pipes 1 failed");
    return run_test(50, 5, 5, "sparse pipes 2 failed");
}

char *test_midlevel_pipes()
{
    return run_test(10, 6, 5, "midlevel pipes failed.");
}

char *all_tests() {
    mu_suite_start();
    mu_run_test(test_maxed_pipes);
    mu_run_test(test_sparse_pipes);
    mu_run_test(test_midlevel_pipes);
    return NULL;
}

RUN_TESTS(all_tests);

