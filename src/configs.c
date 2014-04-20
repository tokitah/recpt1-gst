#include <glib.h>
#include <glib/gprintf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "configs.h"

#define SYSCONFFILE SYSCONFDIR "/" CONFFILENAME

#define DVB_DEV_PREFIX "/dev/dvb/adapter"
#define PT1_DEV_PREFIX "/dev/pt1video"
#define PT1_DEV_PREFIX_T "video"
#define DVB_DEV_PREFIX_LEN 16
#define PT1_DEV_PREFIX_LEN 13
#define PT1_DEV_PREFIX_LEN_H 7
#define PT1_DEV_PREFIX_LEN_T 5

struct configs* configs_create(int argc, char** argv)
{
  GError* gerr = NULL;
  int result;
  int option_index;
  struct option long_options[] = {
    { "b25",       0, NULL, 'b'},
    { "B25",       0, NULL, 'b'},
    { "round",     1, NULL, 'r'},
    { "strip",     0, NULL, 's'},
    { "emm",       0, NULL, 'm'},
    { "EMM",       0, NULL, 'm'},
    { "LNB",       1, NULL, 'n'},
    { "lnb",       1, NULL, 'n'},
    { "udp",       0, NULL, 'u'},
    { "addr",      1, NULL, 'a'},
    { "port",      1, NULL, 'p'},
    { "device",    1, NULL, 'd'},
    { "help",      0, NULL, 'h'},
    { "version",   0, NULL, 'v'},
    { "list",      0, NULL, 'l'},
    { "sid",       1, NULL, 'i'},
    { "input",     1, NULL, 'I'},
    {0, 0, NULL, 0} /* terminate */
  };

  char userconffile[FILENAME_MAX] = {0};

  struct configs* conf = g_malloc0(sizeof(struct configs) );
  // set defaults
  conf->port_to = 1234;

  conf->key_file = g_key_file_new();

  g_sprintf(userconffile, "%s/.%s", getenv("HOME"), CONFFILENAME);
  if( !g_key_file_load_from_file(conf->key_file, userconffile, 0, &gerr) ) {
    gerr = NULL;
    if( !g_key_file_load_from_file(conf->key_file, SYSCONFFILE, 0, &gerr) ) {
      fprintf(stderr, "System-wide config file " SYSCONFFILE " not found\n");
    }
  }

  while((result = getopt_long(argc, argv, "br:smn:ua:p:d:hvli:I:",
                                long_options, &option_index)) != -1) {
    switch(result) {
      case 'b':
        conf->use_b25 = TRUE;
        fprintf(stderr, "using B25...\n");
        break;
      case 's':
        conf->strip = "true";
        fprintf(stderr, "enable B25 strip\n");
        break;
      case 'm':
        conf->emm = "true";
        fprintf(stderr, "enable B25 emm processing\n");
        break;
      case 'u':
        conf->use_udp = TRUE;
        conf->host_to = "localhost";
        fprintf(stderr, "enable UDP broadcasting\n");
        break;
      case 'h':
        fprintf(stderr, "\n");
        conf->mode = SHOW_HELP;
        break;
      case 'v':
        conf->mode = SHOW_VERSION;
        break;
      case 'l':
        conf->mode = SHOW_CHANNELS;
        break;
      /* following options require argument */
      case 'n':
        conf->lnb_voltage = atoi(optarg);
        fprintf(stderr, "LNB = %d\n", conf->lnb_voltage);
        break;
      case 'r':
        conf->round = optarg;
        fprintf(stderr, "set round %s\n", conf->round);
        break;
      case 'a':
        conf->use_udp = TRUE;
        conf->host_to = optarg;
        fprintf(stderr, "UDP destination address: %s\n", conf->host_to);
        break;
      case 'p':
        conf->port_to = atoi(optarg);
        fprintf(stderr, "UDP port: %d\n", conf->port_to);
        break;
      case 'd':
        conf->adapter = util_find_adapter_num(optarg, (strncmp(argv[0], "recpt1", 6) == 0) );
	if(conf->adapter < 0) {
          fprintf(stderr, "device not found: %s\n", optarg);
        }
        else {
          fprintf(stderr, "using device: /dev/dvb/adapter%d\n", conf->adapter);
        }
        break;
      case 'i':
        conf->sid_list = optarg;
        fprintf(stderr, "using splitter: %s\n", conf->sid_list);
        break;
      case 'I':
        conf->input = optarg;
        fprintf(stderr, "using input: %s\n", conf->input);
        break;
    }
  }

