#include <gst/gst.h>
#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "configs.h"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

void show_usage(char *cmd);
void show_channels(struct configs* conf);
void show_version();

gboolean bus_call (GstBus     *bus, GstMessage *msg, gpointer    data);

GstElement* recdvb_append_src(struct configs* conf, GstElement* pipeline, GstElement* prev);
GstElement* recdvb_append_b25unscramble(struct configs* conf, GstElement* pipeline, GstElement* prev);
GstElement* recdvb_append_tsdemuxer(struct configs* conf, GstElement* pipeline, GstElement* prev);
GstElement* recdvb_append_tee(struct configs* conf, GstElement* pipeline, GstElement* prev);
GstElement* recdvb_append_queue(struct configs* conf, GstElement* pipeline, GstElement* prev);
GstElement* recdvb_append_queue2(struct configs* conf, GstElement* pipeline, GstElement* prev);
GstElement* recdvb_append_udpsink(struct configs* conf, GstElement* pipeline, GstElement* prev);
GstElement* recdvb_append_filesink(struct configs* conf, GstElement* pipeline, GstElement* prev);
gboolean    recdvb_rectime_elapsed(GstClock *clock, GstClockTime time, GstClockID id, gpointer user_data);

GST_DEBUG_CATEGORY_STATIC(gst_cat_recdvb);
#define GST_CAT_DEFAULT gst_cat_recdvb

int main(int argc, char *argv[])
{
  GMainLoop *loop = NULL;

  GstBus *bus = NULL;
  GstClock *clock =  NULL;
  GstElement *pipeline = NULL;
//  GstElement *dvbsrc = NULL;
//  GstElement *b25unscramble = NULL;
//  GstElement *filesink = NULL;
//  GstElement *udpsink = NULL;
  GstElement* elem = NULL;
  GstElement* elem2 = NULL;

  struct configs* conf = configs_create(argc, argv);
  if(!conf) {
    fprintf(stderr, "Parse config failed.\n");
    exit(1);
  }

  switch(conf->mode) {
    case SHOW_HELP:
      show_usage(argv[0]);
      exit(0);
      break;
    case SHOW_CHANNELS:
      show_channels(conf);
      exit(0);
      break;
    case SHOW_VERSION:
      show_version();
      exit(0);
      break;
    default:
      //go through.
      break;
  }

  if(conf->adapter < 0 && conf->input == NULL) {
    fprintf(stderr, "Not support automatically device select.\n");
    exit(1);
  }

  loop = g_main_loop_new (NULL, FALSE);
  if(!loop) {
    fprintf(stderr, "g_main_loop_new failed.\n");
    exit(1);
  }

  /* Initialisation */
  gst_init (&argc, &argv);
  GST_DEBUG_CATEGORY_INIT (gst_cat_recdvb, "recdvb", 0, "recdvb program");

  GST_DEBUG("launch recdvb.");

  /* Set up the pipeline */
  pipeline = gst_pipeline_new ("recdvb");

  elem = recdvb_append_src(conf, pipeline, NULL);
  elem = recdvb_append_b25unscramble(conf, pipeline, elem);
  //elem = recdvb_append_tsdemuxer(conf, pipeline, elem);
  elem = recdvb_append_tee(conf, pipeline, elem);

  elem2 = recdvb_append_queue(conf, pipeline, elem);
  elem2 = recdvb_append_udpsink(conf, pipeline, elem2);

  elem2 = recdvb_append_queue2(conf, pipeline, elem);
  elem2 = recdvb_append_filesink(conf, pipeline, elem2);

  /* we add a message handler */
  clock = gst_pipeline_get_clock(GST_PIPELINE(pipeline));
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, bus_call, loop);

  GST_INFO("rectime=%lld", conf->rectime);
  if(conf->rectime >= 0) {
    GstClockTime endtime = gst_clock_get_time(clock) + (conf->rectime * GST_SECOND);
    GstClockID clkid = gst_clock_new_single_shot_id(clock, endtime);

    GST_DEBUG("clkid = %p, endtime=%llu", clkid, endtime);
    if(!gst_clock_id_wait_async(clkid, recdvb_rectime_elapsed, loop, NULL) ) {
      //err
    }
  }

  /* Set the pipeline to "playing" state*/
  //g_print ("Now playing: %s\n", argv[1]);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Iterate */
  g_print ("Running...\n");
  g_main_loop_run (loop);

// --- EXIT MAINLOOP ---

  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));

  return 0;
}

void show_usage(char *cmd)
{
  fprintf(stderr, 
"Usage: %s [options] channel rectime destfile\n"
"Options:\n"
"  --b25:               Decrypt using BCAS card\n"
"    --round N:         Specify round number\n"
"    --strip:           Strip null stream\n"
"    --EMM:             Instruct EMM operation\n"
"  --udp:               Turn on udp broadcasting\n"
"    --addr hostname:   Hostname or address to connect\n"
"    --port portnumber: Port number to connect\n"
"  --device devicefile: Specify devicefile to use\n"
"  --lnb voltage:       Specify LNB voltage (0, 11, 15)\n"
"  --sid SID1,SID2,...: Specify SID number in CSV format (101,102,...)\n"
"  --help:              Show this help\n"
"  --version:           Show version\n"
"  --list:              Show channel list\n"
"\n"
"Remarks:\n"
"  if rectime  is '-', records indefinitely.\n"
"  if destfile is '-', stdout is used for output.\n", cmd);


}

