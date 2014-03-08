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
  int adapter;
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

char*          configs_query_b25plugin_name(struct configs* conf);
gsize          configs_query_b25plugin_options_length(struct configs* conf);
gboolean       configs_query_b25plugin_option(struct configs* conf, int index, char** key, char** value);

int                 configs_query_channels_list(struct configs* conf, int* list);
int                 configs_query_channels_num(struct configs* conf);
struct channel_info configs_query_channel(struct configs* conf, int ch_num);
struct channel_info configs_query_channel_by_index(struct configs* conf, int index);

int util_find_adapter_num(const char* device, int chardev_remap);

#ifdef __cplusplus
}
#endif

#endif //CONFIGS_H__
