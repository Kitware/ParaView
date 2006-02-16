// -*- c++ -*-

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */


#include "DobranoVizWindow.h"
#include <QApplication>

#ifdef VTK_USE_MPI
# include <mpi.h>
#endif

int main(int argc, char* argv[])
{
#ifdef VTK_USE_MPI
  int myId =0;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD,&myId);
#endif

  // Create the main window
  QApplication application(argc, argv);

/** \todo Figure-out how to export Qt's resource symbols from a DLL, so we can use them here */
#if !(defined(WIN32) && defined(PARAQ_BUILD_SHARED_LIBS))
  Q_INIT_RESOURCE(pqWidgets);
#endif
  
  DobranoVizWindow main_window;
  main_window.resize(800, 600);
  main_window.show();

  int ret = application.exec();

#ifdef VTK_USE_MPI
  MPI_Finalize();
#endif

  return ret;
}
