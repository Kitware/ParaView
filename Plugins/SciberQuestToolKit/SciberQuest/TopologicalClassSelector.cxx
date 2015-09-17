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

#include "TopologicalClassSelector.h"

#include "vtkDataSet.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCellData.h"
#include "vtkAppendFilter.h"
#include "vtkThreshold.h"

//-----------------------------------------------------------------------------
TopologicalClassSelector::TopologicalClassSelector()
    :
  Input(0),
  Append(0)
{
  this->Initialize();
}

//-----------------------------------------------------------------------------
TopologicalClassSelector::~TopologicalClassSelector()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
void TopologicalClassSelector::Initialize()
{
  this->Clear();
  this->Append=vtkAppendFilter::New();
}

//-----------------------------------------------------------------------------
void TopologicalClassSelector::Clear()
{
  if (this->Input)
    {
    this->Input->Delete();
    }

  if (this->Append)
    {
    this->Append->Delete();
    this->Append=0;
    }
}

//-----------------------------------------------------------------------------
void TopologicalClassSelector::SetInput(vtkDataSet *input)
{
  if (this->Input==input)
    {
    return;
    }

  if (this->Input)
    {
    this->Input->Delete();
    }

  this->Input=input;

  if (this->Input)
    {
    this->Input=input->NewInstance();
    this->Input->ShallowCopy(input);
    this->Input->GetCellData()->SetActiveScalars("IntersectColor");
    }
}

//-----------------------------------------------------------------------------
void TopologicalClassSelector::AppendRange(double v0, double v1)
{
  vtkThreshold *threshold=vtkThreshold::New();
  threshold->SetInputData(this->Input);
  threshold->SetInputArrayToProcess(
        0,
        0,
        0,
        vtkDataObject::FIELD_ASSOCIATION_CELLS,
        "IntersectColor");
  threshold->ThresholdBetween(v0,v1);
  threshold->Update();

  vtkUnstructuredGrid *ug=threshold->GetOutput();

  this->Append->AddInputData(ug);

  threshold->Delete();
}

//-----------------------------------------------------------------------------
vtkUnstructuredGrid *TopologicalClassSelector::GetOutput()
{
  this->Append->Update();
  vtkUnstructuredGrid *ug=this->Append->GetOutput();

//   std::cerr << "Geting output" << std::endl;
//   ug->Print(std::cerr);

  return ug;
}
