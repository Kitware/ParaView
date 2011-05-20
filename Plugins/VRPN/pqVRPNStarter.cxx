/*=========================================================================

   Program: ParaView
   Module:    pqVRPNStarter.cxx

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
#include "pqVRPNStarter.h"

// Server Manager Includes.

// Qt Includes.
#include <QtDebug>
#include <QTimer>
// ParaView Includes.
#include "ParaViewVRPN.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkVRGenericStyle.h"
#include "vtkVRQueue.h"
#include "vtkVRQueueHandler.h"

//-----------------------------------------------------------------------------
pqVRPNStarter::pqVRPNStarter(QObject* p/*=0*/)
  : QObject(p)
{
  this->EventQueue = NULL;
  this->Handler = NULL; 
  this->InputDevice = NULL;
}

//-----------------------------------------------------------------------------
pqVRPNStarter::~pqVRPNStarter()
{
  delete this->EventQueue;
  delete this->Handler;
  delete this->InputDevice;
}

//-----------------------------------------------------------------------------
void pqVRPNStarter::onStartup()
{
  Q_ASSERT(this->InputDevice == NULL);

  this->EventQueue = new vtkVRQueue(this);
  this->Handler = new vtkVRQueueHandler(this->EventQueue, this);

  // for debugging, until we add support for reading styles from XML we simple
  // create the generic style.
  this->Handler->add(new vtkVRGenericStyle(this));

  this->InputDevice = NULL;

  //qWarning() << "Message from pqVRPNStarter: Application Started";
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions *options = (vtkPVOptions*)pm->GetOptions();
  if(options->GetUseVRPN())
    {
    // Create VRPN event queue

    // Create vrpn client to read device information
    this->InputDevice=new ParaViewVRPN;
    this->InputDevice->SetQueue( this->EventQueue );
    this->InputDevice->SetName(options->GetVRPNAddress());
    this->InputDevice->Init();
    this->InputDevice->start();
    }
  
  this->Handler->start();
}


//-----------------------------------------------------------------------------
void pqVRPNStarter::onShutdown()
{
  this->Handler->stop();
  if (this->InputDevice)
    {
    this->InputDevice->terminate();
    }
  // qWarning() << "Message from pqVRPNStarter: Application Shutting down";
}
