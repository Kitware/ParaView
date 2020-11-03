#!/bin/sh

# Install extra dependencies for ParaView
yum install -y \
   bzip2 patch \
   python-twisted-core python-twisted-web python-twisted-words \
   python3 python3-devel qt5-qtbase-devel qt5-qttools-devel \
   qt5-qtsvg-devel qt5-qtxmlpatterns-devel doxygen openmpi-devel \
   python36-numpy openmpi-devel mesa-libOSMesa-devel \
   python3-pandas python3-pandas-datareader \
   libXcursor-devel

pip3 install sphinx

yum clean all
