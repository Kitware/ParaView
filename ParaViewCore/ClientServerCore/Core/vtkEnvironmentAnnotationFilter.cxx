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
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <assert.h>
#include <map>
#include <vector>
#include <vtksys/ios/sstream>
#include "vtksys/SystemTools.hxx"
#include "vtksys/SystemInformation.hxx"

vtkStandardNewMacro(vtkEnvironmentAnnotationFilter);
//----------------------------------------------------------------------------
vtkEnvironmentAnnotationFilter::vtkEnvironmentAnnotationFilter()
  : AnnotationValue(NULL),
  FileName(NULL),
  PathName(NULL),
  DisplayUserName(false),
  DisplaySystemName(false),
  DisplayFileName(false),
  DisplayDate(false),
  DisplayPath(false)
{
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkEnvironmentAnnotationFilter::~vtkEnvironmentAnnotationFilter()
{
  this->DisplayDate = false;
  this->DisplaySystemName = false;
  this->DisplayUserName = false;
  this->DisplayFileName = false;
  this->DisplayPath = false;
  this->AnnotationValue = NULL;
  this->SetFileName(0);
  this->SetPathName(0);
}

//----------------------------------------------------------------------------
void vtkEnvironmentAnnotationFilter::UpdateAnnotationValue()
{
  delete [] this->AnnotationValue;
  this->AnnotationValue = NULL;
  std::string value = "";
  if (this->DisplayUserName)
  {
    #if defined(_WIN32)
    value += std::string(getenv("USERNAME")) + "\n";
    #elif defined(_WIN16)
    value += std::string(getenv("USERNAME"))+ "\n";
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
    if (this->FileName == NULL)
    {
      this->SetFileName("");
    }
    const char * fname = vtksys::SystemTools::GetFilenameName(std::string(this->FileName)).c_str();
    value += std::string(fname) + "\n";
  }
  if (this->DisplayPath)
  {
    if (this->PathName == NULL)
    {
      this->SetPathName("(Null path)");
    }
    value += std::string(this->PathName);
  }

  
  this->AnnotationValue = vtksys::SystemTools::DuplicateString(value.c_str());
}

//----------------------------------------------------------------------------
int vtkEnvironmentAnnotationFilter::RequestData(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  if (input == NULL)
   {
    return 0;
   }
  // initialize variables.
  this->UpdateAnnotationValue();

  // Update the output data
  vtkStringArray* data = vtkStringArray::New();
  data->SetName("Text");
  data->SetNumberOfComponents(1);
  data->InsertNextValue(this->AnnotationValue);

  vtkTable* output = vtkTable::GetData(outputVector);
  output->AddColumn(data);
  data->FastDelete();

  if (vtkMultiProcessController::GetGlobalController() &&
    vtkMultiProcessController::GetGlobalController()->GetLocalProcessId() > 0)
    {
    // reset output on all ranks except the 0 root node.
    output->Initialize();
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
}
