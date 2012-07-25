/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "FieldDisplacementMapData.h"

#include "TerminationCondition.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"

//-----------------------------------------------------------------------------
FieldDisplacementMapData::FieldDisplacementMapData()
        :
    Displacement(0),
    FwdDisplacement(0),
    BwdDisplacement(0)
{
  this->Displacement=vtkFloatArray::New();
  this->Displacement->SetName("displacement");
  this->Displacement->SetNumberOfComponents(3);

  this->FwdDisplacement=vtkFloatArray::New();
  this->FwdDisplacement->SetName("fwd-displacement-map");
  this->FwdDisplacement->SetNumberOfComponents(3);

  this->BwdDisplacement=vtkFloatArray::New();
  this->BwdDisplacement->SetName("bwd-displacement-map");
  this->BwdDisplacement->SetNumberOfComponents(3);
}

//-----------------------------------------------------------------------------
FieldDisplacementMapData::~FieldDisplacementMapData()
{
  this->Displacement->Delete();
  this->FwdDisplacement->Delete();
  this->BwdDisplacement->Delete();
}

//-----------------------------------------------------------------------------
void FieldDisplacementMapData::SetOutput(vtkDataSet *o)
{
  o->GetPointData()->AddArray(this->Displacement);
  o->GetPointData()->AddArray(this->FwdDisplacement);
  o->GetPointData()->AddArray(this->BwdDisplacement);
}

//-----------------------------------------------------------------------------
int FieldDisplacementMapData::SyncScalars()
{
  vtkIdType nLines=this->Lines.size();
  vtkIdType lastLineId=this->Displacement->GetNumberOfTuples();

  float *pDisplacement
    = this->Displacement->WritePointer(3*lastLineId,3*nLines);

  float *pFwdDisplacement
    = this->FwdDisplacement->WritePointer(3*lastLineId,3*nLines);

  float *pBwdDisplacement
    = this->BwdDisplacement->WritePointer(3*lastLineId,3*nLines);

  for (vtkIdType i=0; i<nLines; ++i)
    {
    FieldLine *line=this->Lines[i];

    line->GetDisplacement(pDisplacement);
    pDisplacement+=3;

    line->GetForwardEndPoint(pFwdDisplacement);
    pFwdDisplacement+=3;

    line->GetBackwardEndPoint(pBwdDisplacement);
    pBwdDisplacement+=3;
    }

  return 1;
}
