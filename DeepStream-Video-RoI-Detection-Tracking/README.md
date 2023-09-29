# NVIDIA DeepStream ROI-Based Detection and Tracking

This project is designed to perform object detection specifically on Regions of Interest (ROIs) within video frames. It leverages the capabilities of Nvidia's DeepStream SDK.

## Configuration Files

There are two primary configuration files that guide the processing:

### preprocess-tiling-config.txt

This file allows you to set the Regions of Interest (ROIs) for the pipeline. Modify the coordinates as required to focus on specific areas in your video feed.

### pipeline-config.yml

Use this file to configure the various pipeline parameters, including details about the detection model.

**Note:** In the sample config files we are using YOLOv8 object detection with Nvidia DCF-based tracker. When using a model other than YOLOv8, it's crucial to ensure the `input-tensor-name` in the preprocessing configuration file (`preprocess-tiling-config.txt`) matches the expected input tensor name of the new model. This project is set up with YOLOv8 in mind, so modifications might be required for other models. You also need to update 'config-file-path' of 'nvinfer' inside 'pipeline-config.yml' file.

## Getting Started

1. Clone the repository:
```bash
cd /opt/nvidia/deepstream/deepstream-6.2/sources/
git clone https://github.com/mahshidgh/Nvidia-GStreamer-sample-apps
```
3. Navigate to the project folder:
cd DeepStream-Video-RoI-Detection-Tracking

4. Building the Code

Before building, ensure you set the correct CUDA version:

```bash
export CUDA_VER=<your_cuda_version>
```
or set 'CUDA_VER' inside the Makefile.

Then, build the project:
```bash
sudo make
```

5. Update the configuration files as necessary:
- Define ROIs in `preprocess-tiling-config.txt`
- Set pipeline parameters in `pipeline-config.yml`

6. Run the application:
```bash
./tiling-ds-pipeline pipeline-config.yml
```

## Contributing

Feel free to fork the project, submit PRs, or raise issues if you find any.

## License

This project is licensed under the MIT License.


