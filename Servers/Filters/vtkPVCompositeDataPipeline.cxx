/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPVCompositeDataPipeline.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCompositeDataPipeline.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObject.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationExecutivePortVectorKey.h"
#include "vtkInformation.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVPostFilterExecutive.h"


vtkStandardNewMacro(vtkPVCompositeDataPipeline);

//----------------------------------------------------------------------------
vtkPVCompositeDataPipeline::vtkPVCompositeDataPipeline()
{

}

//----------------------------------------------------------------------------
vtkPVCompositeDataPipeline::~vtkPVCompositeDataPipeline()
{

}

//----------------------------------------------------------------------------
void vtkPVCompositeDataPipeline::CopyDefaultInformation(
  vtkInformation* request, int direction,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  this->Superclass::CopyDefaultInformation(request, direction,
    inInfoVec, outInfoVec);

  if (request->Has(REQUEST_UPDATE_EXTENT()))
    {
    if (this->GetNumberOfInputPorts() > 0)
      {
      int index = 0;
      //TODO:
      //we need to support multiple inputs
      //and multiple connections on the first input
      vtkInformation *inArrayInfo = this->Algorithm->GetInputArrayInformation(index);
      if (inArrayInfo && inArrayInfo->Has(vtkAlgorithm::INPUT_PORT()))
        {
        vtkExecutive* e = this->GetInputExecutive(0,0);
        //make sure the executive is of the correct type
        vtkPVPostFilterExecutive *pvpfe =
          vtkPVPostFilterExecutive::SafeDownCast(e);
        if(pvpfe)
          {
          pvpfe->SetPostArrayToProcessInformation(index,inArrayInfo);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
