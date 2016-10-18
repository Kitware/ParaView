/*=========================================================================

   Program: ParaView
   Module:    pqRemoteControl.cxx

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

#include "pqRemoteControl.h"
#include "ui_pqRemoteControl.h"

#include "pqApplicationCore.h"
#include "pqRemoteControlThread.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"

#include <vtkPVRenderView.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMRenderViewProxy.h>

#include <QDesktopServices>
#include <QHostInfo>
#include <QInputDialog>
#include <QTimer>
#include <QUrl>

//-----------------------------------------------------------------------------
class pqRemoteControl::pqInternal : public Ui::pqRemoteControl
{
public:
  int Port;

  pqRemoteControlThread Server;
};

//-----------------------------------------------------------------------------
pqRemoteControl::pqRemoteControl(QWidget* p, Qt::WindowFlags flags)
  : QDockWidget(p, flags)
{
  this->Internal = new pqInternal;
  QWidget* mainWidget = new QWidget(this);
  this->Internal->setupUi(mainWidget);
  this->setWidget(mainWidget);
  this->setWindowTitle("Mobile Remote Control");
  this->connect(this->Internal->StartButton, SIGNAL(clicked()), SLOT(onButtonClicked()));
  this->connect(this->Internal->DocLabel, SIGNAL(linkActivated(const QString&)),
    SLOT(onLinkClicked(const QString&)));
  this->connect(&this->Internal->Server, SIGNAL(requestExportScene()), SLOT(onExportScene()));

  this->Internal->Port = 40000;
  QString hostName = QHostInfo::localHostName();
  QHostInfo::lookupHost(hostName, this, SLOT(onHostLookup(const QHostInfo&)));
}

//-----------------------------------------------------------------------------
pqRemoteControl::~pqRemoteControl()
{
  this->onStop();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqRemoteControl::onButtonClicked()
{
  if (this->Internal->StartButton->text() == "Start")
  {
    this->onStart();
  }
  else if (this->Internal->StartButton->text() == "Stop")
  {
    this->onStop();
  }
}

//-----------------------------------------------------------------------------
void pqRemoteControl::onStart()
{
  if (!this->Internal->Server.createServer(this->Internal->Port))
  {
    return;
  }

  this->Internal->StatusLabel->setText(
    QString("Status: waiting for connection on port %1").arg(this->Internal->Port));
  this->Internal->StartButton->setText("Stop");
  this->checkForConnection();
}

//-----------------------------------------------------------------------------
void pqRemoteControl::onStop()
{
  this->Internal->StartButton->setText("Start");
  this->Internal->StatusLabel->setText("Status: inactive");

  if (this->Internal->Server.clientIsConnected())
  {
    this->Internal->Server.shouldQuit();
    this->Internal->Server.wait();
  }
  else
  {
    this->Internal->Server.close();
  }
}

//-----------------------------------------------------------------------------
void pqRemoteControl::checkForConnection()
{
  if (!this->Internal->Server.serverIsOpen())
  {
    return;
  }

  if (this->Internal->Server.checkForConnection())
  {
    this->Internal->StatusLabel->setText("Status: active");
    this->onNewConnection();
  }
  else
  {
    QTimer::singleShot(100, this, SLOT(checkForConnection()));
  }
}

//-----------------------------------------------------------------------------
void pqRemoteControl::updateCamera()
{
  if (!this->Internal->Server.clientIsConnected())
  {
    this->onStop();
    return;
  }

  pqRenderView* renView = this->renderView();
  if (!renView || !this->Internal->Server.hasNewCameraState())
  {
    QTimer::singleShot(33, this, SLOT(updateCamera()));
    return;
  }

  const pqRemoteControlThread::CameraStateStruct cameraState = this->Internal->Server.cameraState();

  double cameraPosition[3] = { cameraState.Position[0], cameraState.Position[1],
    cameraState.Position[2] };
  double cameraFocalPoint[3] = { cameraState.FocalPoint[0], cameraState.FocalPoint[1],
    cameraState.FocalPoint[2] };
  double cameraViewUp[3] = { cameraState.ViewUp[0], cameraState.ViewUp[1], cameraState.ViewUp[2] };

  vtkSMRenderViewProxy* viewProxy = renView->getRenderViewProxy();
  vtkSMPropertyHelper(viewProxy, "CameraPosition").Set(cameraPosition, 3);
  vtkSMPropertyHelper(viewProxy, "CameraFocalPoint").Set(cameraFocalPoint, 3);
  vtkSMPropertyHelper(viewProxy, "CameraViewUp").Set(cameraViewUp, 3);

  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(viewProxy->GetClientSideView());
  if (view)
  {
    view->ResetCameraClippingRange();
  }
  renView->render();

  QTimer::singleShot(33, this, SLOT(updateCamera()));
}

//-----------------------------------------------------------------------------
void pqRemoteControl::onLinkClicked(const QString& link)
{
  if (link == "changeport")
  {
    QString title = "Change port";
    QString label = "Enter port:";
    bool ok = true;
    int newPort = QInputDialog::getInt(this, title, label, this->Internal->Port, 0, 65535, 1, &ok);
    if (ok)
    {
      this->Internal->Port = newPort;
    }
  }
  else
  {
    QDesktopServices::openUrl(link);
  }
}

//-----------------------------------------------------------------------------
void pqRemoteControl::onHostLookup(const QHostInfo& hostInfo)
{
  if (hostInfo.error() != QHostInfo::NoError || hostInfo.addresses().empty())
  {
    qDebug() << "Lookup failed:" << hostInfo.errorString();
    return;
  }

  QString hostName = hostInfo.hostName();
  QString hostAddress = hostInfo.addresses()[0].toString();

  this->Internal->HostLabel->setText(
    QString("Host: %1<br>Address: %2").arg(hostName).arg(hostAddress));
}

//-----------------------------------------------------------------------------
pqRenderView* pqRemoteControl::renderView()
{
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqRenderView*> views = smModel->findItems<pqRenderView*>();
  if (views.empty())
  {
    return 0;
  }

  return views[0];
}

//-----------------------------------------------------------------------------
void pqRemoteControl::onExportScene()
{
  pqRenderView* renView = this->renderView();
  vtkRenderWindow* renderWindow = renView ? renView->getRenderViewProxy()->GetRenderWindow() : 0;

  this->Internal->StatusLabel->setText("Status: exporting scene");
  this->Internal->Server.exportScene(renderWindow);
  this->Internal->StatusLabel->setText("Status: active");
}

//-----------------------------------------------------------------------------
void pqRemoteControl::onNewConnection()
{
  this->Internal->Server.start();
  this->updateCamera();
}
