PROJECT(MPEG2ENC)

SET(srcs
  conform.c  motion.c    putbits.c  putpic.c  quantize.c  stats.c
  fdctref.c  puthdr.c   putseq.c  ratectl.c   transfrm.c
  idct.c     predict.c   putmpg.c   putvlc.c  readpic.c   writepic.c
  vtkMPEG2WriterHelper.cxx
  )

INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  )

ADD_LIBRARY(vtkMPEG2Encode ${srcs})
TARGET_LINK_LIBRARIES(vtkMPEG2Encode vtkIO vtkImaging)
