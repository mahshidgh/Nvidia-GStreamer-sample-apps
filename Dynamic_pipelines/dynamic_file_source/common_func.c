#include "common_func.h"

const gchar* print_pad_type(GstPad *pad) 
{
  GstCaps *caps;
  GstStructure *structure;
  const gchar *media_type;

  // Get the current capabilities of the pad
  caps = gst_pad_get_current_caps(pad);
  if (!caps) {
    // If the pad has no current capabilities, try getting its template capabilities
    caps = gst_pad_get_pad_template_caps(pad);
  }

  if (caps) {
    // Get the first structure from the capabilities
    structure = gst_caps_get_structure(caps, 0);

    // Get the media type from the structure
    media_type = gst_structure_get_name(structure);
    g_print("Pad type: %s\n", media_type);

    gst_caps_unref(caps);
  } else {
    g_print("No capabilities found for the pad.\n");
  }
  return media_type;
}


const gchar* gst_pad_link_return_to_string(GstPadLinkReturn ret) {
    switch (ret) {
        case GST_PAD_LINK_OK:
            return "GST_PAD_LINK_OK";
        case GST_PAD_LINK_WRONG_HIERARCHY:
            return "GST_PAD_LINK_WRONG_HIERARCHY";
        case GST_PAD_LINK_WAS_LINKED:
            return "GST_PAD_LINK_WAS_LINKED";
        case GST_PAD_LINK_WRONG_DIRECTION:
            return "GST_PAD_LINK_WRONG_DIRECTION";
        case GST_PAD_LINK_NOFORMAT:
            return "GST_PAD_LINK_NOFORMAT";
        case GST_PAD_LINK_NOSCHED:
            return "GST_PAD_LINK_NOSCHED";
        case GST_PAD_LINK_REFUSED:
            return "GST_PAD_LINK_REFUSED";
        default:
            return "UNKNOWN";
    }
}


void set_terminal_mode(void) {
    struct termios old_tio, new_tio;

    /* get the terminal settings for stdin */
    tcgetattr(STDIN_FILENO,&old_tio);

    /* we want to keep the old setting to restore them at the end */
    new_tio=old_tio;

    /* disable canonical mode (buffered i/o) and local echo */
    new_tio.c_lflag &=(~ICANON & ~ECHO);

    /* set the new settings immediately */
    tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);
}

void reset_terminal_mode(void) {
    struct termios old_tio;

    /* re-set old terminal settings */
    tcgetattr(STDIN_FILENO,&old_tio);
    tcsetattr(STDIN_FILENO,TCSANOW,&old_tio);
}


/*
gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
  BusCallData *bus_call_data = (BusCallData *)data;
  GMainLoop *loop = bus_call_data->loop;
  GstElement* uridecodebin = bus_call_data->uridecodebin;
  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS:
      g_print("End of stream\n");
      clock_t start_time, end_time;
      double elapsed_time;

      start_time = clock(); // Start the timer
      GstElement *pipeline = GST_ELEMENT(GST_MESSAGE_SRC(msg));
      GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_READY);
      if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to pause the pipeline. Exiting.\n");
        return -1;
      }
      g_object_set(G_OBJECT(uridecodebin), "uri", "file:///opt/nvidia/deepstream/deepstream-6.2/sources/DeepStream-Yolo/videos/construction_video.mp4", NULL);
      ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
      if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to pause the pipeline. Exiting.\n");
        return -1;
      }
      end_time = clock(); // Stop the timer
      elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
      g_print("Elapsed time: %.6f seconds\n", elapsed_time);
      //g_main_loop_quit(loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar *debug;
      GError *error;

      gst_message_parse_error(msg, &error, &debug);
      g_free(debug);

      g_printerr("Error: %s\n", error->message);
      g_error_free(error);

      g_main_loop_quit(loop);
      break;
    }

    default:
      break;
  }

  return TRUE;
}
*/

// gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
//   GMainLoop *loop = (GMainLoop *)data;
  
//   switch (GST_MESSAGE_TYPE(msg)) {
//     case GST_MESSAGE_EOS:
//       g_print("End of stream\n");
//       //g_main_loop_quit(loop);
//       //break;

//     case GST_MESSAGE_ERROR: {
//       gchar *debug;
//       GError *error;

//       gst_message_parse_error(msg, &error, &debug);
//       g_free(debug);

//       g_printerr("Error: %s\n", error->message);
//       g_error_free(error);

//       g_main_loop_quit(loop);
//       break;
//     }

//     default:
//       break;
//   }

//   return TRUE;
// }


