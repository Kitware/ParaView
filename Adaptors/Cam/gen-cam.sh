#! /bin/bash
CAM_ROOT=~/src/cesm1_2_2
GENF90=$CAM_ROOT/tools/cprnc/genf90/genf90.pl
files="box_rearrange.F90
piodarray.F90
pio_spmd_utils.F90
pionfwrite_mod.F90
rearrange.F90"

if [ $# -lt 1 ]; then
    echo "`basename $0` d|r|g"
    exit
fi

if [ "x$1" = "xd" ]; then
    # Debug
    # copy the .F90.in files over .F90 so that
    # gdb works correctly
    pushd $CAM_ROOT/models/utils/pio
    for i in $files; do
        cp ${i}.in $i
    done
    popd
elif [ "x$1" = "xg" ]; then
    # Generate
    # generate .F90 from .F90.in
    pushd $CAM_ROOT/models/utils/pio
    for i in $files; do
        perl $GENF90 ${i}.in > $i
    done
    popd
else
    # Restore
    # generate .F90 from .F90.in
    pushd $CAM_ROOT/models/utils/pio
    for i in $files; do
        svn revert $i
    done
    popd
fi
