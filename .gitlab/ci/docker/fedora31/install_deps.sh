#!/bin/sh

# Install extra dependencies for ParaView
yum install -y \
   bzip2 patch \
   python3-twisted python3-autobahn \
   python3 python3-devel qt5-qtbase-devel qt5-qttools-devel \
   qt5-qtsvg-devel qt5-qtxmlpatterns-devel doxygen openmpi-devel \
   python3-numpy openmpi-devel mesa-libOSMesa-devel \
   python3-pandas python3-pandas-datareader \
   mesa-libOSMesa-devel mesa-libOSMesa \
   python3-sphinx python3-pip \
   libXcursor-devel

python3 -m pip install wslink

yum clean all
