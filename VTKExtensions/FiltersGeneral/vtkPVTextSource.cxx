// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVTextSource.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkPVTextSource);
//----------------------------------------------------------------------------
vtkPVTextSource::vtkPVTextSource()
{
  this->Text = nullptr;
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkPVTextSource::~vtkPVTextSource()
{
  this->SetText(nullptr);
}

//----------------------------------------------------------------------------
int vtkPVTextSource::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* vtkNotUsed(info))
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVTextSource::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // This is needed to ensure that vtkPVTextSource::RequestData() gets called on
  // all ranks.
  outputVector->GetInformationObject(0)->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);
  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVTextSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkTable* output = this->GetOutput();

  vtkStringArray* data = vtkStringArray::New();
  data->SetName("Text");
  data->SetNumberOfComponents(1);
  data->InsertNextValue(this->Text ? this->Text : "");
  output->AddColumn(data);
  data->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVTextSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Text: " << (this->Text ? this->Text : "(none)") << endl;
}
