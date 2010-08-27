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
#include "vtkInformation.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVPostFilterExecutive);

vtkInformationKeyMacro(vtkPVPostFilterExecutive, POST_ARRAYS_TO_PROCESS, InformationVector);

//----------------------------------------------------------------------------
vtkPVPostFilterExecutive::vtkPVPostFilterExecutive()
{

}

//----------------------------------------------------------------------------
vtkPVPostFilterExecutive::~vtkPVPostFilterExecutive()
{

}

//----------------------------------------------------------------------------
int vtkPVPostFilterExecutive::NeedToExecuteData(
  int outputPort,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  return 1;
}

//----------------------------------------------------------------------------
vtkInformation *vtkPVPostFilterExecutive::GetPostArrayToProcessInformation(int idx)
{
  // add this info into the algorithms info object
  vtkInformationVector *inArrayVec =
    this->Algorithm->GetInformation()->Get(POST_ARRAYS_TO_PROCESS());
  if (!inArrayVec)
    {
    inArrayVec = vtkInformationVector::New();
    this->Algorithm->GetInformation()->Set(POST_ARRAYS_TO_PROCESS(),inArrayVec);
    inArrayVec->Delete();
    }
  vtkInformation *inArrayInfo = inArrayVec->GetInformationObject(idx);
  if (!inArrayInfo)
    {
    inArrayInfo = vtkInformation::New();
    inArrayVec->SetInformationObject(idx,inArrayInfo);
    inArrayInfo->Delete();
    }
  return inArrayInfo;
}

//----------------------------------------------------------------------------
void vtkPVPostFilterExecutive::SetPostArrayToProcessInformation(int idx, vtkInformation *inInfo)
{
  vtkInformation *info = this->GetPostArrayToProcessInformation(idx);
  info->Copy(inInfo,1);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPostFilterExecutive::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
