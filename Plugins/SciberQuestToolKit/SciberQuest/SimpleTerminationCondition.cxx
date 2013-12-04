/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#include "SimpleTerminationCondition.h"


//-----------------------------------------------------------------------------
void SimpleTerminationCondition::InitializeColorMapper()
{
  // Initialize the mapper, color scheme as follows:
  // 0   -> problem domain
  // 1   -> s1
  //    ...
  // n   -> sn
  // n+1 -> noise
  std::vector<string> names;
  names.push_back("problem domain");
  names.insert(names.end(),this->SurfaceNames.begin(),this->SurfaceNames.end());
  names.push_back("noise");

  size_t nSurf=this->Surfaces.size()+1; // not 2 because cmap obj assumes n+1.
  this->CMap.BuildColorMap(nSurf,names);
}
