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
#include "QVTKWidget.h"
#include "vtkCollection.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVTrackballRoll.h"
#include "vtkPVTrackballRotate.h"
#include "vtkPVTrackballZoom.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSMInteractionUndoStackBuilder.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUndoStack.h"
#include "vtkTrackballPan.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMGlobalPropertiesManager.h"

// Qt includes.
#include <QFileInfo>
#include <QList>
#include <QPointer>
#include <QtDebug>
#include <QEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QSet>
#include <QPrinter>
#include <QPainter>
#include <QGridLayout>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqLinkViewWidget.h"
#include "pqOptions.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"
#include "vtkPVAxesWidget.h"

pqRenderView::ManipulatorType pqRenderView::DefaultManipulatorTypes[] = 
{
    { 1, 0, 0, "Rotate"},
    { 2, 0, 0, "Pan"},
    { 3, 0, 0, "Zoom"},
    { 1, 1, 0, "Roll"},
    { 2, 1, 0, "Rotate"},
    { 3, 1, 0, "Pan"},
    { 1, 0, 1, "Zoom"},
    { 2, 0, 1, "Rotate"},
    { 3, 0, 1, "Zoom"},
};

class pqRenderView::pqInternal
{
public:
  vtkSmartPointer<vtkPVAxesWidget> OrientationAxesWidget;
  vtkSmartPointer<vtkSMProxy> CenterAxesProxy;

  vtkSmartPointer<vtkSMUndoStack> InteractionUndoStack;
  vtkSmartPointer<vtkSMInteractionUndoStackBuilder> UndoStackBuilder;

  QList<pqRenderView* > LinkedUndoStacks;
  bool UpdatingStack;

  bool InitializedWidgets;
  pqInternal()
    {
    this->UpdatingStack = false;
    this->InitializedWidgets = false;
    this->OrientationAxesWidget = vtkSmartPointer<vtkPVAxesWidget>::New();

    this->InteractionUndoStack = vtkSmartPointer<vtkSMUndoStack>::New();
    this->InteractionUndoStack->SetClientOnly(true);
    this->UndoStackBuilder = 
      vtkSmartPointer<vtkSMInteractionUndoStackBuilder>::New();
    this->UndoStackBuilder->SetUndoStack(
      this->InteractionUndoStack);
    }

  ~pqInternal()
    {
    }
};

//-----------------------------------------------------------------------------
void pqRenderView::InternalConstructor(vtkSMViewProxy* renModule)
{
  this->Internal = new pqRenderView::pqInternal();

  // we need to fire signals when undo stack changes.
  this->getConnector()->Connect(this->Internal->InteractionUndoStack,
    vtkCommand::ModifiedEvent, this, SLOT(onUndoStackChanged()),
    0, 0, Qt::QueuedConnection);

  this->ResetCenterWithCamera = true;
  this->UseMultipleRepresentationSelection = false;
  this->getConnector()->Connect(
    renModule, vtkCommand::ResetCameraEvent,
    this, SLOT(onResetCameraEvent()));
}

//-----------------------------------------------------------------------------
pqRenderView::pqRenderView( const QString& group,
                            const QString& name, 
                            vtkSMViewProxy* renModule, 
                            pqServer* server, 
                            QObject* _parent/*=null*/) : 
  Superclass(renderViewType(), group, name, renModule, server, _parent)
{
  this->InternalConstructor(renModule);
}

//-----------------------------------------------------------------------------
pqRenderView::pqRenderView( const QString& type,
                            const QString& group, 
                            const QString& name, 
                            vtkSMViewProxy* renModule, 
                            pqServer* server, 
                            QObject* _parent/*=null*/) : 
  Superclass(type, group, name, renModule, server, _parent)
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
QWidget* pqRenderView::createWidget() 
{
  QWidget* vtkwidget = this->Superclass::createWidget();

  // add a link view menu
  QAction* act = new QAction("Link Camera...", this);
  vtkwidget->addAction(act);
  QObject::connect(act, SIGNAL(triggered(bool)),
    this, SLOT(linkToOtherView()));
  return vtkwidget;
}