  gboolean fileless = FALSE;

  if(argc - optind < 3) {
    if(argc - optind == 2 && conf->use_udp) {
      fprintf(stderr, "Fileless UDP broadcasting\n");
      fileless = TRUE;
    }
    else if(conf->mode == CMD_EXECUTE) {
      conf->mode = SHOW_HELP;
    }
  }

  if(conf->mode == CMD_EXECUTE) {
    char* endptr;
    conf->channel = g_ascii_strtoll(argv[optind], &endptr, 10);

    if(endptr != (argv[optind] + strlen(argv[optind])) ) {
      fprintf(stderr, "Invalid channel %s\n", argv[optind]);
      g_key_file_free(conf->key_file);
      g_free(conf);
      return NULL;
    }
    else {
      struct channel_info ch = configs_query_channel(conf, conf->channel);
      if(ch.ch <= 0) {
        fprintf(stderr, "Ch%d not found.\n", conf->channel);
        g_key_file_free(conf->key_file);
        g_free(conf);
        return NULL;
      }
    }

    if(g_strcmp0(argv[optind+1], "-") == 0) {
      conf->rectime = -1;
    }
    else {
      conf->rectime = g_ascii_strtoll(argv[optind + 1], &endptr, 10);
      if(endptr != (argv[optind+1] + strlen(argv[optind+1])) ) {
        fprintf(stderr, "Invalid rectime %s\n", argv[optind+1]);
        g_key_file_free(conf->key_file);
        g_free(conf);
        return NULL;
      }
    }

    if(!fileless) { 
      conf->destfile = argv[optind + 2];
    }
    else {
      conf->destfile = NULL;
    }
  }

  return conf;
}

char* configs_query_plugin_name(struct configs* conf, const char* plugin)
{
  GError* gerr = NULL;
  char* value = g_key_file_get_string(conf->key_file, "general", plugin, &gerr);

  return value;
}

gsize configs_query_plugin_options_length(struct configs* conf, const char* group)
{
  GError* gerr = NULL;
  gsize grplength;
  gchar** keys = g_key_file_get_keys(conf->key_file, group, &grplength, &gerr);

  g_strfreev(keys);

  return grplength;
}

gboolean configs_query_plugin_option(struct configs* conf, int index, char** key, char** value, const char* group)
{
  GError* gerr = NULL;
  gsize grplength;
  gchar** keys = g_key_file_get_keys(conf->key_file, group, &grplength, &gerr);

  if(!keys) {
    return FALSE;
  }
   
  if( gerr && (gerr->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND ||
               gerr->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND) ) {
    return FALSE;
  }

  *value = g_key_file_get_string(conf->key_file, group, keys[index], &gerr);
  if(gerr && (gerr->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND ||
              gerr->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND) ) {
    return FALSE;
  }
  *key = keys[index];
  return TRUE;
}

char* configs_query_b25plugin_name(struct configs* conf)
{
  return configs_query_plugin_name(conf, "b25");
}

gsize configs_query_b25plugin_options_length(struct configs* conf)
{
  return configs_query_plugin_options_length(conf, "b25");
}

gboolean configs_query_b25plugin_option(struct configs* conf, int index, char** key, char** value)
{
  return configs_query_plugin_option(conf, index, key, value, "b25");
}


char* configs_query_sidplugin_name(struct configs* conf)
{
  return configs_query_plugin_name(conf, "sid");
}

gsize configs_query_sidplugin_options_length(struct configs* conf)
{
  return configs_query_plugin_options_length(conf, "sid");
}

