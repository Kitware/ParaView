/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "postream.h"

#ifdef WIN32
  #include <Winsock2.h>
#else
  #include <unistd.h>
#endif

#ifndef SQTK_WITHOUT_MPI
#include "SQMPICHWarningSupression.h"
#include <mpi.h>
#endif

//-----------------------------------------------------------------------------
std::ostream &pCerr()
{
  int WorldRank=0;
  #ifndef SQTK_WITHOUT_MPI
  int ok;
  MPI_Initialized(&ok);
  if (ok) MPI_Comm_rank(MPI_COMM_WORLD,&WorldRank);
  #endif

  char host[256]={'\0'};
  gethostname(host,256);

  std::cerr << "[" << host << ":" << WorldRank << "] ";

  return std::cerr;
}