//-----------------------------------------------------------------------------
// This method is called for all pqRenderView objects irrespective
// of whether it is created from state/undo-redo/python or by the GUI. Hence
// don't change any render module properties here.
void pqRenderView::initializeWidgets()
{
  if (this->Internal->InitializedWidgets)
    {
    return;
    }

  this->Internal->InitializedWidgets = true;

  vtkSMRenderViewProxy* renModule = this->getRenderViewProxy();

  QVTKWidget* vtkwidget = qobject_cast<QVTKWidget*>(this->getWidget());
  if (vtkwidget)
    {
    vtkwidget->SetRenderWindow(renModule->GetRenderWindow());
    }

  vtkPVGenericRenderWindowInteractor* iren = renModule->GetInteractor();

  // Init axes actor.
  // FIXME: Convert OrientationAxesWidget to a first class representation.
  this->Internal->OrientationAxesWidget->SetParentRenderer(
    renModule->GetRenderer());
  this->Internal->OrientationAxesWidget->SetViewport(0, 0, 0.25, 0.25);
  this->Internal->OrientationAxesWidget->SetInteractor(iren);
  this->Internal->OrientationAxesWidget->SetEnabled(1);
  this->Internal->OrientationAxesWidget->SetInteractive(0);

  // Set up some global property links by default.
  vtkSMGlobalPropertiesManager* globalPropertiesManager =
    pqApplicationCore::instance()->getGlobalPropertiesManager();
  this->getConnector()->Connect(
    globalPropertiesManager->GetProperty("TextAnnotationColor"),
    vtkCommand::ModifiedEvent, this, SLOT(textAnnotationColorChanged()));
  this->textAnnotationColorChanged();

  // setup the center axes.
  this->initializeCenterAxes();
  
  // ensure that center axis visibility etc. is updated as per user's
  // preferences.
  this->restoreAnnotationSettings();

  this->Internal->UndoStackBuilder->SetRenderView(renModule);
}

//-----------------------------------------------------------------------------
// Sets default values for the underlying proxy.  This is during the 
// initialization stage of the pqProxy for proxies created by the GUI itself 
// i.e. for proxies loaded through state or created by python client or 
// undo/redo, this method won't be called. 
void pqRenderView::setDefaultPropertyValues()
{
  vtkSMProxy* proxy = this->getProxy();
  if (!pqApplicationCore::instance()->getOptions()->GetDisableLightKit())
    {
    pqSMAdaptor::setElementProperty(proxy->GetProperty("UseLight"), 1);
    pqSMAdaptor::setElementProperty(proxy->GetProperty("LightSwitch"), 0);
    }
  this->Superclass::setDefaultPropertyValues();
  this->clearUndoStack();
}

