/*=========================================================================

   Program: ParaView
   Module:    pqVRStarter.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqVRStarter.h"
#include <QtDebug>
#include <QTimer>
#include "vtkPVVRConfig.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkVRInteractorStyleFactory.h"
#include "vtkVRQueue.h"
#include "pqVRQueueHandler.h"
#include "pqApplicationCore.h"
#include "pqVRConnectionManager.h"

//-----------------------------------------------------------------------------
class pqVRStarter::pqInternals
{
public:
  pqVRConnectionManager *ConnectionManager;
  vtkVRQueue* EventQueue;
  pqVRQueueHandler* Handler;
  vtkVRInteractorStyleFactory *StyleFactory;
};

//-----------------------------------------------------------------------------
pqVRStarter::pqVRStarter(QObject* p/*=0*/)
  : QObject(p)
{
  this->Internals = new pqInternals;
  this->Internals->EventQueue = NULL;
  this->Internals->Handler = NULL;
  this->Internals->StyleFactory = NULL;
  this->IsShutdown = true;
}

//-----------------------------------------------------------------------------
pqVRStarter::~pqVRStarter()
{
  if(!this->IsShutdown)
    {
      this->onShutdown();
    }
}

//-----------------------------------------------------------------------------
void pqVRStarter::onStartup()
{
  if (!this->IsShutdown)
    {
    qWarning() << "pqVRStarter: Cannot startup -- already started.";
    return;
    }
  this->IsShutdown = false;
  this->Internals->EventQueue = vtkVRQueue::New();
  this->Internals->ConnectionManager = new pqVRConnectionManager(this->Internals->EventQueue,this);
  pqVRConnectionManager::setInstance(this->Internals->ConnectionManager);
  this->Internals->Handler = new pqVRQueueHandler(this->Internals->EventQueue, this);
  pqVRQueueHandler::setInstance(this->Internals->Handler);
  this->Internals->StyleFactory = vtkVRInteractorStyleFactory::New();
  vtkVRInteractorStyleFactory::SetInstance(this->Internals->StyleFactory);
}

//-----------------------------------------------------------------------------
void pqVRStarter::onShutdown()
{
  if (this->IsShutdown)
    {
    qWarning() << "pqVRStarter: Cannot shutdown -- not started yet.";
    return;
    }
  this->IsShutdown = true;
  pqVRConnectionManager::setInstance(NULL);
  pqVRQueueHandler::setInstance(NULL);
  vtkVRInteractorStyleFactory::SetInstance(NULL);
  delete this->Internals->Handler;
  delete this->Internals->ConnectionManager;
  this->Internals->EventQueue->Delete();
  this->Internals->StyleFactory->Delete();
}
