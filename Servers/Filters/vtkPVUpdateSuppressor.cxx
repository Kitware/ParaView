/*=========================================================================

  Program:   ParaView
  Module:    vtkPVUpdateSuppressor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVUpdateSuppressor.h"

#include "vtkCommand.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkObjectFactory.h"
#include "vtkDataSet.h"
#include "vtkPolyData.h"
#include "vtkCollection.h"

vtkCxxRevisionMacro(vtkPVUpdateSuppressor, "1.18");
vtkStandardNewMacro(vtkPVUpdateSuppressor);
vtkCxxSetObjectMacro(vtkPVUpdateSuppressor,Input,vtkDataSet);

//----------------------------------------------------------------------------
vtkPVUpdateSuppressor::vtkPVUpdateSuppressor()
{
  this->Input = 0;

  this->UpdatePiece = 0;
  this->UpdateNumberOfPieces = 1;

  this->CachedGeometry = NULL;
  this->CachedGeometryLength = 0;
}

//----------------------------------------------------------------------------
vtkPVUpdateSuppressor::~vtkPVUpdateSuppressor()
{
  this->SetInput(0);
  this->RemoveAllCaches();
}

//----------------------------------------------------------------------------
vtkPolyData* vtkPVUpdateSuppressor::GetPolyDataOutput()
{
  vtkDataSet* o = this->GetOutput();
  return vtkPolyData::SafeDownCast(o);
}

//----------------------------------------------------------------------------
vtkDataSet* vtkPVUpdateSuppressor::GetOutput()
{
  if (this->NumberOfOutputs < 1 || this->Outputs[0] == 0)
    {
    vtkDataSet* input = this->GetInput();
    if (input == 0)
      {
      return 0;
      }
    vtkDataSet* output = input->NewInstance();
    this->SetOutput(output);
    output->Delete();
    }
  return static_cast<vtkDataSet*>(this->Outputs[0]);
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::ForceUpdate()
{
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();

  if (input == 0)
    {
    vtkErrorMacro("Missing input.");
    return;
    }

  // int fixme; // I do not like this hack.  How can we get rid of it?
  // Assume the input is the collection filter.
  // Client needs to modify the collection filter because it is not
  // connected to a pipeline.
  if (input->GetSource() && 
      (input->GetSource()->IsA("vtkMPIMoveData") ||
       input->GetSource()->IsA("vtkCollectPolyData") ||
       input->GetSource()->IsA("vtkMPIDuplicatePolyData") ||
       input->GetSource()->IsA("vtkM2NDuplicate") ||
       input->GetSource()->IsA("vtkM2NCollect") ||
       input->GetSource()->IsA("vtkMPIDuplicateUnstructuredGrid") ||
       input->GetSource()->IsA("vtkPVDuplicatePolyData")))
    {
    input->GetSource()->Modified();
    }

  input->SetUpdatePiece(this->UpdatePiece);
  input->SetUpdateNumberOfPieces(this->UpdateNumberOfPieces);
  input->SetUpdateGhostLevel(0);
  input->Update();

  unsigned long t2;
#ifdef VTK_USE_EXECUTIVES
  vtkDemandDrivenPipeline *ddp = 0;
  if (input->GetSource())
    {
    ddp = 
      vtkDemandDrivenPipeline::SafeDownCast(
        input->GetSource()->GetExecutive());
    }
  if (ddp)
    {
    ddp->UpdateInformation();
    t2 = ddp->GetPipelineMTime();
    }
#else
  t2 = input->GetPipelineMTime();
#endif
  if (t2 > this->UpdateTime || output->GetDataReleased())
    {
    output->ShallowCopy(input);
    this->UpdateTime.Modified();
    }
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::Execute()
{
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();
  if (!input || !output)
    {
    return;
    }  
  output->ShallowCopy(input);
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::RemoveAllCaches()
{
  int idx;

  for (idx = 0; idx < this->CachedGeometryLength; ++idx)
    {
    if (this->CachedGeometry[idx])
      {
      this->CachedGeometry[idx]->Delete();
      this->CachedGeometry[idx] = NULL;
      }
    }

  if (this->CachedGeometry)
    {
    delete [] this->CachedGeometry;
    this->CachedGeometry = NULL;
    }
  this->CachedGeometryLength = 0;
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::CacheUpdate(int idx, int num)
{
  vtkDataSet *pd;
  vtkDataSet *output;
  int j;

  if (idx < 0 || idx >= num)
    {
    vtkErrorMacro("Bad cache index: " << idx << " of " << num);
    return;
    }

  if (num != this->CachedGeometryLength)
    {
    this->RemoveAllCaches();
    this->CachedGeometry = new vtkDataSet*[num];
    for (j = 0; j < num; ++j)
      {
      this->CachedGeometry[j] = NULL;
      }
    this->CachedGeometryLength = num;
    }

  output = this->GetOutput();
  pd = this->CachedGeometry[idx];
  if (pd == NULL)
    { // we need to update and save.
    this->ForceUpdate();
    pd = output->NewInstance();
    pd->ShallowCopy(output);
    this->CachedGeometry[idx] = pd;
    pd->Register(this);
    pd->Delete();
    }
  else
    { // Output generated previously.
    output->ShallowCopy(pd);
    }
}


//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Input: (" << this->Input << ")\n";
  os << indent << "UpdatePiece: " << this->UpdatePiece << endl;
  os << indent << "UpdateNumberOfPieces: " << this->UpdateNumberOfPieces << endl;
}
