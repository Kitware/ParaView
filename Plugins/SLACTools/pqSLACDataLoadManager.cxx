// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "pqSLACDataLoadManager.h"
#include "ui_pqSLACDataLoadManager.h"

#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "pqSLACManager.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkNew.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"

#include <QAction>
#include <QPushButton>
#include <QtDebug>

class pqSLACDataLoadManager::pqUI : public Ui::pqSLACDataLoadManager
{
};

//=============================================================================
pqSLACDataLoadManager::pqSLACDataLoadManager(QWidget* p, Qt::WindowFlags f /*=0*/)
  : QDialog(p, f)
{
  pqSLACManager* manager = pqSLACManager::instance();
  this->Server = manager->getActiveServer();

  this->ui = new pqSLACDataLoadManager::pqUI;
  this->ui->setupUi(this);

  // hide the Context Help item (it's a "?" in the Title Bar for Windows, a menu item for Linux)
  this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));

  this->ui->meshFile->setServer(this->Server);
  this->ui->modeFile->setServer(this->Server);
  this->ui->particlesFile->setServer(this->Server);

  this->ui->meshFile->setForceSingleFile(true);
  this->ui->modeFile->setForceSingleFile(false);
  this->ui->particlesFile->setForceSingleFile(false);

  this->ui->meshFile->setExtension(QString("%1 (*.ncdf *.nc)").arg(tr("SLAC Mesh Files")));
  this->ui->modeFile->setExtension(QString("%1 (*.mod *.m?)").arg(tr("SLAC Mode Files")));
  this->ui->particlesFile->setExtension(
    QString("%1 (*.ncdf *.netcdf)").arg(tr("SLAC Particle Files")));

  pqPipelineSource* meshReader = manager->getMeshReader();
  pqPipelineSource* particlesReader = manager->getParticlesReader();
  if (meshReader)
  {
    vtkSMProxy* meshReaderProxy = meshReader->getProxy();
    vtkSMProperty* meshFileName = meshReaderProxy->GetProperty("MeshFileName");
    vtkSMProperty* modeFileName = meshReaderProxy->GetProperty("ModeFileName");
    this->ui->meshFile->setFilenames(pqSMAdaptor::getFileListProperty(meshFileName));
    this->ui->modeFile->setFilenames(pqSMAdaptor::getFileListProperty(modeFileName));
  }
  if (particlesReader)
  {
    vtkSMProxy* particlesReaderProxy = particlesReader->getProxy();
    vtkSMProperty* fileName = particlesReaderProxy->GetProperty("FileName");
    this->ui->particlesFile->setFilenames(pqSMAdaptor::getFileListProperty(fileName));
  }

  QObject::connect(this->ui->meshFile, SIGNAL(filenamesChanged(const QStringList&)), this,
    SLOT(checkInputValid()));

  QObject::connect(this, SIGNAL(accepted()), this, SLOT(setupPipeline()));

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

  if (this->ui->meshFile->filenames().isEmpty())
    valid = false;

  this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

//-----------------------------------------------------------------------------
void pqSLACDataLoadManager::setupPipeline()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  pqSLACManager* manager = pqSLACManager::instance();

  BEGIN_UNDO_SET(tr("SLAC Data Load"));

  // Determine the views.  Do this before deleting existing pipeline objects.
  pqView* meshView = manager->getMeshView();

  // Delete existing pipeline objects.  We will replace them.
  manager->destroyPipelineSourceAndConsumers(manager->getMeshReader());
  manager->destroyPipelineSourceAndConsumers(manager->getParticlesReader());

  QStringList meshFiles = this->ui->meshFile->filenames();
  // This should never really be not empty.
  if (!meshFiles.isEmpty())
  {
    pqSLACManager::instance()->loaderCreatingPipeline();
    pqPipelineSource* meshReader =
      builder->createReader("sources", "SLACReader", meshFiles, this->Server);

    vtkSMSourceProxy* meshReaderProxy = vtkSMSourceProxy::SafeDownCast(meshReader->getProxy());

    // Set up mode (if any).
    QStringList modeFiles = this->ui->modeFile->filenames();
    pqSMAdaptor::setFileListProperty(meshReaderProxy->GetProperty("ModeFileName"), modeFiles);

    // Push changes to server so that when the representation gets updated,
    // it uses the property values we set.
    meshReaderProxy->UpdateVTKObjects();

    // ensures that new timestep range, if any gets fetched from the server.
    meshReaderProxy->UpdatePipelineInformation();

    // ensures that the FrequencyScale and PhaseShift have correct default
    // values.
    meshReaderProxy->GetProperty("FrequencyScale")
      ->Copy(meshReaderProxy->GetProperty("FrequencyScaleInfo"));
    meshReaderProxy->GetProperty("PhaseShift")
      ->Copy(meshReaderProxy->GetProperty("PhaseShiftInfo"));

    // Make representations.
    controller->Show(meshReaderProxy, 0, meshView->getViewProxy());
    controller->Show(meshReaderProxy, 1, meshView->getViewProxy());
    controller->Hide(
      meshReaderProxy, 1, meshView->getViewProxy()); // keeping what old code was doing.

    // We have already made the representations and pushed everything to the
    // server manager.  Thus, there is no state left to be modified.
    meshReader->setModifiedState(pqProxy::UNMODIFIED);
  }

  QStringList particlesFiles = this->ui->particlesFile->filenames();
  if (!particlesFiles.isEmpty())
  {
    pqPipelineSource* particlesReader =
      builder->createReader("sources", "SLACParticleReader", particlesFiles, this->Server);

    // Make representations.
    controller->Show(particlesReader->getSourceProxy(), 0, meshView->getViewProxy());
    controller->SetVisibility(particlesReader->getSourceProxy(), 0, meshView->getViewProxy(),
      manager->actionShowParticles()->isChecked()); // keeping what old code was doing.

    // We have already made the representations and pushed everything to the
    // server manager.  Thus, there is no state left to be modified.
    particlesReader->setModifiedState(pqProxy::UNMODIFIED);
  }

  END_UNDO_SET();
  Q_EMIT this->createdPipeline();
}
