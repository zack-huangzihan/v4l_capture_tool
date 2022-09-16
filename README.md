# v4l_capture_tool

A tool for testing general-purpose cameras using the V4L2 interface

The tool automatically resolves the format and resolution supported by the device, and gets the first one if it supports fetching

The software automatically finds the resolution and output format:
v4l_capture_tool -d /dev/videoX -w 640 -h 480

The user determines the output format and resolution:
v4l_capture_tool -d /dev/videoX  -f NV12 -w 640 -h 480