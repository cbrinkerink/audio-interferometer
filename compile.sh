#!/bin/bash

# For basic-opengl
#/usr/bin/g++ basic-opengl.cpp -o basic-opengl -L/Users/christiaanbrinkerink/Code-projects/FPGA-audio/full-correlator/Client -framework Cocoa -framework IOKit -framework OpenGL -framework CoreVideo /opt/local/lib/libglfw.dylib /opt/local/lib/libassimp.dylib /opt/local/lib/libfreetype.dylib libSTB_IMAGE.a libGLAD.a

# For client
/usr/bin/g++ client.cpp -o client -L/Users/christiaanbrinkerink/Code-projects/FPGA-audio/full-correlator/Client -framework Cocoa -framework IOKit -framework OpenGL -framework CoreVideo /opt/local/lib/libglfw.dylib /opt/local/lib/libassimp.dylib /opt/local/lib/libfreetype.dylib libSTB_IMAGE.a libGLAD.a
