#include<gst/gst.h>
#include <gstnvdsmeta.h>
#include <nvdsmeta_schema.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>



typedef struct _LinkElementsData {
    GstElement *nvvideoconvert;
    GstElement *nvstreammux;
} LinkElementsData;

typedef struct _BusCallData {
    GstElement* uridecodebin;
    GMainLoop *loop;
} BusCallData;

const gchar* print_pad_type(GstPad *pad);
//gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data);
const gchar* gst_pad_link_return_to_string(GstPadLinkReturn ret);
//gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data);
//GstPadProbeReturn nvinfer_src_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data);
void set_terminal_mode(void);
void reset_terminal_mode(void);