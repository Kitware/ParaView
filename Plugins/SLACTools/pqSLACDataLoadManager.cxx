// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSLACDataLoadManager.cxx

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

#include "pqSLACDataLoadManager.h"

#include "pqSLACManager.h"

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

#include "ui_pqSLACDataLoadManager.h"
class pqSLACDataLoadManager::pqUI : public Ui::pqSLACDataLoadManager {};

//=============================================================================
pqSLACDataLoadManager::pqSLACDataLoadManager(QWidget *p,
                                             Qt::WindowFlags f/*=0*/)
  : QDialog(p, f)
{
  pqSLACManager *manager = pqSLACManager::instance();
  this->Server = manager->activeServer();

  this->ui = new pqSLACDataLoadManager::pqUI;
  this->ui->setupUi(this);

  this->ui->meshFile->setServer(this->Server);
  this->ui->modeFile->setServer(this->Server);
  this->ui->particlesFile->setServer(this->Server);

  this->ui->meshFile->setForceSingleFile(true);
  this->ui->modeFile->setForceSingleFile(false);
  this->ui->particlesFile->setForceSingleFile(false);

  this->ui->meshFile->setExtension("SLAC Mesh Files (*.ncdf *.nc)");
  this->ui->modeFile->setExtension("SLAC Mode Files (*.mod *.m?)");
  this->ui->particlesFile->setExtension("SLAC Particle Files (*.ncdf *.netcdf)");

  QObject::connect(
              this->ui->meshFile, SIGNAL(filenamesChanged(const QStringList &)),
              this, SLOT(checkInputValid()));

  QObject::connect(this, SIGNAL(accepted()),
                   this, SLOT(setupPipeline()));

  this->checkInputValid();
}

pqSLACDataLoadManager::~pqSLACDataLoadManager()
{
  delete this->ui;
}

//-----------------------------------------------------------------------------
void pqSLACDataLoadManager::checkInputValid()
{
  bool valid = true;

  if (this->ui->meshFile->filenames().isEmpty()) valid = false;

  this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

//-----------------------------------------------------------------------------
void pqSLACDataLoadManager::setupPipeline()
{
  pqApplicationCore *core = pqApplicationCore::instance();
  pqObjectBuilder *builder = core->getObjectBuilder();
  pqUndoStack *stack = core->getUndoStack();
  pqDisplayPolicy *displayPolicy = core->getDisplayPolicy();

  pqSLACManager *manager = pqSLACManager::instance();

  if (stack) stack->beginUndoSet("SLAC Data Load");

  QStringList meshFiles = this->ui->meshFile->filenames();
  // This should never really be not empty.
  if (!meshFiles.isEmpty())
    {
    pqPipelineSource *meshReader
      = builder->createReader("sources", "SLACReader", meshFiles, this->Server);
    vtkSMProxy *meshReaderProxy = meshReader->getProxy();

    // Make representations.
    pqView *view = manager->view3D();
    pqDataRepresentation *repr;
    repr = displayPolicy->createPreferredRepresentation(
                                     meshReader->getOutputPort(0), view, false);
    repr->setVisible(true);
    repr = displayPolicy->createPreferredRepresentation(
                                     meshReader->getOutputPort(1), view, false);
    repr->setVisible(false);

    // Set up mode (if any).
    QStringList modeFiles = this->ui->modeFile->filenames();
    pqSMAdaptor::setFileListProperty(
                       meshReaderProxy->GetProperty("ModeFileName"), modeFiles);

    // We have already made the representations and pushed everything to the
    // server manager.  Thus, there is no state left to be modified.
    meshReader->setModifiedState(pqProxy::UNMODIFIED);
    }

  QStringList particlesFiles = this->ui->particlesFile->filenames();
  if (!particlesFiles.isEmpty())
    {
    pqPipelineSource *particlesReader
      = builder->createReader("sources", "SLACParticleReader",
                              particlesFiles, this->Server);

    // Make representations.
    pqView *view = manager->view3D();
    pqDataRepresentation *repr
      = displayPolicy->createPreferredRepresentation(
                                particlesReader->getOutputPort(0), view, false);
    repr->setVisible(true);

    // We have already made the representations and pushed everything to the
    // server manager.  Thus, there is no state left to be modified.
    particlesReader->setModifiedState(pqProxy::UNMODIFIED);
    }

  if (stack) stack->endUndoSet();
}
