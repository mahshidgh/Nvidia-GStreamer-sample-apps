#include<gst/gst.h>
#include <gstnvdsmeta.h>
#include "nvdsmeta.h"
#include "nvdsinfer.h"
#include "nvdsgstutils.h"
#include <nvdsmeta_schema.h>
#include "nvds_tracker_meta.h"
#include <time.h>
#include <gst/base/gstbasetransform.h>


typedef struct _LinkElementsData {
    GstElement *nvvideoconvert;
    GstElement *nvstreammux;
} LinkElementsData;

typedef struct _RTSPLinkElementsData {
    GstElement *depay;
    GstElement *convert;
    GstElement *streammux;
} RTSPLinkElementsData;

typedef struct _BusCallData {
    GstElement* uridecodebin;
    GMainLoop *loop;
} BusCallData;

const gchar* print_pad_type(GstPad* pad);
//gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data);
const gchar* gst_pad_link_return_to_string(GstPadLinkReturn ret);
//gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data);
//GstPadProbeReturn nvinfer_src_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data);