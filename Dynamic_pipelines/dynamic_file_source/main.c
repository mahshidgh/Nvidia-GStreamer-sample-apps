#include <gst/gst.h>
//#include <gst/uridecodebin/gsturidecodebin.h>
//#include <gst/nv/nvstreammux.h>
//#include <gst/nvinfer/gstnvinfer.h>
//#include <gst/video/videooverlay.h>
#include <glib.h>
#include <stdio.h>
//#include "gstnvdsmeta.h"
#include <cuda_runtime_api.h>
#include <nvds_yml_parser.h>
//#include <iostream>
#include <time.h>
#include"common_func.h"
#include <stdlib.h>


#define FRAME_WIDTH 1920
#define FRAME_HEIGHT 1080
// GstElement* uridecodebin = gst_element_factory_make("uridecodebin", "uridecodebin");
// GstElement* pipeline = gst_pipeline_new("my-pipeline");
#define MAX_NUM_SOURCES 2
#define GPU_ID 0
#define SET_GPU_ID(object, gpu_id) g_object_set (G_OBJECT (object), "gpu-id", gpu_id, NULL);
gint frame_counter = 0; // Counter for frames
gint chunk_counter = 0;
guint source_id = 0;
gboolean is_EOS = 0;
guint frameRate = 0;
gulong probe_id = 0;
clock_t start_time = 0;
clock_t end_time = 0;
gchar* SOURCE_NAME = "file://videos/%d.mp4";
GMutex eos_lock;


///////// #### Global Elements Initialization #### ///////////////////
//GstElement* uridecodebin = NULL;
GstElement* streammux = NULL;
GstElement *g_source_bin_list[2];

// GstElement* source_convert1 = NULL;
// GstElement *videorate = NULL;
// GstElement *capsfilter = NULL;
// GstElement *source_convert = NULL;
// Create nvinfer element
GstElement *nvinfer  = NULL;
GstElement *fakesink  = NULL;
GstElement* pipeline = NULL;

static void
decodebin_child_added(GstChildProxy *child_proxy, GObject *object,
                      gchar *name, gpointer user_data)
{
    g_print("decodebin child added %s\n", name);
    if (g_strrstr(name, "decodebin") == name)
    {
        g_signal_connect(G_OBJECT(object), "child-added",
                         G_CALLBACK(decodebin_child_added), user_data);
    }
    int current_device = -1;
    cudaGetDevice(&current_device);
    struct cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, current_device);

    if (g_strrstr(name, "nvv4l2decoder") == name)
    {
        if (prop.integrated)
        {
            g_print("[decode child added] gpu type is integrated\n");
            g_object_set(object, "enable-max-performance", TRUE, NULL);
            g_object_set(object, "bufapi-version", TRUE, NULL);
            g_object_set(object, "drop-frame-interval", frameRate, NULL);
            g_object_set(object, "num-extra-surfaces", 0, NULL);
        }
        else
        {
            g_print("[decode child added] gpu type is non-integrated\n");
            g_object_set(object, "gpu-id", GPU_ID, NULL);
            g_object_set(object, "drop-frame-interval", frameRate, NULL);
        }
    }
}

static GstPadProbeReturn
eos_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    if (GST_EVENT_TYPE(GST_PAD_PROBE_INFO_EVENT(info)) == GST_EVENT_EOS)
    {
        // Handle EOS event here
        g_print("End of stream\n");
        is_EOS = 1;
        //GstElement *pipeline = GST_ELEMENT(GST_MESSAGE_SRC(msg));
        //stop_release_source(source_id);
        double elapsed_time;
        end_time = clock();
        elapsed_time = ((double)(end_time - start_time) / CLOCKS_PER_SEC);
        printf("Elapsed time: %.5f seconds\n", elapsed_time);

        printf("Enter 0 to move to next video ...\n");
        // ****** Log the inference latency *****
        char filename[256];
        snprintf(filename, sizeof(filename), "inf_latency/latency.txt");

        /* Open the file for writing */
        FILE *latency_log_file = fopen(filename, "a");
        fprintf(latency_log_file,
                "%f\n",
                elapsed_time);
        fclose(latency_log_file);
        //system("./gt_is_done 0");
        return GST_PAD_PROBE_REMOVE;
    }
    // else
    // {
    //     g_print("eos callback called but no eos\n");
    // }

    // Continue propagation of the event:
    return GST_PAD_PROBE_OK;
}

