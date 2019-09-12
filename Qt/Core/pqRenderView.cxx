/*=========================================================================

   Program: ParaView
   Module:    pqRenderView.cxx

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
#include "pqRenderView.h"

// ParaView Server Manager includes.
#include "pqQVTKWidgetBase.h"
#include "vtkCollection.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkIntArray.h"
#include "vtkPVDataInformation.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderViewSettings.h"
#include "vtkProcessModule.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMInteractionUndoStackBuilder.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSMUndoStack.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredData.h"

// Qt includes.
#include <QEvent>
#include <QFileInfo>
#include <QGridLayout>
#include <QList>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QSet>
#include <QtDebug>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqLinkViewWidget.h"
#include "pqOptions.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"

namespace
{

// converts a vtkSMRepresentationProxy to its corresponding pqDataRepresentation
pqDataRepresentation* findRepresentationFromProxy(vtkSMRepresentationProxy* proxy)
{
  if (!proxy)
  {
    return 0;
  }

  pqServerManagerModel* smm = pqApplicationCore::instance()->getServerManagerModel();

  return smm->findItem<pqDataRepresentation*>(proxy);
}

} // end anonymous namespace

class pqRenderView::pqInternal
{
public:
  vtkSmartPointer<vtkSMUndoStack> InteractionUndoStack;
  vtkSmartPointer<vtkSMInteractionUndoStackBuilder> UndoStackBuilder;
  QList<pqRenderView*> LinkedUndoStacks;
  bool UpdatingStack;
  int CurrentInteractionMode;

  pqInternal()
  {
    this->CurrentInteractionMode = -1;
    this->UpdatingStack = false;
    this->InteractionUndoStack = vtkSmartPointer<vtkSMUndoStack>::New();
    // FIXME this->InteractionUndoStack->SetClientOnly(true);
    this->UndoStackBuilder = vtkSmartPointer<vtkSMInteractionUndoStackBuilder>::New();
    this->UndoStackBuilder->SetUndoStack(this->InteractionUndoStack);
  }

  ~pqInternal() {}
};

namespace
{

std::string GetSelectionModifierAsString(int selectionModifier)
{
  std::string modifier;
  if (selectionModifier == pqView::PV_SELECTION_ADDITION)
  {
    modifier = "ADD";
  }
  else if (selectionModifier == pqView::PV_SELECTION_SUBTRACTION)
  {
    modifier = "SUBTRACT";
  }
  else if (selectionModifier == pqView::PV_SELECTION_TOGGLE)
  {
    modifier = "TOGGLE";
  }

  return modifier;
}

} // end anonymous namespace

//-----------------------------------------------------------------------------
void pqRenderView::InternalConstructor(vtkSMViewProxy* renModule)
{
  this->Internal = new pqRenderView::pqInternal();

  // we need to fire signals when undo stack changes.
  this->getConnector()->Connect(this->Internal->InteractionUndoStack, vtkCommand::ModifiedEvent,
    this, SLOT(onUndoStackChanged()), 0, 0, Qt::QueuedConnection);

  this->ResetCenterWithCamera = true;
  this->UseMultipleRepresentationSelection = false;
  this->getConnector()->Connect(
    renModule, vtkCommand::ResetCameraEvent, this, SLOT(onResetCameraEvent()));

  // Monitor any interaction mode change
  this->getConnector()->Connect(this->getProxy()->GetProperty("InteractionMode"),
    vtkCommand::ModifiedEvent, this, SLOT(onInteractionModeChange()));
}

//-----------------------------------------------------------------------------
pqRenderView::pqRenderView(const QString& group, const QString& name, vtkSMViewProxy* renModule,
  pqServer* server, QObject* _parent /*=null*/)
  : Superclass(renderViewType(), group, name, renModule, server, _parent)
{
  this->InternalConstructor(renModule);
}

//-----------------------------------------------------------------------------
pqRenderView::pqRenderView(const QString& type, const QString& group, const QString& name,
  vtkSMViewProxy* renModule, pqServer* server, QObject* _parent /*=null*/)
  : Superclass(type, group, name, renModule, server, _parent)
{
  this->InternalConstructor(renModule);
}

