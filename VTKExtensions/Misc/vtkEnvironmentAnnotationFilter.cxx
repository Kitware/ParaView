/*=========================================================================

  Program:   ParaView
  Module:    vtkEnvironmentAnnotationFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEnvironmentAnnotationFilter.h"

#include "vtkDataObjectTypes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <assert.h>
#include <map>
#include <vector>

#include "vtksys/SystemInformation.hxx"
#include "vtksys/SystemTools.hxx"
#include <sstream>

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
vtkEnvironmentAnnotationFilter::~vtkEnvironmentAnnotationFilter()
{
}

//----------------------------------------------------------------------------
void vtkEnvironmentAnnotationFilter::UpdateAnnotationValue()
{
  std::string value = "";
  if (this->DisplayUserName)
  {
#if defined(_WIN32)
    value += std::string(getenv("USERNAME")) + "\n";
#elif defined(_WIN16)
    value += std::string(getenv("USERNAME")) + "\n";
#else
    value += std::string(getenv("USER")) + "\n";
#endif
  }
  if (this->DisplaySystemName)
  {
#if defined(_WIN32)
    value += "Windows\n";
#elif defined(_WIN16)
    value += "Windows 16bit\n"
#elif defined(__APPLE_CC__)
    value += "Mac OS X\n";
#elif defined(__linux__)
    value += "Linux\n";
#endif
  }
  if (this->DisplayDate)
  {
    std::string date = vtksys::SystemTools::GetCurrentDateTime("%c");
    value += date + "\n";
  }
  if (this->DisplayFileName)
  {
    if (this->FileName != "")
    {
      if (this->DisplayFilePath)
      {
        std::string path = vtksys::SystemTools::GetFilenamePath(this->FileName);
        if (!path.empty())
        {
          value += path + "/";
        }
      }
      value += vtksys::SystemTools::GetFilenameName(this->FileName);
    }
  }

  this->AnnotationValue = value;
}

//----------------------------------------------------------------------------
int vtkEnvironmentAnnotationFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  if (input == NULL)
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