static void source_pad_added(GstElement *element, GstPad *pad, gpointer data)
{
    g_print("######################pad added ....\n");
    const gchar *str = print_pad_type(pad);
    static guint pad_index = 0; // Choose the pad index based on your use case
    if (strstr(str, "audio") != NULL)
    {
        g_print("pad type is audio returing ... \n");
        goto exit;
    }
    GstElement *nvvideoconvert = (GstElement *)data;
    // GstElement *nvvideoconvert = elements_data->nvvideoconvert;
    // GstElement *nvstreammux = elements_data->nvstreammux;
    //GstElement *parent_bin = GST_ELEMENT_CAST(gst_element_get_parent(element));
    GstPad *sinkpad = gst_element_get_static_pad(nvvideoconvert, "sink");
    
    GstPadLinkReturn link_ret = gst_pad_link(pad, sinkpad);
    if (link_ret != GST_PAD_LINK_OK)
    {
        g_printerr("GstPadLinkReturn: %s\n", gst_pad_link_return_to_string(link_ret));
    }
    else
    {
        g_print("Link succeed.\n");
    }
    gst_object_unref(sinkpad);

    gchar pad_name[16] = { 0 };
    GstPad *sinkpad1 = NULL;
    g_snprintf (pad_name, 15, "sink_%u", source_id);
    sinkpad1 = gst_element_get_request_pad(streammux, pad_name);
    GstPad *srcpad = gst_element_get_static_pad(nvvideoconvert, "src");
    probe_id = gst_pad_add_probe(sinkpad1, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, eos_probe_callback, NULL, NULL);
    GstPadLinkReturn ret;
    /// Inspecting the parent //////
    GstObject *parent = gst_element_get_parent(streammux);
    if (parent)
    {
        g_print("Parent of the streammux: %s\n", GST_OBJECT_NAME(parent));
        gst_object_unref(parent); // Don't forget to unref
    }
    else
    {
        g_print("Element has no parent.\n");
    }
    parent = gst_element_get_parent(nvvideoconvert);
    if (parent)
    {
        g_print("Parent of the sourceConvert: %s %s\n", GST_OBJECT_NAME(parent), GST_OBJECT_NAME(gst_element_get_parent(parent)));
        gst_object_unref(parent); // Don't forget to unref
    }
    else
    {
        g_print("Element has no parent.\n");
    }
    ////// Create ghost pad////////////
    //////
    //GstPad *srcpad = gst_element_get_static_pad(nvvideoconvert, "src");
    GstPad *bin_ghost_pad = gst_ghost_pad_new("src", srcpad);
    gst_pad_set_active(bin_ghost_pad, TRUE);
    gst_element_add_pad(GST_ELEMENT(gst_element_get_parent(nvvideoconvert)), bin_ghost_pad);
    gst_object_unref(srcpad);

    //////////////////////

    ret = gst_pad_link(bin_ghost_pad, sinkpad1);
    if (ret != GST_PAD_LINK_OK)
    {
        g_printerr("Failed to link pads: %s\n", gst_pad_link_return_to_string(ret));
    }
    else
    {
        g_print("Pads linked successfully.\n");
    }
    
    //gst_bin_add_many(GST_BIN(pipeline), parent_bin, NULL);
    gst_object_unref(sinkpad1);
    //gst_object_unref(srcpad);
    //gst_object_unref(parent_bin);

exit:
    g_print("Exiting adding pad ... \n");
}


