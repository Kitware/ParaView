/*=========================================================================

  Program:   ParaView
  Module:    vtkAnnotateGlobalDataFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAnnotateGlobalDataFilter.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vector>
#include <vtkSmartPointer.h>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkAnnotateGlobalDataFilter);
//----------------------------------------------------------------------------
vtkAnnotateGlobalDataFilter::vtkAnnotateGlobalDataFilter()
{
  this->Prefix = 0;
  this->FieldArrayName = 0;
}

//----------------------------------------------------------------------------
vtkAnnotateGlobalDataFilter::~vtkAnnotateGlobalDataFilter()
{
  this->SetPrefix(0);
  this->SetFieldArrayName(0);
}

//----------------------------------------------------------------------------
int vtkAnnotateGlobalDataFilter::RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkInformation* outInfo = this->GetExecutive()->GetOutputInformation(0);

  // Is it a time dependent field ?
  bool timeDependent = false;
  bool isFieldOnABlock =
      (NULL == input->GetFieldData()->GetArray(this->GetFieldArrayName()));
  if(outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
    int nbTimeSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    vtkDataArray* data = input->GetFieldData()->GetArray(this->GetFieldArrayName());
    if(data)
      {
      int nbFieldsValues = data->GetNumberOfTuples();
      timeDependent = (nbTimeSteps == nbFieldsValues);
      }
    else if(input->IsA("vtkCompositeDataSet"))
      {
      vtkCompositeDataSet* multiBlock = vtkCompositeDataSet::SafeDownCast(input);
      vtkSmartPointer<vtkCompositeDataIterator> iter;
      iter.TakeReference(multiBlock->NewIterator());
      iter->GoToFirstItem();
      while(!iter->IsDoneWithTraversal())
        {
        data = iter->GetCurrentDataObject()->GetFieldData()->GetArray(this->GetFieldArrayName());
        if(data)
          {
          int nbFieldsValues = data->GetNumberOfTuples();
          timeDependent = (nbTimeSteps == nbFieldsValues);
          }
        iter->GoToNextItem();
        }
      }
    }

  // Create the expression based on our local properties
  vtksys_ios::ostringstream expression;
  expression << "\"" << this->GetPrefix() << " %s\" % str("
             << (isFieldOnABlock ? "inputMB[0]" : "input")
             << ".FieldData['" << this->GetFieldArrayName() << "']"
             << "[" << (timeDependent ? "t_index" : "0") << ",0])";
  this->SetPythonExpression(expression.str().c_str());

  return this->Superclass::RequestData(request, inputVector, outputVector);;
}

//----------------------------------------------------------------------------
void vtkAnnotateGlobalDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
