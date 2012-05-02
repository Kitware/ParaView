/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVStreamingSphereSource.h"

#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPieceStreamingExtentTranslator.h"
#include "vtkPVCompositeDataPipeline.h"

vtkStandardNewMacro(vtkPVStreamingSphereSource);
//----------------------------------------------------------------------------
vtkPVStreamingSphereSource::vtkPVStreamingSphereSource()
{
}

//----------------------------------------------------------------------------
vtkPVStreamingSphereSource::~vtkPVStreamingSphereSource()
{
}

//----------------------------------------------------------------------------
int vtkPVStreamingSphereSource::RequestInformation(vtkInformation *req,
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  if (!this->Superclass::RequestInformation(req, inputVector, outputVector))
    {
    return 0;
    }

  if (vtkPVCompositeDataPipeline::GetStreamingExtentTranslator(
      outputVector->GetInformationObject(0)) == NULL)
    {
    vtkNew<vtkPieceStreamingExtentTranslator> translator;
    vtkPVCompositeDataPipeline::SetStreamingExtentTranslator(
      outputVector->GetInformationObject(0),
      translator.GetPointer());
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVStreamingSphereSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
