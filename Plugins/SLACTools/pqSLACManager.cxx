// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSLACManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "pqSLACManager.h"

#include "pqSLACDataLoadManager.h"

#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

#include <QMainWindow>
#include <QPointer>
#include <QtDebug>

#include "ui_pqSLACActionHolder.h"

//=============================================================================
class pqSLACManager::pqInternal
{
public:
  Ui::pqSLACActionHolder Actions;
  QWidget *ActionPlaceholder;
};

//=============================================================================
QPointer<pqSLACManager> pqSLACManagerInstance = NULL;

pqSLACManager *pqSLACManager::instance()
{
  if (pqSLACManagerInstance == NULL)
    {
    pqApplicationCore *core = pqApplicationCore::instance();
    if (!core)
      {
      qFatal("Cannot use the SLAC Tools without an application core instance.");
      return NULL;
      }

    pqSLACManagerInstance = new pqSLACManager(core);
    }

  return pqSLACManagerInstance;
}

//-----------------------------------------------------------------------------
pqSLACManager::pqSLACManager(QObject *p) : QObject(p)
{
  this->Internal = new pqSLACManager::pqInternal;

  // This widget serves no real purpose other than initializing the Actions
  // structure created with designer that holds the actions.
  this->Internal->ActionPlaceholder = new QWidget(NULL);
  this->Internal->Actions.setupUi(this->Internal->ActionPlaceholder);

  QObject::connect(this->actionDataLoadManager(), SIGNAL(triggered(bool)),
                   this, SLOT(showDataLoadManager()));
}

pqSLACManager::~pqSLACManager()
{
  delete this->Internal->ActionPlaceholder;
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QAction *pqSLACManager::actionDataLoadManager()
{
  return this->Internal->Actions.actionDataLoadManager;
}

//-----------------------------------------------------------------------------
pqServer *pqSLACManager::activeServer()
{
  pqApplicationCore *app = pqApplicationCore::instance();
  pqServerManagerModel *smModel = app->getServerManagerModel();
  pqServer *server = smModel->getItemAtIndex<pqServer*>(0);
  return server;
}

//-----------------------------------------------------------------------------
QWidget *pqSLACManager::mainWindow()
{
  foreach(QWidget *topWidget, QApplication::topLevelWidgets())
    {
    if (qobject_cast<QMainWindow*>(topWidget)) return topWidget;
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void pqSLACManager::showDataLoadManager()
{
  pqSLACDataLoadManager *dialog = new pqSLACDataLoadManager(this->mainWindow());
  dialog->setAttribute(Qt::WA_DeleteOnClose, true);
  dialog->show();
}