static GstPadProbeReturn nvinfer_src_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
    GstBuffer *buf = (GstBuffer *)info->data;
    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);
    for (NvDsMetaList *l_frame = batch_meta->frame_meta_list; l_frame != NULL; l_frame = l_frame->next)
    {
        NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(l_frame->data);

        /* Increment the frame counter and generate a filename */
        frame_counter++;
        char filename[256];        
        snprintf(filename, sizeof(filename), "detection_output/tmp/bbox_%d.txt", frame_counter);
        
        /* Open the file for writing */
        FILE *bbox_params_dump_file = fopen(filename, "w");
        if (bbox_params_dump_file == NULL) {
            g_print("Could not open file for writing\n");
            continue; // Skip this frame if the file can't be opened
        }

        for (NvDsMetaList *l_obj = frame_meta->obj_meta_list; l_obj != NULL; l_obj = l_obj->next)
        {
            NvDsObjectMeta *obj_meta = (NvDsObjectMeta *)(l_obj->data);
            float left = obj_meta->rect_params.left;
            float top = obj_meta->rect_params.top;
            float right = left + obj_meta->rect_params.width;
            float bottom = top + obj_meta->rect_params.height;
            // Here confidence stores detection confidence, since dump gie output
            // is before tracker plugin
            float confidence = obj_meta->confidence;
            fprintf(bbox_params_dump_file,
                    "%s %f %f %f %f %f\n",
                    obj_meta->obj_label, left, top, right, bottom, confidence);
        }

        /* Close the file */
        fclose(bbox_params_dump_file);
    }

    return GST_PAD_PROBE_OK;
}

/*
static GstPadProbeReturn 
nvinfer_src_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
    GstBuffer *buf = (GstBuffer *)info->data;
    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);

    for (NvDsMetaList *l_frame = batch_meta->frame_meta_list; l_frame != NULL; l_frame = l_frame->next)
    {
        NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(l_frame->data);
        for (NvDsMetaList *l_obj = frame_meta->obj_meta_list; l_obj != NULL; l_obj = l_obj->next)
        {
            NvDsObjectMeta *obj_meta = (NvDsObjectMeta *)(l_obj->data);

            // Get the bounding box coordinates
            float left = obj_meta->rect_params.left;
            float top = obj_meta->rect_params.top;
            float right = left + obj_meta->rect_params.width;
            float bottom = top + obj_meta->rect_params.height;
            // Process the bounding box information, e.g., print it
            g_print("Bounding Box: left=%.2f, top=%.2f, width=%.2f, height=%.2f\n", left, top, right, bottom);
        }
    }

    return GST_PAD_PROBE_OK;
}
*/

