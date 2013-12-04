/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
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
  Threshold(0),
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