void show_channels(struct configs* conf) {

  int chlen = configs_query_channels_num(conf);
  int i=0;
  for(i=0; i<chlen; i++) {
    struct channel_info ch = configs_query_channel_by_index(conf, i);
    if(ch.ch != 0) {
      if(i==0) {
        fprintf(stderr, "Available Channels:\n");
      }
      fprintf(stderr,
      "%3dch: (freq=%8ld) %s\n", ch.ch, ch.frequency, ch.name);
    }
  }
  if(i==0) {
    fprintf(stderr, "No channels available.\n");
  }
}

void show_version() {
  fprintf(stderr, "%s\n", VERSION);
}

gboolean bus_call (GstBus     *bus, GstMessage *msg, gpointer    data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      GST_INFO("End of stream\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

GstElement* recdvb_append_src(struct configs* conf, GstElement* pipeline, GstElement* prev)
{
  GstElement* dvbsrc = NULL;
  if(conf->adapter >= 0) {
    const char* polarity = "h";
    struct channel_info ch = configs_query_channel(conf, conf->channel);
    dvbsrc   = gst_element_factory_make ("dvbsrc",  "dvbsrc");

    if(conf->lnb_voltage > 15) {
      polarity = "v";
    }
    else {
      if(conf->lnb_voltage == 0) {
        fprintf(stderr, "GStreamer(dvbsrc) does not support LNB-OFF.\n");
        fprintf(stderr, "LNB=0 treated as LNB=11.\n");
      }
    }

    g_object_set (G_OBJECT (dvbsrc), "adapter", conf->adapter, NULL);
    g_object_set (G_OBJECT (dvbsrc), "frequency", ch.frequency, NULL);
    g_object_set (G_OBJECT (dvbsrc), "polarity", polarity, NULL);

    GST_DEBUG_OBJECT(dvbsrc, "adapter=%d", conf->adapter);
    GST_DEBUG_OBJECT(dvbsrc, "frequency=%ld", ch.frequency);
    GST_DEBUG_OBJECT(dvbsrc, "lnb_voltage=%d polarity=%s", conf->lnb_voltage, polarity);


  }
  else if(conf->input != NULL) {
    if( strncmp(conf->input, "-", 1) == 0) {
      dvbsrc   = gst_element_factory_make ("fdsrc",  "fdsrc");
      g_object_set (G_OBJECT (dvbsrc), "fd", 0, NULL); //from stdin
      GST_DEBUG_OBJECT(dvbsrc, "fd=0");
    }
    else {
      dvbsrc   = gst_element_factory_make ("filesrc",  "filesrc");
      g_object_set (G_OBJECT (dvbsrc), "location", conf->input, NULL);
      GST_DEBUG_OBJECT(dvbsrc, "location=%s", conf->input);
    }
  }
  else {
    return NULL;
  }

  gst_bin_add(GST_BIN (pipeline), dvbsrc);
  if(prev != NULL) gst_element_link(prev, dvbsrc);
  GST_INFO_OBJECT(prev, "link from %p", prev);
  GST_INFO_OBJECT(dvbsrc, "link to %p", dvbsrc);
  return dvbsrc;
}

static gchar* strsubst(const char* body, const char* target, const char* replace_with)
{
  gchar* dup = g_strdup(body);
  gchar* found = g_strrstr(dup, target);
  const gchar* replace_str = (replace_with ? replace_with : "");
  if(!found) return dup;

  gchar* tail = found + strlen(target);
  *found = '\0';
  gchar* concat = g_strconcat(dup, replace_str, tail, NULL);
  g_free(dup);

  return concat;
}

static gchar* subst_params(struct configs* conf, const char* value)
{
  gchar* subst_round = strsubst(value,       "<round>", conf->round);
  gchar* subst_strip = strsubst(subst_round, "<strip>", conf->strip);
  gchar* subst_emm =   strsubst(subst_strip, "<emm>",   conf->emm);
  g_strchomp(subst_emm);

  g_free(subst_round);
  g_free(subst_strip);

  return subst_emm;
}

GstElement* recdvb_append_b25unscramble(struct configs* conf, GstElement* pipeline, GstElement* prev)
{
  if(!conf->use_b25) {
    return prev;
  }
  char* b25_plugin_name = configs_query_b25plugin_name(conf);
  b25_plugin_name = (b25_plugin_name ? b25_plugin_name : "identity");
  GstElement* b25unscramble = gst_element_factory_make (b25_plugin_name, b25_plugin_name);

  gsize optlen = configs_query_b25plugin_options_length(conf);


  for(int i=0; i<optlen; i++) {
    gchar *key = NULL;
    gchar *value = NULL;
    gchar *subst = NULL;

    configs_query_b25plugin_option(conf, i, &key, &value);
    subst = subst_params(conf, value);

    if(strlen(subst) != 0) {
      GST_INFO_OBJECT(b25unscramble, "%s=%s", key, subst);
      gst_util_set_object_arg (G_OBJECT (b25unscramble), key, subst);
    }

    g_free(subst);
  }

  gst_bin_add(GST_BIN (pipeline), b25unscramble);
  if(prev != NULL) gst_element_link(prev, b25unscramble);
  GST_INFO_OBJECT(prev, "link from %p", prev);
  GST_INFO_OBJECT(b25unscramble, "link to %p", b25unscramble);
  return b25unscramble;
}

GstElement* recdvb_append_tsdemuxer(struct configs* conf, GstElement* pipeline, GstElement* prev)
{
  if(!conf->sid_list) {
    return prev;
  }
  GstElement *mpegtsdemux = gst_element_factory_make ("mpegts", "mpegtsdemux");
  g_object_set (G_OBJECT (mpegtsdemux), "es-pids", conf->host_to, NULL);
  gst_bin_add(GST_BIN (pipeline), mpegtsdemux);
  if(prev != NULL) gst_element_link(prev, mpegtsdemux);
  GST_INFO_OBJECT(prev, "link from %p", prev);
  GST_INFO_OBJECT(mpegtsdemux, "link to %p", mpegtsdemux);
  return mpegtsdemux;
}

GstElement* recdvb_append_tee(struct configs* conf, GstElement* pipeline, GstElement* prev)
{
  GstElement *tee = gst_element_factory_make ("tee", "tee");
  gst_bin_add(GST_BIN (pipeline), tee);
  if(prev != NULL) gst_element_link(prev, tee);
  GST_INFO_OBJECT(prev, "link from %p", prev);
  GST_INFO_OBJECT(tee, "link to %p", tee);
  return tee;
}

GstElement* recdvb_append_queue(struct configs* conf, GstElement* pipeline, GstElement* prev)
{
  GstElement *queue = gst_element_factory_make ("queue", "queue");
  gst_bin_add(GST_BIN (pipeline), queue);
  if(prev != NULL) gst_element_link(prev, queue);
  GST_INFO_OBJECT(prev, "link from %p", prev);
  GST_INFO_OBJECT(queue, "link to %p", queue);
  return queue;
}

GstElement* recdvb_append_queue2(struct configs* conf, GstElement* pipeline, GstElement* prev)
{
  GstElement *queue = gst_element_factory_make ("queue", "queue2");
  gst_bin_add(GST_BIN (pipeline), queue);
  if(prev != NULL) gst_element_link(prev, queue);
  GST_INFO_OBJECT(prev, "link from %p", prev);
  GST_INFO_OBJECT(queue, "link to %p", queue);
  return queue;
}

GstElement* recdvb_append_udpsink(struct configs* conf, GstElement* pipeline, GstElement* prev)
{
  if(!conf->use_udp) {
    return prev;
  }
  GstElement *udpsink = gst_element_factory_make ("udpsink", "udpsink");
  g_object_set (G_OBJECT (udpsink), "host", conf->host_to, NULL);
  g_object_set (G_OBJECT (udpsink), "port", conf->port_to, NULL);
  GST_DEBUG_OBJECT(udpsink, "host=%s", conf->host_to);
  GST_DEBUG_OBJECT(udpsink, "port=%d", conf->port_to);
  gst_bin_add(GST_BIN (pipeline), udpsink);
  if(prev != NULL) gst_element_link(prev, udpsink);
  GST_INFO_OBJECT(prev, "link from %p", prev);
  GST_INFO_OBJECT(udpsink, "link to %p", udpsink);
  return udpsink;
}

GstElement* recdvb_append_filesink(struct configs* conf, GstElement* pipeline, GstElement* prev)
{
  if(!conf->destfile) {
    return prev;
  }
  GstElement *filesink = gst_element_factory_make ("filesink", "filesink");
  g_object_set (G_OBJECT (filesink), "location", conf->destfile, NULL);
  GST_DEBUG_OBJECT(filesink, "location=%s", conf->destfile);
  gst_bin_add(GST_BIN (pipeline), filesink);
  if(prev != NULL) gst_element_link(prev, filesink);
  GST_INFO_OBJECT(prev, "link from %p", prev);
  GST_INFO_OBJECT(filesink, "link to %p", filesink);
  return filesink;
}

gboolean recdvb_rectime_elapsed(GstClock *clock, GstClockTime time, GstClockID id, gpointer user_data)
{
  GMainLoop *loop = (GMainLoop*)user_data;
  GST_DEBUG("callbacked %p %llu", id, time);
  g_main_loop_quit (loop);
  return TRUE;
}
