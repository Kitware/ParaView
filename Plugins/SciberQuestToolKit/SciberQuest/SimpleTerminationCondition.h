/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef SimpleTerminationCondition_h
#define SimpleTerminationCondition_h

#include "TerminationCondition.h"

// The open termination condition asigns a unique color
// to permutation of surfaces and a noise category. Noise
// include the termnination cases problem domain, field null,
// short integration.
class SimpleTerminationCondition : public TerminationCondition
{
public:
  // Description:
  // Build the color mapper with the folowing scheme:
  //
  // 0   -> problem domain
  // 1   -> s1
  //    ...
  // n   -> sn
  // n+1 -> noise=(field null,short integration)
  virtual void InitializeColorMapper();

  // Description:
  // Return the indentifier for the special termination cases.
  virtual int GetProblemDomainSurfaceId(){ return 0; };
  virtual int GetFieldNullId(){ return this->Surfaces.size()+1; }
  virtual int GetShortIntegrationId(){ return this->Surfaces.size()+1; }
};

#endif

// VTK-HeaderTest-Exclude: SimpleTerminationCondition.h
