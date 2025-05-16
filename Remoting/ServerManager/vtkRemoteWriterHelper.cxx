// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRemoteWriterHelper.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkClientServerStream.h"
#include "vtkDataObject.h"
#include "vtkImageWriter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkThreadedCallbackQueue.h"
#include "vtksys/SystemTools.hxx"

#include <mutex>
#include <unordered_map>

vtkStandardNewMacro(vtkRemoteWriterHelper);
vtkCxxSetObjectMacro(vtkRemoteWriterHelper, Writer, vtkAlgorithm);
vtkCxxSetObjectMacro(vtkRemoteWriterHelper, Interpreter, vtkClientServerInterpreter);

namespace
{
using FutureContainer = std::unordered_map<std::string,
  std::pair<int, vtkThreadedCallbackQueue::SharedFutureBasePointer>>;

/**
 * This queue collects shared futures produced by the asynchronous callback queue
 * used to write the files that support this feature.
 * One can know that every enqueued files are written if this queue is empty or after calling
 * `Wait()` on each shared future
 * When a file has been written, its shared future in removed from this hash map
 */
FutureContainer SharedFutures;
std::mutex FutureMutex;

//============================================================================
struct FutureWorker
{
  FutureWorker(const std::string& fileName)
    : TimeStamp(Counter++)
    , FileName(fileName)
  {
  }

  void operator()(vtkImageWriter* writer)
  {
    writer->Write();
    std::lock_guard<std::mutex> lock(FutureMutex);
    auto it = SharedFutures.find(vtksys::SystemTools::CollapseFullPath(this->FileName));
    if (it->second.first == this->TimeStamp)
    {
      SharedFutures.erase(it);
    }
  }

  static std::atomic_int Counter;
  int TimeStamp;
  std::string FileName;
};

std::atomic_int FutureWorker::Counter{ 0 };
}

//----------------------------------------------------------------------------
vtkRemoteWriterHelper::vtkRemoteWriterHelper()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
  this->SetInterpreter(vtkClientServerInterpreterInitializer::GetGlobalInterpreter());
}

//----------------------------------------------------------------------------
vtkRemoteWriterHelper::~vtkRemoteWriterHelper()
{
  this->SetWriter(nullptr);
  this->SetInterpreter(nullptr);
}

//-----------------------------------------------------------------------------
void vtkRemoteWriterHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Writer: " << this->Writer << endl;
  os << indent << "OutputDestination: " << this->OutputDestination << endl;
  os << indent << "Interpreter: " << this->Interpreter << endl;
}

//----------------------------------------------------------------------------
int vtkRemoteWriterHelper::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return this->Superclass::FillInputPortInformation(port, info);
}

//----------------------------------------------------------------------------
int vtkRemoteWriterHelper::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  auto session =
    vtkPVSession::SafeDownCast(vtkProcessModule::GetProcessModule()->GetActiveSession());
  const vtkPVSession::ServerFlags roles = session->GetProcessRoles();

  vtkThreadedCallbackQueue* callbackQueue =
    vtkProcessModule::GetProcessModule()->GetCallbackQueue();

  auto writeLocally = [this, callbackQueue](vtkSmartPointer<vtkDataObject>&& input)
  {
    if (!this->TryWritingInBackground)
    {
      this->WriteLocally(input);
    }
    else if (auto imageWriter =
               vtkSmartPointer<vtkImageWriter>(vtkImageWriter::SafeDownCast(this->Writer)))
    {
      this->Writer->SetInputDataObject(std::move(input));
      {
        if (this->GetState() != vtkRemoteWriterHelper::WRITE)
        {
          return;
        }
        ::FutureWorker worker{ imageWriter->GetFileName() };
        // We need to lock guard modifying SharedFutures because the function
        // we are pushing removes its futures from it in an asynchronous way
        std::lock_guard<std::mutex> lock(::FutureMutex);
        auto future = callbackQueue->Push(worker, imageWriter);
        worker.FileName = imageWriter->GetFileName();
        ::SharedFutures.emplace(vtksys::SystemTools::CollapseFullPath(imageWriter->GetFileName()),
          std::make_pair(worker.TimeStamp, future));
      }
    }
    else
    {
      this->WriteLocally(input);
    }
  };

  if (this->OutputDestination != vtkPVSession::CLIENT &&
    this->OutputDestination != vtkPVSession::DATA_SERVER &&
    this->OutputDestination != vtkPVSession::DATA_SERVER_ROOT)
  {
    vtkErrorMacro("Invalid 'OutputDestination' specified: " << this->OutputDestination);
    return 0;
  }

  if (this->OutputDestination == vtkPVSession::CLIENT)
  {
    if ((roles & vtkPVSession::CLIENT) != 0)
    {
      // client (or builtin)
      writeLocally(vtkDataObject::GetData(inputVector[0]));
    }
    else
    {
      // on server-rank; nothing to do.
    }
  }
  else
  {
    if ((roles & vtkPVSession::CLIENT) != 0)
    {
      // client
      auto input = vtkDataObject::GetData(inputVector[0], 0);
      if (auto controller = session->GetController(vtkPVSession::DATA_SERVER_ROOT))
      {
        controller->Send(input, /*remote process id*/ 1, /*tag -- makeup some number*/ 102802);
      }
      else
      {
        // controller is null in built-in mode.
        // not in client-server mode, must be in builtin mode, just write locally.
        writeLocally(input);
      }
    }
    else
    {
      // on server-rank. this can be on any of the data-server ranks or render-server ranks. We
      // only want to write data on data-server-root node.
      if ((roles & vtkPVSession::DATA_SERVER) != 0)
      {
        if (auto controller = session->GetController(vtkPVSession::CLIENT))
        {
          writeLocally(
            vtkSmartPointer<vtkDataObject>::Take(controller->ReceiveDataObject(1, 102802)));
        }
        else
        {
          // controller is null on satellite ranks when running in parallel. nothing to do on this
          // rank.
        }
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkRemoteWriterHelper::Wait(const std::string& fileName)
{
  auto it = ::SharedFutures.find(vtksys::SystemTools::CollapseFullPath(fileName));
  if (it != ::SharedFutures.end())
  {
    it->second.second->Wait();
  }
}

//----------------------------------------------------------------------------
void vtkRemoteWriterHelper::Wait()
{
  std::vector<vtkThreadedCallbackQueue::SharedFutureBasePointer> filenames;
  for (auto& item : ::SharedFutures)
  {
    filenames.push_back(item.second.second);
  }
  vtkProcessModule::GetProcessModule()->GetCallbackQueue()->Wait(filenames);
}

//----------------------------------------------------------------------------
void vtkRemoteWriterHelper::WriteLocally(vtkDataObject* input)
{
  if (this->Writer)
  {
    vtkLogF(TRACE, "Writing file locally using writer %s", vtkLogIdentifier(this->Writer));
    this->Writer->SetInputDataObject(input);
    std::string function;
    switch (this->GetState())
    {
      case vtkRemoteWriterHelper::START:
        function = "Start";
        break;
      case vtkRemoteWriterHelper::WRITE:
        function = "Write";
        break;
      case vtkRemoteWriterHelper::END:
        function = "End";
        break;
      default:
        break;
    }
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << this->Writer << function.c_str()
           << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
    this->Writer->SetInputDataObject(nullptr);
  }
  else
  {
    vtkErrorMacro("No writer specified! Failed to write.");
  }
}

//----------------------------------------------------------------------------
int vtkRemoteWriterHelper::Write()
{
  // always write even if the data hasn't changed
  this->Modified();
  this->Update();
  return 1;
}
