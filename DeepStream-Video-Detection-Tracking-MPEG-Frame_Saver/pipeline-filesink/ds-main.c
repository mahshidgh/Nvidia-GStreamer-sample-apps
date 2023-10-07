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
#include "common_func.h"

#define FRAME_WIDTH 1920
#define FRAME_HEIGHT 1080
// GstElement* uridecodebin = gst_element_factory_make("uridecodebin", "uridecodebin");
// GstElement* pipeline = gst_pipeline_new("my-pipeline");
gint frame_counter = 0; // Counter for frames
gint chunk_counter = 0;
gboolean is_EOS = 0;
void on_pad_added_tee(GstElement *element, GstPad *pad, gpointer data)
{
    GstPad *sinkpad;
    GstElement *downstream = (GstElement *)data;

    /* We can now link this pad with the downstream element */
    sinkpad = gst_element_get_static_pad(downstream, "sink");

    if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK)
    {
        g_error("************** [Tee] Failed to link elements!\n");
    }
    else
    {
        g_print(" [Tee] linked successfully\n");
    }

    gst_object_unref(sinkpad);
}

static GstPadProbeReturn nvinfer_src_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
    GstBuffer *buf = (GstBuffer *)info->data;
    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);

    for (NvDsMetaList *l_frame = batch_meta->frame_meta_list; l_frame != NULL; l_frame = l_frame->next)
    {
        NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(l_frame->data);
        /* Increment the frame counter and generate a filename */
        //frame_counter++;
        guint frame_number = frame_meta->frame_num;
        char filename[256];
        snprintf(filename, sizeof(filename), "detection_out/%d.txt", frame_number);

        /* Open the file for writing */
        FILE *bbox_params_dump_file = fopen(filename, "w");
        if (bbox_params_dump_file == NULL)
        {
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

static GstPadProbeReturn nvtracker_src_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
    GstBuffer *buf = (GstBuffer *)info->data;
    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);

    for (NvDsMetaList *l_frame = batch_meta->frame_meta_list; l_frame != NULL; l_frame = l_frame->next)
    {
        NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(l_frame->data);
        /* Increment the frame counter and generate a filename */
        //frame_counter++;
        guint frame_number = frame_meta->frame_num;
        char filename[256];
        snprintf(filename, sizeof(filename), "track_out/%d.txt", frame_number);

        /* Open the file for writing */
        FILE *bbox_params_dump_file = fopen(filename, "w");
        if (bbox_params_dump_file == NULL)
        {
            g_print("Could not open file for writing\n");
            continue; // Skip this frame if the file can't be opened
        }

        for (NvDsMetaList *l_obj = frame_meta->obj_meta_list; l_obj != NULL; l_obj = l_obj->next)
        {
            NvDsObjectMeta *obj_meta = (NvDsObjectMeta *)(l_obj->data);
            float left = obj_meta->tracker_bbox_info.org_bbox_coords.left;
            float top = obj_meta->tracker_bbox_info.org_bbox_coords.top;
            float right = left + obj_meta->tracker_bbox_info.org_bbox_coords.width;
            float bottom = top + obj_meta->tracker_bbox_info.org_bbox_coords.height;
            // Here confidence stores detection confidence, since dump gie output
            // is before tracker plugin
            float confidence = obj_meta->confidence;
            fprintf(bbox_params_dump_file,
                    "%s %lu %f %f %f %f %f\n",
                    obj_meta->obj_label, obj_meta->object_id, left, top, right, bottom, confidence);

           
        }
        /* Close the file */
        fclose(bbox_params_dump_file);
        // [dump past frames] ---- start -----
        // [past frames] --- end -----
        }

    return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn nvtracker_past_src_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
    GstBuffer *buf = (GstBuffer *)info->data;
    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);

    // dump past frame tracked objects appending current frame objects
    gchar bbox_file[1024] = {0};
    FILE *bbox_params_dump_file = NULL;

    NvDsPastFrameObjBatch *pPastFrameObjBatch = NULL;
    NvDsUserMetaList *bmeta_list = NULL;
    NvDsUserMeta *user_meta = NULL;
    for (bmeta_list = batch_meta->batch_user_meta_list; bmeta_list != NULL;
         bmeta_list = bmeta_list->next)
    {
        user_meta = (NvDsUserMeta *)bmeta_list->data;
        if (user_meta && user_meta->base_meta.meta_type == NVDS_TRACKER_PAST_FRAME_META)
        {
            pPastFrameObjBatch =
                (NvDsPastFrameObjBatch *)(user_meta->user_meta_data);
            for (uint si = 0; si < pPastFrameObjBatch->numFilled; si++)
            {
                NvDsPastFrameObjStream *objStream = (pPastFrameObjBatch->list) + si;
                guint stream_id = (guint)(objStream->streamID);
                for (uint li = 0; li < objStream->numFilled; li++)
                {
                    NvDsPastFrameObjList *objList = (objStream->list) + li;
                    for (uint oi = 0; oi < objList->numObj; oi++)
                    {
                        NvDsPastFrameObj *obj = (objList->list) + oi;
                        g_snprintf(bbox_file, sizeof(bbox_file), "track_out/%d.txt", (guint)obj->frameNum);
                        float left = obj->tBbox.left;
                        float right = left + obj->tBbox.width;
                        float top = obj->tBbox.top;
                        float bottom = top + obj->tBbox.height;
                        // Past frame object confidence given by tracker
                        float confidence = obj->confidence;
                        bbox_params_dump_file = fopen(bbox_file, "a");
                        if (!bbox_params_dump_file)
                        {
                            continue;
                        }
                        fprintf(bbox_params_dump_file,
                                "%s %lu %f %f %f %f %f\n",
                                objList->objLabel, objList->uniqueId, left, top, right, bottom,
                                confidence);
                        fclose(bbox_params_dump_file);
                    }
                }
            }
        }
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

static gboolean
bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_EOS:
        g_print("End of stream\n");
        g_main_loop_quit(loop);
        break;

    case GST_MESSAGE_ERROR:
    {
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

static void
cb_drained(GstElement *uridecodebin, gpointer user_data)
{
    if (is_EOS == 0)
    {
        g_print("******************* chunk not yet ended!*******************************\n");
        return;
    }
    else
    {
        g_print("EOS ---- updating the source file ... \n");
    }
    GstElement *pipeline = (GstElement *)user_data;
    /* Get the source from the pipeline */
    //GstElement *uridecodebin = gst_bin_get_by_name (GST_BIN (pipeline), "uridecodebin");
    /* Pause the pipeline */
    gst_element_set_state(pipeline, GST_STATE_PAUSED);

    /* Wait for user to press 'r' */
    printf("Press 'r' to resume with next video...\n");
    int c;
    do
    {
        c = getchar();
    } while (c != 'r' && c != 'R');

    /* Set the URI to the next video and resume */
    frame_counter = 0;
    chunk_counter++;
    g_object_set(uridecodebin, "uri", "chunks/inf.mp4", NULL);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    is_EOS = 0;
    /* Unreference the source */
    //gst_object_unref (uridecodebin);
}

static void source_pad_added(GstElement *element, GstPad *pad, gpointer data)
{
    g_print("######################pad added ....\n");
    const gchar *str = print_pad_type(pad);
    static guint pad_index = 0; // Choose the pad index based on your use case
    if (strstr(str, "audio") != NULL )
    {
         g_print("pad type is audio returing ... \n");
         goto exit;
    } 

    RTSPLinkElementsData *elements_data = (RTSPLinkElementsData *)data;
    GstElement *depay = elements_data->depay;
    GstElement *convert = elements_data->convert;
    GstElement *streammux = elements_data->streammux;

    GstPad *sinkpad = gst_element_get_static_pad(depay, "sink");
    //    guint pad_index = 0; // Choose the pad index based on your use case
    //    gchar pad_name[16];
    //    g_snprintf(pad_name, sizeof(pad_name), "sink_%u", pad_index);
    //    g_print("pad name is %s\n", pad_name);
    //    GstPad *sinkpad = gst_element_get_request_pad(data, pad_name);

    //const gchar *pad_type
    
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

    // Now link nvvideoconvert to nvstreammux
    GstPad *srcpad = gst_element_get_static_pad(convert, "src");
   
    gchar pad_name[16];
    g_snprintf(pad_name, sizeof(pad_name), "sink_%u", pad_index);
    GstPad *streammux_sinkpad = gst_element_get_request_pad(streammux, pad_name);

    if (gst_pad_link(srcpad, streammux_sinkpad) != GST_PAD_LINK_OK)
    {
        g_printerr("Failed to link nvvideoconvert and nvstreammux.\n");
    }
    else
    {
        g_print("nvvideoconvert and nvstreammux linked.\n");
    }

    gst_object_unref(srcpad);
    gst_object_unref(streammux_sinkpad);
    pad_index++;

    exit:
        g_print("Exiting adding pad ... \n");
}

int main(int argc, char *argv[])
{
    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create uridecodebin element
    GstElement *source = gst_element_factory_make ("rtspsrc", "source");
    GstElement *depay = gst_element_factory_make ("rtph264depay", "depay");
    GstElement *parse = gst_element_factory_make ("h264parse", "parse");
    GstElement *decoder = gst_element_factory_make ("nvv4l2decoder", "decoder");
    GstElement *queue = gst_element_factory_make ("queue", "queue");
    GstElement *convert = gst_element_factory_make ("nvvideoconvert", "convert");

    // Create nvstreammux element
    GstElement *streammux = gst_element_factory_make("nvstreammux", "stream-mux");

    // Create nvinfer element
    GstElement *nvinfer = gst_element_factory_make("nvinfer", "nvinfer");

    // Create tracker element
    GstElement *tracker = gst_element_factory_make("nvtracker", "tracker");

    // Create videoconvert element
    GstElement *infer_convert = gst_element_factory_make("nvvideoconvert", "infer_convert");

    // Create nvosd element
    GstElement *nvosd = gst_element_factory_make("nvdsosd", "nvosd");

    // Create videoconvert element
    GstElement *videoconvert = gst_element_factory_make("nvvideoconvert", "videoconvert");

    // Create encoder element
    GstElement *encoder = gst_element_factory_make("nvv4l2h264enc", "encoder");

    GstElement *h264parse = gst_element_factory_make("h264parse", "h264parse");

    // Create muxer element
    GstElement *muxer = gst_element_factory_make("qtmux", "muxer");

    GstElement *filesink = gst_element_factory_make("filesink", "filesink");

    // ********** Creat elements for retrieving the frames ****************
    // Create new elements
    GstElement *tee = gst_element_factory_make("tee", "tee");
    GstElement *queue1 = gst_element_factory_make("queue", "queue1");
    GstElement *queue2 = gst_element_factory_make("queue", "queue2");
    GstElement *image_convert = gst_element_factory_make("nvvideoconvert", "image_convert");
    GstElement *image_encoder = gst_element_factory_make("jpegenc", "image_encoder");
    GstElement *image_sink = gst_element_factory_make("multifilesink", "image_sink");

    // Check if element creation was successful
    if (!source || !depay || !parse || !decoder || !queue || !convert || !streammux || !nvinfer || !infer_convert || !nvosd || !videoconvert || !encoder || !h264parse || !muxer || !filesink || !tee || !queue1 || !queue2 || !image_convert || !image_encoder || !image_sink)
    {
        g_printerr("One or more elements could not be created. Exiting.\n");
        return -1;
    }
    // Configure the multifilesink
    g_object_set(image_sink, "location", "frames/%d.jpg", "post-messages", TRUE, "async", FALSE, NULL);

    // Set the properties of the elements
    if (g_str_has_suffix(argv[1], ".yml") || g_str_has_suffix(argv[1], ".yaml"))
    {
        //setting parameters

        //nvds_parse_uridecodebin(uridecodebin, argv[1], "uridecodebin");
        //g_object_set(G_OBJECT(uridecodebin), "uri", "rtsp://104.196.207.139:8554/s1", NULL);
        nvds_parse_streammux(streammux, argv[1], "streammux");
        nvds_parse_gie(nvinfer, argv[1], "nvinfer");
        nvds_parse_file_sink(filesink, argv[1], "filesink");

        /* Set all the necessary properties of the nvinfer element,
        * the necessary ones are : */
        // g_object_set (G_OBJECT (nvinfer),
        // "config-file-path", "infer-config.yml", NULL);
    }
    /* Modify the source's properties */
    g_object_set (source, "location", "rtsp://34.148.223.212:8554/s1", "protocols", 0x00000004, "latency", 1000, "drop-on-latency", TRUE, "timeout", 5000000, NULL);
    g_object_set (decoder, "cudadec-memtype", 0, "num-extra-surfaces", 1, NULL);
    g_object_set (queue, "leaky", 2, "max-size-buffers", 1, NULL);
  

    g_object_set(G_OBJECT(encoder), "bitrate", 15000000, NULL);

    // Set up the pipeline
    GstElement *pipeline = gst_pipeline_new("my-pipeline");
    gst_bin_add_many(GST_BIN(pipeline), source, depay, parse, decoder, queue, convert, NULL);
    gst_bin_add(GST_BIN(pipeline), streammux);
    //gst_bin_add(GST_BIN(pipeline), queue1);
    gst_bin_add(GST_BIN(pipeline), nvinfer);
    gst_bin_add(GST_BIN(pipeline), tracker);
    //gst_bin_add(GST_BIN(pipeline), queue2);
    // Add the elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), tee, queue1, queue2, image_convert, image_encoder, image_sink, NULL);
    gst_bin_add(GST_BIN(pipeline), infer_convert);
    gst_bin_add(GST_BIN(pipeline), nvosd);
    //gst_bin_add(GST_BIN(pipeline), queue3);
    gst_bin_add(GST_BIN(pipeline), videoconvert);
    // gst_bin_add(GST_BIN(pipeline), queue4);
    gst_bin_add(GST_BIN(pipeline), encoder);
    gst_bin_add(GST_BIN(pipeline), h264parse);
    gst_bin_add(GST_BIN(pipeline), muxer);
    gst_bin_add(GST_BIN(pipeline), filesink);

    // Link the elements. Note: you need to handle the 'pad-added' signal when linking dynamic pads.
    // Configure the tracker
    g_object_set(G_OBJECT(tracker), "tracker-width", 832, "tracker-height", 832, "ll-lib-file", "/opt/nvidia/deepstream/deepstream-6.2/lib/libnvds_nvmultiobjecttracker.so", "ll-config-file", "/opt/nvidia/deepstream/deepstream-6.2/sources/DeepStream-Yolo/config_tracker_NvDCF_perf.yml",
                 "gpu-id", 0, "enable-batch-process", TRUE, "enable-past-frame", TRUE, "display-tracking-id", TRUE, "compute-hw", 1, NULL);


    // Create link element
    RTSPLinkElementsData elements_data;
    elements_data.depay = depay;
    elements_data.convert = convert;
    elements_data.streammux = streammux;




    // Link elements together
    g_signal_connect(source, "pad-added", G_CALLBACK(source_pad_added), &elements_data);
    // Dynamic source file
   // g_signal_connect(uridecodebin, "drained", G_CALLBACK(cb_drained), pipeline);

   if (!gst_element_link_many (depay, parse, decoder, queue, convert, NULL) ) {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (pipeline);
        return -1;
    }
    
    if (!gst_element_link_many(streammux, nvinfer, NULL))
    {
        g_printerr("Failed to link queue1 elements and elements between them. Exiting.\n");
        return -1;
    }

 
    if (!gst_element_link_many(nvinfer, tracker, tee, NULL))
    {
        g_printerr("Failed to link nvinfer to tee. Exiting.\n");
        return -1;
    }
    // Handle 'pad-added' signal from the tee element
    GstPad *tee_src_pad_1, *tee_src_pad_2;
    GstPad *queue1_sink_pad, *queue2_sink_pad;
    GstPadTemplate *tee_src_pad_template1, *tee_src_pad_template2;
    /* Request a pad from tee for each branch and link */
    tee_src_pad_template1 = gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (tee), "src_%u");
    tee_src_pad_1 = gst_element_request_pad (tee, tee_src_pad_template1, NULL, NULL);
    queue1_sink_pad = gst_element_get_static_pad(queue1, "sink");
    if (gst_pad_link(tee_src_pad_1, queue1_sink_pad) != GST_PAD_LINK_OK)
    {
        g_error("Failed to link tee to queue1");
    }
    gst_object_unref(queue1_sink_pad);

    tee_src_pad_template2 = gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (tee), "src_%u");
    tee_src_pad_2 = gst_element_request_pad (tee, tee_src_pad_template2, NULL, NULL);
    queue2_sink_pad = gst_element_get_static_pad(queue2, "sink");
    if (gst_pad_link(tee_src_pad_2, queue2_sink_pad) != GST_PAD_LINK_OK)
    {
        g_error("Failed to link tee to queue2");
    }
    gst_object_unref(queue2_sink_pad);

    if (!gst_element_link_many(queue1, infer_convert, nvosd, NULL))
    {
        g_printerr("Failed to link queue1 elements and elements between them. Exiting.\n");
        return -1;
    }

    if (!gst_element_link_many(nvosd, videoconvert, NULL))
    {
        g_printerr("Failed to link nvosd elements and elements between them. Exiting.\n");
        return -1;
    }

    if (!gst_element_link_many(videoconvert, encoder, h264parse, muxer, filesink, NULL))
    {
        g_printerr("Failed to link queue4 elements and elements between them. Exiting.\n");
        return -1;
    }

    if (!gst_element_link_many(queue2, image_convert, image_encoder, image_sink, NULL))
    {
        g_printerr("Failed to link queue2 elements and elements between them. Exiting.\n");
        return -1;
    }

 
  
 
    // Register a probe on the source pad of the nvinfer element to retrieve the bounding box information
    GstPad *nvinfer_src_pad = gst_element_get_static_pad(nvinfer, "src");
    gst_pad_add_probe(nvinfer_src_pad, GST_PAD_PROBE_TYPE_BUFFER, nvinfer_src_pad_buffer_probe, NULL, NULL);
    gst_object_unref(nvinfer_src_pad);

    GstPad *nvtracker_src_pad = gst_element_get_static_pad(tracker, "src");
    gst_pad_add_probe(nvtracker_src_pad, GST_PAD_PROBE_TYPE_BUFFER, nvtracker_src_pad_buffer_probe, NULL, NULL);
    gst_pad_add_probe(nvtracker_src_pad, GST_PAD_PROBE_TYPE_BUFFER, nvtracker_past_src_pad_buffer_probe, NULL, NULL);
    gst_object_unref(nvtracker_src_pad);


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
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

 

    // Start playing the pipeline
    g_printerr("Starting the pipeline ... \n");
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Failed to start pipeline. Exiting.\n");
        return -1;
    }
    else
    {
        g_printerr("succeed to start pipeline. Exiting.\n");
    }

    // Run the main loop
    g_main_loop_run(loop);

    // Clean Up
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    g_main_loop_unref(loop);
    return 0;
}
