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
#include "ui_pqSLACActionHolder.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqSLACDataLoadManager.h"
#include "pqSMAdaptor.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqXYChartView.h"
#include "vtkAlgorithm.h"
#include "vtkNew.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMChartSeriesSelectionDomain.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkTable.h"
#include "vtkTemporalRanges.h"

#include <QMainWindow>
#include <QPointer>
#include <QtDebug>

//=============================================================================
class pqSLACManager::pqInternal
{
public:
  Ui::pqSLACActionHolder Actions;
  QWidget* ActionPlaceholder;
};

//=============================================================================
QPointer<pqSLACManager> pqSLACManagerInstance = nullptr;

pqSLACManager* pqSLACManager::instance()
{
  if (pqSLACManagerInstance == nullptr)
  {
    pqApplicationCore* core = pqApplicationCore::instance();
    if (!core)
    {
      qFatal("Cannot use the SLAC Tools without an application core instance.");
      return nullptr;
    }

    pqSLACManagerInstance = new pqSLACManager(core);
  }

  return pqSLACManagerInstance;
}

//-----------------------------------------------------------------------------
pqSLACManager::pqSLACManager(QObject* p)
  : QObject(p)
{
  this->Internal = new pqSLACManager::pqInternal;

  this->ScaleFieldsByCurrentTimeStep = true;

  // This widget serves no real purpose other than initializing the Actions
  // structure created with designer that holds the actions.
  this->Internal->ActionPlaceholder = new QWidget(nullptr);
  this->Internal->Actions.setupUi(this->Internal->ActionPlaceholder);

  this->actionShowParticles()->setChecked(true);

  QObject::connect(
    this->actionDataLoadManager(), SIGNAL(triggered(bool)), this, SLOT(showDataLoadManager()));
  QObject::connect(this->actionShowEField(), SIGNAL(triggered(bool)), this, SLOT(showEField()));
  QObject::connect(this->actionShowBField(), SIGNAL(triggered(bool)), this, SLOT(showBField()));
  QObject::connect(
    this->actionShowParticles(), SIGNAL(toggled(bool)), this, SLOT(showParticles(bool)));
  QObject::connect(this->actionSolidMesh(), SIGNAL(triggered(bool)), this, SLOT(showSolidMesh()));
  QObject::connect(this->actionWireframeSolidMesh(), SIGNAL(triggered(bool)), this,
    SLOT(showWireframeSolidMesh()));
  QObject::connect(this->actionWireframeAndBackMesh(), SIGNAL(triggered(bool)), this,
    SLOT(showWireframeAndBackMesh()));
  QObject::connect(this->actionPlotOverZ(), SIGNAL(triggered(bool)), this, SLOT(createPlotOverZ()));
  QObject::connect(
    this->actionToggleBackgroundBW(), SIGNAL(triggered(bool)), this, SLOT(toggleBackgroundBW()));
  QObject::connect(this->actionShowStandardViewpoint(), SIGNAL(triggered(bool)), this,
    SLOT(showStandardViewpoint()));
  QObject::connect(
    this->actionTemporalResetRange(), SIGNAL(triggered(bool)), this, SLOT(resetRangeTemporal()));
  QObject::connect(this->actionCurrentTimeResetRange(), SIGNAL(triggered(bool)), this,
    SLOT(resetRangeCurrentTime()));

  this->checkActionEnabled();
}

