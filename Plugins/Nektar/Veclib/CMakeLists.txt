
project(vec C Fortran)

include_directories(${Nektar_SOURCE_DIR}/include)

if(NOT WIN32)
# need some try compiles
  set(CMAKE_C_FLAGS "-fPIC -ffast-math -funroll-loops -fstrict-aliasing -w")
  set(CMAKE_Fortran_FLAGS "-fPIC")
endif()

set(veclib_math_sources
  math/xfft.c
  math/xvrand.c
  math/xfill.c
  math/xneg.c
  math/xvneg.c
  math/xvadd.c
  math/xvsub.c
  math/xsmul.c
  math/xvmul.c
  math/xvrecp.c
  math/xvdiv.c
  math/xzero.c
  math/xsadd.c
  math/xsdiv.c
  math/xvabs.c
  math/xvamax.c
  math/xvsin.c
  math/xvcos.c
  math/xvsqrt.c
  math/xvlg10.c
  math/xvexp.c
  )

set(veclib_triad_sources
  triads/xsvtvm.c
  triads/xsvtvp.c
  triads/xsvvmt.c
  triads/xsvvpt.c
  triads/xsvvtm.c
  triads/xsvvtp.c
  triads/xvvtvm.c
  triads/xvvtvp.c
  triads/xvvpvt.c
  triads/xvvmvt.c
  triads/xsvtsp.c
  triads/xvvvtm.c
  )

set(veclib_relational_sources
  relational/xseq.c
  relational/xsle.c
  relational/xslt.c
  relational/xsne.c
  )

set(veclib_reduce_sources
  reduction/xsum.c
  reduction/ixmax.c
  reduction/ixmin.c
  reduction/icount.c
  reduction/ifirst.c
  )

#CONVERT  = xvfloa  vdble   vsngl  xvrand xvreal xvimag xvcmplx xbrev
set(veclib_conversion_sources
  conversion/xvfloa.c
  conversion/vdble.c
  conversion/vsngl.c
  conversion/xvreal.c
  conversion/xvimag.c
  conversion/xvcmplx.c
  conversion/xbrev.c
  )

set(veclib_misc_sources
  misc/xvpoly.c
  misc/xscatr.c
  misc/xgathr.c
  misc/xramp.c
  misc/xcndst.c
  misc/xmask.c
  misc/polint.c
  misc/mxm.c
  misc/mxv.c
  misc/mxva.c
  misc/spline.c
  misc/veclib.c
  misc/dclock.c
  misc/mach.c
  misc/zbesj.f
  misc/fftdf.f
  )

# MEMORY  = xvector xmatrix icopy
set(veclib_memory_sources
  memory/xvector.c
  memory/xmatrix.c
  memory/icopy.c
  )


add_library(vec STATIC 
  ${veclib_math_sources}
  ${veclib_triad_sources}
  ${veclib_relational_sources}
  ${veclib_reduce_sources}
  ${veclib_conversion_sources}
  ${veclib_misc_sources}
  ${veclib_memory_sources}
  )

set_target_properties(vec PROPERTIES LINKER_LANGUAGE C)
