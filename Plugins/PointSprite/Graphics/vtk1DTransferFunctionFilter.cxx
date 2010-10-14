/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk1DTransferFunctionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtk1DTransferFunctionFilter
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#include "vtk1DTransferFunctionFilter.h"

#include "vtkObjectFactory.h"
#include "vtk1DTransferFunctionChooser.h"
#include "vtkDataArray.h"
#include "vtkType.h"
#include "vtkInformation.h"
#include "vtkDataObject.h"
#include "vtkAbstractArray.h"
#include "vtkInformationVector.h"
#include "vtkFieldData.h"
#include "vtkTable.h"
#include "vtkDataSet.h"
#include "vtkGraph.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include <sstream>


vtkStandardNewMacro(vtk1DTransferFunctionFilter)

vtkCxxSetObjectMacro(vtk1DTransferFunctionFilter, TransferFunction, vtk1DTransferFunction)

vtk1DTransferFunctionFilter::vtk1DTransferFunctionFilter()
{
  this->TransferFunction = vtk1DTransferFunctionChooser::New();
  this->Enabled = true;
  this->OutputArrayName = NULL;
  this->OutputArrayType = VTK_DOUBLE;
  this->ForceSameTypeAsInputArray = 1;
  this->ConcatenateOutputNameWithInput = 0;
}

vtk1DTransferFunctionFilter::~vtk1DTransferFunctionFilter()
{
  this->SetTransferFunction(NULL);
  this->SetOutputArrayName(NULL);
}

int vtk1DTransferFunctionFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

// Description:
// This is called by the superclass.
// This is the method you should override.
int vtk1DTransferFunctionFilter::RequestData(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  // check valid output
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (!output)
    {
    return 0;
    }

  // check input type
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  output->ShallowCopy(input);

  if (!this->Enabled)
    {
    return 1;
    }

  // Get the input array to map
  vtkDataArray* inArray = this->GetInputArrayToProcess(0, input);
  if(!inArray)
    {
    return 0;
    }

  // create the output array and add it to the output
  vtkDataArray* outArray;
  if(this->ForceSameTypeAsInputArray)
    {
    outArray = inArray->NewInstance();
    }
  else
    {
    outArray = vtkDataArray::SafeDownCast(vtkAbstractArray::CreateArray(this->OutputArrayType));
    }

  // set the name of the mapped array
  vtkstd::ostringstream sstr;
  if(this->ConcatenateOutputNameWithInput)
    {
    sstr << inArray->GetName() << this->OutputArrayName;
    }
  else
    {
    sstr << this->OutputArrayName;
    }
  outArray->SetName(sstr.str().data());

  int added = this->SetOutputArray(output, outArray);
  outArray->Delete();

  if(!added)
    {
    vtkErrorMacro("impossible to add the mapped array to the output, aborting");
    return 0;
    }

  // map the array
  this->TransferFunction->MapArray(inArray, outArray);

  return 1;
}

unsigned long vtk1DTransferFunctionFilter::GetMTime()
{
  unsigned long tfmtime = 0;
  unsigned long supermtime = 0;

  if(this->TransferFunction != NULL)
    {
    tfmtime = this->TransferFunction->GetMTime();
    }

  supermtime = this->Superclass::GetMTime();

  return (tfmtime > supermtime ? tfmtime : supermtime);
}

int vtk1DTransferFunctionFilter::SetOutputArray(vtkDataObject* output,
    vtkDataArray* array)
{
  if (!output)
    {
    return 0;
    }

  vtkInformationVector *inArrayVec = this->Information->Get(
      INPUT_ARRAYS_TO_PROCESS());
  if (!inArrayVec)
    {
    vtkErrorMacro
    ("Attempt to get an input array for an index that has not been specified");
    return 0;
    }
  vtkInformation *inArrayInfo = inArrayVec->GetInformationObject(0);
  if (!inArrayInfo)
    {
    vtkErrorMacro
    ("Attempt to get an input array for an index that has not been specified");
    return 0;
    }

  int fieldAssoc = inArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());

  if (inArrayInfo->Has(vtkDataObject::FIELD_NAME()))
    {
    if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_NONE)
      {
      vtkFieldData *fd = output->GetFieldData();
      fd->AddArray(array);
      return 1;
      }

    if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_ROWS)
      {
      vtkTable *outputT = vtkTable::SafeDownCast(output);
      if (!outputT)
        {
        vtkErrorMacro("Attempt to get row data from a non-table");
        return 0;
        }
      vtkFieldData *fd = outputT->GetRowData();
      fd->AddArray(array);
      return 1;
      }

    if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_VERTICES || fieldAssoc
        == vtkDataObject::FIELD_ASSOCIATION_EDGES)
      {
      vtkGraph *outputG = vtkGraph::SafeDownCast(output);
      if (!outputG)
        {
        vtkErrorMacro("Attempt to get vertex or edge data from a non-graph");
        return 0;
        }
      vtkFieldData *fd = 0;
      if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_VERTICES)
        {
        fd = outputG->GetVertexData();
        }
      else
        {
        fd = outputG->GetEdgeData();
        }
      fd->AddArray(array);
      return 1;
      }

    if (vtkGraph::SafeDownCast(output) && fieldAssoc
        == vtkDataObject::FIELD_ASSOCIATION_POINTS)
      {
      vtkGraph::SafeDownCast(output)-> GetVertexData()->AddArray(array);
      return 1;
      }

    vtkDataSet *outputDS = vtkDataSet::SafeDownCast(output);
    if (!outputDS)
      {
      vtkErrorMacro("Attempt to get point or cell data from a data object");
      return 0;
      }

    if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_POINTS)
      {
      outputDS->GetPointData()->AddArray(array);
      return 1;
      }
    if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
      {
      outputDS->GetPointData()->AddArray(array);
      return 1;
      }

    outputDS->GetCellData()->AddArray(array);
    }
  else
    {
    vtkDataSet *outputDS = vtkDataSet::SafeDownCast(output);
    if (!outputDS)
      {
      vtkErrorMacro("Attempt to get point or cell data from a data object");
      return 0;
      }
    //int fType = inArrayInfo->Get(vtkDataObject::FIELD_ATTRIBUTE_TYPE());
    if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_POINTS)
      {
      outputDS->GetPointData()->AddArray(array);
      }
    if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
      {
      outputDS->GetPointData()->AddArray(array);
      return 1;
      }

    outputDS->GetCellData()->AddArray(array);
    return 1;
    }
  return 0;
}

void    vtk1DTransferFunctionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Enabled " << this->Enabled << endl;
  os << indent << "OutputArrayName " << this->OutputArrayName << endl;
  os << indent << "ConcatenateOutputNameWithInput " << this->ConcatenateOutputNameWithInput << endl;
  os << indent << "OutputArrayType " << this->OutputArrayType << endl;
  os << indent << "ForceSameTypeAsInputArray " << this->ForceSameTypeAsInputArray << endl;
  os << indent << "TransferFunction " << this->TransferFunction << endl;
  if(this->TransferFunction)
    {
    this->TransferFunction->PrintSelf(os, indent.GetNextIndent());
    }
}

