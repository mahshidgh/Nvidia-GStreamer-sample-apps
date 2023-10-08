# DeepStream RTSP Pipeline README

## Overview

This DeepStream pipeline provides functionality to process video streams fetched from an RTSP source. The pipeline handles video frames, decodes them, sets the framerate, and sends them through an inference model for object detection. Subsequently, objects detected are then tracked using NVIDIA's tracker. The final results can be visualized with bounding boxes around detected objects and their tracking IDs. We are using `YOLOv8` as the object detector model and `Nvidia DCF tracker` for tracking. You can change these models in the config file `pipeline_config.yml`.


## How to Set Up and Run

### Prerequisites

1. Ensure you have DeepStream 6.2 (or higher) and its dependencies installed.

2. You will need a YAML parser to read the configuration files. If not installed, you install it using the following command:
    ```bash
    sudo apt-get install libyaml-dev
    ```

### Configuring the Pipeline

The structure of the configuration file is as follows:

```yaml
rtspsrc:
  location: <RTSP_SOURCE>
  # ... other parameters ...

streammux:
  # ...

capsfilter:
  framerate: <FRAME_RATE>

nvinfer:
  config-file-path: <PATH_TO_MODEL_CONFIG>
  # ...

nvtracker:
  # ...
```

**Changing Parameters**:
- A sample config file is in `pipeline_config.yml`.
- To change the RTSP source, modify the `location` parameter under `rtspsrc`.
- To adjust the frame rate, modify the `framerate` parameter under `capsfilter`.
- To use a different model for inference, update the `config-file-path` parameter under `nvinfer`.
### Build and Compile
```bash
sudo make
```

### Running the Pipeline

To run the pipeline:

```bash
./rtsp-ds pipeline_config.yml
```


## Notes
1. You need to create a directory named `track_out` whereever you are running the pipeline to save the tracker output.
2. There is a time stamp attached to the tracker output, in the first line of the `txt` file.
3. To enable detection output uncomment the following lines:
```c
 GstPad *nvinfer_src_pad = gst_element_get_static_pad(nvinfer, "src");
 gst_pad_add_probe(nvinfer_src_pad, GST_PAD_PROBE_TYPE_BUFFER, nvinfer_src_pad_buffer_probe, NULL, NULL);
 gst_object_unref(nvinfer_src_pad);
```