static void
stop_release_source(gint source_id)
{
    g_mutex_lock (&eos_lock);
    GstStateChangeReturn state_return;
    gchar pad_name[16];
    GstPad *sinkpad = NULL;
    g_print("removing source %d \n", source_id);
    state_return =
        gst_element_set_state(g_source_bin_list[source_id], GST_STATE_NULL);
    switch (state_return)
    {
    case GST_STATE_CHANGE_SUCCESS:
        g_print("STATE CHANGE SUCCESS\n\n");
        g_snprintf(pad_name, 15, "sink_%u", source_id);
        sinkpad = gst_element_get_static_pad(streammux, pad_name);
        gst_pad_send_event(sinkpad, gst_event_new_eos());
        gst_pad_send_event(sinkpad, gst_event_new_flush_stop(FALSE));
        gst_element_release_request_pad(streammux, sinkpad);
        g_print("STATE CHANGE SUCCESS %p\n\n", sinkpad);
        gst_object_unref(sinkpad);
        gst_bin_remove(GST_BIN(pipeline), g_source_bin_list[source_id]);
        break;
    case GST_STATE_CHANGE_FAILURE:
        g_print("STATE CHANGE FAILURE\n\n");
        break;
    case GST_STATE_CHANGE_ASYNC:
        g_print("STATE CHANGE ASYNC\n\n");
        g_snprintf(pad_name, 15, "sink_%u", source_id);
        sinkpad = gst_element_get_static_pad(streammux, pad_name);
        gst_pad_send_event(sinkpad, gst_event_new_eos());
        gst_pad_send_event(sinkpad, gst_event_new_flush_stop(FALSE));
        gst_element_release_request_pad(streammux, sinkpad);
        g_print("STATE CHANGE ASYNC %p\n\n", sinkpad);
        gst_object_unref(sinkpad);
        gst_bin_remove(GST_BIN(pipeline), g_source_bin_list[source_id]);
        break;
    case GST_STATE_CHANGE_NO_PREROLL:
        g_print("STATE CHANGE NO PREROLL\n\n");
        break;
    default:
        break;
    }
    g_mutex_unlock (&eos_lock);
}
static GstElement *
create_uridecode_bin(guint index)
{
    GstElement *bin = NULL;
    gchar bin_name[31] = {};
    gchar source_name[31] = {};
    gchar sourceConv_name[31] = {};
    gchar uribin_name[100] = {};
    //gchar capsfilter_name[31] = {};

    g_snprintf(uribin_name, 100, SOURCE_NAME, index);
    g_print("creating uridecodebin for [%s]\n", uribin_name);
    g_snprintf(bin_name, 30, "source-bin-%d", index);
    g_snprintf(source_name, 30, "source-%d", index);
    g_snprintf(sourceConv_name, 30, "sourceConverter-%d", index);
    bin = gst_bin_new(bin_name);
    if (!bin)
    {
        g_printerr("Failed to create bin.\n");
        return NULL;
    }
    GstElement *uridecodebin = gst_element_factory_make("uridecodebin", source_name);
    GstElement *source_convert1 = gst_element_factory_make("nvvideoconvert", sourceConv_name);
    if (!uridecodebin || !source_convert1 )
    {
        g_printerr("Failed to create uridecodebin or source_vonvert.\n");
        return NULL;
    }
    g_object_set(G_OBJECT(uridecodebin), "uri", uribin_name, NULL);
    g_signal_connect(G_OBJECT(uridecodebin), "pad-added", G_CALLBACK(source_pad_added), source_convert1);
    gst_bin_add_many(GST_BIN(bin), uridecodebin, source_convert1, NULL);
    g_print ("source name %s %s %s\n", bin_name, source_name, sourceConv_name);
    g_signal_connect (G_OBJECT (uridecodebin), "child-added", G_CALLBACK (decodebin_child_added), &index);
   
    return bin;
}

static gboolean
bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_EOS:
        g_print("End of stream bus_call\n");
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

gboolean stdin_cb(GIOChannel *channel, GIOCondition condition, gpointer data)
{
    gchar *string;
    gsize size;
    GstElement *pipeline = (GstElement*) data;
    GstStateChangeReturn state_return;

    if (condition & G_IO_HUP)
        return FALSE;  // Error or EOF.

    g_io_channel_read_line(channel, &string, &size, NULL, NULL);
    
    // ... Process input (string) here ...
    char *endptr;
    long number = strtol(string, &endptr, 10);
    if (endptr != string) {
        // Conversion successful if the input was not empty
        printf("Received number: %ld\n", number);
    }
    else {
        // Handle invalid input
        printf("Invalid input: %s\n", string);
    }
    frameRate = (guint) number;

    g_free(string);
    // Continue the pipeline
    system("rm -r  detection_output/tmp/*");
    stop_release_source (source_id);
    source_id++;
    source_id = source_id % MAX_NUM_SOURCES;
    GstElement* source_bin = create_uridecode_bin (source_id);
    if (!source_bin)
    {
        g_printerr("Failed to create source bin. Exiting.\n");
        return -1;
    }
    g_source_bin_list[source_id] = source_bin;
    gst_bin_add(GST_BIN(pipeline), source_bin);
    state_return =
        gst_element_set_state(g_source_bin_list[source_id], GST_STATE_PLAYING);
    switch (state_return)
    {
    case GST_STATE_CHANGE_SUCCESS:
        g_print("STATE CHANGE SUCCESS\n\n");
        break;
    case GST_STATE_CHANGE_FAILURE:
        g_print("STATE CHANGE FAILURE\n\n");
        break;
    case GST_STATE_CHANGE_ASYNC:
        g_print("STATE CHANGE ASYNC\n\n");
        state_return =
            gst_element_get_state(g_source_bin_list[source_id], NULL, NULL,
                                  GST_CLOCK_TIME_NONE);
        break;
    case GST_STATE_CHANGE_NO_PREROLL:
        g_print("STATE CHANGE NO PREROLL\n\n");
        break;
    default:
        break;
    }
    //g_source_bin_list[source_id] = create_uridecode_bin(source_id);
    //g_object_set(G_OBJECT(uridecodebin), "uri", SOURCE_NAME, NULL);
    //system("rm -r DS_bbox_golden/tmp/*");
    start_time = clock();
    return TRUE; // Keep the event source.
}


