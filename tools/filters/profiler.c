#include <filter.h>
#include <dbg.h>
#include <adt/hash.h>

#include <sys/time.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

typedef struct Settings {
  bstring host;
  bstring ip;
  int port;
  bstring prefix;
} Settings;

Settings *settings = NULL;

hash_t* sessions_hash = NULL;

int profsess_cmp(const void *ps0, const void *ps1);
hash_val_t profsess_hash(const void *conn);
bstring get_string_setting(tns_value_t *config, const char* key);
long get_long_setting(tns_value_t *config, const char* key);

bstring get_address(const char* server);

StateEvent filter_transition(StateEvent state, Connection *conn, tns_value_t *config)
{
  check(settings != NULL, "Settings not loaded");

  struct timeval *currentTime = malloc(sizeof(struct timeval));
  check_mem(currentTime);

  int rc = gettimeofday(currentTime, NULL);
  check(rc == 0, "Failed to get time of day");

  debug("Host: %s", bdata(settings->host));
  debug("Port: %ld", settings->port);

  switch(state) {
      case REQ_RECV: {
          int rc = hash_alloc_insert(sessions_hash, conn, currentTime);
          check(rc == 1, "Failed to insert timestamp into hash.");
          break;
      }
      case RESP_SENT: {
          hnode_t *startTimeNode = hash_lookup(sessions_hash, conn);
          struct timeval *startTime = startTimeNode->hash_data;
          
          long int startTimeInt = startTime->tv_sec * 1000 + startTime->tv_usec / 1000;
          long int endTimeInt = (currentTime->tv_sec * 1000 + currentTime->tv_usec / 1000);
          long int diffInt = endTimeInt - startTimeInt;
          debug("Request took %ldms", diffInt);

          free(startTime);
          hash_delete_free(sessions_hash, startTimeNode);
          break;
      }
      default :
          sentinel("Unknown state received by filter.");
          break;
  }
  return state;
  
error:
  return CLOSE;
}


StateEvent *filter_init(Server *srv, bstring load_path, int *out_nstates, tns_value_t *config)
{
  debug("Loading settings");

  bstring host = get_string_setting(config, "host");
  int port = (int)get_long_setting(config, "port");

  settings = (Settings *)malloc(sizeof(Settings));
  settings->host = bstrcpy(host);
  settings->port = port;

  bdestroy(host);

  StateEvent states[] = {REQ_RECV, RESP_SENT};
  *out_nstates = Filter_states_length(states);
  check(*out_nstates == 2, "Wrong states array length.");
  debug("[Profiler:filter_init] initted filter with needed states:");

  sessions_hash = hash_create(MAX_HASH_COUNT, profsess_cmp, profsess_hash);
  check_mem(sessions_hash);
  debug("[Profiler:filter_init] initted sessions hash");

  return Filter_state_list(states, *out_nstates);

error:
  return NULL;
}

bstring get_string_setting(tns_value_t *config, const char* key)
{ 
  check(tns_get_type(config) == tns_tag_dict, "Invalid config type: %c", tns_get_type(config)); 
  hash_t *settings = config->value.dict;

  bstring name = bfromcstr(key);
  hnode_t *hostNode = hash_lookup(settings, name);
  bdestroy(name);

  check(hostNode != NULL, "Missing setting %s in call.", name);

  check(tns_get_type(hostNode->hash_data) == tns_tag_string, "Invalid host config value"); 
  tns_value_t *setting = (tns_value_t *)hostNode->hash_data;
  bstring val = (bstring)setting->value.string;

  return val;

error:
  return NULL;
}

long get_long_setting(tns_value_t *config, const char* key)
{
  check(tns_get_type(config) == tns_tag_dict, "Invalid config type: %c", tns_get_type(config)); 
  hash_t *settings = config->value.dict;

  bstring name = bfromcstr(key);
  hnode_t *hostNode = hash_lookup(settings, name);
  bdestroy(name);

  check(hostNode != NULL, "Missing setting %s in call.", name);

  check(tns_get_type(hostNode->hash_data) == tns_tag_number, "Invalid host config value"); 
  tns_value_t *setting = (tns_value_t *)hostNode->hash_data;
  long val = (long)setting->value.number;

  return val;

error:
  return (long)NULL;
}

hash_val_t profsess_hash(const void *data)
{
  hash_val_t ret = (long)data;
  return ret;
}

int profsess_cmp(const void *ps0, const void *ps1)
{
  return memcmp(ps0, ps1, sizeof(Connection *));
}
