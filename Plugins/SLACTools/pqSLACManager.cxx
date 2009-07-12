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

#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMProxy.h"

#include "pqActiveView.h"
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"
#include "pqView.h"

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

  this->actionShowParticles()->setChecked(true);

  QObject::connect(this->actionDataLoadManager(), SIGNAL(triggered(bool)),
                   this, SLOT(showDataLoadManager()));
  QObject::connect(this->actionShowEField(), SIGNAL(triggered(bool)),
                   this, SLOT(showEField()));
  QObject::connect(this->actionShowBField(), SIGNAL(triggered(bool)),
                   this, SLOT(showBField()));
  QObject::connect(this->actionShowParticles(), SIGNAL(toggled(bool)),
                   this, SLOT(showParticles(bool)));
  QObject::connect(this->actionSolidMesh(), SIGNAL(triggered(bool)),
                   this, SLOT(showSolidMesh()));
  QObject::connect(this->actionWireframeSolidMesh(), SIGNAL(triggered(bool)),
                   this, SLOT(showWireframeSolidMesh()));
  QObject::connect(this->actionWireframeAndBackMesh(), SIGNAL(triggered(bool)),
                   this, SLOT(showWireframeAndBackMesh()));

  this->checkActionEnabled();
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

QAction *pqSLACManager::actionShowEField()
{
  return this->Internal->Actions.actionShowEField;
}

QAction *pqSLACManager::actionShowBField()
{
  return this->Internal->Actions.actionShowBField;
}

QAction *pqSLACManager::actionShowParticles()
{
  return this->Internal->Actions.actionShowParticles;
}

QAction *pqSLACManager::actionSolidMesh()
{
  return this->Internal->Actions.actionSolidMesh;
}

QAction *pqSLACManager::actionWireframeSolidMesh()
{
  return this->Internal->Actions.actionWireframeSolidMesh;
}

