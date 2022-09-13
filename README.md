# audio-interferometer
Using an FPGA to sample audio from multiple digital microphones and perform correlation, then visualise it on a PC. Presently, this repository is a hodgepodge of things including source files, executables, libraries and build scripts. I have tried to stay cross-platform, but everything so far only has been made to compile on OSX.

## Sources used
I have used OpenGL C++ code from the LearnOpenGL tutorial series (specifically, tutorial 4.1 from section 1). I got the serial connection stuff from a question on stackoverflow :)

The Ethernet core is from LiteEth, it is an independently synthesized core using the gen.py script in that codebase.
