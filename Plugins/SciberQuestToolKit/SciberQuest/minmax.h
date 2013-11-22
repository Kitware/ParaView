/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/

#ifndef minmax_h
#define minmax_h

//*****************************************************************************
template<typename T>
T max(const T &a, const T &b){ return a<b?b:a; }

//*****************************************************************************
template<typename T>
T min(const T &a, const T &b){ return a<b?a:b; }

#endif

// VTK-HeaderTest-Exclude: minmax.h
