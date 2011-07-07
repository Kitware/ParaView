/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "FieldTraceData.h"

#include "TerminationCondition.h"
#include "vtkDataSet.h"
#include "vtkCellData.h"

//-----------------------------------------------------------------------------
FieldTraceData::FieldTraceData()
      :
    IntersectColor(0)//,
    //SourceId(0)
{
  this->IntersectColor=vtkIntArray::New();
  this->IntersectColor->SetName("IntersectColor");

  // this->SourceId=vtkIntArray::New();
  // this->SourceId->SetName("SourceId");

  this->Tcon=new TerminationCondition;
}

//-----------------------------------------------------------------------------
FieldTraceData::~FieldTraceData()
{
  this->IntersectColor->Delete();
  // this->SourceId->Delete();
  this->ClearFieldLines();

  delete this->Tcon;
}

//-----------------------------------------------------------------------------
void FieldTraceData::SetOutput(vtkDataSet *o)
{
  o->GetCellData()->AddArray(this->IntersectColor);
  // o->GetCellData()->AddArray(this->SourceId);
}

//-----------------------------------------------------------------------------
int *FieldTraceData::Append(vtkIntArray *ia, int nn)
{
  vtkIdType ne=ia->GetNumberOfTuples();
  return ia->WritePointer(ne,nn);
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

//-----------------------------------------------------------------------------
int FieldTraceData::SyncScalars()
{
  vtkIdType nLines=this->Lines.size();

  vtkIdType lastLineId=this->IntersectColor->GetNumberOfTuples();

  int *pColor=this->IntersectColor->WritePointer(lastLineId,nLines);
  // int *pId=this->SourceId->WritePointer(lastLineId,nLines);

  for (vtkIdType i=0; i<nLines; ++i)
    {
    FieldLine *line=this->Lines[i];

    *pColor=this->Tcon->GetTerminationColor(line);
    ++pColor;

    // *pId=line->GetSeedId();
    // ++pId;
    }
  return 1;
}

//-----------------------------------------------------------------------------
void FieldTraceData::PrintLegend(int reduce)
{
  if (reduce)
    {
    this->Tcon->SqueezeColorMap(this->IntersectColor);
    }
  else
    {
    this->Tcon->PrintColorMap();
    }
}
