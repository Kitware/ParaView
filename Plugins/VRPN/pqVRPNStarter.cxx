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
#include "vtkVRPNConnection.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkPVXMLElement.h"
#include "vtkVRGenericStyle.h"
#include "vtkVRHeadTrackingStyle.h"
#include "vtkVRActiveObjectManipulationStyle.h"
#include "vtkVRQueue.h"
#include "vtkVRQueueHandler.h"
#include "vtkVRVectorPropertyStyle.h"
#include "pqApplicationCore.h"
#include "vtkVRConnectionManager.h"

class pqVRPNStarter::pqInternals
{
public:
  vtkVRConnectionManager *ConnectionManager;
  vtkVRQueue* EventQueue;
  vtkVRQueueHandler* Handler;

};

//-----------------------------------------------------------------------------
pqVRPNStarter::pqVRPNStarter(QObject* p/*=0*/)
  : QObject(p)
{
  this->Internals = new pqInternals;
  this->Internals->EventQueue = NULL;
  this->Internals->Handler = NULL;
  this->InputDevice[0] = NULL;
  this->InputDevice[1] = NULL;
}

//-----------------------------------------------------------------------------
pqVRPNStarter::~pqVRPNStarter()
{
  delete this->Internals->EventQueue;
  delete this->Internals->Handler;
  delete this->InputDevice[0];
  delete this->InputDevice[1];
}

//-----------------------------------------------------------------------------
void pqVRPNStarter::onStartup()
{
  Q_ASSERT(this->InputDevice[0] == NULL);
  Q_ASSERT(this->InputDevice[1] == NULL);

   this->Internals->EventQueue = new vtkVRQueue(this);
  this->Internals->ConnectionManager =
    new vtkVRConnectionManager(this->Internals->EventQueue,this);
  this->Internals->Handler = new vtkVRQueueHandler(this->Internals->EventQueue, this);

  // for debugging, until we add support for reading styles from XML we simple
  // create the generic style.
  this->Internals->Handler->add(new vtkVRActiveObjectManipulationStyle(this));
  vtkVRHeadTrackingStyle* headTracking = new vtkVRHeadTrackingStyle(this);
  headTracking->setName("kinect.head");
  this->Internals->Handler->add(headTracking);

  this->InputDevice[0] = NULL;
  this->InputDevice[1] = NULL;

  //qWarning() << "Message from pqVRPNStarter: Application Started";
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions *options = (vtkPVOptions*)pm->GetOptions();
  if(options->GetUseVRPN())
    {
    // Create vrpn client to read device information
    this->InputDevice[0]=new vtkVRPNConnection;
    this->InputDevice[0]->SetQueue( this->Internals->EventQueue );
    this->InputDevice[0]->SetName( "kinect" );
    this->InputDevice[0]->SetAddress("Tracker0@192.168.1.126");
    this->InputDevice[0]->AddTracking( "0", "head" );
    this->InputDevice[0]->AddTracking( "13", "right-hand" );
    // this->InputDevice[0]->Init();
    // this->InputDevice[0]->start();

    this->InputDevice[1]=new vtkVRPNConnection;
    this->InputDevice[1]->SetQueue( this->Internals->EventQueue );
    this->InputDevice[1]->SetName( "space-navigator" );
    this->InputDevice[1]->SetAddress("device0@localhost");
    this->InputDevice[1]->AddAnalog( "0", "crown" );
    this->InputDevice[1]->AddButton( "0", "left-button" );
    this->InputDevice[1]->AddButton( "1", "right-button" );
    // this->InputDevice[1]->Init();
    // this->InputDevice[1]->start();
    }
  this->Internals->ConnectionManager->add( this->InputDevice[0] );
  this->Internals->ConnectionManager->add( this->InputDevice[1] );
  this->Internals->ConnectionManager->start();
  this->Internals->Handler->start();
}


//-----------------------------------------------------------------------------
void pqVRPNStarter::onShutdown()
{
  this->Internals->Handler->stop();
  if (this->InputDevice[0])
    {
    this->InputDevice[0]->Stop();
    }
  if (this->InputDevice[1])
    {
    this->InputDevice[1]->Stop();
    }
  // qWarning() << "Message from pqVRPNStarter: Application Shutting down";
}