pqSLACManager::~pqSLACManager()
{
  delete this->Internal->ActionPlaceholder;
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QAction* pqSLACManager::actionDataLoadManager()
{
  return this->Internal->Actions.actionDataLoadManager;
}

QAction* pqSLACManager::actionShowEField()
{
  return this->Internal->Actions.actionShowEField;
}

QAction* pqSLACManager::actionShowBField()
{
  return this->Internal->Actions.actionShowBField;
}

QAction* pqSLACManager::actionShowParticles()
{
  return this->Internal->Actions.actionShowParticles;
}

QAction* pqSLACManager::actionSolidMesh()
{
  return this->Internal->Actions.actionSolidMesh;
}

QAction* pqSLACManager::actionWireframeSolidMesh()
{
  return this->Internal->Actions.actionWireframeSolidMesh;
}

QAction* pqSLACManager::actionWireframeAndBackMesh()
{
  return this->Internal->Actions.actionWireframeAndBackMesh;
}

QAction* pqSLACManager::actionPlotOverZ()
{
  return this->Internal->Actions.actionPlotOverZ;
}

QAction* pqSLACManager::actionToggleBackgroundBW()
{
  return this->Internal->Actions.actionToggleBackgroundBW;
}

QAction* pqSLACManager::actionShowStandardViewpoint()
{
  return this->Internal->Actions.actionShowStandardViewpoint;
}

QAction* pqSLACManager::actionTemporalResetRange()
{
  return this->Internal->Actions.actionTemporalResetRange;
}

QAction* pqSLACManager::actionCurrentTimeResetRange()
{
  return this->Internal->Actions.actionCurrentTimeResetRange;
}

//-----------------------------------------------------------------------------
pqServer* pqSLACManager::getActiveServer()
{
  pqApplicationCore* app = pqApplicationCore::instance();
  pqServerManagerModel* smModel = app->getServerManagerModel();
  pqServer* server = smModel->getItemAtIndex<pqServer*>(0);
  return server;
}

//-----------------------------------------------------------------------------
QWidget* pqSLACManager::getMainWindow()
{
  foreach (QWidget* topWidget, QApplication::topLevelWidgets())
  {
    if (qobject_cast<QMainWindow*>(topWidget))
      return topWidget;
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
pqView* pqSLACManager::findView(pqPipelineSource* source, int port, const QString& viewType)
{
  // Step 1, try to find a view in which the source is already shown.
  if (source)
  {
    foreach (pqView* view, source->getViews())
    {
      pqDataRepresentation* repr = source->getRepresentation(port, view);
      if (repr && repr->isVisible())
        return view;
    }
  }

  // Step 2, check to see if the active view is the right type.
  pqView* view = pqActiveObjects::instance().activeView();
  if (view->getViewType() == viewType)
    return view;

  // Step 3, check all the views and see if one is the right type and not
  // showing anything.
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smModel = core->getServerManagerModel();
  foreach (view, smModel->findItems<pqView*>())
  {
    if (view && (view->getViewType() == viewType) &&
      (view->getNumberOfVisibleRepresentations() < 1))
    {
      return view;
    }
  }

  // Give up.  A new view needs to be created.
  return nullptr;
}

pqView* pqSLACManager::getMeshView()
{
  return this->findView(this->getMeshReader(), 0, pqRenderView::renderViewType());
}

pqRenderView* pqSLACManager::getMeshRenderView()
{
  return reinterpret_cast<pqRenderView*>(this->getMeshView());
}

pqView* pqSLACManager::getPlotView()
{
  return this->findView(this->getPlotFilter(), 0, pqXYChartView::XYChartViewType());
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqSLACManager::findPipelineSource(const char* SMName)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smModel = core->getServerManagerModel();

  QList<pqPipelineSource*> sources = smModel->findItems<pqPipelineSource*>(this->getActiveServer());
  foreach (pqPipelineSource* s, sources)
  {
    if (strcmp(s->getProxy()->GetXMLName(), SMName) == 0)
      return s;
  }

  return nullptr;
}

pqPipelineSource* pqSLACManager::getMeshReader()
{
  return this->findPipelineSource("SLACReader");
}

pqPipelineSource* pqSLACManager::getParticlesReader()
{
  return this->findPipelineSource("SLACParticleReader");
}

pqPipelineSource* pqSLACManager::getPlotFilter()
{
  return this->findPipelineSource("ProbeLine");
}

pqPipelineSource* pqSLACManager::getTemporalRanges()
{
  return this->findPipelineSource("TemporalRanges");
}

//-----------------------------------------------------------------------------
static void destroyPortConsumers(pqOutputPort* port)
{
  foreach (pqPipelineSource* consumer, port->getConsumers())
  {
    pqSLACManager::destroyPipelineSourceAndConsumers(consumer);
  }
}

void pqSLACManager::destroyPipelineSourceAndConsumers(pqPipelineSource* source)
{
  if (!source)
    return;

  foreach (pqOutputPort* port, source->getOutputPorts())
  {
    destroyPortConsumers(port);
  }

  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();
  builder->destroy(source);
}

//-----------------------------------------------------------------------------
void pqSLACManager::showDataLoadManager()
{
  pqSLACDataLoadManager* dialog = new pqSLACDataLoadManager(this->getMainWindow());
  dialog->setAttribute(Qt::WA_DeleteOnClose, true);
  QObject::connect(dialog, SIGNAL(createdPipeline()), this, SLOT(checkActionEnabled()));
  QObject::connect(dialog, SIGNAL(createdPipeline()), this, SLOT(showEField()));
  QObject::connect(dialog, SIGNAL(createdPipeline()), this, SLOT(showStandardViewpoint()));
#ifdef AUTO_FIND_TEMPORAL_RANGE
  QObject::connect(dialog, SIGNAL(createdPipeline()), this, SLOT(resetRangeTemporal()));
#endif
  dialog->show();
}

//-----------------------------------------------------------------------------
void pqSLACManager::checkActionEnabled()
{
  pqPipelineSource* meshReader = this->getMeshReader();
  pqPipelineSource* particlesReader = this->getParticlesReader();

  if (!meshReader)
  {
    this->actionShowEField()->setEnabled(false);
    this->actionShowBField()->setEnabled(false);
    this->actionSolidMesh()->setEnabled(false);
    this->actionWireframeSolidMesh()->setEnabled(false);
    this->actionWireframeAndBackMesh()->setEnabled(false);
    this->actionPlotOverZ()->setEnabled(false);
    this->actionTemporalResetRange()->setEnabled(false);
    this->actionCurrentTimeResetRange()->setEnabled(false);
  }
  else
  {
    pqOutputPort* outputPort = meshReader->getOutputPort(0);
    vtkPVDataInformation* dataInfo = outputPort->getDataInformation();
    vtkPVDataSetAttributesInformation* pointFields = dataInfo->GetPointDataInformation();

    this->actionShowEField()->setEnabled(pointFields->GetArrayInformation("efield") != nullptr);
    this->actionShowBField()->setEnabled(pointFields->GetArrayInformation("bfield") != nullptr);

    this->actionSolidMesh()->setEnabled(true);
    this->actionWireframeSolidMesh()->setEnabled(true);
    this->actionWireframeAndBackMesh()->setEnabled(true);

    this->actionPlotOverZ()->setEnabled(pointFields->GetArrayInformation("efield") != nullptr);

    this->actionTemporalResetRange()->setEnabled(true);
    this->actionCurrentTimeResetRange()->setEnabled(true);
  }

  this->actionShowParticles()->setEnabled(particlesReader != nullptr);
}

//-----------------------------------------------------------------------------
void pqSLACManager::showField(QString name)
{
  this->showField(name.toLocal8Bit().data());
}

void pqSLACManager::showField(const char* name)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqUndoStack* stack = core->getUndoStack();

  pqPipelineSource* meshReader = this->getMeshReader();
  if (!meshReader)
    return;

  pqView* view = this->getMeshView();
  if (!view)
    return;

  // Get the (downcasted) representation.
  pqDataRepresentation* _repr = meshReader->getRepresentation(0, view);
  pqPipelineRepresentation* repr = qobject_cast<pqPipelineRepresentation*>(_repr);
  if (!repr)
  {
    qWarning() << "Could not find representation object.";
    return;
  }

  // Get information about the field we are supposed to be showing.
  vtkPVDataInformation* dataInfo = repr->getInputDataInformation();
  vtkPVDataSetAttributesInformation* pointInfo = dataInfo->GetPointDataInformation();
  vtkPVArrayInformation* arrayInfo = pointInfo->GetArrayInformation(name);
  if (!arrayInfo)
    return;

  if (stack)
    stack->beginUndoSet(QString("Show field %1").arg(name));

  this->CurrentFieldName = name;

  // Set the field to color by.
  vtkSMPVRepresentationProxy::SetScalarColoring(repr->getProxy(), name, vtkDataObject::POINT);

  // Adjust the color map to be rainbow.
  pqScalarsToColors* lut = repr->getLookupTable();
  vtkSMProxy* lutProxy = lut->getProxy();

  pqSMAdaptor::setEnumerationProperty(lutProxy->GetProperty("ColorSpace"), "HSV");

  // Control points are 4-tuples comprising scalar value + RGB
  QList<QVariant> RGBPoints;
  RGBPoints << 0.0 << 0.0 << 0.0 << 1.0;
  RGBPoints << 1.0 << 1.0 << 0.0 << 0.0;
  pqSMAdaptor::setMultipleElementProperty(lutProxy->GetProperty("RGBPoints"), RGBPoints);

  // NaN color is a 3-tuple RGB.
  QList<QVariant> NanColor;
  NanColor << 0.5 << 0.5 << 0.5;
  pqSMAdaptor::setMultipleElementProperty(lutProxy->GetProperty("NanColor"), NanColor);

  // Set up range of scalars to best we know of.
  pqPipelineSource* temporalRanges = this->getTemporalRanges();
  if (temporalRanges)
  {
#ifdef AUTO_FIND_TEMPORAL_RANGE
    // NOTE TO DEVELOPER:
    // ClientDeliveryStrategy is no longer available. However
    // pqOutputPort::getTemporalDataInformation() is available to fetch temporal
    // data information for any pipeline source. This code can then be updated
    // to use this API.

    // Retrieve the ranges of data over all time.
    vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
    vtkSMRepresentationStrategy* delivery = vtkSMRepresentationStrategy::SafeDownCast(
      pm->NewProxy("strategies", "ClientDeliveryStrategy"));
    vtkSMSourceProxy* temporalRangesProxy =
      vtkSMSourceProxy::SafeDownCast(temporalRanges->getProxy());
    delivery->AddInput(temporalRangesProxy, nullptr);
    delivery->Update();
    vtkAlgorithm* alg = vtkAlgorithm::SafeDownCast(delivery->GetOutput()->GetClientSideObject());
    vtkTable* ranges = vtkTable::SafeDownCast(alg->GetOutputDataObject(0));
    vtkAbstractArray* rangeData = ranges->GetColumnByName(name);
    if (!rangeData)
    {
      QString magName = QString("%1_M").arg(name);
      rangeData = ranges->GetColumnByName(magName.toLocal8Bit().data());
    }

    this->CurrentFieldRangeKnown = true;
    this->CurrentFieldRange[0] =
      rangeData->GetVariantValue(vtkTemporalRanges::MINIMUM_ROW).ToDouble();
    this->CurrentFieldRange[1] =
      rangeData->GetVariantValue(vtkTemporalRanges::MAXIMUM_ROW).ToDouble();
    this->CurrentFieldAverage =
      rangeData->GetVariantValue(vtkTemporalRanges::AVERAGE_ROW).ToDouble();

    // Cleanup.
    delivery->Delete();
#endif
  }
  else
  {
    this->CurrentFieldRangeKnown = false;
  }

  if (this->ScaleFieldsByCurrentTimeStep || !this->CurrentFieldRangeKnown)
  {
    // Set the range of the scalars to the current range of the field.
    double range[2];
    arrayInfo->GetComponentRange(-1, range);
    lut->setScalarRange(range[0], range[1]);
  }
  else
  {
    lut->setScalarRange(0.0, 2.0 * this->CurrentFieldAverage);
  }

  lutProxy->UpdateVTKObjects();

  this->updatePlotField();

  if (stack)
    stack->endUndoSet();

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
void pqSLACManager::updatePlotField()
{
  // Get the plot source, view, and representation.
  pqPipelineSource* plotFilter = this->getPlotFilter();
  if (!plotFilter)
    return;

  pqView* plotView = this->getPlotView();
  if (!plotView)
    return;

  pqDataRepresentation* repr = plotFilter->getRepresentation(plotView);
  if (!repr)
    return;
  vtkSMProxy* reprProxy = repr->getProxy();

  QString fieldName = this->CurrentFieldName;

  if (fieldName == "Solid Color")
    fieldName = "efield";

  // Iterate over all the series.  Turn them all off except the one associated
  // with the viewed mesh.
  auto domain =
    reprProxy->GetProperty("SeriesVisibility")->FindDomain<vtkSMChartSeriesSelectionDomain>();
  if (domain == nullptr)
  {
    return;
  }

  QList<QVariant> visibility;
  for (unsigned int i = 0; i < domain->GetNumberOfStrings(); i++)
  {
    QString seriesName = domain->GetString(i);
    if ((fieldName == seriesName) || (fieldName + "_Magnitude" == seriesName))
    {
      fieldName = seriesName;
      visibility << seriesName << 1;

      double color[3] = { 0.0, 0.0, 0.0 };
      vtkSMPropertyHelper(reprProxy, "SeriesColor")
        .SetStatus(seriesName.toLocal8Bit().data(), color, 3);

      vtkSMPropertyHelper(reprProxy, "SeriesLineThickness")
        .SetStatus(seriesName.toLocal8Bit().data(), 1);

      vtkSMPropertyHelper(reprProxy, "SeriesLineStyle")
        .SetStatus(seriesName.toLocal8Bit().data(), 1);
    }
    else
    {
      visibility << seriesName << 0;
    }
  }
  pqSMAdaptor::setMultipleElementProperty(reprProxy->GetProperty("SeriesVisibility"), visibility);
  reprProxy->UpdateVTKObjects();

  // Update the view options.
  vtkSMProxy* viewProxy = plotView->getProxy();

  pqSMAdaptor::setElementProperty(viewProxy->GetProperty("ShowLegend"), 0);

  pqSMAdaptor::setElementProperty(viewProxy->GetProperty("LeftAxisTitle"), fieldName);

  if (this->CurrentFieldRangeKnown)
  {
    QList<QVariant> axisBehavior;
    axisBehavior << 1 << 0 << 0 << 0;
    pqSMAdaptor::setMultipleElementProperty(viewProxy->GetProperty("AxisBehavior"), axisBehavior);

    QList<QVariant> axisRange;
    axisRange << this->CurrentFieldRange[0] << this->CurrentFieldRange[1] << 0.0 << 0.0 << 0.0
              << 0.0 << 0.0 << 0.0;
    pqSMAdaptor::setMultipleElementProperty(viewProxy->GetProperty("AxisRange"), axisRange);
  }
  else
  {
    QList<QVariant> axisBehavior;
    axisBehavior << 0 << 0 << 0 << 0;
    pqSMAdaptor::setMultipleElementProperty(viewProxy->GetProperty("AxisBehavior"), axisBehavior);
  }

  viewProxy->UpdateVTKObjects();

  plotView->render();
}

//-----------------------------------------------------------------------------
void pqSLACManager::showParticles(bool show)
{
  pqPipelineSource* reader = this->getParticlesReader();
  if (!reader)
    return;

  pqView* view = this->getMeshView();
  if (!view)
    return;

  pqDataRepresentation* repr = reader->getRepresentation(view);
  repr->setVisible(show);

  view->render();
}

//-----------------------------------------------------------------------------
void pqSLACManager::showSolidMesh()
{
  pqPipelineSource* reader = this->getMeshReader();
  if (!reader)
    return;

  pqView* view = this->getMeshView();
  if (!view)
    return;

  pqDataRepresentation* repr = reader->getRepresentation(0, view);
  if (!repr)
    return;
  vtkSMProxy* reprProxy = repr->getProxy();

  pqApplicationCore* core = pqApplicationCore::instance();
  pqUndoStack* stack = core->getUndoStack();

  if (stack)
    stack->beginUndoSet("Show Solid Mesh");

  pqSMAdaptor::setEnumerationProperty(reprProxy->GetProperty("Representation"), "Surface");
  pqSMAdaptor::setEnumerationProperty(
    reprProxy->GetProperty("BackfaceRepresentation"), "Follow Frontface");

  reprProxy->UpdateVTKObjects();

  if (stack)
    stack->endUndoSet();

  view->render();
}

void pqSLACManager::showWireframeSolidMesh()
{
  pqPipelineSource* reader = this->getMeshReader();
  if (!reader)
    return;

  pqView* view = this->getMeshView();
  if (!view)
    return;

  pqDataRepresentation* repr = reader->getRepresentation(0, view);
  if (!repr)
    return;
  vtkSMProxy* reprProxy = repr->getProxy();

  pqApplicationCore* core = pqApplicationCore::instance();
  pqUndoStack* stack = core->getUndoStack();

  if (stack)
    stack->beginUndoSet("Show Wireframe Mesh");

  pqSMAdaptor::setEnumerationProperty(
    reprProxy->GetProperty("Representation"), "Surface With Edges");
  pqSMAdaptor::setEnumerationProperty(
    reprProxy->GetProperty("BackfaceRepresentation"), "Follow Frontface");

  reprProxy->UpdateVTKObjects();

  if (stack)
    stack->endUndoSet();

  view->render();
}

void pqSLACManager::showWireframeAndBackMesh()
{
  pqPipelineSource* reader = this->getMeshReader();
  if (!reader)
    return;

  pqView* view = this->getMeshView();
  if (!view)
    return;

  pqDataRepresentation* repr = reader->getRepresentation(0, view);
  if (!repr)
    return;
  vtkSMProxy* reprProxy = repr->getProxy();

  pqApplicationCore* core = pqApplicationCore::instance();
  pqUndoStack* stack = core->getUndoStack();

  if (stack)
    stack->beginUndoSet("Show Wireframe Front and Solid Back");

  pqSMAdaptor::setEnumerationProperty(reprProxy->GetProperty("Representation"), "Wireframe");
  pqSMAdaptor::setEnumerationProperty(reprProxy->GetProperty("BackfaceRepresentation"), "Surface");

  reprProxy->UpdateVTKObjects();

  if (stack)
    stack->endUndoSet();

  view->render();
}

//-----------------------------------------------------------------------------
void pqSLACManager::createPlotOverZ()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  pqPipelineSource* meshReader = this->getMeshReader();
  if (!meshReader)
    return;

  BEGIN_UNDO_SET("Plot Over Z");

  // Determine view.  Do this before deleting existing pipeline objects.
  pqView* plotView = this->getPlotView();

  // Delete existing plot objects.  We will replace them.
  this->destroyPipelineSourceAndConsumers(this->getPlotFilter());

  // Turn on reading the internal volume, which is necessary for plotting
  // through the volume.
  vtkSMProxy* meshReaderProxy = meshReader->getProxy();
  pqSMAdaptor::setElementProperty(meshReaderProxy->GetProperty("ReadInternalVolume"), true);
  meshReaderProxy->UpdateVTKObjects();
  meshReader->updatePipeline();

  // Get the mesh data bounds (which we will use later to set up the plot).
  vtkPVDataInformation* dataInfo = meshReader->getOutputPort(1)->getDataInformation();
  double bounds[6];
  dataInfo->GetBounds(bounds);

  // Create the plot filter.
  QMap<QString, QList<pqOutputPort*> > namedInputs;
  QList<pqOutputPort*> inputs;
  inputs.push_back(meshReader->getOutputPort(1));
  namedInputs["Input"] = inputs;
  pqPipelineSource* plotFilter =
    builder->createFilter("filters", "ProbeLine", namedInputs, this->getActiveServer());

  // Set up the line for the plot.  The line is one of the inputs to the filter
  // which is implicitly set up through a proxy list domain.
  vtkSMProxy* plotProxy = plotFilter->getProxy();
  pqSMProxy lineProxy = pqSMAdaptor::getProxyProperty(plotProxy->GetProperty("Source"));
  if (!lineProxy)
  {
    qWarning() << "Could not retrieve plot line source.  "
               << "Plot not set up correctly.";
  }
  else
  {
    QList<QVariant> minPoint;
    minPoint << 0.0 << 0.0 << bounds[4];
    pqSMAdaptor::setMultipleElementProperty(lineProxy->GetProperty("Point1"), minPoint);
    QList<QVariant> maxPoint;
    maxPoint << 0.0 << 0.0 << bounds[5];
    pqSMAdaptor::setMultipleElementProperty(lineProxy->GetProperty("Point2"), maxPoint);
    pqSMAdaptor::setElementProperty(lineProxy->GetProperty("Resolution"), 1000);
    lineProxy->UpdateVTKObjects();
  }
  plotFilter->updatePipeline();

  // Make representation
  if (plotView)
  {
    controller->Show(plotFilter->getSourceProxy(), 0, plotView->getViewProxy());
  }
  else
  {
    auto newview = vtkSMViewProxy::SafeDownCast(
      controller->ShowInPreferredView(plotFilter->getSourceProxy(), 0, nullptr));
    if (newview)
    {
      const auto& activeObjects = pqActiveObjects::instance();
      controller->AssignViewToLayout(
        newview, activeObjects.activeLayout(), activeObjects.activeLayoutLocation());
    }
    else
    {
      qWarning() << "Failed to create 'Plot View'.";
    }
  }

  this->updatePlotField();

  // We have already made the representations and pushed everything to the
  // server manager.  Thus, there is no state left to be modified.
  meshReader->setModifiedState(pqProxy::UNMODIFIED);
  plotFilter->setModifiedState(pqProxy::UNMODIFIED);

  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqSLACManager::toggleBackgroundBW()
{
  pqRenderView* view = this->getMeshRenderView();
  if (!view)
    return;

  vtkSMProxy* viewProxy = view->getProxy();
  vtkSMProperty* smProperty = viewProxy->GetProperty("Background");
  if (!smProperty)
  {
    return;
  }

  double color[3];
  vtkSMPropertyHelper helper(smProperty);
  helper.Get(color, 3);

  if ((color[0] == 0.0) && (color[1] == 0.0) && (color[2] == 0.0))
  {
    color[0] = color[1] = color[2] = 1.0;
    helper.Set(color, 3);
  }
  else if ((color[0] == 1.0) && (color[1] == 1.0) && (color[2] == 1.0))
  {
    smProperty->ResetToXMLDefaults();
  }
  else
  {
    color[0] = color[1] = color[2] = 0.0;
    helper.Set(color, 3);
  }
  viewProxy->UpdateVTKObjects();
  view->render();
}

//-----------------------------------------------------------------------------
void pqSLACManager::showStandardViewpoint()
{
  pqRenderView* view = qobject_cast<pqRenderView*>(this->getMeshView());
  if (view)
  {
    view->resetViewDirection(1, 0, 0, 0, 1, 0);
  }
  view->render();
}

//-----------------------------------------------------------------------------
void pqSLACManager::resetRangeTemporal()
{
  this->ScaleFieldsByCurrentTimeStep = false;

  // Check to see if the ranges are already computed.
  if (this->getTemporalRanges())
  {
    this->showField(this->CurrentFieldName);
    return;
  }

  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();
  pqUndoStack* stack = core->getUndoStack();

  pqPipelineSource* meshReader = this->getMeshReader();
  if (!meshReader)
    return;

  if (stack)
    stack->beginUndoSet("Compute Ranges Over Time");

  // Turn on reading the internal volume, which is necessary for plotting
  // through the volume.
  vtkSMProxy* meshReaderProxy = meshReader->getProxy();
  pqSMAdaptor::setElementProperty(meshReaderProxy->GetProperty("ReadInternalVolume"), true);
  meshReaderProxy->UpdateVTKObjects();
  meshReader->updatePipeline();

  // Create the temporal ranges filter.
  pqPipelineSource* rangeFilter = builder->createFilter("filters", "TemporalRanges", meshReader, 1);

  this->showField(this->CurrentFieldName);

  // We have already pushed everything to the server manager, and I don't want
  // to bother making representations.  Thus, it is unnecessary to make any
  // further modifications.
  meshReader->setModifiedState(pqProxy::UNMODIFIED);
  rangeFilter->setModifiedState(pqProxy::UNMODIFIED);

  if (stack)
    stack->endUndoSet();
}

//-----------------------------------------------------------------------------
void pqSLACManager::resetRangeCurrentTime()
{
  this->ScaleFieldsByCurrentTimeStep = true;
  this->showField(this->CurrentFieldName);
}