gboolean configs_query_sidplugin_option(struct configs* conf, int index, char** key, char** value)
{
  return configs_query_plugin_option(conf, index, key, value, "sid");
}

int configs_query_channels_num(struct configs* conf)
{
  return configs_query_channels_list(conf, NULL);
}

int configs_query_channels_list(struct configs* conf, int* list)
{
  gchar** groups = g_key_file_get_groups(conf->key_file, NULL);
  if(!groups) {
    return -1;
  }

  int count = 0;
  for(int i=0; groups[i] != NULL; i++) {
    if(strncmp(groups[i], "ch", 2) == 0) {
      if(list != NULL) {
        list[count] = g_ascii_strtoll(groups[i] + 2, NULL, 10);
      }
      count++;
    }
  }
  return count;
}

static struct channel_info configs_query_channel_impl(struct configs* conf, int ch_num, int index)
{
  GError* gerr = NULL;
  char chstr[255];
  struct channel_info ch = {0};
  int ch_idx = 0;
  gchar** groups = g_key_file_get_groups(conf->key_file, NULL);
  if(!groups) {
    return ch;
  }

  g_snprintf(chstr, 255, "ch%d", ch_num);

  for(int i=0; groups[i] != NULL; i++) {
    if(strncmp(groups[i], "ch", 2) == 0) {
      if(g_strcmp0(groups[i], chstr) == 0 || ch_idx == index) {
        int freq = g_key_file_get_integer(conf->key_file, groups[i], "frequency", &gerr);
        if(gerr) {
          fprintf(stderr, "Error: %s: Not defined frequency.\n", groups[i]);
          goto end;
      	}

        int sid = g_key_file_get_integer(conf->key_file, groups[i], "streamid", &gerr);
        if(gerr && gerr->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
          //ignore key missing
          fprintf(stderr, "err %s: %d.\n", gerr->message, gerr->code);
          goto end;
      	}

        char* name = g_key_file_get_string(conf->key_file, groups[i], "name", &gerr);
        if(gerr && gerr->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
          //ignore key missing
          fprintf(stderr, "err %s: %d.\n", gerr->message, gerr->code);
          goto end;
      	}

        ch.ch = atoi(groups[i]+2);
        ch.frequency = freq;
        ch.streamid = sid;
        ch.name = name;
        break;
      }
      ch_idx++;
    }
  }

end:
  g_strfreev(groups);
  return ch;
}

struct channel_info configs_query_channel_by_index(struct configs* conf, int index)
{
  return configs_query_channel_impl(conf, -1, index);
}
struct channel_info configs_query_channel(struct configs* conf, int ch_num)
{
  return configs_query_channel_impl(conf, ch_num, -1);
}

int util_find_adapter_num(const char* device, int chardev_remap) {

  size_t devlen = strlen(device);
  if( devlen == (DVB_DEV_PREFIX_LEN+1) && 
      strncmp(device, DVB_DEV_PREFIX, DVB_DEV_PREFIX_LEN) == 0) {
    char adapter_num = (device[DVB_DEV_PREFIX_LEN] - '0');
    if(0 <= adapter_num && adapter_num <= 9) {
      return adapter_num;
    }
  }
  else if( chardev_remap &&
           devlen == (PT1_DEV_PREFIX_LEN+1) &&
           (strncmp(device, PT1_DEV_PREFIX, PT1_DEV_PREFIX_LEN_H) == 0) &&
            strncmp(device+(PT1_DEV_PREFIX_LEN_H+1), PT1_DEV_PREFIX_T, PT1_DEV_PREFIX_LEN_T) == 0) {
    int mapped = -1;
    char adapter_num = (device[PT1_DEV_PREFIX_LEN] - '0');

    // swap 1-2.
    switch(adapter_num) {
      case 0:
        mapped = 0;
        break;
      case 1:
        mapped = 2;
        break;
      case 2:
        mapped = 1;
        break;
      case 3:
        mapped = 3;
        break;
    }
    if(mapped != -1) {
      fprintf(stderr, "%s mapped to /dev/dvb/adapter%d\n", device, mapped);
      return mapped;
    }
  }
  return -1;
}

