#include <gst/gst.h>

static void on_pad_added(GstElement *element, GstPad* pad, gpointer user_data)
{
  GstPad *sinkpad;
  GstElement *decoder = (GstElement *)user_data;

  sinkpad = gst_element_get_static_pad(decoder, "sink");
  gst_pad_link(pad, sinkpad);
  gst_object_unref(sinkpad);
}

int main(int argc, char *argv[])
{
  GstElement *pipeline;
  GstElement *source;
  GstElement *demuxer;
  GstElement *parser;
  GstElement *decoder;
  GstElement *converter;
  GstElement *sink;
  GstBus *bus;
  GstMessage *message;
  GstStateChangeReturn ret;

  gst_init(&argc, &argv);

  source = gst_element_factory_make("rtmpsrc", NULL);
  demuxer = gst_element_factory_make("flvdemux", NULL);
  parser = gst_element_factory_make("h264parse", NULL);
  decoder = gst_element_factory_make("avdec_h264", NULL);
  converter = gst_element_factory_make("videoconvert", NULL);
  sink = gst_element_factory_make("autovideosink", NULL);

  pipeline = gst_pipeline_new("pipeline");

  if (!pipeline || !source || !demuxer || !parser || !decoder || !sink)
  {
    g_printerr("Not all elements could be created.\n");

    return -1;
  }

  gst_bin_add_many(GST_BIN(pipeline), source, demuxer, parser, decoder, converter, sink, NULL);

  if (!gst_element_link(source, demuxer))
  {
    g_printerr("Elements could not be linked\n");
    gst_object_unref(pipeline);

    return -1;
  }

  if (!gst_element_link_many(parser, decoder, converter, sink, NULL))
  {
    g_printerr("Element could not be linked\n");
    gst_object_unref(pipeline);

    return -1;
  }

  g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), parser);

  g_object_set(source, "location", "rtmp://localhost:1935/stream/video", NULL);

  ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);

  if (ret == GST_STATE_CHANGE_FAILURE)
  {
    g_printerr("Unable to set pipeline to the playing state\n");
    gst_object_unref(pipeline);

    return -1;
  }

  bus = gst_element_get_bus(pipeline);
  message = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GstMessageType(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

  if (message != NULL)
  {
    GError * error;
    gchar *debug_info;

    switch (GST_MESSAGE_TYPE(message))
    {
      case GST_MESSAGE_ERROR:
        gst_message_parse_error(message, &error, &debug_info);
        g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(message->src), error->message);
        g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
        g_clear_error(&error);
        g_free(debug_info);
        break;
      case GST_MESSAGE_EOS:
        g_print("End of stream");
        break;
      default:
        g_printerr("Unknown message received\n");
        break;
    }
    gst_message_unref(message);
  }

  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return 0;
}
