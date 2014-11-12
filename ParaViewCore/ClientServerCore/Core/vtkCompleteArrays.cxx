/*=========================================================================

  Program:   ParaView
  Module:    vtkCompleteArrays.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompleteArrays.h"

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkClientServerStream.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkShortArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

vtkStandardNewMacro(vtkCompleteArrays);

vtkCxxSetObjectMacro(vtkCompleteArrays,Controller,vtkMultiProcessController);

//-----------------------------------------------------------------------------
vtkCompleteArrays::vtkCompleteArrays()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
  if (this->Controller)
    {
    this->Controller->Register(this);
    }
}

//-----------------------------------------------------------------------------
vtkCompleteArrays::~vtkCompleteArrays()
{
  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    this->Controller = NULL;
    }
}

//-----------------------------------------------------------------------------
int vtkCompleteArrays::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Initialize
  //
  vtkDebugMacro(<<"Completing array");

  output->CopyStructure( input );
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  if(this->Controller->GetNumberOfProcesses() <= 1)
    {
    return 1;
    }
  int myProcId = this->Controller->GetLocalProcessId();

  // array is [num points on proc 0, num cells on proc 0,
  //           num points on this proc, num cells on this proc,
  //           proc id if this process has point,
  //           proc id if this process has cells]

  vtkIdType localInfo[6] = {-1, -1, input->GetNumberOfPoints(),
                           input->GetNumberOfCells(), -1, -1};
  if(myProcId == 0)
    {
    localInfo[0] = input->GetNumberOfPoints();
    localInfo[1] = input->GetNumberOfCells();
    }
  if(input->GetNumberOfPoints() > 0)
    {
    localInfo[4] = myProcId;
    }
  if(input->GetNumberOfCells() > 0)
    {
    localInfo[5] = myProcId;
    }
  vtkIdType globalInfo[6];
  this->Controller->AllReduce(localInfo, globalInfo, 6, vtkCommunicator::MAX_OP);

  // Idea is that if process 0 doesn't have the proper point data and cell data
  // arrays (if cells exist) then we need to get that information from another proc.
  // We only need to get that information from one process so we look for the highest
  // process id with cells (if they exist) or points (if no cells exist) to
  // provide that information.
  if( globalInfo[1] > 0 || (globalInfo[3] == 0 && globalInfo[0] > 0) ||
      globalInfo[2] == 0 )
    {
    // process 0 has all of the needed information already (globalInfo[2] == 0
    // means there is no information at all)
    return 1;
    }

  int infoProc = globalInfo[5]; // a process that has the information proc 0 needs
  if(globalInfo[3] == 0)
    { // there are no cells so we find a process with points
    infoProc = globalInfo[4];
    }

  if (myProcId == 0)
    {
    // Receive and collected information from the remote processes.
    vtkPVDataInformation* dataInfo = vtkPVDataInformation::New();
    vtkPVDataInformation* tmpInfo = vtkPVDataInformation::New();

    int length = 0;
    this->Controller->Receive(&length, 1, infoProc, 389002);
    unsigned char* data = new unsigned char[length];
    this->Controller->Receive(data, length, infoProc, 389003);
    vtkClientServerStream css;
    css.SetData(data, length);
    tmpInfo->CopyFromStream(&css);
    delete [] data;
    dataInfo->AddInformation(tmpInfo);

    this->FillArrays(
      output->GetPointData(), dataInfo->GetPointDataInformation());
    this->FillArrays(
      output->GetCellData(), dataInfo->GetCellDataInformation());
    vtkPointSet* ps = vtkPointSet::SafeDownCast(output);
    if (ps)
      {
      vtkDataArray* pointArray = this->CreateArray(
        dataInfo->GetPointArrayInformation());
      if (!pointArray)
        {
        vtkErrorMacro("Could not create point array. "
                      "The output will not contain points.");
        }
      else
        {
        vtkPoints* pts = vtkPoints::New();
        pts->SetData(pointArray);
        pointArray->Delete();
        ps->SetPoints(pts);
        pts->Delete();
        }
      }
    dataInfo->Delete();
    tmpInfo->Delete();
    }
  else if(myProcId == infoProc)
    {
    vtkClientServerStream css;
    vtkPVDataInformation* dataInfo = vtkPVDataInformation::New();
    dataInfo->SetSortArrays(0);
    dataInfo->CopyFromObject(output);
    dataInfo->CopyToStream(&css);
    size_t length;
    const unsigned char* data;
    css.GetData(&data, &length);
    int len = static_cast<int>(length);
    this->Controller->Send(&len, 1, 0, 389002);
    this->Controller->Send(const_cast<unsigned char*>(data), len, 0, 389003);
    dataInfo->Delete();
    }

  return 1;
}

//-----------------------------------------------------------------------------
vtkDataArray* vtkCompleteArrays::CreateArray(vtkPVArrayInformation* aInfo)
{
  vtkDataArray* array = 0;
  switch (aInfo->GetDataType())
    {
    case VTK_FLOAT:
      array = vtkFloatArray::New();
      break;
    case VTK_DOUBLE:
      array = vtkDoubleArray::New();
      break;
    case VTK_INT:
      array = vtkIntArray::New();
      break;
    case VTK_CHAR:
      array = vtkCharArray::New();
      break;
    case VTK_ID_TYPE:
      array = vtkIdTypeArray::New();
      break;
    case VTK_LONG:
      array = vtkLongArray::New();
      break;
    case VTK_SHORT:
      array = vtkShortArray::New();
      break;
    case VTK_UNSIGNED_CHAR:
      array = vtkUnsignedCharArray::New();
      break;
    case VTK_UNSIGNED_INT:
      array = vtkUnsignedIntArray::New();
      break;
    case VTK_UNSIGNED_LONG:
      array = vtkUnsignedLongArray::New();
      break;
    case VTK_UNSIGNED_SHORT:
      array = vtkUnsignedShortArray::New();
      break;
    default:
      array = NULL;
    }
  if (array)
    {
    array->SetNumberOfComponents(aInfo->GetNumberOfComponents());
    array->SetName(aInfo->GetName());
    }

  return array;

}

//-----------------------------------------------------------------------------
void vtkCompleteArrays::FillArrays(vtkDataSetAttributes* da, 
                                   vtkPVDataSetAttributesInformation* attrInfo)
{
  int num, idx;
  vtkPVArrayInformation* arrayInfo;
  vtkDataArray* array;

  da->Initialize();
  num = attrInfo->GetNumberOfArrays();
  for (idx = 0; idx < num; ++idx)
    {
    arrayInfo = attrInfo->GetArrayInformation(idx);
    array = this->CreateArray(arrayInfo);
    if (array)
      {
      switch (attrInfo->IsArrayAnAttribute(idx))
        {
        case vtkDataSetAttributes::SCALARS:
          da->SetScalars(array);
          break;
        case vtkDataSetAttributes::VECTORS:
          da->SetVectors(array);
          break;
        case vtkDataSetAttributes::NORMALS:
          da->SetNormals(array);
          break;
        case vtkDataSetAttributes::TENSORS:
          da->SetTensors(array);
          break;
        case vtkDataSetAttributes::TCOORDS:
          da->SetTCoords(array);
          break;
        default:
          da->AddArray(array);
        }
      array->Delete();
      array = NULL;
      }
    }
}


//-----------------------------------------------------------------------------
void vtkCompleteArrays::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->Controller)
    {
    os << indent << "Controller: " << this->Controller << endl;
    }
  else
    {
    os << indent << "Controller: (none)\n";
    }
}
