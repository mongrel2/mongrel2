#include "minunit.h"
#include <assert.h>
#include <mem/halloc.h>

#include <superpoll.h>
#include "zmq_compat.h"

SuperPoll *TEST_POLL = NULL;

/* Code for this unit test taken from:  pipetest.c
 *	Scalability test of async poll functionality on pipes.
 *	Copyright 502, 506 Red Hat, Inc.
 *	Portions Copyright 501 Davide Libenzi <davidel@xmailserver.org>.
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

void really_send_toke(struct token *toke)
{
	int *fds;
	int fd;
	int pipe_idx = nr_pipes - toke->thread - 1;

	fds = pipefds[pipe_idx].fds;
	fd = fds[WRITE];

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

int read_and_process_token(int fd)
{
	struct token *toke;
	struct fdinfo *inf = &fdinfo[fd];
	char *buf = inf->buf;
	int nr;
	nr = read(fd, buf, BUFSIZE);
    check(nr != -1, "Failed to read from fd that was supposedly ready: %d", fd);

	toke = (struct token *)buf;

	while (nr >= BUFSIZE) {
		process_token(fd, toke, BUFSIZE);
		toke++;
		nr -= BUFSIZE;
	}

	check(nr == 0, "Didn't write everythig to pipe.");

    return 0;

error:
    return -1;
}


void makeapipe(int idx, int hot)
{
	int *fds = pipefds[idx].fds;
	struct fdinfo *inf;
	int i;
    int rc = 0;

	check(pipe(fds) == 0, "Failed to make pipe #%d", idx);

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

    rc = SuperPoll_add(TEST_POLL, NULL, NULL, fds[READ], 'r', hot);
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

void makepipes(int nr, int hot)
{
	int i;
	for (i=0; i<nr; i++)
		makeapipe(i, hot);
	nr_pipes = nr;
}


void closepipes(int nr)
{
    int i = 0;
    for(i = 0; i < nr; i++) {
        free(fdinfo[pipefds[i].fds[READ]].buf);
        close(pipefds[i].fds[READ]);

        free(fdinfo[pipefds[i].fds[WRITE]].buf);
        close(pipefds[i].fds[WRITE]);
    }

    memset(fdinfo, 0, sizeof(fdinfo));
    memset(pipefds, 0, sizeof(pipefds));

    done = 0;
    nr_token_passes = 0;
}

int run_main_loop(int hot)
{
    int i = 0;
	int nfds = 0;
    int rc = 0;
    PollResult result;
    
    rc = PollResult_init(TEST_POLL, &result);
    check(rc == 0, "Failed to initialize the poll result.");

	while (!done) {
		send_pending_tokes();

		nfds = SuperPoll_poll(TEST_POLL, &result, -1);
        check(nfds >= 0, "superpoll failed.");

		for (i = 0; i < nfds; i++) {
			if (result.hits[i].ev.revents & ZMQ_POLLIN) {
                // don't add it back in if there was an error along the way
				if(read_and_process_token(result.hits[i].ev.fd) == 0) {
                    rc = SuperPoll_add(TEST_POLL, NULL, NULL, result.hits[i].ev.fd, 'r', hot);
                }
			}
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

char *run_test(int nr, int threads, int gens, int hot, char *fail_msg)
{
    debug("TEST IS: %s", fail_msg);

	struct timeval stv, etv;
	long long usecs, passes_per_sec;
    int rc = 0;
    FILE *perf = NULL;

    check(nr >= threads, "You can't have the nr less than threads.");

	max_threads = threads;
	max_generation = gens;

    perf = fopen("tests/perf.log", "a+");
    check(perf, "Failed to open tests/perf.log");
    pid_t mypid = getpid();


    fprintf(perf, "%s %d %d %d %ld %d ", hot ? "poll" : "epoll",
            mypid, nr, max_threads, max_generation, BUFSIZE);


	makepipes(nr, hot);

	send_pending_tokes();

	gettimeofday(&stv, NULL);

	seedthreads(max_threads);

    rc = run_main_loop(hot);
    check(rc == 0, "Looks like the main loop failed for '%s'", fail_msg);

	gettimeofday(&etv, NULL);

	etv.tv_sec -= stv.tv_sec;
	etv.tv_usec -= stv.tv_usec;

	if (etv.tv_usec < 0) {
		etv.tv_usec += 1000000;
		etv.tv_sec -= 1;
	}

    fprintf(perf, "%ld %ld.%06ld ", nr_token_passes, etv.tv_sec, (long int)etv.tv_usec);

	usecs = etv.tv_usec + etv.tv_sec * 1000000LL;
    if(usecs == 0) usecs++; // avoid divide-by-zero on some computers
	passes_per_sec = nr_token_passes * 1000000LL * 100;
	passes_per_sec /= usecs ;

    fprintf(perf, "%Ld.%02Ld\n", passes_per_sec / 100, passes_per_sec % 100);

    closepipes(nr);
    fclose(perf);

	return NULL;

error:
    closepipes(nr);
    if(perf) fclose(perf);
    return fail_msg;
}

char *test_maxed_pipes_hot()
{
    return run_test(50, 50, 50, 1, "max pipes 1 failed");
}


char *test_sparse_pipes_hot()
{
    return run_test(50, 5, 50, 1, "sparse pipes 1 failed");
}

char *test_midlevel_pipes_hot()
{
    return run_test(50, 25, 50, 1, "midlevel pipes failed.");
}

char *test_maxed_pipes_idle()
{
    return run_test(50, 50, 50, 0, "max idle pipes 1 failed");
}


char *test_sparse_pipes_idle()
{
    return run_test(50, 5, 50, 0, "sparse pipes 1 failed");
}

char *test_midlevel_pipes_idle()
{
    return run_test(50, 25, 50, 0, "midlevel pipes failed.");
}

char *test_totally_maxed()
{
    return run_test(400, 350, 10, 0, "midlevel pipes failed.");
}

char *all_tests() {
    mu_suite_start();

    SuperPoll_get_max_fd();
    TEST_POLL = SuperPoll_create();

    mu_run_test(test_sparse_pipes_hot);
    mu_run_test(test_maxed_pipes_hot);
    mu_run_test(test_midlevel_pipes_hot);

#ifdef __linux__
    mu_run_test(test_sparse_pipes_idle);
    mu_run_test(test_maxed_pipes_idle);
    mu_run_test(test_midlevel_pipes_idle);
    mu_run_test(test_totally_maxed);
#endif

    SuperPoll_destroy(TEST_POLL);

    return NULL;
}

RUN_TESTS(all_tests);