//-----------------------------------------------------------------------------
pqRenderView::~pqRenderView()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMRenderViewProxy* pqRenderView::getRenderViewProxy() const
{
  return vtkSMRenderViewProxy::SafeDownCast(this->getViewProxy());
}

//-----------------------------------------------------------------------------
void pqRenderView::initialize()
{
  this->Superclass::initialize();

  // initialize the interaction undo-redo stack.
  vtkSMRenderViewProxy* viewProxy = this->getRenderViewProxy();
  this->Internal->UndoStackBuilder->SetRenderView(viewProxy);
}

//-----------------------------------------------------------------------------
QWidget* pqRenderView::createWidget()
{
  QWidget* vtkwidget = this->Superclass::createWidget();
  if (pqQVTKWidgetBase* qvtkwidget = qobject_cast<pqQVTKWidgetBase*>(vtkwidget))
  {
    vtkSMRenderViewProxy* renModule = this->getRenderViewProxy();
    qvtkwidget->setRenderWindow(renModule->GetRenderWindow());
    // This is needed to ensure that the interactor is initialized with
    // ParaView specific interactor styles etc.
    renModule->SetupInteractor(qvtkwidget->interactor());
  }
  return vtkwidget;
}

//-----------------------------------------------------------------------------
void pqRenderView::onResetCameraEvent()
{
  if (this->ResetCenterWithCamera)
  {
    this->resetParallelScale();
    this->resetCenterOfRotation();
  }
}

//-----------------------------------------------------------------------------
void pqRenderView::resetCamera()
{
  this->fakeInteraction(true);
  this->getRenderViewProxy()->ResetCamera();
  this->fakeInteraction(false);
  this->render();
}

//-----------------------------------------------------------------------------
void pqRenderView::resetCenterOfRotation()
{
  // Update center of rotation.
  vtkSMProxy* viewproxy = this->getProxy();
  viewproxy->UpdatePropertyInformation();
  QList<QVariant> values =
    pqSMAdaptor::getMultipleElementProperty(viewproxy->GetProperty("CameraFocalPointInfo"));
  this->setCenterOfRotation(values[0].toDouble(), values[1].toDouble(), values[2].toDouble());
}

//-----------------------------------------------------------------------------
void pqRenderView::resetParallelScale()
{
  // Update parallel scale.
  vtkSMProxy* viewproxy = this->getProxy();
  viewproxy->UpdatePropertyInformation();
  QVariant parallelScaleInfo =
    pqSMAdaptor::getElementProperty(viewproxy->GetProperty("CameraParallelScaleInfo"));
  this->setParallelScale(parallelScaleInfo.toDouble());
}

