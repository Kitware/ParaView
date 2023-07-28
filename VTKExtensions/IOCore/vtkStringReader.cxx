// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStringReader.h"

#include "vtkDataObject.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"

#include "vtksys/FStream.hxx"
#include <sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkStringReader);

//----------------------------------------------------------------------------
vtkStringReader::vtkStringReader()
{
  this->SetNumberOfInputPorts(0);
  // the output ports should be 0 but the vtkSMSourceProxy does not like that
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkStringReader::~vtkStringReader() = default;

//----------------------------------------------------------------------------
void vtkStringReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "String: " << this->String << endl;
  os << indent << "FileName: " << this->FileName << endl;
}

//----------------------------------------------------------------------------
int vtkStringReader::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* vtkNotUsed(info))
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkStringReader::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // the output ports should be 0 but the vtkSMSourceProxy does not like that
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
vtkTypeBool vtkStringReader::ProcessRequest(
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
int vtkStringReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  if (this->FileName.empty())
  {
    vtkErrorMacro("FileName not set.");
    return 0;
  }

  vtksys::ifstream ifs(this->FileName.c_str(), ios::in);
  if (!ifs.is_open())
  {
    vtkErrorMacro("Could not open file: " << this->FileName);
    return 0;
  }

  std::stringstream buffer;
  buffer << ifs.rdbuf();
  this->String = buffer.str();

  ifs.close();

  return 1;
}