//-----------------------------------------------------------------------------
void pqRenderView::restoreDefaultLightSettings()
{
  this->Superclass::restoreDefaultLightSettings();
  if (!pqApplicationCore::instance()->getOptions()->GetDisableLightKit())
    {
    vtkSMProxy* proxy = this->getProxy();
    pqSMAdaptor::setElementProperty(proxy->GetProperty("UseLight"), 1);
    pqSMAdaptor::setElementProperty(proxy->GetProperty("LightSwitch"), 0);
    proxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
// Create a center axes if one doesn't already exist.
void pqRenderView::initializeCenterAxes()
{
  if (this->Internal->CenterAxesProxy.GetPointer())
    {
    // sanity check.
    return;
    }

  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMProxy* centerAxes = pxm->NewProxy("representations", "AxesRepresentation");
  centerAxes->SetConnectionID(this->getServer()->GetConnectionID());
  QList<QVariant> scaleValues;
  scaleValues << .25 << .25 << .25;
  pqSMAdaptor::setMultipleElementProperty(
    centerAxes->GetProperty("Scale"), scaleValues);
  pqSMAdaptor::setElementProperty(centerAxes->GetProperty("Pickable"), 0);
  centerAxes->UpdateVTKObjects();
  this->Internal->CenterAxesProxy = centerAxes;

  vtkSMViewProxy* renView = this->getViewProxy();

  // Update the center axes position whenever the center of rotation changes.
  this->getConnector()->Connect(
    renView->GetProperty("CenterOfRotation"), 
    vtkCommand::ModifiedEvent, this, SLOT(updateCenterAxes()));
  
  // Add to render module without using properties. That way it does not
  // get saved in state.
  renView->AddRepresentation(
    vtkSMRepresentationProxy::SafeDownCast(centerAxes));
  centerAxes->Delete();

  this->updateCenterAxes();
}

//-----------------------------------------------------------------------------
void pqRenderView::onResetCameraEvent()
{
  if (this->ResetCenterWithCamera)
    {
    this->resetCenterOfRotation();
    }

  // Ensures that the scale factor is correctly set for the center axes
  // on reset camera.
  this->updateCenterAxes();
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
    pqSMAdaptor::getMultipleElementProperty(
      viewproxy->GetProperty("CameraFocalPointInfo"));
  this->setCenterOfRotation(
    values[0].toDouble(), values[1].toDouble(), values[2].toDouble());
}

//-----------------------------------------------------------------------------
vtkImageData* pqRenderView::captureImage(int magnification)
{
  if (this->getWidget()->isVisible())
    {
    return this->getRenderViewProxy()->CaptureWindow(magnification);
    }

  // Don't return any image when the view is not visible.
  return NULL;
}

//-----------------------------------------------------------------------------
const int* pqRenderView::defaultBackgroundColor() const
{
  static int defaultBackground[3] = { 84, 89, 109 };
  return defaultBackground;
}

//-----------------------------------------------------------------------------
void pqRenderView::setOrientationAxesVisibility(bool visible)
{
  this->Internal->OrientationAxesWidget->SetEnabled(visible? 1: 0);
}

//-----------------------------------------------------------------------------
bool pqRenderView::getOrientationAxesVisibility() const
{
  return this->Internal->OrientationAxesWidget->GetEnabled();
}

//-----------------------------------------------------------------------------
void pqRenderView::setOrientationAxesInteractivity(bool interactive)
{
  this->Internal->OrientationAxesWidget->SetInteractive(interactive? 1: 0);
}

//-----------------------------------------------------------------------------
bool pqRenderView::getOrientationAxesInteractivity() const
{
  return this->Internal->OrientationAxesWidget->GetInteractive();
}

//-----------------------------------------------------------------------------
void pqRenderView::setOrientationAxesOutlineColor(const QColor& color)
{
  this->Internal->OrientationAxesWidget->SetOutlineColor(
    color.redF(), color.greenF(), color.blueF());
}

//-----------------------------------------------------------------------------
QColor pqRenderView::getOrientationAxesOutlineColor() const
{
  QColor color;
  double* dcolor = this->Internal->OrientationAxesWidget->GetOutlineColor();
  color.setRgbF(dcolor[0], dcolor[1], dcolor[2]);
  return color;
}

//-----------------------------------------------------------------------------
void pqRenderView::setOrientationAxesLabelColor(const QColor& color)
{
  this->Internal->OrientationAxesWidget->SetAxisLabelColor(
    color.redF(), color.greenF(), color.blueF());
}

//-----------------------------------------------------------------------------
QColor pqRenderView::getOrientationAxesLabelColor() const
{
  QColor color;
  double* dcolor = this->Internal->OrientationAxesWidget->GetAxisLabelColor();
  color.setRgbF(dcolor[0], dcolor[1], dcolor[2]);
  return color;
}

//-----------------------------------------------------------------------------
void pqRenderView::updateCenterAxes()
{
  if (!this->getCenterAxesVisibility())
    {
    return;
    }

  double center[3];
  QList<QVariant> val =
    pqSMAdaptor::getMultipleElementProperty(
      this->getProxy()->GetProperty("CenterOfRotation"));
  center[0] = val[0].toDouble();
  center[1] = val[1].toDouble();
  center[2] = val[2].toDouble();

  QList<QVariant> positionValues;
  positionValues << center[0] << center[1] << center[2];

  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->CenterAxesProxy->GetProperty("Position"),
    positionValues);

  // Reset size of the axes.
  double bounds[6];
  this->getRenderViewProxy()->ComputeVisiblePropBounds(bounds);
  double widths[3];
  widths[0] = (bounds[1]-bounds[0]);
  widths[1] = (bounds[3]-bounds[2]);
  widths[2] = (bounds[5]-bounds[4]);
  // lets make some thickness in all directions
  double diameterOverTen = qMax(widths[0], qMax(widths[1], widths[2])) / 10.0;
  widths[0] = widths[0] < diameterOverTen ? diameterOverTen : widths[0];
  widths[1] = widths[1] < diameterOverTen ? diameterOverTen : widths[1];
  widths[2] = widths[2] < diameterOverTen ? diameterOverTen : widths[2];

  QList<QVariant> scaleValues;
  scaleValues << (widths[0])*0.25 << (widths[1])*0.25 << (widths[2])*0.25;

  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->CenterAxesProxy->GetProperty("Scale"),
    scaleValues);

  this->Internal->CenterAxesProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqRenderView::setCenterOfRotation(double x, double y, double z)
{
  QList<QVariant> positionValues;
  positionValues << x << y << z;

  // this modifies the CenterOfRotation property resulting to a call
  // to updateCenterAxes().
  vtkSMProxy* viewproxy = this->getProxy();
  pqSMAdaptor::setMultipleElementProperty(
    viewproxy->GetProperty("CenterOfRotation"),
    positionValues);
  viewproxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqRenderView::getCenterOfRotation(double center[3]) const
{
  QList<QVariant> val =
    pqSMAdaptor::getMultipleElementProperty(
      this->getProxy()->GetProperty("CenterOfRotation"));
  center[0] = val[0].toDouble();
  center[1] = val[1].toDouble();
  center[2] = val[2].toDouble();
}

//-----------------------------------------------------------------------------
void pqRenderView::setCenterAxesVisibility(bool visible)
{
  pqSMAdaptor::setElementProperty(
    this->Internal->CenterAxesProxy->GetProperty("Visibility"),
    visible? 1 : 0);
  this->Internal->CenterAxesProxy->UpdateVTKObjects();
  this->getProxy()->MarkModified(0);
  if (visible)
    {
    // since updateCenterAxes does not do anything unless the axes is visible,
    // we update the center when the axes becomes visible so that we are assured
    // that the correct center of rotation is used.
    this->updateCenterAxes();
    }
}

//-----------------------------------------------------------------------------
bool pqRenderView::getCenterAxesVisibility() const
{
  if (this->Internal->CenterAxesProxy.GetPointer()==0)
    {
    return false;
    }

  return pqSMAdaptor::getElementProperty(
    this->Internal->CenterAxesProxy->GetProperty("Visibility")).toBool();
}

//-----------------------------------------------------------------------------
void pqRenderView::restoreAnnotationSettings()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString sgroup = this->viewSettingsGroup();
  settings->beginGroup(sgroup);
  // Orientation Axes settings.
  settings->beginGroup("OrientationAxes");
  if (settings->contains("Visibility"))
    {
    this->setOrientationAxesVisibility(
      settings->value("Visibility").toBool());
    }
  if (settings->contains("Interactivity"))
    {
    this->setOrientationAxesInteractivity(
      settings->value("Interactivity").toBool());
    }
  if (settings->contains("OutlineColor"))
    {
    this->setOrientationAxesOutlineColor(
      settings->value("OutlineColor").value<QColor>());
    }
  if (settings->contains("LabelColor"))
    {
    this->setOrientationAxesLabelColor(
      settings->value("LabelColor").value<QColor>());
    }
  settings->endGroup();

  // Center Axes settings.
  settings->beginGroup("CenterAxes");
  if (settings->contains("Visibility"))
    {
    this->setCenterAxesVisibility(
      settings->value("Visibility").toBool());
    }
  if (settings->contains("ResetCenterWithCamera"))
    {
    this->ResetCenterWithCamera =
      settings->value("ResetCenterWithCamera").toBool();
    }
  settings->endGroup();
  settings->endGroup();
}

//-----------------------------------------------------------------------------
void pqRenderView::restoreSettings(bool only_global)
{
  this->Superclass::restoreSettings(only_global);
  if (!only_global)
    {
    this->restoreAnnotationSettings();
    }
}

//-----------------------------------------------------------------------------
void pqRenderView::saveSettings()
{
  this->Superclass::saveSettings();

  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString sgroup = this->viewSettingsGroup();
  settings->beginGroup(sgroup);

  // Orientation Axes settings.
  settings->beginGroup("OrientationAxes");
  settings->setValue("Visibility", 
    this->getOrientationAxesVisibility());
  settings->setValue("Interactivity",
    this->getOrientationAxesInteractivity());
  settings->setValue("OutlineColor",
    this->getOrientationAxesOutlineColor());
  settings->setValue("LabelColor",
    this->getOrientationAxesLabelColor());
  settings->endGroup();

  // Center Axes settings.
  settings->beginGroup("CenterAxes");
  settings->setValue("Visibility",
    this->getCenterAxesVisibility());
  settings->setValue("ResetCenterWithCamera",
    this->ResetCenterWithCamera);
  settings->endGroup();

  settings->endGroup();
}

//-----------------------------------------------------------------------------
void pqRenderView::linkToOtherView()
{
  pqLinkViewWidget* linkWidget = new pqLinkViewWidget(this);
  linkWidget->setAttribute(Qt::WA_DeleteOnClose);
  QPoint pos = this->getWidget()->mapToGlobal(QPoint(2,2));
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
    double look_x, double look_y, double look_z,
    double up_x, double up_y, double up_z)
{
  vtkSMProxy* proxy = this->getProxy();

  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraPosition"), 0, 0);
  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraPosition"), 1, 0);
  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraPosition"), 2, 0);

  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraFocalPoint"), 0, look_x);
  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraFocalPoint"), 1, look_y);
  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraFocalPoint"), 2, look_z);

  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraViewUp"), 0, up_x);
  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraViewUp"), 1, up_y);
  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraViewUp"), 2, up_z);
  proxy->UpdateVTKObjects();

  this->resetCamera();
  this->render();
}

//-----------------------------------------------------------------------------
void pqRenderView::selectOnSurface(int rect[4], bool expand)
{
  QList<pqOutputPort*> opPorts;
  this->selectOnSurfaceInternal(rect, opPorts, false, expand, false);
  this->emitSelectionSignal(opPorts);
}

//-----------------------------------------------------------------------------
void pqRenderView::emitSelectionSignal(QList<pqOutputPort*> opPorts)
{
  // Fire selection event to let the world know that this view selected
  // something.
  if(opPorts.count()>0)
    {
    emit this->selected(opPorts.value(0));
    }
  else
    {
    emit this->selected(0);
    }

  if(this->UseMultipleRepresentationSelection)
    {
    emit this->multipleSelected(opPorts);
    }
}

//-----------------------------------------------------------------------------
void pqRenderView::collectSelectionPorts(
  vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources,
  QList<pqOutputPort*>& output_ports,
  bool expand,
  bool select_blocks)
{
  if (!selectedRepresentations ||
    selectedRepresentations->GetNumberOfItems()<=0)
    {
    return;
    }
  
  if (!selectionSources ||
    selectionSources->GetNumberOfItems()<=0)
    {
    return;
    }

  if (selectedRepresentations->GetNumberOfItems()!=
    selectionSources->GetNumberOfItems())
    {
    return;
    }

  for (int i=0; i<selectedRepresentations->GetNumberOfItems(); i++)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      selectedRepresentations->GetItemAsObject(i));
    vtkSmartPointer<vtkSMSourceProxy> selectionSource = vtkSMSourceProxy::SafeDownCast(
      selectionSources->GetItemAsObject(i));

    pqServerManagerModel* smmodel = 
      pqApplicationCore::instance()->getServerManagerModel();
    pqDataRepresentation* pqRepr = smmodel->findItem<pqDataRepresentation*>(repr);
    if (!repr)
      {
      // No data display was selected (or none that is registered).
      continue;
      }
    pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
    vtkSMSourceProxy* selectedSource = vtkSMSourceProxy::SafeDownCast(
      opPort->getSource()->getProxy());

    if (select_blocks)
      {
      // convert the index based selection to vtkSelectionNode::BLOCKS selection.
      vtkSMSourceProxy* newSelSource = vtkSMSourceProxy::SafeDownCast(
        vtkSMSelectionHelper::ConvertSelection(vtkSelectionNode::BLOCKS,
          selectionSource,
          selectedSource, opPort->getPortNumber()));
      selectionSource.TakeReference(newSelSource);
      }

    if (expand)
      {
      vtkSMSelectionHelper::MergeSelection(selectionSource,
        opPort->getSelectionInput(),
        selectedSource, opPort->getPortNumber());
      }
    opPort->setSelectionInput(selectionSource, 0);
    output_ports.append(opPort);
    }
}

