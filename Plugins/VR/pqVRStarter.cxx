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
#include "vtkVRQueue.h"
#include "vtkVRQueueHandler.h"
#include "pqApplicationCore.h"
#include "vtkVRConnectionManager.h"

//-----------------------------------------------------------------------------
class pqVRStarter::pqInternals
{
public:
  vtkVRConnectionManager *ConnectionManager;
  vtkVRQueue* EventQueue;
  vtkVRQueueHandler* Handler;
};

//-----------------------------------------------------------------------------
pqVRStarter::pqVRStarter(QObject* p/*=0*/)
  : QObject(p)
{
  this->Internals = new pqInternals;
  this->Internals->EventQueue = NULL;
  this->Internals->Handler = NULL;
  this->IsShutdown = false;
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
  this->Internals->EventQueue = new vtkVRQueue(this);
  this->Internals->ConnectionManager = new vtkVRConnectionManager(this->Internals->EventQueue,this);
  this->Internals->Handler = new vtkVRQueueHandler(this->Internals->EventQueue, this);
  this->Internals->ConnectionManager->start();
  this->Internals->Handler->start();
  //qWarning() << "Message from pqVRStarter: Application Started";
}

//-----------------------------------------------------------------------------
void pqVRStarter::onShutdown()
{
  this->Internals->Handler->stop();
  this->Internals->ConnectionManager->stop();
  delete this->Internals->Handler;
  delete this->Internals->ConnectionManager;
  delete this->Internals->EventQueue;
  this->IsShutdown = true;
  // qWarning() << "Message from pqVRStarter: Application Shutting down";
}