QAction *pqSLACManager::actionWireframeAndBackMesh()
{
  return this->Internal->Actions.actionWireframeAndBackMesh;
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
pqView *pqSLACManager::view3D()
{
  pqView *view = pqActiveView::instance().current();
  // TODO: Check to make sure the active view is 3D.  If not, find one.  This
  // can be done by getting a pqServerManagerModel (from pqAppliationCore)
  // and querying pqView.  If no view is valid, create one.  Probably look
  // at pqDisplayPolicy::createPreferredRepresentation for that one.
  //
  // On second thought, since I have to be able to query for the mesh file
  // anyway, perhaps I should just find that and return a view in which that
  // is defined.  Then let pqSLACDataLoadManager create a necessary view on
  // creating the mesh reader if necessary.
  return view;
}

//-----------------------------------------------------------------------------
pqPipelineSource *pqSLACManager::findPipelineSource(const char *SMName)
{
  pqApplicationCore *core = pqApplicationCore::instance();
  pqServerManagerModel *smModel = core->getServerManagerModel();

  QList<pqPipelineSource*> sources
    = smModel->findItems<pqPipelineSource*>(this->activeServer());
  foreach(pqPipelineSource *s, sources)
    {
    if (strcmp(s->getProxy()->GetXMLName(), SMName) == 0) return s;
    }

  return NULL;
}

pqPipelineSource *pqSLACManager::meshReader()
{
  return this->findPipelineSource("SLACReader");
}

pqPipelineSource *pqSLACManager::particlesReader()
{
  return this->findPipelineSource("SLACParticleReader");
}

//-----------------------------------------------------------------------------
static void destroyPortConsumers(pqOutputPort *port)
{
  foreach (pqPipelineSource *consumer, port->getConsumers())
    {
    pqSLACManager::destroyPipelineSourceAndConsumers(consumer);
    }
}

void pqSLACManager::destroyPipelineSourceAndConsumers(pqPipelineSource *source)
{
  if (!source) return;

  foreach (pqOutputPort *port, source->getOutputPorts())
    {
    destroyPortConsumers(port);
    }

  pqApplicationCore *core = pqApplicationCore::instance();
  pqObjectBuilder *builder = core->getObjectBuilder();
  builder->destroy(source);
}

//-----------------------------------------------------------------------------
void pqSLACManager::showDataLoadManager()
{
  pqSLACDataLoadManager *dialog = new pqSLACDataLoadManager(this->mainWindow());
  dialog->setAttribute(Qt::WA_DeleteOnClose, true);
  QObject::connect(dialog, SIGNAL(createdPipeline()),
                   this, SLOT(checkActionEnabled()));
  dialog->show();
}

//-----------------------------------------------------------------------------
void pqSLACManager::checkActionEnabled()
{
  if (!this->meshReader())
    {
    this->actionShowEField()->setEnabled(false);
    this->actionShowBField()->setEnabled(false);
    this->actionSolidMesh()->setEnabled(false);
    this->actionWireframeSolidMesh()->setEnabled(false);
    this->actionWireframeAndBackMesh()->setEnabled(false);
    }
  else
    {
    pqOutputPort *outputPort = this->meshReader()->getOutputPort(0);
    vtkPVDataInformation *dataInfo = outputPort->getDataInformation();
    vtkPVDataSetAttributesInformation *pointFields
      = dataInfo->GetPointDataInformation();

    this->actionShowEField()->setEnabled(
                            pointFields->GetArrayInformation("efield") != NULL);
    this->actionShowBField()->setEnabled(
                            pointFields->GetArrayInformation("bfield") != NULL);

    this->actionSolidMesh()->setEnabled(true);
    this->actionWireframeSolidMesh()->setEnabled(true);
    this->actionWireframeAndBackMesh()->setEnabled(true);
    }

  this->actionShowParticles()->setEnabled(this->particlesReader() != NULL);
}

//-----------------------------------------------------------------------------
void pqSLACManager::showField(const char *name)
{
  pqPipelineSource *reader = this->meshReader();
  if (!reader) return;

  pqView *view = this->view3D();
  if (!view) return;

  // Get the (downcasted) representation.
  pqDataRepresentation *_repr = reader->getRepresentation(0, view);
  pqPipelineRepresentation *repr
    = qobject_cast<pqPipelineRepresentation*>(_repr);
  if (!repr)
    {
    qWarning() << "Could not find representation object.";
    return;
    }

  // Set the field to color by.
  repr->setColorField(QString("%1 (point)").arg(name));

  // Adjust the color map to be rainbow.
  pqScalarsToColors *lut = repr->getLookupTable();
  vtkSMProxy *lutProxy = lut->getProxy();

  pqSMAdaptor::setEnumerationProperty(lutProxy->GetProperty("ColorSpace"),
                                      "HSV");

  // Control points are 4-tuples comprising scalar value + RGB
  QList<QVariant> RGBPoints;
  RGBPoints << 0.0 << 0.0 << 0.0 << 1.0;
  RGBPoints << 1.0 << 1.0 << 0.0 << 0.0;
  pqSMAdaptor::setMultipleElementProperty(lutProxy->GetProperty("RGBPoints"),
                                          RGBPoints);

  // Set the range of the scalars to the current range of the field.
  double range[2];
  vtkPVDataInformation *dataInfo = repr->getInputDataInformation();
  vtkPVDataSetAttributesInformation *pointInfo
    = dataInfo->GetPointDataInformation();
  vtkPVArrayInformation *arrayInfo = pointInfo->GetArrayInformation(name);
  arrayInfo->GetComponentRange(-1, range);
  lut->setScalarRange(range[0], range[1]);

  lutProxy->UpdateVTKObjects();

  view->render();
}

void pqSLACManager::showEField()
{
  this->showField("efield");
}

void pqSLACManager::showBField()
{
  this->showField("bfield");
}

//-----------------------------------------------------------------------------
void pqSLACManager::showParticles(bool show)
{
  pqPipelineSource *reader = this->particlesReader();
  if (!reader) return;

  pqView *view = this->view3D();
  if (!view) return;

  pqDataRepresentation *repr = reader->getRepresentation(view);
  repr->setVisible(show);
}

//-----------------------------------------------------------------------------
void pqSLACManager::showSolidMesh()
{
  pqPipelineSource *reader = this->meshReader();
  if (!reader) return;

  pqView *view = this->view3D();
  if (!view) return;

  pqDataRepresentation *repr = reader->getRepresentation(0, view);
  if (!repr) return;
  vtkSMProxy *reprProxy = repr->getProxy();

  pqSMAdaptor::setEnumerationProperty(
                           reprProxy->GetProperty("Representation"), "Surface");
  pqSMAdaptor::setEnumerationProperty(
          reprProxy->GetProperty("BackfaceRepresentation"), "Follow Frontface");

  reprProxy->UpdateVTKObjects();
}

void pqSLACManager::showWireframeSolidMesh()
{
  pqPipelineSource *reader = this->meshReader();
  if (!reader) return;

  pqView *view = this->view3D();
  if (!view) return;

  pqDataRepresentation *repr = reader->getRepresentation(0, view);
  if (!repr) return;
  vtkSMProxy *reprProxy = repr->getProxy();

  pqSMAdaptor::setEnumerationProperty(
                reprProxy->GetProperty("Representation"), "Surface With Edges");
  pqSMAdaptor::setEnumerationProperty(
          reprProxy->GetProperty("BackfaceRepresentation"), "Follow Frontface");

  reprProxy->UpdateVTKObjects();
}

void pqSLACManager::showWireframeAndBackMesh()
{
  pqPipelineSource *reader = this->meshReader();
  if (!reader) return;

  pqView *view = this->view3D();
  if (!view) return;

  pqDataRepresentation *repr = reader->getRepresentation(0, view);
  if (!repr) return;
  vtkSMProxy *reprProxy = repr->getProxy();

  pqSMAdaptor::setEnumerationProperty(
                         reprProxy->GetProperty("Representation"), "Wireframe");
  pqSMAdaptor::setEnumerationProperty(
                   reprProxy->GetProperty("BackfaceRepresentation"), "Surface");

  reprProxy->UpdateVTKObjects();
}
