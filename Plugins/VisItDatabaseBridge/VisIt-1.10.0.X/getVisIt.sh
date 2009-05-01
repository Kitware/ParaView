#!/bin/bash   
LOG=log.txt
date > $LOG

VISIT_VER=1.10.0
PREFIX=https://wci.llnl.gov/codes/visit/
OPTS=--no-check-certificate

TARBALLS="\
    boxlib.tar.gz\
    libccmio-2.6.1.tar.gz\
    cfitsio3006.tar.gz\
    cgnslib_2.4-3.tar.gz\
    exodusii-4.46.tar.gz\
    gdal-1.3.2.tar.gz\
    H5Part-20070711.tar.gz\
    HDF4.2r1.tar.gz\
    hdf5-1.6.8.tar.gz\
    hdf5-1.8.1.tar.gz\
    netcdf.tar.gz\
    silo-4.6.2.tar.gz\
    szip-2.1.tar.gz\
    "

for tgz in $TARBALLS
do
  echo "Downloading $tgz"
  wget -a $LOG --no-check-certificate $PREFIX/3rd_party/$tgz 
  echo "Unzipping $tgz"
  tar xzfv $tgz 2>&1 >> $LOG
done

# Get VisIt
wget -a $LOG --no-check-certificate $PREFIX/$VISIT_VER/visit$VISIT_VER.tar.gz
tar xzfv visit$VISIT_VER.tar.gz 2>&1 >> $LOG

# Clean up
# for d in `find ./ -maxdepth 1 -type d | grep ./[a-Z]`
# do 
#    rm -rf $d;
# done
# 
# Unzip
# for f in `ls -1 *.gz` 
# do
#   tar xzfv $f
# done