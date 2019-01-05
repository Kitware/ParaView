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
#include "pqApplicationCore.h"
#include "pqTestUtility.h"
#include "pqVRConnectionManager.h"
#include "pqVRQueueHandler.h"
#include "pqWidgetEventPlayer.h"
#include "vtkPVVRConfig.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkVRInteractorStyleFactory.h"
#include "vtkVRQueue.h"
#include <QTimer>
#include <QtDebug>

// Used for testing:
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
#include <pqVRPNConnection.h>
class pqVREventPlayer : public pqWidgetEventPlayer
{
  typedef pqWidgetEventPlayer Superclass;

public:
  pqVREventPlayer(QObject* p)
    : Superclass(p)
  {
  }
  virtual bool playEvent(QObject*, const QString& command, const QString& arguments, bool& error)
  {
    if (command == "pqVREvent")
    {
      if (arguments.startsWith("vrpn_trackerEvent"))
      {
        // Syntax is (one line:)
        // "vrpn_trackerEvent:[connName];[sensorid];[pos_x],[pos_y],[pos_z];
        // [quat_w],[quat_x],[quat_y],[quat_z]"
        QRegExp capture("vrpn_trackerEvent:"
                        "([\\w.@]+);" // Connection name
                        "(\\d+);"     // sensor id
                        "([\\d.-]+)," // pos_x
                        "([\\d.-]+)," // pos_y
                        "([\\d.-]+);" // pos_z
                        "([\\d.-]+)," // quat_w
                        "([\\d.-]+)," // quat_x
                        "([\\d.-]+)," // quat_y
                        "([\\d.-]+)$" // quat_z
          );
        int ind = capture.indexIn(arguments);
        if (ind < 0)
        {
          qWarning() << "pqVREventPlayer: bad arguments:" << command;
          error = true;
          return false;
        }
        vrpn_TRACKERCB event;
        QString connName;
        connName = capture.cap(1);
        event.sensor = capture.cap(2).toInt();
        event.pos[0] = capture.cap(3).toDouble();
        event.pos[1] = capture.cap(4).toDouble();
        event.pos[2] = capture.cap(5).toDouble();
        event.quat[0] = capture.cap(6).toDouble();
        event.quat[1] = capture.cap(7).toDouble();
        event.quat[2] = capture.cap(8).toDouble();
        event.quat[3] = capture.cap(9).toDouble();
        pqVRConnectionManager* mgr = pqVRConnectionManager::instance();
        pqVRPNConnection* conn = mgr->GetVRPNConnection(connName);
        if (!conn)
        {
          qWarning() << "pqVREventPlayer: bad connection name:" << command;
          error = true;
          return false;
        }
        conn->newTrackerValue(event);
        return true;
      }
      else
      {
        error = true;
      }
      return true;
    }
    else
    {
      return false;
    }
  }
};
#endif // PARAVIEW_PLUGIN_VRPlugin_USE_VRPN

//-----------------------------------------------------------------------------
class pqVRStarter::pqInternals
{
public:
  pqVRConnectionManager* ConnectionManager;
  vtkVRQueue* EventQueue;
  pqVRQueueHandler* Handler;
  vtkVRInteractorStyleFactory* StyleFactory;
};

//-----------------------------------------------------------------------------
pqVRStarter::pqVRStarter(QObject* p /*=0*/)
  : QObject(p)
{
  this->Internals = new pqInternals;
  this->Internals->EventQueue = NULL;
  this->Internals->Handler = NULL;
  this->Internals->StyleFactory = NULL;

#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
  pqVREventPlayer* player = new pqVREventPlayer(NULL);
  pqApplicationCore::instance()->testUtility()->eventPlayer()->addWidgetEventPlayer(player);
#endif // PARAVIEW_PLUGIN_VRPlugin_USE_VRPN

  this->IsShutdown = true;
}

//-----------------------------------------------------------------------------
pqVRStarter::~pqVRStarter()
{
  if (!this->IsShutdown)
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
  this->Internals->ConnectionManager = new pqVRConnectionManager(this->Internals->EventQueue, this);
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
