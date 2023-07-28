// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEnvironmentAnnotationFilter.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVStringFormatter.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include "vtksys/SystemTools.hxx"

vtkStandardNewMacro(vtkEnvironmentAnnotationFilter);
//----------------------------------------------------------------------------
vtkEnvironmentAnnotationFilter::vtkEnvironmentAnnotationFilter()
  : DisplayUserName(false)
  , DisplaySystemName(false)
  , DisplayFileName(false)
  , DisplayFilePath(false)
  , DisplayDate(false)
{
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkEnvironmentAnnotationFilter::~vtkEnvironmentAnnotationFilter() = default;

//----------------------------------------------------------------------------
void vtkEnvironmentAnnotationFilter::UpdateAnnotationValue()
{
  std::string formattedString;
  if (this->DisplayUserName)
  {
    formattedString += "{ENV_username}\n";
  }
  if (this->DisplaySystemName)
  {
    formattedString += "{ENV_os}\n";
  }
  if (this->DisplayDate)
  {
    formattedString += "{GLOBAL_date:%a %m %b %Y %I:%M:%S %p %Z}\n";
  }
  if (this->DisplayFileName)
  {
    if (!this->FileName.empty())
    {
      std::string filenameArgument;
      if (this->DisplayFilePath)
      {
        std::string path = vtksys::SystemTools::GetFilenamePath(this->FileName);
        if (!path.empty())
        {
          filenameArgument += path + "/";
        }
      }
      filenameArgument += vtksys::SystemTools::GetFilenameName(this->FileName);

      vtkPVStringFormatter::PushScope(fmt::arg("filename", filenameArgument));
      formattedString += "{filename}";
    }
  }

  this->AnnotationValue = vtkPVStringFormatter::Format(formattedString);

  if (this->DisplayFileName && !this->FileName.empty())
  {
    vtkPVStringFormatter::PopScope();
  }
}

//----------------------------------------------------------------------------
int vtkEnvironmentAnnotationFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  if (input == nullptr)
  {
    return 0;
  }
  if (!vtkMultiProcessController::GetGlobalController() ||
    vtkMultiProcessController::GetGlobalController()->GetLocalProcessId() <= 0)
  {
    // initialize variables.
    this->UpdateAnnotationValue();

    // Update the output data
    vtkSmartPointer<vtkStringArray> data = vtkSmartPointer<vtkStringArray>::New();
    data->SetName("Text");
    data->SetNumberOfComponents(1);
    data->InsertNextValue(this->AnnotationValue);

    vtkTable* output = vtkTable::GetData(outputVector);
    output->AddColumn(data);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnvironmentAnnotationFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void vtkEnvironmentAnnotationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << this->FileName << endl;
  os << indent << "AnnotationValue: " << this->AnnotationValue << endl;
  os << indent << "DisplayUserName: " << (this->DisplayUserName ? "True" : "False") << endl;
  os << indent << "DisplayFileName: " << (this->DisplayFileName ? "True" : "False") << endl;
  os << indent << "DisplayFilePath: " << (this->DisplayFilePath ? "True" : "False") << endl;
  os << indent << "DisplaySystemName: " << (this->DisplaySystemName ? "True" : "False") << endl;
  os << indent << "DisplayDate: " << (this->DisplayDate ? "True" : "False") << endl;
}
