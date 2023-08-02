// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStringWriter.h"

#include "vtkDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"

#include "vtksys/FStream.hxx"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkStringWriter);

//----------------------------------------------------------------------------
vtkStringWriter::vtkStringWriter()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(0);
}

//----------------------------------------------------------------------------
vtkStringWriter::~vtkStringWriter() = default;

//----------------------------------------------------------------------------
void vtkStringWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "String: " << this->String << endl;
  os << indent << "FileName: " << this->FileName << endl;
}

//----------------------------------------------------------------------------
int vtkStringWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* vtkNotUsed(info))
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkStringWriter::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* vtkNotUsed(info))
{
  return 1;
}

//----------------------------------------------------------------------------
vtkTypeBool vtkStringWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkStringWriter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  if (this->String.empty() || this->FileName.empty())
  {
    vtkErrorMacro("String or FileName not set.");
    return 0;
  }

  vtksys::ofstream ofs(this->FileName.c_str(), ios::out);
  if (!ofs.is_open())
  {
    vtkErrorMacro("Could not open file: " << this->FileName);
    return 0;
  }
  ofs << this->String;
  ofs.close();

  return 1;
}

//----------------------------------------------------------------------------
int vtkStringWriter::Write()
{
  // always write even if the data hasn't changed
  this->Modified();
  this->Update();
  return 1;
}
