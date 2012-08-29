/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqVRDockPanel.h"
#include "ui_pqVRDockPanel.h"

#include <QtDebug>

#include "vtkVRConnectionManager.h"
#include "pqVRAddConnectionDialog.h"

class pqVRDockPanel::pqInternals : public Ui::VRDockPanel
{
public:
};

//-----------------------------------------------------------------------------
void pqVRDockPanel::constructor()
{
  this->setWindowTitle("VR Panel");
  QWidget* container = new QWidget(this);
  this->Internals = new pqInternals();
  this->Internals->setupUi(container);
  this->setWidget(container);

  QObject::connect(this->Internals->addConnection,
    SIGNAL(clicked()), this, SLOT(addConnection()));

  vtkVRConnectionManager* mgr = vtkVRConnectionManager::instance();
  QObject::connect(mgr, SIGNAL(connectionsChanged()),
    this, SLOT(updateConnections()));
}

//-----------------------------------------------------------------------------
pqVRDockPanel::~pqVRDockPanel()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::updateConnections()
{
  this->Internals->connectionsTable->clear();
  
  vtkVRConnectionManager* mgr = vtkVRConnectionManager::instance();
  QList<QString> connectionNames = mgr->connectionNames();
  foreach (const QString& name, connectionNames)
    {
    this->Internals->connectionsTable->addItem(name);
    }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::addConnection()
{
  pqVRAddConnectionDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted)
    {
    qDebug() << "add new connection.";
    vtkVRConnectionManager* mgr = vtkVRConnectionManager::instance();

    // create and add new connection to mgr.

    }
}
