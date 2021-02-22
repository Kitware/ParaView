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
#include "vtkDataObject.h"
#include "vtkDataObjectTypes.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkGenericDataObjectWriter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkPolyData.h"
#include "vtkProcessModule.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <sstream>

vtkStandardNewMacro(vtkClientServerMoveData);
vtkCxxSetObjectMacro(vtkClientServerMoveData, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkClientServerMoveData::vtkClientServerMoveData()
{
  this->OutputDataType = VTK_POLY_DATA;
  this->WholeExtent[0] = 0;
  this->WholeExtent[1] = -1;
  this->WholeExtent[2] = 0;
  this->WholeExtent[3] = -1;
  this->WholeExtent[4] = 0;
  this->WholeExtent[5] = -1;
  this->Controller = nullptr;
  this->ProcessType = AUTO;
}

//-----------------------------------------------------------------------------
vtkClientServerMoveData::~vtkClientServerMoveData()
{
  this->SetController(nullptr);
}

//-----------------------------------------------------------------------------
int vtkClientServerMoveData::FillInputPortInformation(int idx, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return this->Superclass::FillInputPortInformation(idx, info);
}

//----------------------------------------------------------------------------
int vtkClientServerMoveData::RequestDataObject(
  vtkInformation* vtkNotUsed(reqInfo), vtkInformationVector**, vtkInformationVector* outputVector)
{
  const char* outTypeStr = vtkDataObjectTypes::GetClassNameFromTypeId(this->OutputDataType);

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());
  if (!output || !output->IsA(outTypeStr))
  {
    vtkDataObject* newOutput = vtkDataObjectTypes::NewDataObject(this->OutputDataType);
    if (!newOutput)
    {
      vtkErrorMacro("Could not create chosen output data type: " << outTypeStr);
      return 0;
    }
    info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    this->GetOutputPortInformation(0)->Set(
      vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
    newOutput->Delete();
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkClientServerMoveData::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
  {
    return this->Superclass::RequestInformation(request, inputVector, outputVector);
  }
  else
  {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent, 6);
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkClientServerMoveData::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* input = nullptr;
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
  {
    input = inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
  }

  vtkMultiProcessController* controller = this->Controller;
  int processType = this->ProcessType;
  if (this->ProcessType == AUTO)
  {
    vtkPVSession* session =
      vtkPVSession::SafeDownCast(vtkProcessModule::GetProcessModule()->GetSession());
    if (!session)
    {
      vtkErrorMacro("No active ParaView session");
      return 0;
    }
    if (vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_CLIENT)
    {
      controller = session->GetController(vtkPVSession::DATA_SERVER);
      processType = CLIENT;
    }
    else
    {
      controller = session->GetController(vtkPVSession::CLIENT);
      processType = SERVER;
    }
  }

  if (controller)
  {
    bool is_server = (processType == SERVER);
    bool is_client = (processType == CLIENT);
    if (is_server)
    {
      vtkDebugMacro("Server Root: Send input data to client.");
      output->ShallowCopy(input);
      return this->SendData(input, controller);
    }
    else if (is_client)
    {
      vtkDebugMacro("Client: Get data from server and put it on the output.");
      // This is a client node.
      // If it is a selection, use the XML serializer.
      // Otherwise, use the communicator.

      vtkDataObject* data = this->ReceiveData(controller);
      if (data)
      {
        if (output->IsA(data->GetClassName()))
        {
          output->ShallowCopy(data);
        }
        else
        {
          outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), data);
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
int vtkClientServerMoveData::SendData(vtkDataObject* input, vtkMultiProcessController* controller)
{
  // This is a server root node.
  // If it is a selection, use the XML serializer.
  // Otherwise, use the communicator.
  if (this->OutputDataType == VTK_SELECTION)
  {
    // Convert to XML.
    vtkSelection* sel = vtkSelection::SafeDownCast(input);
    if (!sel)
    {
      int size = 0;
      return controller->Send(&size, 1, 1, vtkClientServerMoveData::TRANSMIT_DATA_OBJECT);
    }
    else
    {
      std::ostringstream res;
      vtkSelectionSerializer::PrintXML(res, vtkIndent(), 1, sel);

      // Send the size of the string.
      int size = static_cast<int>(res.str().size());
      controller->Send(&size, 1, 1, vtkClientServerMoveData::TRANSMIT_DATA_OBJECT);
      // Send the XML string.
      return controller->Send(
        res.str().c_str(), size, 1, vtkClientServerMoveData::TRANSMIT_DATA_OBJECT);
    }
  }

  return controller->Send(input, 1, vtkClientServerMoveData::TRANSMIT_DATA_OBJECT);
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkClientServerMoveData::ReceiveData(vtkMultiProcessController* controller)
{
  vtkDataObject* data = nullptr;
  if (this->OutputDataType == VTK_SELECTION)
  {
    // Get the size of the string.
    int size = 0;
    controller->Receive(&size, 1, 1, vtkClientServerMoveData::TRANSMIT_DATA_OBJECT);
    if (size == 0)
    {
      return nullptr;
    }
    char* xml = new char[size + 1];
    // Get the string itself.
    controller->Receive(xml, size, 1, vtkClientServerMoveData::TRANSMIT_DATA_OBJECT);
    xml[size] = 0;

    // Parse the XML.
    vtkSelection* sel = vtkSelection::New();
    vtkSelectionSerializer::Parse(xml, sel);
    delete[] xml;
    data = sel;
  }
  else
  {
    data = controller->ReceiveDataObject(1, vtkClientServerMoveData::TRANSMIT_DATA_OBJECT);
  }
  return data;
}

//-----------------------------------------------------------------------------
void vtkClientServerMoveData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "WholeExtent: " << this->WholeExtent[0] << ", " << this->WholeExtent[1] << ", "
     << this->WholeExtent[2] << ", " << this->WholeExtent[3] << ", " << this->WholeExtent[4] << ", "
     << this->WholeExtent[5] << endl;
  os << indent << "OutputDataType: " << this->OutputDataType << endl;
  os << indent << "ProcessType: " << this->ProcessType << endl;
  os << indent << "Controller: " << this->Controller << endl;
}