//-----------------------------------------------------------------------------
void pqRenderView::selectOnSurfaceInternal(
  int rect[4], QList<pqOutputPort*>& pqOutputPorts,
  bool select_points,
  bool expand,
  bool select_blocks)
{
  vtkSMRenderViewProxy* renderModuleP = this->getRenderViewProxy();

  vtkSmartPointer<vtkCollection> selectedRepresentations = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> surfaceSelections = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> selectionSources = 
    vtkSmartPointer<vtkCollection>::New();
  if (!renderModuleP->SelectOnSurface(rect[0], rect[1], rect[2], rect[3], 
      selectedRepresentations, selectionSources, surfaceSelections, 
      this->UseMultipleRepresentationSelection, select_points))
    {
    return;
    }

  this->collectSelectionPorts(selectedRepresentations, 
    selectionSources, pqOutputPorts, expand, select_blocks);
}

//-----------------------------------------------------------------------------
void pqRenderView::selectPointsOnSurface(int rect[4], bool expand)
{
  QList<pqOutputPort*> output_ports;
  this->selectOnSurfaceInternal(rect, output_ports, true, expand, false);
  // Fire selection event to let the world know that this view selected
  // something.
  this->emitSelectionSignal(output_ports);
}

//-----------------------------------------------------------------------------
void pqRenderView::selectFrustum(int rect[4])
{
  vtkSMRenderViewProxy* renderModuleP = this->getRenderViewProxy();

  vtkSmartPointer<vtkCollection> selectedRepresentations = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> frustumSelections = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> selectionSources = 
    vtkSmartPointer<vtkCollection>::New();
  
  QList<pqOutputPort*> output_ports;
  if (!renderModuleP->SelectFrustum(rect[0], rect[1], rect[2], rect[3], 
    selectedRepresentations, selectionSources, frustumSelections, 
    this->UseMultipleRepresentationSelection))
    {
    this->emitSelectionSignal(output_ports);
    return;
    }

  this->collectSelectionPorts(selectedRepresentations, 
    selectionSources, output_ports, false, false);

  // Fire selection event to let the world know that this view selected
  // something.
  this->emitSelectionSignal(output_ports);
}

