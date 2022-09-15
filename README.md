# v4l_capture_tool

A tool for testing general-purpose cameras using the V4L2 interface

The tool automatically resolves the format and resolution supported by the device, and gets the first one if it supports fetching

v4l_capture_tool -d /dev/videoX -w 640 -h 480