//-----------------------------------------------------------------------------
void pqRenderView::setOrientationAxesVisibility(bool visible)
{
  vtkSMPropertyHelper(this->getProxy(), "OrientationAxesVisibility", true).Set(visible ? 1 : 0);
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
bool pqRenderView::getOrientationAxesVisibility() const
{
  return vtkSMPropertyHelper(this->getProxy(), "OrientationAxesVisibility", true).GetAsInt() == 0
    ? false
    : true;
}

//-----------------------------------------------------------------------------
void pqRenderView::setOrientationAxesInteractivity(bool interactive)
{
  vtkSMPropertyHelper(this->getProxy(), "OrientationAxesInteractivity").Set(interactive ? 1 : 0);
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
bool pqRenderView::getOrientationAxesInteractivity() const
{
  return vtkSMPropertyHelper(this->getProxy(), "OrientationAxesInteractivity").GetAsInt() == 0
    ? false
    : true;
}

//-----------------------------------------------------------------------------
void pqRenderView::setOrientationAxesOutlineColor(const QColor& color)
{
  double colorf[3];
  colorf[0] = color.redF();
  colorf[1] = color.greenF();
  colorf[2] = color.blueF();
  vtkSMPropertyHelper(this->getProxy(), "OrientationAxesOutlineColor").Set(colorf, 3);
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
QColor pqRenderView::getOrientationAxesOutlineColor() const
{
  QColor color;
  double dcolor[3];
  vtkSMPropertyHelper(this->getProxy(), "OrientationAxesOutlineColor").Get(dcolor, 3);
  color.setRgbF(dcolor[0], dcolor[1], dcolor[2]);
  return color;
}

//-----------------------------------------------------------------------------
void pqRenderView::setOrientationAxesLabelColor(const QColor& color)
{
  double colorf[3];
  colorf[0] = color.redF();
  colorf[1] = color.greenF();
  colorf[2] = color.blueF();
  vtkSMPropertyHelper(this->getProxy(), "OrientationAxesLabelColor", true).Set(colorf, 3);
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
QColor pqRenderView::getOrientationAxesLabelColor() const
{
  QColor color;
  double dcolor[3] = { 1, 1, 1 };
  vtkSMPropertyHelper(this->getProxy(), "OrientationAxesLabelColor", true).Get(dcolor, 3);
  color.setRgbF(dcolor[0], dcolor[1], dcolor[2]);
  return color;
}

//-----------------------------------------------------------------------------
void pqRenderView::setCenterOfRotation(double x, double y, double z)
{
  QList<QVariant> positionValues;
  positionValues << x << y << z;
  vtkSMProxy* viewproxy = this->getProxy();
  pqSMAdaptor::setMultipleElementProperty(
    viewproxy->GetProperty("CenterOfRotation"), positionValues);
  viewproxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqRenderView::setParallelScale(double scale)
{
  vtkSMProxy* viewproxy = this->getProxy();
  pqSMAdaptor::setElementProperty(viewproxy->GetProperty("CameraParallelScale"), scale);
  viewproxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqRenderView::getCenterOfRotation(double center[3]) const
{
  QList<QVariant> val =
    pqSMAdaptor::getMultipleElementProperty(this->getProxy()->GetProperty("CenterOfRotation"));
  center[0] = val[0].toDouble();
  center[1] = val[1].toDouble();
  center[2] = val[2].toDouble();
}

//-----------------------------------------------------------------------------
void pqRenderView::setCenterAxesVisibility(bool visible)
{
  pqSMAdaptor::setElementProperty(
    this->getProxy()->GetProperty("CenterAxesVisibility"), visible ? 1 : 0);
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
bool pqRenderView::getCenterAxesVisibility() const
{
  return pqSMAdaptor::getElementProperty(this->getProxy()->GetProperty("CenterAxesVisibility"))
    .toBool();
}

//-----------------------------------------------------------------------------
void pqRenderView::linkToOtherView()
{
  pqLinkViewWidget* linkWidget = new pqLinkViewWidget(this);
  linkWidget->setAttribute(Qt::WA_DeleteOnClose);
  QPoint pos = this->widget()->mapToGlobal(QPoint(2, 2));
  linkWidget->move(pos);
  linkWidget->show();
}

//-----------------------------------------------------------------------------
void pqRenderView::onUndoStackChanged()
{
  bool can_undo = this->Internal->InteractionUndoStack->CanUndo();
  bool can_redo = this->Internal->InteractionUndoStack->CanRedo();

  emit this->canUndoChanged(can_undo);
  emit this->canRedoChanged(can_redo);
}

//-----------------------------------------------------------------------------
bool pqRenderView::canUndo() const
{
  return this->Internal->InteractionUndoStack->CanUndo();
}

//-----------------------------------------------------------------------------
bool pqRenderView::canRedo() const
{
  return this->Internal->InteractionUndoStack->CanRedo();
}

//-----------------------------------------------------------------------------
void pqRenderView::undo()
{
  this->Internal->InteractionUndoStack->Undo();
  this->getProxy()->UpdateVTKObjects();
  this->render();

  this->fakeUndoRedo(false, false);
}

//-----------------------------------------------------------------------------
void pqRenderView::redo()
{
  this->Internal->InteractionUndoStack->Redo();
  this->getProxy()->UpdateVTKObjects();
  this->render();

  this->fakeUndoRedo(true, false);
}

//-----------------------------------------------------------------------------
void pqRenderView::linkUndoStack(pqRenderView* other)
{
  if (other == this)
  {
    // Sanity check, nothing to link if both are same.
    return;
  }

  this->Internal->LinkedUndoStacks.push_back(other);

  // Clear all linked stacks until now.
  this->clearUndoStack();
}

//-----------------------------------------------------------------------------
void pqRenderView::unlinkUndoStack(pqRenderView* other)
{
  if (!other || other == this)
  {
    return;
  }
  this->Internal->LinkedUndoStacks.removeAll(other);
}

//-----------------------------------------------------------------------------
void pqRenderView::clearUndoStack()
{
  if (this->Internal->UpdatingStack)
  {
    return;
  }
  this->Internal->UpdatingStack = true;
  this->Internal->InteractionUndoStack->Clear();
  foreach (pqRenderView* other, this->Internal->LinkedUndoStacks)
  {
    if (other)
    {
      other->clearUndoStack();
    }
  }
  this->Internal->UpdatingStack = false;
}

//-----------------------------------------------------------------------------
void pqRenderView::fakeUndoRedo(bool fake_redo, bool self)
{
  if (this->Internal->UpdatingStack)
  {
    return;
  }
  this->Internal->UpdatingStack = true;
  if (self)
  {
    if (fake_redo)
    {
      this->Internal->InteractionUndoStack->PopRedoStack();
    }
    else
    {
      this->Internal->InteractionUndoStack->PopUndoStack();
    }
  }
  foreach (pqRenderView* other, this->Internal->LinkedUndoStacks)
  {
    if (other)
    {
      other->fakeUndoRedo(fake_redo, true);
    }
  }
  this->Internal->UpdatingStack = false;
}

//-----------------------------------------------------------------------------
void pqRenderView::fakeInteraction(bool start)
{
  if (this->Internal->UpdatingStack)
  {
    return;
  }

  this->Internal->UpdatingStack = true;

  if (start)
  {
    this->Internal->UndoStackBuilder->StartInteraction();
  }
  else
  {
    this->Internal->UndoStackBuilder->EndInteraction();
  }

  foreach (pqRenderView* other, this->Internal->LinkedUndoStacks)
  {
    other->fakeInteraction(start);
  }
  this->Internal->UpdatingStack = false;
}

//-----------------------------------------------------------------------------
void pqRenderView::resetViewDirection(
  double look_x, double look_y, double look_z, double up_x, double up_y, double up_z)
{
  vtkSMProxy* proxy = this->getProxy();
  double pos[3] = { 0, 0, 0 };
  double focal_point[3] = { look_x, look_y, look_z };
  double view_up[3] = { up_x, up_y, up_z };
  vtkSMPropertyHelper(proxy, "CameraPosition").Set(pos, 3);
  vtkSMPropertyHelper(proxy, "CameraFocalPoint").Set(focal_point, 3);
  vtkSMPropertyHelper(proxy, "CameraViewUp").Set(view_up, 3);
  proxy->UpdateVTKObjects();
  this->resetCamera();
  this->render();
}

//-----------------------------------------------------------------------------
void pqRenderView::selectOnSurface(int rect[4], int selectionModifier)
{
  QList<pqOutputPort*> opPorts;
  this->selectOnSurfaceInternal(rect, opPorts, false, selectionModifier, false);
  this->emitSelectionSignal(opPorts);
}

//-----------------------------------------------------------------------------
void pqRenderView::emitSelectionSignal(QList<pqOutputPort*> opPorts)
{
  // Fire selection event to let the world know that this view selected
  // something.
  if (opPorts.count() > 0)
  {
    emit this->selected(opPorts.value(0));
  }
  else
  {
    emit this->selected(0);
  }

  if (this->UseMultipleRepresentationSelection)
  {
    emit this->multipleSelected(opPorts);
  }
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqRenderView::pick(int pos[2])
{
  BEGIN_UNDO_EXCLUDE();
  vtkSMRenderViewProxy* renderView = this->getRenderViewProxy();
  vtkSMRepresentationProxy* repr = renderView->Pick(pos[0], pos[1]);
  pqDataRepresentation* pq_repr = findRepresentationFromProxy(repr);
  END_UNDO_EXCLUDE();
  if (pq_repr)
  {
    emit this->picked(pq_repr->getOutputPortFromInput());
  }
  return pq_repr;
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqRenderView::pickBlock(int pos[2], unsigned int& flatIndex)
{
  BEGIN_UNDO_EXCLUDE();
  vtkSMRenderViewProxy* renderView = this->getRenderViewProxy();
  vtkSMRepresentationProxy* repr = renderView->PickBlock(pos[0], pos[1], flatIndex);
  pqDataRepresentation* pq_repr = findRepresentationFromProxy(repr);
  END_UNDO_EXCLUDE();
  if (pq_repr)
  {
    emit this->picked(pq_repr->getOutputPortFromInput());
  }
  return pq_repr;
}

//-----------------------------------------------------------------------------
void pqRenderView::collectSelectionPorts(vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources, QList<pqOutputPort*>& output_ports, int selectionModifier,
  bool select_blocks)
{
  if (!selectedRepresentations || selectedRepresentations->GetNumberOfItems() <= 0)
  {
    return;
  }

  if (!selectionSources || selectionSources->GetNumberOfItems() <= 0)
  {
    return;
  }

  if (selectedRepresentations->GetNumberOfItems() != selectionSources->GetNumberOfItems())
  {
    return;
  }

  for (int i = 0; i < selectedRepresentations->GetNumberOfItems(); i++)
  {
    vtkSMRepresentationProxy* repr =
      vtkSMRepresentationProxy::SafeDownCast(selectedRepresentations->GetItemAsObject(i));
    vtkSmartPointer<vtkSMSourceProxy> selectionSource =
      vtkSMSourceProxy::SafeDownCast(selectionSources->GetItemAsObject(i));

    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    pqDataRepresentation* pqRepr = smmodel->findItem<pqDataRepresentation*>(repr);
    if (!repr)
    {
      // No data display was selected (or none that is registered).
      continue;
    }
    pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
    vtkSMSourceProxy* selectedSource =
      vtkSMSourceProxy::SafeDownCast(opPort->getSource()->getProxy());

    if (select_blocks)
    {
      // convert the index based selection to vtkSelectionNode::BLOCKS selection.
      vtkSMSourceProxy* newSelSource =
        vtkSMSourceProxy::SafeDownCast(vtkSMSelectionHelper::ConvertSelection(
          vtkSelectionNode::BLOCKS, selectionSource, selectedSource, opPort->getPortNumber()));
      selectionSource.TakeReference(newSelSource);
    }

    switch (selectionModifier)
    {
      case (pqView::PV_SELECTION_ADDITION):
        vtkSMSelectionHelper::MergeSelection(
          selectionSource, opPort->getSelectionInput(), selectedSource, opPort->getPortNumber());
        break;
      case (pqView::PV_SELECTION_SUBTRACTION):
        vtkSMSelectionHelper::SubtractSelection(
          selectionSource, opPort->getSelectionInput(), selectedSource, opPort->getPortNumber());
        break;
      case (pqView::PV_SELECTION_TOGGLE):
        vtkSMSelectionHelper::ToggleSelection(
          selectionSource, opPort->getSelectionInput(), selectedSource, opPort->getPortNumber());
        break;
      default:
        break;
    }

    opPort->setSelectionInput(selectionSource, 0);
    output_ports.append(opPort);
  }
}

//-----------------------------------------------------------------------------
void pqRenderView::selectOnSurfaceInternal(int rect[4], QList<pqOutputPort*>& pqOutputPorts,
  bool select_points, int selectionModifier, bool select_blocks)
{
  BEGIN_UNDO_EXCLUDE();

  vtkSMRenderViewProxy* renderModuleP = this->getRenderViewProxy();

  // Local variables for tracing
  std::string modifier = GetSelectionModifierAsString(selectionModifier);

  std::vector<int> rectVector(4);
  for (size_t i = 0; i < 4; ++i)
  {
    rectVector[i] = rect[i];
  }

  vtkSmartPointer<vtkCollection> selectedRepresentations = vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> selectionSources = vtkSmartPointer<vtkCollection>::New();
  if (select_points)
  {
    if (!renderModuleP->SelectSurfacePoints(rect, selectedRepresentations, selectionSources,
          this->UseMultipleRepresentationSelection, selectionModifier, select_blocks))
    {
      END_UNDO_EXCLUDE();
      return;
    }
    SM_SCOPED_TRACE(CallFunction)
      .arg("SelectSurfacePoints")
      .arg("Rectangle", rectVector)
      .arg("Modifier", modifier.size() > 0 ? modifier.c_str() : nullptr)
      .arg("comment", "create a surface points selection");
  }
  else
  {
    if (!renderModuleP->SelectSurfaceCells(rect, selectedRepresentations, selectionSources,
          this->UseMultipleRepresentationSelection, selectionModifier, select_blocks))
    {
      END_UNDO_EXCLUDE();
      return;
    }
    if (select_blocks)
    {
      SM_SCOPED_TRACE(CallFunction)
        .arg("SelectSurfaceBlocks")
        .arg("Rectangle", rectVector)
        .arg("Modifier", modifier.size() > 0 ? modifier.c_str() : nullptr)
        .arg("comment", "create a frustum selection of cells");
    }
    else
    {
      SM_SCOPED_TRACE(CallFunction)
        .arg("SelectSurfaceCells")
        .arg("Rectangle", rectVector)
        .arg("Modifier", modifier.size() > 0 ? modifier.c_str() : nullptr)
        .arg("comment", "create a surface cells selection");
    }
  }

  END_UNDO_EXCLUDE();
  this->collectSelectionPorts(
    selectedRepresentations, selectionSources, pqOutputPorts, selectionModifier, select_blocks);
}

//-----------------------------------------------------------------------------
void pqRenderView::selectPointsOnSurface(int rect[4], int selectionModifier)
{
  QList<pqOutputPort*> output_ports;
  this->selectOnSurfaceInternal(rect, output_ports, true, selectionModifier, false);
  // Fire selection event to let the world know that this view selected
  // something.
  this->emitSelectionSignal(output_ports);
}

//-----------------------------------------------------------------------------
void pqRenderView::selectPolygonPoints(vtkIntArray* polygon, int selectionModifier)
{
  QList<pqOutputPort*> output_ports;
  this->selectPolygonInternal(polygon, output_ports, true, selectionModifier, false);
  // Fire selection event to let the world know that this view selected
  // something.
  this->emitSelectionSignal(output_ports);
}

//-----------------------------------------------------------------------------
void pqRenderView::selectPolygonCells(vtkIntArray* polygon, int selectionModifier)
{
  QList<pqOutputPort*> output_ports;
  this->selectPolygonInternal(polygon, output_ports, false, selectionModifier, false);
  // Fire selection event to let the world know that this view selected
  // something.
  this->emitSelectionSignal(output_ports);
}

//-----------------------------------------------------------------------------
void pqRenderView::selectPolygonInternal(vtkIntArray* polygon, QList<pqOutputPort*>& pqOutputPorts,
  bool select_points, int selectionModifier, bool select_blocks)
{
  vtkSMRenderViewProxy* renderModuleP = this->getRenderViewProxy();
  vtkSmartPointer<vtkCollection> selectedRepresentations = vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> selectionSources = vtkSmartPointer<vtkCollection>::New();

  // Local variables for tracing
  std::string modifier = GetSelectionModifierAsString(selectionModifier);

  size_t polygonLength = static_cast<size_t>(polygon->GetNumberOfValues());
  std::vector<int> polygonVector(polygonLength);
  for (size_t i = 0; i < polygonLength; ++i)
  {
    polygonVector[i] = static_cast<int>(polygon->GetValue(static_cast<vtkIdType>(i)));
  }

  BEGIN_UNDO_EXCLUDE();
  if (select_points)
  {
    if (!renderModuleP->SelectPolygonPoints(polygon, selectedRepresentations, selectionSources,
          this->UseMultipleRepresentationSelection))
    {
      END_UNDO_EXCLUDE();
      return;
    }
    SM_SCOPED_TRACE(CallFunction)
      .arg("SelectSurfacePoints")
      .arg("Polygon", polygonVector)
      .arg("Modifier", modifier.size() > 0 ? modifier.c_str() : nullptr)
      .arg("comment", "create a surface points polygon selection");
  }
  else
  {
    if (!renderModuleP->SelectPolygonCells(polygon, selectedRepresentations, selectionSources,
          this->UseMultipleRepresentationSelection))
    {
      END_UNDO_EXCLUDE();
      return;
    }
    SM_SCOPED_TRACE(CallFunction)
      .arg("SelectSurfaceCells")
      .arg("Polygon", polygonVector)
      .arg("Modifier", modifier.size() > 0 ? modifier.c_str() : nullptr)
      .arg("comment", "create a surface cells polygon selection");
  }

  END_UNDO_EXCLUDE();
  this->collectSelectionPorts(
    selectedRepresentations, selectionSources, pqOutputPorts, selectionModifier, select_blocks);
}

//-----------------------------------------------------------------------------
void pqRenderView::selectFrustum(int rect[4])
{
  vtkSMRenderViewProxy* renderModuleP = this->getRenderViewProxy();

  vtkSmartPointer<vtkCollection> selectedRepresentations = vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> selectionSources = vtkSmartPointer<vtkCollection>::New();
  QList<pqOutputPort*> output_ports;

  BEGIN_UNDO_EXCLUDE();
  if (!renderModuleP->SelectFrustumCells(
        rect, selectedRepresentations, selectionSources, this->UseMultipleRepresentationSelection))
  {
    END_UNDO_EXCLUDE();
    this->emitSelectionSignal(output_ports);
    return;
  }

  std::vector<int> rectVector(4);
  for (size_t i = 0; i < 4; ++i)
  {
    rectVector[i] = rect[i];
  }

  SM_SCOPED_TRACE(CallFunction)
    .arg("SelectCellsThrough")
    .arg("Rectangle", rectVector)
    .arg("comment", "create a frustum selection of cells");

  END_UNDO_EXCLUDE();

  this->collectSelectionPorts(
    selectedRepresentations, selectionSources, output_ports, pqView::PV_SELECTION_DEFAULT, false);

  // Fire selection event to let the world know that this view selected
  // something.
  this->emitSelectionSignal(output_ports);
}

//-----------------------------------------------------------------------------
void pqRenderView::selectFrustumPoints(int rect[4])
{
  vtkSMRenderViewProxy* renderModuleP = this->getRenderViewProxy();

  vtkSmartPointer<vtkCollection> selectedRepresentations = vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> selectionSources = vtkSmartPointer<vtkCollection>::New();
  QList<pqOutputPort*> output_ports;

  BEGIN_UNDO_EXCLUDE();
  if (!renderModuleP->SelectFrustumPoints(
        rect, selectedRepresentations, selectionSources, this->UseMultipleRepresentationSelection))
  {
    END_UNDO_EXCLUDE();
    this->emitSelectionSignal(output_ports);
    return;
  }

  std::vector<int> rectVector(4);
  for (size_t i = 0; i < 4; ++i)
  {
    rectVector[i] = rect[i];
  }

  SM_SCOPED_TRACE(CallFunction)
    .arg("SelectPointsThrough")
    .arg("Rectangle", rectVector)
    .arg("comment", "create a frustum selection of points");

  END_UNDO_EXCLUDE();

  this->collectSelectionPorts(
    selectedRepresentations, selectionSources, output_ports, pqView::PV_SELECTION_DEFAULT, false);

  // Fire selection event to let the world know that this view selected
  // something.
  this->emitSelectionSignal(output_ports);
}

//-----------------------------------------------------------------------------
void pqRenderView::selectBlock(int rectangle[4], int selectionModifier)
{
  bool block = this->blockSignals(true);
  QList<pqOutputPort*> opPorts;
  this->selectOnSurfaceInternal(rectangle, opPorts, false, selectionModifier, true);

  this->blockSignals(block);
  this->emitSelectionSignal(opPorts);
}

//-----------------------------------------------------------------------------
void pqRenderView::updateInteractionMode(pqOutputPort* opPort)
{
  // Check default mode
  vtkPVRenderViewSettings* settings = vtkPVRenderViewSettings::GetInstance();
  int defaultMode = settings->GetDefaultInteractionMode();
  if (vtkPVRenderViewSettings::ALWAYS_2D == defaultMode)
  {
    vtkSMPropertyHelper(this->getProxy(), "InteractionMode")
      .Set(vtkPVRenderView::INTERACTION_MODE_2D);
    this->getProxy()->UpdateProperty("InteractionMode", 0);
    return;
  }
  else if (vtkPVRenderViewSettings::ALWAYS_3D == defaultMode)
  {
    vtkSMPropertyHelper(this->getProxy(), "InteractionMode")
      .Set(vtkPVRenderView::INTERACTION_MODE_3D);
    this->getProxy()->UpdateProperty("InteractionMode", 1);
    return;
  }

  // (else) Set interaction mode based on extents (vtkPVRenderViewSettings::AUTOMATIC)
  vtkPVDataInformation* datainfo = opPort->getDataInformation();
  QString className = datainfo ? datainfo->GetDataClassName() : QString();

  // Regardless the type of data, see if it is flat
  double bounds[6];
  datainfo->GetBounds(bounds);

  double focal[3] = { (bounds[0] + bounds[1]) / 2, (bounds[2] + bounds[3]) / 2,
    (bounds[4] + bounds[5]) / 2 };
  double position[3] = { 0, 0, 0 };
  double viewUp[3] = { 0, 0, 0 };
  bool is2DDataSet = false;

  // Update camera infos
  for (int i = 0; i < 3; i++)
  {
    if (bounds[i * 2 + 1] - bounds[i * 2] == 0 && !is2DDataSet)
    {
      position[i] = focal[i] + 10000;
      viewUp[(i + 2) % 3] = 1;
      is2DDataSet = true;
    }
    else
    {
      position[i] = focal[i];
    }
  }

  // FIXME: move this logic to server-manager.
  SM_SCOPED_TRACE(PropertiesModified)
    .arg(this->getProxy())
    .arg("comment", "changing interaction mode based on data extents");
  if (is2DDataSet)
  {
    // Update camera position
    vtkSMPropertyHelper(this->getProxy(), "CameraFocalPoint").Set(focal, 3);
    vtkSMPropertyHelper(this->getProxy(), "CameraPosition").Set(position, 3);
    vtkSMPropertyHelper(this->getProxy(), "CameraViewUp").Set(viewUp, 3);

    // Update the interaction
    vtkSMPropertyHelper(this->getProxy(), "InteractionMode")
      .Set(vtkPVRenderView::INTERACTION_MODE_2D);
    this->getProxy()->UpdateVTKObjects();
  }
  else
  {
    // Update the interaction
    vtkSMPropertyHelper(this->getProxy(), "InteractionMode")
      .Set(vtkPVRenderView::INTERACTION_MODE_3D);
    this->getProxy()->UpdateProperty("InteractionMode", 1);
  }
}
//-----------------------------------------------------------------------------
void pqRenderView::onInteractionModeChange()
{
  int mode = -1;
  vtkSMPropertyHelper(this->getProxy(), "InteractionMode").Get(&mode);
  if (mode != this->Internal->CurrentInteractionMode)
  {
    this->Internal->CurrentInteractionMode = mode;
    emit updateInteractionMode(this->Internal->CurrentInteractionMode);
  }
}

//-----------------------------------------------------------------------------
void pqRenderView::setCursor(const QCursor& c)
{
  if (QWidget* wdg = this->widget())
  {
    wdg->setCursor(c);
  }
}
