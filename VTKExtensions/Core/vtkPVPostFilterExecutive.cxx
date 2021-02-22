/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPVPostFilterExecutive.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPostFilterExecutive.h"

#include "vtkAlgorithm.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVPostFilterExecutive);

vtkInformationKeyMacro(vtkPVPostFilterExecutive, POST_ARRAYS_TO_PROCESS, InformationVector);
vtkInformationKeyMacro(vtkPVPostFilterExecutive, POST_ARRAY_COMPONENT_KEY, StringVector);

//----------------------------------------------------------------------------
vtkPVPostFilterExecutive::vtkPVPostFilterExecutive() = default;

//----------------------------------------------------------------------------
vtkPVPostFilterExecutive::~vtkPVPostFilterExecutive() = default;

//----------------------------------------------------------------------------
int vtkPVPostFilterExecutive::NeedToExecuteData(
  int outputPort, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec)
{
  if (this->Algorithm->GetInformation()->Has(POST_ARRAYS_TO_PROCESS()))
  {
    return true;
  }
  return this->Superclass::NeedToExecuteData(outputPort, inInfoVec, outInfoVec);
}

//----------------------------------------------------------------------------
vtkInformation* vtkPVPostFilterExecutive::GetPostArrayToProcessInformation(int idx)
{
  // add this info into the algorithms info object
  vtkInformationVector* inArrayVec =
    this->Algorithm->GetInformation()->Get(POST_ARRAYS_TO_PROCESS());
  if (!inArrayVec)
  {
    inArrayVec = vtkInformationVector::New();
    this->Algorithm->GetInformation()->Set(POST_ARRAYS_TO_PROCESS(), inArrayVec);
    inArrayVec->Delete();
  }
  vtkInformation* inArrayInfo = inArrayVec->GetInformationObject(idx);
  if (!inArrayInfo)
  {
    inArrayInfo = vtkInformation::New();
    inArrayVec->SetInformationObject(idx, inArrayInfo);
    inArrayInfo->Delete();
  }
  return inArrayInfo;
}

//----------------------------------------------------------------------------
void vtkPVPostFilterExecutive::SetPostArrayToProcessInformation(int idx, vtkInformation* inInfo)
{
  vtkInformation* info = this->GetPostArrayToProcessInformation(idx);
  if (!this->MatchingPropertyInformation(info, inInfo))
  {
    info->Copy(inInfo, 1);
    info->Set(vtkPVPostFilterExecutive::POST_ARRAY_COMPONENT_KEY(), "_");
  }
}

//----------------------------------------------------------------------------
bool vtkPVPostFilterExecutive::MatchingPropertyInformation(
  vtkInformation* inputArrayInfo, vtkInformation* postArrayInfo)
{
  return (inputArrayInfo && postArrayInfo && inputArrayInfo->Has(vtkDataObject::FIELD_NAME()) &&
    postArrayInfo->Has(vtkDataObject::FIELD_NAME()) &&
    inputArrayInfo->Get(vtkAlgorithm::INPUT_PORT()) ==
      postArrayInfo->Get(vtkAlgorithm::INPUT_PORT()) &&
    inputArrayInfo->Get(vtkAlgorithm::INPUT_CONNECTION()) ==
      postArrayInfo->Get(vtkAlgorithm::INPUT_CONNECTION()) &&
    inputArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION()) ==
      postArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION()) &&
    strcmp(inputArrayInfo->Get(vtkDataObject::FIELD_NAME()),
      postArrayInfo->Get(vtkDataObject::FIELD_NAME())) == 0);
}

//----------------------------------------------------------------------------
void vtkPVPostFilterExecutive::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
