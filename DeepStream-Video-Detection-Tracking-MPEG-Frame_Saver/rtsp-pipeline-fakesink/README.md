# Object Detection and Frame Storage Pipeline

## Overview

This GStreamer pipeline is designed for robust video frame processing, specifically for object detection. Once processed, the pipeline stores both the detected bounding box (bbox) information and the actual frames in MPEG format.

## Motivation

The inception of this pipeline came from a distinct need: to cater to a request for a solution that not only yields bounding box information from video frames but also retains the frames. Storing frames is essential when further post-processing, such as anomaly detection, is required.

While this pipeline stands efficient on its own, it is architected to be interfaced seamlessly with other applications for post-processing. This modular approach provides flexibility and ensures that as new processing requirements emerge, they can be easily integrated.

**Note**: If you're considering adding computationally intensive post-processing steps, it's advisable to integrate them within the GStreamer pipeline itself. This integration often leads to more efficient and faster processing, ensuring real-time or near-real-time performance.

## Potential Applications

- **Anomaly Detection**: With stored frames, anomalies that were not captured during the initial object detection phase can be retrospectively analyzed.
  
- **Post-processing Integration**: The pipeline can easily interface with other applications, offering flexibility for diverse post-processing needs.
  
- **Research and Development**: For teams looking to build upon or refine object detection capabilities, having access to both bbox data and the original frames can be invaluable.


## RST Pipleine with Fakesink 
This pipeline runs object detection and tracking on video frames with specified frame rate. The output of this pipeline are the selected frames, and the bounding box coordinates and the IDs.
### Instructions
1. Navigate to the folder:
```bash
cd rtsp-pipeline-fakesink
```
2. Modify the Makefile to set CUDA_VER
3. Compile:
```bash
sudo make
```
4. Modify the configuration file: ''pipeline-config.yml
5. Run
```bash
mv rtsp-ds-fksink ../
mv pipeline-config.yml ../
cd ../
./rtsp-ds-fksink pipeline-config.yml
```

## Conclusion

With increasing demand in video processing tasks, having a reliable and adaptable solution like this pipeline is paramount. Whether you're diving into further video analytics, research, or simply need a robust object detection mechanism, this pipeline provides a solid foundation.

