// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSierraPlotToolsDataLoadManager.cxx

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

#include "warningState.h"

#include "pqSierraPlotToolsDataLoadManager.h"
#include "pqSierraPlotToolsManager.h"

#include "vtkSMProxy.h"

#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqDisplayPolicy.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"

#include <QPushButton>
#include <QtDebug>

#include "ui_pqSierraPlotToolsDataLoadManager.h"

// used to show line number in #pragma message
#define STRING2(x) #x
#define STRING(x) STRING2(x)

class pqSierraPlotToolsDataLoadManager::pqUI : public Ui::pqSierraPlotToolsDataLoadManager
{
};

//=============================================================================
pqSierraPlotToolsDataLoadManager::pqSierraPlotToolsDataLoadManager(
  QWidget* p, Qt::WindowFlags f /*=0*/)
  : QDialog(p, f)
{
  pqSierraPlotToolsManager* manager = pqSierraPlotToolsManager::instance();
  this->Server = manager->getActiveServer();

  this->ui = new pqSierraPlotToolsDataLoadManager::pqUI;
  this->ui->setupUi(this);

  this->ui->meshFile->setServer(this->Server);

  this->ui->meshFile->setForceSingleFile(true);

  this->ui->meshFile->setExtension("ExodusIIReader Files (*.exo *.g *.e *.ex2 *.ex2v2 *.gen "
                                   "*.exoII *.exii *.0 *.00 *.000 *.0000)");

  pqPipelineSource* meshReader = manager->getMeshReader();
  if (meshReader)
  {
    vtkSMProxy* meshReaderProxy = meshReader->getProxy();
    vtkSMProperty* meshFileName = meshReaderProxy->GetProperty("MeshFileName");
    this->ui->meshFile->setFilenames(pqSMAdaptor::getFileListProperty(meshFileName));
  }

  QObject::connect(this->ui->meshFile, SIGNAL(filenamesChanged(const QStringList&)), this,
    SLOT(checkInputValid()));

  QObject::connect(this, SIGNAL(accepted()), this, SLOT(setupPipeline()));

  this->checkInputValid();
}

pqSierraPlotToolsDataLoadManager::~pqSierraPlotToolsDataLoadManager()
{
  delete this->ui;
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsDataLoadManager::checkInputValid()
{
  bool valid = true;

  if (this->ui->meshFile->filenames().isEmpty())
    valid = false;

  this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

//-----------------------------------------------------------------------------
void pqSierraPlotToolsDataLoadManager::setupPipeline()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();
  pqUndoStack* stack = core->getUndoStack();
  pqDisplayPolicy* displayPolicy = core->getDisplayPolicy();

  pqSierraPlotToolsManager* manager = pqSierraPlotToolsManager::instance();

  if (stack)
    stack->beginUndoSet("ExodusIIReader Data Load");

  // Determine the views.  Do this before deleting existing pipeline objects.
  pqView* meshView = manager->getMeshView();

  // Delete existing pipeline objects.  We will replace them.
  manager->destroyPipelineSourceAndConsumers(manager->getMeshReader());

  QStringList meshFiles = this->ui->meshFile->filenames();
  // This should never really be not empty.
  if (!meshFiles.isEmpty())
  {
    pqPipelineSource* meshReader =
      builder->createReader("sources", "ExodusIIReader", meshFiles, this->Server);

    vtkSMProxy* meshReaderProxy = meshReader->getProxy();

    // Push changes to server so that when the representation gets updated,
    // it uses the property values we set.
    meshReaderProxy->UpdateVTKObjects();

    // Make representations.
    displayPolicy->setRepresentationVisibility(meshReader->getOutputPort(0), meshView, true);

    // We have already made the representations and pushed everything to the
    // server manager.  Thus, there is no state left to be modified.
    meshReader->setModifiedState(pqProxy::UNMODIFIED);
  }

  if (stack)
    stack->endUndoSet();

  emit this->createdPipeline();
}