//-----------------------------------------------------------------------------
void pqRenderView::selectFrustumPoints(int rect[4])
{
  vtkSMRenderViewProxy* renderModuleP = this->getRenderViewProxy();

  vtkSmartPointer<vtkCollection> selectedRepresentations = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> frustumSelections = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> selectionSources = 
    vtkSmartPointer<vtkCollection>::New();

  QList<pqOutputPort*> output_ports;
  if (!renderModuleP->SelectFrustum(rect[0], rect[1], rect[2], rect[3], 
    selectedRepresentations, selectionSources, frustumSelections, 
    this->UseMultipleRepresentationSelection, true))
    {
    this->emitSelectionSignal(output_ports);
    return;
    }

  this->collectSelectionPorts(selectedRepresentations, 
    selectionSources, output_ports, false, false);

  // Fire selection event to let the world know that this view selected
  // something.
  this->emitSelectionSignal(output_ports);
}

//-----------------------------------------------------------------------------
void pqRenderView::selectBlock(int rectangle[4], bool expand)
{
  bool block = this->blockSignals(true);
  QList<pqOutputPort*> opPorts;
  this->selectOnSurfaceInternal(rectangle, opPorts, false, expand, true);
  this->blockSignals(block);
  this->emitSelectionSignal(opPorts);
}

//-----------------------------------------------------------------------------
void pqRenderView::textAnnotationColorChanged()
{
  // Set up some global property links by default.
  vtkSMGlobalPropertiesManager* globalPropertiesManager =
    pqApplicationCore::instance()->getGlobalPropertiesManager();
  double value[3];
  vtkSMPropertyHelper(globalPropertiesManager, "TextAnnotationColor").Get(
    value, 3);
  this->Internal->OrientationAxesWidget->SetAxisLabelColor(value[0], value[1],
    value[2]);
}