int main(int argc, char* argv[]) {

   // set_terminal_mode();
    // Initialize GStreamer
    gst_init(&argc, &argv);
    GstElement* source_bin;

    // Create uridecodebin element
    pipeline = gst_pipeline_new("my-pipeline");
    source_bin = create_uridecode_bin(source_id);
    g_print("[Main] source created \n");
    g_source_bin_list[source_id] = source_bin;
    gst_bin_add (GST_BIN (pipeline), source_bin);
    g_mutex_init (&eos_lock);
    streammux = gst_element_factory_make("nvstreammux", "stream-mux");
    // Create nvinfer element
    nvinfer = gst_element_factory_make("nvinfer", "nvinfer");
    fakesink = gst_element_factory_make("fakesink", "fakesink");
    // Check if element creation was successful
    if (!streammux || !nvinfer || !fakesink) {
        g_printerr("One or more elements could not be created. Exiting.\n");
        return -1;
    }

    // Set the properties of the elements
    if (g_str_has_suffix (argv[1], ".yml") || g_str_has_suffix (argv[1], ".yaml")) 
    {
    //setting parameters

       // nvds_parse_uridecodebin(uridecodebin, argv[1],"uridecodebin");
        nvds_parse_streammux(streammux, argv[1],"streammux");
        nvds_parse_gie(nvinfer, argv[1],"nvinfer");
        
    }

    // Set up the pipeline
    g_object_set(G_OBJECT(streammux), "batched-push-timeout", 25000, NULL);
    //g_object_set(G_OBJECT(streammux), "batch-size", 30, NULL);
    g_object_set(G_OBJECT(streammux), "drop-pipeline-eos", 1, NULL);
    SET_GPU_ID(streammux, GPU_ID);
    gst_bin_add_many(GST_BIN(pipeline), streammux, nvinfer, fakesink, NULL);

    if (!gst_element_link_many(streammux, nvinfer, fakesink, NULL))
    {
        g_printerr("Elements could not be linked. Exiting.\n");
        return -1;
    }

    // Register a probe on the source pad of the nvinfer element to retrieve the bounding box information
    GstPad *nvinfer_src_pad = gst_element_get_static_pad(nvinfer, "src");
    gst_pad_add_probe(nvinfer_src_pad, GST_PAD_PROBE_TYPE_BUFFER, nvinfer_src_pad_buffer_probe, NULL, NULL);
    gst_object_unref(nvinfer_src_pad);

    /* Create a GMainLoop */
    //Method 1
    // g_print("$$$$$$$$$$$$$$$ creating bus_call **************\n");
    // GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    // BusCallData* bus_call_data;
    // bus_call_data->uridecodebin = uridecodebin;
    // bus_call_data->loop = loop;
    // GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    // gst_bus_add_watch(bus, bus_call, bus_call_data);
    // gst_object_unref(bus);
    /* Get the bus and attach a watch function to it */

    //############## Method 2
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GIOChannel *stdin_channel;
    stdin_channel = g_io_channel_unix_new(fileno(stdin));
    g_io_add_watch(stdin_channel, G_IO_IN | G_IO_HUP, stdin_cb, pipeline);
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);
    

    // Start playing the pipeline
    g_printerr("Starting the pipeline ... \n");
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to start pipeline. Exiting.\n");
        return -1;
    }
    else
    {
        g_printerr("succeed to start pipeline. Exiting.\n");
    }
    gst_element_set_state(source_bin, GST_STATE_PLAYING);

    start_time = clock();
    // Run the main loop
    g_main_loop_run(loop);

    // Clean Up
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    g_main_loop_unref(loop);
    g_mutex_clear (&eos_lock);
    //g_free (g_source_bin_list);
    //reset_terminal_mode();
    return 0;
}

