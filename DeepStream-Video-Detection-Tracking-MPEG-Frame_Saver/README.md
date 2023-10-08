# Object Detection and Frame Storage Pipeline

## Overview

This GStreamer pipeline is designed for robust video frame processing, specifically for object detection. Once processed, the pipeline stores both the detected bounding box (bbox) information and the actual frames in MPEG format.

## Motivation

The inception of this pipeline came from a distinct need: to cater to a request for a solution that not only yields bounding box information from video frames but also retains the frames. Storing frames is essential when further post-processing, such as anomaly detection, is required.

While this system works well by itself, it's designed to easily interface with other applications for additional post-processing. 

**Note**: If you're considering adding computationally intensive post-processing steps, it's recommended to integrate them within the GStreamer pipeline itself. This integration often leads to more efficient and faster processing, ensuring real-time or near-real-time performance.

## Potential Applications

- **Anomaly Detection**: With stored frames, anomalies that were not captured during the initial object detection phase can be retrospectively analyzed.
  
- **Post-processing Integration**: The pipeline can easily interface with other applications, offering flexibility for diverse post-processing needs.
  
- **Research and Development**: For teams looking to build upon or refine object detection capabilities, having access to both bbox data and the original frames can be invaluable.


## RTSP Pipleine with Fakesink 
This pipeline runs object detection and tracking on video frames with specified frame rate. The output of this pipeline are the selected frames (in `./frames/`), and the bounding box coordinates and the IDs (in `./track_out/`).
### Instructions
1. Navigate to the folder:
```bash
cd pipeline-fakesink
```
2. Modify the Makefile to set CUDA_VER
3. Compile:
```bash
sudo make
```
In the current script, frame rate is set to 30 fps, if you would like to change to higher or lower frame-rate change `30` in the following line in `ds-main.c`:
```bash
GstCaps *caps = gst_caps_from_string("video/x-raw,framerate=(fraction)30/1");
```
4. Modify the configuration file: ``pipeline-config.yml``
5. Run
```bash
mv ds-fksink ../
mv pipeline-config.yml ../
cd ../
./rtsp-ds-fksink pipeline-config.yml
```

## RTSP Pipleine with Filesink 
This pipeline runs object detection and tracking on video frames with specified frame rate. The output of this pipeline are the selected frames (in `./frames/`), the bounding box coordinates and the IDs (in `./track_out/`), **as well as  the output video with overlayed boundign boxes and IDs** (`output.mp4`, you can change the name in `pipeline-config.yml`).
### Instructions
1. Navigate to the folder:
```bash
cd pipeline-filesink
```
2. Modify the Makefile to set CUDA_VER
3. Compile:
```bash
sudo make
```
Compared to the fakesink pipeline, this pipelines's frame rate is equal to the input so you cannot change the frame-rate. But you can use the same code used in the fakesink to do so.
4. Modify the configuration file: ``pipeline-config.yml``
5. Run
```bash
mv ds-filesink ../
mv pipeline-config.yml ../
cd ../
./ds-fksfilesinkink pipeline-config.yml
```

## Related Pipelines
GStreamer object detection and tracking pipleine with the capability of setting the frame-rate is in:
``
Nvidia-GStreamer-sample-apps/DeepStream-Video-Detection-Tracking-Selective-Frame-Rate
``
