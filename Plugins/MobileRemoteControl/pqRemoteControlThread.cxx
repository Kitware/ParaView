/*=========================================================================

   Program: ParaView
   Module:    pqRemoteControlThread.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqRemoteControlThread.h"

#include <vtkClientSocket.h>
#include <vtkNew.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkServerSocket.h>
#include <vtkSmartPointer.h>
#include <vtkSocketCollection.h>
#include <vtkWebGLExporter.h>
#include <vtkWebGLObject.h>

#include <QMutex>
#include <QWaitCondition>

//-----------------------------------------------------------------------------
class pqRemoteControlThread::pqInternal
{
public:
  pqInternal()
  {
    this->ShouldQuit = false;
    this->NewCameraState = false;
  }

  vtkSmartPointer<vtkClientSocket> Socket;
  vtkSmartPointer<vtkServerSocket> ServerSocket;
  vtkSmartPointer<vtkSocketCollection> SocketCollection;

  vtkSmartPointer<vtkWebGLExporter> Exporter;
  CameraStateStruct CameraState;

  bool NewCameraState;
  bool ShouldQuit;
  QMutex Lock;
  QWaitCondition WaitCondition;
};

//-----------------------------------------------------------------------------
pqRemoteControlThread::pqRemoteControlThread()
{
  this->Internal = new pqInternal;
}

//-----------------------------------------------------------------------------
pqRemoteControlThread::~pqRemoteControlThread()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
bool pqRemoteControlThread::createServer(int port)
{
  this->Internal->ServerSocket = vtkSmartPointer<vtkServerSocket>::New();
  return (this->Internal->ServerSocket->CreateServer(port) == 0);
}

//-----------------------------------------------------------------------------
bool pqRemoteControlThread::serverIsOpen()
{
  return (this->Internal->ServerSocket != 0);
}

//-----------------------------------------------------------------------------
bool pqRemoteControlThread::clientIsConnected()
{
  return (this->Internal->Socket != 0);
}

//-----------------------------------------------------------------------------
bool pqRemoteControlThread::checkForConnection()
{
  if (!this->Internal->ServerSocket)
  {
    return false;
  }

  vtkClientSocket* socket = this->Internal->ServerSocket->WaitForConnection(1);
  if (!socket)
  {
    return false;
  }

  this->Internal->ServerSocket = NULL;
  this->Internal->Socket = socket;
  socket->Delete();

  this->Internal->SocketCollection = vtkSmartPointer<vtkSocketCollection>::New();
  this->Internal->SocketCollection->AddItem(this->Internal->Socket);
  return true;
}

//-----------------------------------------------------------------------------
void pqRemoteControlThread::close()
{
  QMutexLocker locker(&this->Internal->Lock);
  this->Internal->ShouldQuit = true;
  this->Internal->ServerSocket = NULL;
  this->Internal->Socket = NULL;
  this->Internal->SocketCollection = NULL;
  this->Internal->Exporter = NULL;
}

//-----------------------------------------------------------------------------
void pqRemoteControlThread::shouldQuit()
{
  QMutexLocker locker(&this->Internal->Lock);
  this->Internal->ShouldQuit = true;
}

//-----------------------------------------------------------------------------
void pqRemoteControlThread::exportSceneOnMainThread()
{
  this->Internal->Lock.lock();
  emit this->requestExportScene();
  this->Internal->WaitCondition.wait(&this->Internal->Lock);
  this->Internal->Lock.unlock();
}

//-----------------------------------------------------------------------------
void pqRemoteControlThread::exportScene(vtkRenderWindow* renderWindow)
{
  if (renderWindow)
  {
    if (!this->Internal->Exporter)
    {
      this->Internal->Exporter = vtkSmartPointer<vtkWebGLExporter>::New();
    }
    this->Internal->Exporter->parseScene(renderWindow->GetRenderers(), "\"view\"", VTK_PARSEALL);
  }
  else
  {
    this->Internal->Exporter = 0;
  }
  this->Internal->WaitCondition.wakeOne();
}

//-----------------------------------------------------------------------------
bool pqRemoteControlThread::sendSceneInfo()
{
  const char* metadata =
    this->Internal->Exporter ? this->Internal->Exporter->GenerateMetadata() : 0;
  unsigned long long length = metadata ? strlen(metadata) : 0;

  if (this->Internal->Socket->Send(&length, sizeof(length)) == 0)
  {
    return false;
  }

  if (this->Internal->Socket->Send(metadata, length) == 0)
  {
    return false;
  }

  return (this->Internal->ShouldQuit != true);
}

//-----------------------------------------------------------------------------
bool pqRemoteControlThread::sendObjects()
{
  if (!this->Internal->Exporter)
  {
    return (this->Internal->ShouldQuit != true);
  }

  for (int i = 0; i < this->Internal->Exporter->GetNumberOfObjects(); ++i)
  {
    vtkWebGLObject* obj = this->Internal->Exporter->GetWebGLObject(i);

    for (int partIndex = 0; partIndex < obj->GetNumberOfParts(); ++partIndex)
    {

      bool skip = false;
      if (this->Internal->Socket->Receive(&skip, 1) == 0)
      {
        return false;
      }

      if (skip)
      {
        continue;
      }

      unsigned long long length = obj->GetBinarySize(partIndex);
      if (this->Internal->Socket->Send(&length, sizeof(length)) == 0)
      {
        return false;
      }
      if (this->Internal->Socket->Send(obj->GetBinaryData(partIndex), length) == 0)
      {
        return false;
      }
      if (this->Internal->ShouldQuit)
      {
        return false;
      }
    }
  }

  return (this->Internal->ShouldQuit != true);
}

//-----------------------------------------------------------------------------
bool pqRemoteControlThread::receiveCameraState()
{

  CameraStateStruct camState;
  if (this->Internal->Socket->Receive(&camState, sizeof(CameraStateStruct)) == 0)
  {
    this->close();
    return false;
  }

  QMutexLocker locker(&this->Internal->Lock);
  this->Internal->CameraState = camState;
  this->Internal->NewCameraState = true;

  return (this->Internal->ShouldQuit != true);
}

//-----------------------------------------------------------------------------
pqRemoteControlThread::CameraStateStruct pqRemoteControlThread::cameraState()
{
  QMutexLocker locker(&this->Internal->Lock);
  this->Internal->NewCameraState = false;
  return this->Internal->CameraState;
}

//-----------------------------------------------------------------------------
bool pqRemoteControlThread::hasNewCameraState()
{
  QMutexLocker locker(&this->Internal->Lock);
  return this->Internal->NewCameraState;
}

//-----------------------------------------------------------------------------
bool pqRemoteControlThread::waitForSocketActivity()
{
  int selectResult = 0;
  while (!selectResult && !this->Internal->ShouldQuit)
  {
    selectResult = this->Internal->SocketCollection->SelectSockets(300);
    if (selectResult == -1)
    {
      this->close();
      return false;
    }
  }

  return (this->Internal->ShouldQuit != true);
}

//-----------------------------------------------------------------------------
bool pqRemoteControlThread::sendCommand(int command)
{
  if (this->Internal->Socket->Send(&command, sizeof(command)) == 0)
  {
    this->close();
    return false;
  }

  return (this->Internal->ShouldQuit != true);
}

//-----------------------------------------------------------------------------
bool pqRemoteControlThread::receiveCommand(int& command)
{
  if (this->Internal->Socket->Receive(&command, sizeof(command)) == 0)
  {
    this->close();
    return false;
  }

  return (this->Internal->ShouldQuit != true);
}

//-----------------------------------------------------------------------------
bool pqRemoteControlThread::handleCommand(int command)
{
  if (command == SEND_METADATA_COMMAND)
  {
    this->exportSceneOnMainThread();
    return this->sendSceneInfo();
  }
  else if (command == SEND_OBJECTS_COMMAND)
  {
    return this->sendObjects();
  }
  else if (command == RECEIVE_CAMERA_STATE_COMMAND)
  {
    return this->receiveCameraState();
  }
  else if (command == HEARTBEAT_COMMAND)
  {
    return true;
  }
  else
  {
    this->close();
    return false;
  }
}

//-----------------------------------------------------------------------------
void pqRemoteControlThread::run()
{
  this->Internal->ShouldQuit = false;

  while (!this->Internal->ShouldQuit)
  {

    if (!this->sendCommand(READY_COMMAND))
    {
      break;
    }

    if (!this->waitForSocketActivity())
    {
      break;
    }

    int command = 0;
    if (!this->receiveCommand(command))
    {
      break;
    }

    if (!this->handleCommand(command))
    {
      break;
    }
  }

  this->close();
}
