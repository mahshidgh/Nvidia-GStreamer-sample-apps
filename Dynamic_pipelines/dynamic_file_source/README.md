# GStreamer Pipeline with YOLO Object Detection

This repository contains a GStreamer pipeline integrated with the YOLO object detector. The primary function of this pipeline is to process a sequence of video files and output object detection results as well as logging the inference latency in seconds.

## Features

- **Dynamic Video Processing**: The pipeline is capable of handling a sequence of video files dynamically. It processes each video without the need to pause or terminate the pipeline.

- **Detection Output**: For every frame processed, the detection results are output as text files in the directory `detection_output/tmp`.

- **Inference Latency Logging**: The inference latency for each video is logged in the `inf_latency` directory, with the latency values recorded in seconds.

- **Seamless Transition Between Videos**: Upon completion of a video, the pipeline prompts with 'Enter 0 to move to the next video ...'. The user can then enter '0' to proceed to the next video, which is named as `videos/o$i.mp4`.

## Customization

- **Signal Integration**: Instead of the print statement for transitioning between videos, a signal can be generated. This signal can then be connected to a callback function that automatically moves to the next video.

- **Configuration**: Before running the pipeline, make sure to edit the `config_infer_primary_yoloV8-832.txt` file. This configuration file should point to the location of your YOLO weights, the configuration file, and the TensorRT engine file.

## Installation and Running

1. **Compilation**: To compile the pipeline, run the following command:
   ```bash
   sudo make
   ```

2. **Execution**: To run the pipeline, use the command:
   ```bash
   ./gst-dynamic-source pipeline-config.yml
   ```

Ensure that all configurations are set correctly before executing the pipeline.

---

**Note**: This repository assumes you have the necessary dependencies for GStreamer and YOLO object detection installed on your system.
```
