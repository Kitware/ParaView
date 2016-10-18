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

#include "vtkAlgorithmOutput.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUpdateSuppressorPipeline.h"

#include <map>

#ifdef MYDEBUG
#define vtkMyDebug(x) cout << x;
#else
#define vtkMyDebug(x)
#endif

vtkStandardNewMacro(vtkPVUpdateSuppressor);
//----------------------------------------------------------------------------
vtkPVUpdateSuppressor::vtkPVUpdateSuppressor()
{
  this->Enabled = true;
}

//----------------------------------------------------------------------------
vtkPVUpdateSuppressor::~vtkPVUpdateSuppressor()
{
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::SetEnabled(bool enable)
{
  if (this->Enabled == enable)
  {
    return;
  }
  this->Enabled = enable;

  // This is not right. This will cause the update-suppressor to always
  // re-execute when ForceUpdate() is called, which causes the sub-sequent
  // filters/mappers to re-execute as well. This was resulting in display lists
  // never being used.
  // this->Modified();
  vtkUpdateSuppressorPipeline* executive =
    vtkUpdateSuppressorPipeline::SafeDownCast(this->GetExecutive());
  if (executive)
  {
    executive->SetEnabled(enable);
  }
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::ForceUpdate()
{
  bool enabled = this->Enabled;
  this->SetEnabled(false);
  this->Update();
  this->SetEnabled(enabled);
  this->ForcedUpdateTimeStamp.Modified();
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVUpdateSuppressor::CreateDefaultExecutive()
{
  vtkUpdateSuppressorPipeline* executive = vtkUpdateSuppressorPipeline::New();
  executive->SetEnabled(this->Enabled);
  return executive;
}

//----------------------------------------------------------------------------
int vtkPVUpdateSuppressor::RequestDataObject(vtkInformation* vtkNotUsed(reqInfo),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* outInfo = outputVector->GetInformationObject(i);
      vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
      {
        vtkDataObject* newOutput = input->NewInstance();
        outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
        this->GetOutputPortInformation(i)->Set(
          vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
      }
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVUpdateSuppressor::RequestData(vtkInformation* vtkNotUsed(reqInfo),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // RequestData is only called by its executive when
  // (Enabled==off) and thus acting as a passthrough filter
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  output->ShallowCopy(input);
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Enabled: " << this->Enabled << endl;
}
