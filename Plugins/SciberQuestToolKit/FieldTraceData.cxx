/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "FieldTraceData.h"

#include "TerminationCondition.h"
#include "vtkDataSet.h"
#include "vtkCellData.h"

//-----------------------------------------------------------------------------
FieldTraceData::FieldTraceData()
{
  this->Tcon=new TerminationCondition;
}

//-----------------------------------------------------------------------------
FieldTraceData::~FieldTraceData()
{
  this->ClearFieldLines();

  delete this->Tcon;
}

//-----------------------------------------------------------------------------
void FieldTraceData::ClearFieldLines()
{
  size_t nLines=this->Lines.size();
  for (size_t i=0; i<nLines; ++i)
    {
    delete this->Lines[i];
    }
  this->Lines.clear();
}
