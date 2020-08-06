#include "vtkNetworkImageWriter.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkClientServerStream.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"

#include <cstring>

#ifndef FILENAME_MAX
#define FILENAME_MAX 255
#endif

vtkStandardNewMacro(vtkNetworkImageWriter);

//----------------------------------------------------------------------------
vtkNetworkImageWriter::vtkNetworkImageWriter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->SetInterpreter(vtkClientServerInterpreterInitializer::GetGlobalInterpreter());
}

//----------------------------------------------------------------------------
vtkNetworkImageWriter::~vtkNetworkImageWriter()
{
}

//-----------------------------------------------------------------------------
void vtkNetworkImageWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkNetworkImageWriter, Writer, vtkAlgorithm);

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkNetworkImageWriter, Interpreter, vtkClientServerInterpreter);

//----------------------------------------------------------------------------
int vtkNetworkImageWriter::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return this->Superclass::FillInputPortInformation(port, info);
}

//------------------------------------------------------------------------------
int vtkNetworkImageWriter::FillOutputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return this->Superclass::FillOutputPortInformation(port, info);
}

//----------------------------------------------------------------------------
int vtkNetworkImageWriter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto session =
    vtkPVSession::SafeDownCast(vtkProcessModule::GetProcessModule()->GetActiveSession());
  const vtkPVSession::ServerFlags roles = session->GetProcessRoles();

  if (this->OutputDestination == CLIENT)
  {
    if ((roles & vtkPVSession::CLIENT) != 0)
    {
      // client (or builtin)
      auto input = vtkDataObject::GetData(inputVector[0], 0);
      this->WriteLocally(input);
      return 1;
    }
    else
    {
      // on server-rank; nothing to do.
      return 1;
    }
  }
  else if (this->OutputDestination == SERVER)
  {
    if ((roles & vtkPVSession::CLIENT) != 0)
    {
      // client
      auto input = vtkImageData::GetData(inputVector[0], 0);
      if (auto controller = session->GetController(vtkPVSession::DATA_SERVER_ROOT))
      {
        controller->Send(input, /*remote process id*/ 1, /*tag -- makeup some number*/ 102802);
      }
      else
      {
        // controller is null in built-in mode.
        // not in client-server mode, must be in builtin mode, just write locally.
        this->WriteLocally(input);
      }
      return 1;
    }
    else
    {
      // on server-rank. this can be on any of the data-server ranks or render-server ranks. We
      // only want to write data on data-server-root node.
      vtkPVSession::ServerFlags roles = session->GetProcessRoles();
      if ((roles & vtkPVSession::DATA_SERVER) != 0)
      {
        if (auto controller = session->GetController(vtkPVSession::CLIENT))
        {
          vtkNew<vtkImageData> data;
          controller->Receive(data, /*remote process id*/ 1, /*tag -- same as Send()*/ 102802);
          this->WriteLocally(data);
        }
        else
        {
          // controller is null on satellite ranks when running in parallel. nothing to do on this
          // rank.
        }
      }
      return 1;
    }
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
void vtkNetworkImageWriter::WriteLocally(vtkDataObject* input)
{
  if (this->Writer)
  {
    this->Writer->SetInputDataObject(input);

    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << this->Writer << "Write"
           << vtkClientServerStream::End;

    this->Interpreter->ProcessStream(stream);
  }
}
