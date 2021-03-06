#ifndef CONFIGS_H__
#define CONFIGS_H__

#ifdef __cplusplus
extern "C" {
#endif

struct GKeyFile;

struct channel_info {
  char* name;
  int ch;
  long frequency;
  int streamid;
};

enum mode {
  CMD_EXECUTE,
  SHOW_HELP,
  SHOW_CHANNELS,
  SHOW_VERSION
};

struct configs {
  GKeyFile* key_file; 
//  char *device;
  int adapter;// = -1;
  int use_b25;
   char* round;// = 4;
   char* strip;
   char* emm;
  int use_udp;
   const char *host_to;
   int port_to;// = 1234;
  int lnb_voltage;
  char* sid_list;

  char *input;

  char* destfile;
  int channel;
  long long rectime;
  enum mode mode;
};

struct configs* configs_create(int argc, char** argv);

char*          configs_query_plugin_name(struct configs* conf, const char* plugin);
gsize          configs_query_plugin_options_length(struct configs* conf, const char* group);
gboolean       configs_query_plugin_option(struct configs* conf, int index, char** key, char** value, const char* group);

char*          configs_query_b25plugin_name(struct configs* conf);
gsize          configs_query_b25plugin_options_length(struct configs* conf);
gboolean       configs_query_b25plugin_option(struct configs* conf, int index, char** key, char** value);

char*          configs_query_sidplugin_name(struct configs* conf);
gsize          configs_query_sidplugin_options_length(struct configs* conf);
gboolean       configs_query_sidplugin_option(struct configs* conf, int index, char** key, char** value);

int                 configs_query_channels_list(struct configs* conf, int* list);
int                 configs_query_channels_num(struct configs* conf);
struct channel_info configs_query_channel(struct configs* conf, int ch_num);
struct channel_info configs_query_channel_by_index(struct configs* conf, int index);

int config_query_mapped_device(struct configs* conf, const char* device);

#ifdef __cplusplus
}
#endif

#endif //CONFIGS_H__
