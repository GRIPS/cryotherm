Installation
------------
First, install the correct version of libusb:

    sudo apt-get install libusb-1.0.0-dev

Second, run make.

How to use
----------
The executable needs root access, so

    sudo ./cryotherm

A file called `monitorboard.dat` will be continuously updated with the most recent readings (appended at the end).
A second file called `monitorboard-oneline.dat` will have only the most recent readings (single line).

Notes
-----
The code in libusb/ is from the SDK, except with the location of libusb.h corrected.
