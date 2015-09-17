/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
