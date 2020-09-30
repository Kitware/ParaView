#! /bin/sh

# Configures the simulation
CAM_ROOT=~/src/cesm1_2_2

INC_NETCDF=/usr/include;export INC_NETCDF
LIB_NETCDF=/usr/lib;export LIB_NETCDF

camcfg=$CAM_ROOT/models/atm/cam/bld

# serial
# $camcfg/configure -fc gfortran -dyn fv -hgrid 10x15 -nospmd -nosmp -debug -catalyst -test

######################################################################
# See ~/src/cesm1_2_2/models/atm/cam/bld/config_files/horiz_grid.xml
# for available grid sizes

######################################################################
# Possible -hgrid for eul
# eul 512x1024
# eul 256x512
# eul 128x256
# eul 64x128
# eul 48x96
# eul 32x64
# eul 8x16
# eul 1x1

######################################################################
# Possible -hgrid for sld
# sld 64x128
# sld 32x64
# sld 8x16


######################################################################
# Possible -hgrid for Finite Volume (fv)
# 0.23x0.31
# 0.47x0.63
# 0.5x0.625
# 0.9x1.25
# 1x1.25
# 1.9x2.5
# 2x2.5
# 2.5x3.33
# 4x5
# 10x15

# SPMD (mpi) Finite Volume
# fv, cam5, trop_mam3
# $camcfg/configure -fc mpif90 -fc_type gnu -cc mpicc -dyn fv -hgrid 10x15 -ntasks 2 -debug -nosmp -catalyst -test

# SPMD (mpi)
# fv, cam5, trop_mam3
#$camcfg/configure -fc mpif90 -fc_type gnu -cc mpicc -dyn fv -hgrid 4x5 -ntasks 4 -debug -nosmp -catalyst -test


# SPMD (mpi) Finite Volume
# fv, cam5, trop_mam3
#$camcfg/configure -fc mpif90 -fc_type gnu -cc mpicc -dyn fv -hgrid 2.5x3.33 -ntasks 8 -debug -nosmp -catalyst -test

######################################################################
# Possible -hgrid for Spectral Element (se)
# ne2np4
# ne5np8
# ne7np8
# ne10np4
# ne16np4 * -nx 13826 -ny 1
# ne16np8
# ne21np4
# ne30np4 * -nx 48602 -ny 1
# ne30np8
# ne60np4
# ne120np4
# ne240np4

# SPMD (mpi) Spectral Element
# -dyn se, -phys cam5, -chem trop_mam3
$camcfg/configure -fc mpif90 -fc_type gnu -cc mpicc -dyn se -hgrid ne16np4 -ntasks 2 -debug -nosmp -catalyst -test

# SPMD (mpi) Spectral Element
# -dyn se, -phys cam5, -chem trop_mam3cd
#$camcfg/configure -fc mpif90 -fc_type gnu -cc mpicc -dyn se -hgrid ne30np4 -ntasks 2 -debug -nosmp -catalyst -test

# SMP (multi-threading)
#$camcfg/configure -fc=gfortran -dyn fv -hgrid 10x15 -nospmd -nthreads 6 -test
