/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __pOstream_h
#define __pOstream_h

#include "vtkSciberQuestModule.h" // for export macro
#include <iostream> // for ostream


/**
Helper that prints rank of caller and returns cerr.
*/
VTKSCIBERQUEST_EXPORT std::ostream &pCerr();

#endif

// VTK-HeaderTest-Exclude: postream.h
