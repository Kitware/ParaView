/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPVPostFilter.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPostFilter.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVPostFilterExecutive.h"

#include "vtkDataArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"

#include "vtkCellDataToPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkArrayCalculator.h"

vtkStandardNewMacro(vtkPVPostFilter);

namespace
{
  enum PropertyConversion
    {
    PointToCell = 1 << 1,
    CellToPoint = 1 << 2,
    AlsoVectorProperty = 1 << 4
    };
  enum VectorConversion
    {
    Component = 1 << 1,
    Magnitude = 1 << 2,
    };
}

//----------------------------------------------------------------------------
vtkPVPostFilter::vtkPVPostFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPVPostFilter::~vtkPVPostFilter()
{
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVPostFilter::CreateDefaultExecutive()
{
  return vtkPVPostFilterExecutive::New();
}
//----------------------------------------------------------------------------
int vtkPVPostFilter::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input)
    {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());
      if (!output || !output->IsA(input->GetClassName()))
        {
        vtkDataObject* newOutput = input->NewInstance();
        newOutput->SetPipelineInformation(info);
        newOutput->Delete();
        }
      }
    return 1;
    }
  return 0;
}


//----------------------------------------------------------------------------
int vtkPVPostFilter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{

  //we need to just copy the data, so we can fixup the output as needed
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* input= inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output= outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (output && input)
    {
    output->ShallowCopy(input);
    if (this->Information->Has(vtkPVPostFilterExecutive::POST_ARRAYS_TO_PROCESS()) )
      {
      this->DoAnyNeededConversions(request,inputVector,outputVector);
      }
    }
  return 1;
}
//----------------------------------------------------------------------------
int vtkPVPostFilter::DoAnyNeededConversions(  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int port = 0;
  vtkInformation *inInfo = inputVector[port]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  //get the array to convert info
  vtkInformationVector* postVector =
    this->Information->Get(vtkPVPostFilterExecutive::POST_ARRAYS_TO_PROCESS());
  vtkInformation *postArrayInfo = postVector->GetInformationObject(0);

  int ret = this->DeterminePointCellConversion(postArrayInfo,input);
  if ( ret & PointToCell )
    {
    vtkPointDataToCellData *converter = vtkPointDataToCellData::New();
    //we need to connect this to our inputs outputport!
    converter->SetInputConnection(this->GetOutputPort(port));
    converter->PassPointDataOn();
    converter->Update();

    output->ShallowCopy(converter->GetOutputDataObject(0));
    converter->Delete();
    }
  else if ( ret & CellToPoint )
    {
    vtkCellDataToPointData *converter = vtkCellDataToPointData::New();
    //we need to connect this to our inputs outputport!
    converter->SetInputConnection(this->GetOutputPort(port));
    converter->PassCellDataOn();
    converter->Update();

    output->ShallowCopy(converter->GetOutputDataObject(0));
    converter->Delete();
    }

  //determine if this a vector to scalar conversion
  if ( ret == 0 || ret & AlsoVectorProperty )
    {
    //the current plan is that we are going to have the name
    //of the property signify what component we should extract
    //so PROPERTY_COMPONENT NAME
    ret = this->DoVectorConversion(postArrayInfo,input,output);
    }

  return (ret != 0);
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::DeterminePointCellConversion(vtkInformation *postArrayInfo,
                                                 vtkDataObject *inputObj)
{
  //determine if this is a point || cell conversion
  int retCode = 0;
  const char* name;
  int fieldAssociation = postArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());
  vtkDataSet *dsInput = vtkDataSet::SafeDownCast(inputObj);

  if (dsInput && fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    name = postArrayInfo->Get(vtkDataObject::FIELD_NAME());
    if (!dsInput->GetPointData()->HasArray(name) &&
      dsInput->GetCellData()->HasArray(name))
      {
      retCode |= CellToPoint;
      if ( dsInput->GetCellData()->GetArray(name)->GetNumberOfTuples() > 1 )
        {
        retCode |= AlsoVectorProperty;
        }
      }

    }
  else if(dsInput&&fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    name = postArrayInfo->Get(vtkDataObject::FIELD_NAME());
    if (!dsInput->GetCellData()->HasArray(name) &&
      dsInput->GetPointData()->HasArray(name))
      {
      retCode |= PointToCell;
      if ( dsInput->GetPointData()->GetArray(name)->GetNumberOfTuples() > 1 )
        {
        retCode |= AlsoVectorProperty;
        }
      }
    }
  return retCode;
}

//----------------------------------------------------------------------------
int vtkPVPostFilter::DoVectorConversion(vtkInformation *postArrayInfo,
                                      vtkDataObject *input,vtkDataObject *output)
{
  const char *mangledName, *name, *keyName;
  mangledName = postArrayInfo->Get(vtkDataObject::FIELD_NAME());
  keyName = postArrayInfo->Get(vtkPVPostFilterExecutive::POST_ARRAY_COMPONENT_KEY());
  if ( !keyName || !mangledName )
    {
    return 0;
    }
  std::string key(keyName);
  std::string tempName(mangledName);
  size_t found;
  found = tempName.rfind(key);
  if (found == std::string::npos)
    {
    //no vector conversion needed
    return 0;
    }

  //lets grab the array now
  std::string compIndex = tempName.substr(found+1,tempName.size());
  tempName = tempName.substr(0,found);
  name = tempName.c_str();
  int fieldAssociation = postArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());
  vtkDataArray *vectorProp = NULL;
  vtkDataSet *dsInput = vtkDataSet::SafeDownCast(input);
  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    if (dsInput->GetPointData()->HasArray(name))
      {
      vectorProp = dsInput->GetPointData()->GetArray(name);
      }
    }
  else if(fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    if (dsInput->GetCellData()->HasArray(name))
      {
      vectorProp = dsInput->GetCellData()->GetArray(name);
      }
    }

  if(!vectorProp)
    {
    //we failed to find the property
    return 0;
    }


  //convert the component index
  int cIndex = atoi(compIndex.c_str());
  //the resulting name will support component names
  std::string resultName = tempName;
  const char* cn =  vectorProp->GetComponentName(cIndex);
  if (cn)
    {
    resultName += cn;
    }
  else
    {
    resultName += compIndex;
    }

  vtkArrayCalculator *calc = vtkArrayCalculator::New();
  calc->SetInputConnection(0,this->GetInputConnection(0,0));
  if ( fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS )
    {
    calc->SetAttributeModeToUsePointData();
    }
  else if ( fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS )
    {
    calc->SetAttributeModeToUseCellData();
    }
  calc->AddScalarVariable(mangledName,name,cIndex);
  calc->SetFunction(mangledName);
  calc->SetResultArrayName(resultName.c_str());

  output->ShallowCopy(calc->GetOutputDataObject(0));
  calc->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVPostFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
