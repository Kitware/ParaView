/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientServerMoveData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClientServerMoveData.h"

#include "vtkCharArray.h"
#include "vtkClientConnection.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTypes.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkGenericDataObjectWriter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkServerConnection.h"
#include "vtkSocketController.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkClientServerMoveData);
vtkCxxRevisionMacro(vtkClientServerMoveData, "1.6");
vtkCxxSetObjectMacro(vtkClientServerMoveData, ProcessModuleConnection, 
  vtkProcessModuleConnection);

//-----------------------------------------------------------------------------
vtkClientServerMoveData::vtkClientServerMoveData()
{
  this->ProcessModuleConnection = 0;
  this->OutputDataType = VTK_POLY_DATA;
}

//-----------------------------------------------------------------------------
vtkClientServerMoveData::~vtkClientServerMoveData()
{
  this->SetProcessModuleConnection(0);
}

//-----------------------------------------------------------------------------
int vtkClientServerMoveData::FillInputPortInformation(int idx, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return this->Superclass::FillInputPortInformation(idx, info);
}

//----------------------------------------------------------------------------
int vtkClientServerMoveData::RequestDataObject(
  vtkInformation* vtkNotUsed(reqInfo), 
  vtkInformationVector**, 
  vtkInformationVector* outputVector)
{
  const char *outTypeStr = 
    vtkDataObjectTypes::GetClassNameFromTypeId(this->OutputDataType);

  // for each output
  for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
    {
    vtkInformation* info = outputVector->GetInformationObject(i);
    vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());
    if (!output || !output->IsA(outTypeStr)) 
      {
      vtkDataObject* newOutput = 
        vtkDataObjectTypes::NewDataObject(this->OutputDataType);
      if (!newOutput)
        {
        vtkErrorMacro("Could not create chosen output data type: "
                      << outTypeStr);
        return 0;
        }
      newOutput->SetPipelineInformation(info);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
      newOutput->Delete();
      }
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkClientServerMoveData::RequestData(vtkInformation*,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* input = 0;
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
    {
    input = inputVector[0]->GetInformationObject(0)->Get(
        vtkDataObject::DATA_OBJECT());
    }

  vtkRemoteConnection* rc = vtkRemoteConnection::SafeDownCast(
    this->ProcessModuleConnection);
  if (rc)
    {
    vtkSocketController* controller = rc->GetSocketController();
    if (this->ProcessModuleConnection->IsA("vtkClientConnection"))
      {
      vtkDebugMacro("Server Root: Send input data to client.");
      // This is a server root node.
      return this->SendData(controller, input);
      }
    else if (this->ProcessModuleConnection->IsA("vtkServerConnection"))
      {
      vtkDebugMacro("Client: Get data from server and put it on the output.");
      // This is a client node.
      vtkDataObject* data = this->ReceiveData(controller);
      if (data)
        {
        if (output->IsA(data->GetClassName()))
          {
          output->ShallowCopy(data);
          }
        else
          {
          data->SetPipelineInformation(outputVector->GetInformationObject(0));
          }
        data->Delete();
        return 1;
        }
      }
    }

  vtkDebugMacro("Shallow copying input to output.");
  // If not a remote connection, nothing more to do than
  // act as a pass through filter.
  output->ShallowCopy(input);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkClientServerMoveData::SendData(vtkSocketController* controller,
  vtkDataObject* in_data)
{
  vtkDataObject* data = in_data->NewInstance();
  data->ShallowCopy(in_data);

  vtkGenericDataObjectWriter* writer = vtkGenericDataObjectWriter::New();
  writer->SetInput(data);
  writer->SetFileTypeToBinary();
  writer->WriteToOutputStringOn();
  writer->Write();


  int data_length = writer->GetOutputStringLength();
  char* raw_data = writer->RegisterAndGetOutputString();
  writer->Delete();
  data->Delete();

  controller->Send(&data_length, 1, 1, 
    vtkClientServerMoveData::COMMUNICATION_DATA_LENGTH);
  controller->Send(raw_data, data_length, 1, 
    vtkClientServerMoveData::COMMUNICATION_DATA);
  delete []raw_data;
  return 1;
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkClientServerMoveData::ReceiveData(vtkSocketController* controller)
{
  int data_length = 0;
  controller->Receive(&data_length, 1, 1, 
    vtkClientServerMoveData::COMMUNICATION_DATA_LENGTH);
  char* raw_data = new char[data_length + 10];
  controller->Receive(raw_data, data_length, 1,
    vtkClientServerMoveData::COMMUNICATION_DATA);

  vtkGenericDataObjectReader* reader= vtkGenericDataObjectReader::New();
  reader->ReadFromInputStringOn();
  vtkCharArray* string_data = vtkCharArray::New();
  string_data->SetArray(raw_data, data_length, 1);
  reader->SetInputArray(string_data);
  reader->Update();

  vtkDataObject* output = reader->GetOutput()->NewInstance();
  output->ShallowCopy(reader->GetOutput());

  reader->Delete();
  string_data->Delete();
  delete []raw_data;

  return output;
}

//-----------------------------------------------------------------------------
void vtkClientServerMoveData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ProcessModuleConnection: " << this->ProcessModuleConnection
    << endl;
}
