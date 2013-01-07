Block OpenNI
============
A C++ wrapper for OpenNI

2011-06-20:
This block is going through heavy remake and so old code/methods are mixed with
new methods for the moment.  I will be changing the way things are handled at
the moment when i'm finished. All for better support on multiple devices.

Basic description
=================

Support for several generators as Image, IR, Depth and User.
Skeleton tracking is also supported.

Installation
============

You need to install the sensor drivers, OpenNI and NITE.
Note! PrimeSense drivers do not work with the Kinect. 


So here's a step-by-step installationg guide:
(The links you'll find here are the latest unstable at the time of writing)

- OpenNI 1.3.2.3
- Nite 1.4.1.2
- SensorKinect 5.0.3.4

Windows 32bit:
--------------

Download/Install OpenNI:
http://openni.org/downloadfiles/opennimodules/openni-binaries/latest-unstable/163-openni-unstable-build-for-windows-x86-32-bit-v1-3-2/download

Download/Install Avin2's SensorKinect:
https://github.com/avin2/SensorKinect/blob/unstable/Bin/SensorKinect-Win-OpenSource32-5.0.3.4.msi

Download/Install NITE:
http://openni.org/downloadfiles/opennimodules/openni-compliant-middleware-binaries/latest-unstable/177-primesense-nite-unstable-build-for-windows-x86-32-bit-v1-4-1/download

At this moment you should now be able to run the samples in OpenNI.  Go to
OpenNI's folder and browse to /Samples/Bin/Release. Run NiViewer.exe. You
should now be looking at a window with the RGB and Depth feed coming from the
kinect.

MacOSX 32bit:
-------------

### 1. Install libtool/libusb ###

#### With MacPorts: ####

Download/Install MacPorts
- (Snow Leopard 10.6) https://distfiles.macports.org/MacPorts/MacPorts-2.0.0-10.5-Leopard.dmg
- (Lion 10.7) https://distfiles.macports.org/MacPorts/MacPorts-2.0.0-10.7-Lion.dmg

In the terminal:

    sudo port install libtool
    sudo port install libusb-devel +universal

#### With Hombrew: ####

Install Homebrew: in the terminal:

    ruby -e "$(curl -fsSkL raw.github.com/mxcl/homebrew/go)"

Install the libs from the terminal:

    brew install libtool
    brew install libusb --universal


### 2. Download/Untar OpenNI ###

http://openni.org/downloadfiles/opennimodules/openni-binaries/latest-unstable/162-openni-unstable-build-for-macosx-10-6-universal-x86x64-3264-bit-v1-3-2/download

Open the terminal, goto openni's folder you just untar'ed.

    sudo ./install.sh

### 3. Download Avin2's SensorKinect ###

https://github.com/avin2/SensorKinect/blob/unstable/Bin/SensorKinect-Bin-MacOSX-v5.0.3.4.tar.bz2

Open the terminal, goto SensorKinect's folder you just untar'ed.

    sudo ./install.sh

### 4. Download NITE ###

http://openni.org/downloadfiles/opennimodules/openni-compliant-middleware-binaries/latest-unstable/176-primesense-nite-unstable-build-for-macosx-10-6-universal-x86x64-3264-bit-v1-4-1/download

Open the terminal, goto NITE's folder you just untar'ed.

    sudo ./install.sh

At this moment you should now be able to run the samples in OpenNI. Go to
OpenNI's folder and browse to /Samples/Bin/Release (use terminal). Run
./NiViewer. You should now be looking at a window with the RGB and Depth feed
coming from the kinect

	
Linux
-----

Check Avin2's SensorKinect repo (@github) for more information.
