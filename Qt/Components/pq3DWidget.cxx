/*=========================================================================

   Program: ParaView
   Module:    pq3DWidget.cxx

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
#include "pq3DWidget.h"

// ParaView Server Manager includes.
#include "vtkBoundingBox.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

// Qt includes.
#include <QtDebug>
#include <QPointer>
#include <QShortcut>

// ParaView GUI includes.
#include "pq3DWidgetInterface.h"
#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqBoxWidget.h"
#include "pqDistanceWidget.h"
#include "pqImplicitPlaneWidget.h"
#include "pqInterfaceTracker.h"
#include "pqLineSourceWidget.h"
#include "pqRubberBandHelper.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqPointSourceWidget.h"
#include "pqProxy.h"
#include "pqRenderViewBase.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqSphereWidget.h"
#include "pqSplineWidget.h"

#include "vtkPVConfig.h"
#ifdef PARAVIEW_ENABLE_PYTHON
#include "pqPythonManager.h"
#include "pqPythonDialog.h"
#include "pqPythonShell.h"
#endif

namespace
{
  vtkSMProxySelectionModel* getSelectionModel(vtkSMProxy* proxy)
    {
    vtkSMSessionProxyManager* pxm =
      proxy->GetSession()->GetSessionProxyManager();
    return pxm->GetSelectionModel("ActiveSources");
    }
}

//-----------------------------------------------------------------------------
class pq3DWidget::pqStandardWidgets : public pq3DWidgetInterface
{
public:
  pq3DWidget* newWidget(const QString& name,
    vtkSMProxy* referenceProxy,
    vtkSMProxy* controlledProxy)
    {
    pq3DWidget *widget = 0;
    if (name == "Plane")
      {
      widget = new pqImplicitPlaneWidget(referenceProxy, controlledProxy, 0);
      }
    else if (name == "Box")
      {
      widget = new pqBoxWidget(referenceProxy, controlledProxy, 0);
      }
    else if (name == "Handle")
      {
      widget = new pqHandleWidget(referenceProxy, controlledProxy, 0);
      }
    else if (name == "PointSource")
      {
      widget = new pqPointSourceWidget(referenceProxy, controlledProxy, 0);
      }
    else if (name == "LineSource")
      {
      widget = new pqLineSourceWidget(referenceProxy, controlledProxy, 0);
      }
    else if (name == "Line")
      {
      widget = new pqLineWidget(referenceProxy, controlledProxy, 0);
      }
    else if (name == "Distance")
      {
      widget = new pqDistanceWidget(referenceProxy, controlledProxy, 0);
      }
    else if (name == "Sphere")
      {
      widget = new pqSphereWidget(referenceProxy, controlledProxy, 0);
      }
    else if (name == "Spline")
      {
      widget = new pqSplineWidget(referenceProxy, controlledProxy, 0);
      }
    return widget;
    }
};

//-----------------------------------------------------------------------------
class pq3DWidgetInternal
{
public:
  pq3DWidgetInternal() :
    IgnorePropertyChange(false),
    WidgetVisible(true),
    Selected(false),
    LastWidgetVisibilityGoal(true)
  {
  this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->IsMaster = pqApplicationCore::instance()->getActiveServer()->isMaster();
  }
    
  vtkSmartPointer<vtkSMProxy> ReferenceProxy;
  vtkSmartPointer<vtkSMNewWidgetRepresentationProxy> WidgetProxy;
  vtkSmartPointer<vtkCommand> ControlledPropertiesObserver;
  vtkSmartPointer<vtkPVXMLElement> Hints;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  QMap<vtkSmartPointer<vtkSMProperty>, vtkSmartPointer<vtkSMProperty> > PropertyMap;

  /// Used to avoid recursion when updating the controlled properties
  bool IgnorePropertyChange;
  /// Stores the visible/hidden state of the 3D widget (controlled by the user)
  bool WidgetVisible;
  /// Stores the selected/not selected state of the 3D widget (controlled by the owning panel)
  bool Selected;

  pqRubberBandHelper PickHelper;
  QKeySequence PickSequence;
  QPointer<QShortcut> PickShortcut;
  bool IsMaster;
  bool LastWidgetVisibilityGoal;
};

//-----------------------------------------------------------------------------
pq3DWidget::pq3DWidget(vtkSMProxy* refProxy, vtkSMProxy* pxy, QWidget* _p) :
  pqProxyPanel(pxy, _p),
  Internal(new pq3DWidgetInternal())
{
  this->UseSelectionDataBounds = false;
  this->Internal->ReferenceProxy = refProxy;

  this->Internal->ControlledPropertiesObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, 
      &pq3DWidget::onControlledPropertyChanged));
  this->Internal->IgnorePropertyChange = false;

  this->setControlledProxy(pxy);

  QObject::connect(&this->Internal->PickHelper,
    SIGNAL(intersectionFinished(double, double, double)),
    this, SLOT(pick(double, double, double)));

  QObject::connect( pqApplicationCore::instance(),
                    SIGNAL(updateMasterEnableState(bool)),
                    this, SLOT(updateMasterEnableState(bool)));

  QObject::connect( &pqActiveObjects::instance(),
                    SIGNAL(sourceNotification(pqPipelineSource*,char*)),
                    this,
                    SLOT(handleSourceNotification(pqPipelineSource*,char*)));
}

//-----------------------------------------------------------------------------
pq3DWidget::~pq3DWidget()
{
  this->setView(0);
  this->setControlledProxy(0);
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QList<pq3DWidget*> pq3DWidget::createWidgets(vtkSMProxy* refProxy, vtkSMProxy* pxy)
{
  QList<pq3DWidget*> widgets;

  QList<pq3DWidgetInterface*> interfaces =
    pqApplicationCore::instance()->interfaceTracker()->interfaces<pq3DWidgetInterface*>();

  vtkPVXMLElement* hints = pxy->GetHints();
  unsigned int max = hints->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < max; cc++)
    {
    vtkPVXMLElement* element = hints->GetNestedElement(cc);
    if (QString("PropertyGroup") == element->GetName())
      {
      QString widgetType = element->GetAttribute("type");
      pq3DWidget *widget = 0;

      // Create the widget from plugins.
      foreach (pq3DWidgetInterface* iface, interfaces)
        {
        widget = iface->newWidget(widgetType, refProxy, pxy);
        if (widget)
          {
          break;
          }
        }
      if (!widget)
        {
        // try to create the standard widget if the plugins fail.
        pqStandardWidgets standardWidgets;
        widget = standardWidgets.newWidget(widgetType, refProxy, pxy);
        }
      if (widget)
        {
        widget->setHints(element);
        widgets.push_back(widget);
        }
      }
    }
  return widgets;
}

//-----------------------------------------------------------------------------
pqRenderViewBase* pq3DWidget::renderView() const
{
  return qobject_cast<pqRenderViewBase*>(this->view());
}

//-----------------------------------------------------------------------------
void pq3DWidget::pickingSupported(const QKeySequence& key)
{
  this->Internal->PickSequence = key;
}

//-----------------------------------------------------------------------------
void pq3DWidget::setView(pqView* pqview)
{ 
  pqRenderViewBase* rview = this->renderView();
  if (pqview == rview)
    {
    this->Superclass::setView(pqview);
    return;
    }

  // This test has been added to support proxy that have been created on
  // different servers. We return if we switch from a view from a server
  // to another view from another server.
  vtkSMProxy* widget = this->getWidgetProxy();
  if ((widget && pqview &&
       pqview->getProxy()->GetSession() != widget->GetSession())
      ||
      (rview && pqview &&
       rview->getProxy()->GetSession() != pqview->getProxy()->GetSession()))
    {
    return;
    }

  // get rid of old shortcut.
  delete this->Internal->PickShortcut;

  bool cur_visbility = this->widgetVisible();
  this->hideWidget();

  if (rview && widget)
    {
    // To add/remove the 3D widget display from the view module.
    // we don't use the property. This is so since the 3D widget add/remove 
    // should not get saved in state or undo-redo. 
    vtkSMPropertyHelper(
      rview->getProxy(), "HiddenRepresentations").Remove(widget);
    rview->getProxy()->UpdateVTKObjects();
    }

  this->Superclass::setView(pqview);
  this->Internal->PickHelper.setView(pqview);

  rview = this->renderView();
  if (rview && !this->Internal->PickSequence.isEmpty())
    {
    this->Internal->PickShortcut = new QShortcut(
      this->Internal->PickSequence, pqview->getWidget());
    QObject::connect(this->Internal->PickShortcut, SIGNAL(activated()),
      &this->Internal->PickHelper, SLOT(triggerFastIntersect()));
    }

  if (rview && widget)
    {
    // To add/remove the 3D widget display from the view module.
    // we don't use the property. This is so since the 3D widget add/remove 
    // should not get saved in state or undo-redo. 
    this->updateWidgetVisibility();
    vtkSMPropertyHelper(
      rview->getProxy(), "HiddenRepresentations").Add(widget);
    rview->getProxy()->UpdateVTKObjects();
    }

  if (cur_visbility)
    {
    this->showWidget();
    }
  this->updatePickShortcut();
}

//-----------------------------------------------------------------------------
void pq3DWidget::render()
{
  if (pqRenderViewBase* rview = this->renderView())
    {
    rview->render();
    }
}

//-----------------------------------------------------------------------------
void pq3DWidget::onControlledPropertyChanged()
{
  if (this->Internal->IgnorePropertyChange)
    {
    return;
    }

  // Synchronize the 3D and Qt widgets with the controlled properties
  this->reset();
}

//-----------------------------------------------------------------------------
void pq3DWidget::setWidgetProxy(vtkSMNewWidgetRepresentationProxy* pxy)
{
  this->Internal->VTKConnect->Disconnect();

    vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  pqRenderViewBase* rview = this->renderView();
  vtkSMProxy* viewProxy = rview? rview->getProxy() : NULL;
  if (rview && widget)
    {
    // To add/remove the 3D widget display from the view module.
    // we don't use the property. This is so since the 3D widget add/remove 
    // should not get saved in state or undo-redo. 
    vtkSMPropertyHelper(viewProxy,"HiddenRepresentations").Remove(widget);
    viewProxy->UpdateVTKObjects();
    rview->render();
    }

  this->Internal->WidgetProxy = pxy;

  if (pxy)
    {
    this->Internal->VTKConnect->Connect(pxy, vtkCommand::StartInteractionEvent,
      this, SIGNAL(widgetStartInteraction()));
    this->Internal->VTKConnect->Connect(pxy, vtkCommand::InteractionEvent,
      this, SLOT(setModified()));
    this->Internal->VTKConnect->Connect(pxy, vtkCommand::InteractionEvent,
      this, SIGNAL(widgetInteraction()));
    this->Internal->VTKConnect->Connect(pxy, vtkCommand::EndInteractionEvent,
      this, SIGNAL(widgetEndInteraction()));
    }

  if (rview && pxy)
    {
    // To add/remove the 3D widget display from the view module.
    // we don't use the property. This is so since the 3D widget add/remove 
    // should not get saved in state or undo-redo. 
    this->updateWidgetVisibility();
    vtkSMPropertyHelper(viewProxy,"HiddenRepresentations").Add(widget);
    viewProxy->UpdateVTKObjects();
    rview->render();
    }
}

//-----------------------------------------------------------------------------
vtkSMNewWidgetRepresentationProxy* pq3DWidget::getWidgetProxy() const
{
  return this->Internal->WidgetProxy;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pq3DWidget::getControlledProxy() const
{
  return this->proxy();
}

//-----------------------------------------------------------------------------
vtkSMProxy* pq3DWidget::getReferenceProxy() const
{
  return this->Internal->ReferenceProxy;
}

//-----------------------------------------------------------------------------
void pq3DWidget::setControlledProxy(vtkSMProxy* /*pxy*/)
{
  foreach(vtkSMProperty* controlledProperty, this->Internal->PropertyMap)
    {
    controlledProperty->RemoveObserver(
      this->Internal->ControlledPropertiesObserver);
    }
  this->Internal->PropertyMap.clear();
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* pq3DWidget::getHints() const
{
  return this->Internal->Hints;
}

//-----------------------------------------------------------------------------
void pq3DWidget::setHints(vtkPVXMLElement* hints)
{
  this->Internal->Hints = hints;
  if (!hints)
    {
    return;
    }

  if (!this->proxy())
    {
    qDebug() << "pq3DWidget::setHints must be called only after the controlled "
      << "proxy has been set.";
    return;
    }
  if (QString("PropertyGroup") != hints->GetName())
    {
    qDebug() << "Argument to setHints must be a <PropertyGroup /> element.";
    return;
    }

  vtkSMProxy* pxy = this->proxy();
  unsigned int max_props = hints->GetNumberOfNestedElements();
  for (unsigned int i=0; i < max_props; i++)
    {
    vtkPVXMLElement* propElem = hints->GetNestedElement(i);
    this->setControlledProperty(propElem->GetAttribute("function"),
      pxy->GetProperty(propElem->GetAttribute("name")));
    }
}

//-----------------------------------------------------------------------------
void pq3DWidget::setControlledProperty(const char* function,
  vtkSMProperty* controlled_property)
{
  this->Internal->PropertyMap.insert(
    this->Internal->WidgetProxy->GetProperty(function),
    controlled_property);

  controlled_property->AddObserver(vtkCommand::ModifiedEvent,
    this->Internal->ControlledPropertiesObserver);
}

//-----------------------------------------------------------------------------
void pq3DWidget::setControlledProperty(vtkSMProperty* widget_property, 
  vtkSMProperty* controlled_property)
{
  this->Internal->PropertyMap.insert(
    widget_property,
    controlled_property);
    
  controlled_property->AddObserver(vtkCommand::ModifiedEvent,
    this->Internal->ControlledPropertiesObserver);
}

//-----------------------------------------------------------------------------
void pq3DWidget::accept()
{
  this->Internal->IgnorePropertyChange = true;
  QMap<vtkSmartPointer<vtkSMProperty>, 
    vtkSmartPointer<vtkSMProperty> >::const_iterator iter;
  for (iter = this->Internal->PropertyMap.constBegin() ;
    iter != this->Internal->PropertyMap.constEnd(); 
    ++iter)
    {
    iter.value()->Copy(iter.key());
    }
  if (this->proxy())
    {
    this->proxy()->UpdateVTKObjects();
    }
  this->Internal->IgnorePropertyChange = false;
}

//-----------------------------------------------------------------------------
void pq3DWidget::reset()
{
  // We don't want to fire any widget modified events while resetting the 
  // 3D widget, hence we block all signals. Otherwise, on reset, we fire a
  // widget modified event, which makes the accept button enabled again.
  this->blockSignals(true);
  QMap<vtkSmartPointer<vtkSMProperty>, 
    vtkSmartPointer<vtkSMProperty> >::const_iterator iter;
  for (iter = this->Internal->PropertyMap.constBegin() ;
    iter != this->Internal->PropertyMap.constEnd(); 
    ++iter)
    {
    iter.key()->Copy(iter.value());
    }

  if (this->Internal->WidgetProxy)
    {
    this->Internal->WidgetProxy->UpdateVTKObjects();
    this->Internal->WidgetProxy->UpdatePropertyInformation();
    this->render();
    }
  this->blockSignals(false);
}

//-----------------------------------------------------------------------------
bool pq3DWidget::widgetVisible() const
{
  return this->Internal->WidgetVisible;
}

//-----------------------------------------------------------------------------
bool pq3DWidget::widgetSelected() const
{
  return this->Internal->Selected;
}

//-----------------------------------------------------------------------------
void pq3DWidget::select()
{
  if(true != this->Internal->Selected)
    {
    this->Internal->Selected = true;
    this->updateWidgetVisibility();
    }
}

//-----------------------------------------------------------------------------
void pq3DWidget::deselect()
{
  if(false != this->Internal->Selected)
    {
    this->Internal->Selected = false;
    this->updateWidgetVisibility();
    }
}

//-----------------------------------------------------------------------------
void pq3DWidget::setWidgetVisible(bool visible)
{
  if(this->Internal->IsMaster)
    {
    this->Internal->LastWidgetVisibilityGoal = visible;
    }

  if( visible != this->Internal->WidgetVisible &&
      ((!this->Internal->IsMaster && !visible) || this->Internal->IsMaster))
    {
    this->Internal->WidgetVisible = visible;
    this->updateWidgetVisibility();

    // Handle trace to support show/hide actions
#ifdef PARAVIEW_ENABLE_PYTHON
    pqApplicationCore* core = pqApplicationCore::instance();
    pqPythonManager* manager =
        qobject_cast<pqPythonManager*>(core->manager("PYTHON_MANAGER"));
    if (manager && manager->interpreterIsInitialized() &&
        manager->canStopTrace() && this->renderView())
      {
      QString script =
          QString("try:\n"
                  "  paraview.smtrace\n"
                  "  paraview.smtrace.trace_change_widget_visibility('%1')\n"
                  "except AttributeError: pass\n").arg(
            visible ? "ShowWidget" : "HideWidget");
      pqPythonShell* shell = manager->pythonShellDialog()->shell();
      shell->executeScript(script);
      }
#endif
    
    emit this->widgetVisibilityChanged(visible);
    }
}

//-----------------------------------------------------------------------------
void pq3DWidget::showWidget()
{
  this->setWidgetVisible(true);
}

//-----------------------------------------------------------------------------
void pq3DWidget::hideWidget()
{
  this->setWidgetVisible(false);
}

//-----------------------------------------------------------------------------
int pq3DWidget::getReferenceInputBounds(double bounds[6]) const
{
  vtkSMProxy* refProxy = this->getReferenceProxy();
  if (!refProxy)
    {
    return 0;
    }
  
  vtkSMSourceProxy* input = NULL;
  vtkSMInputProperty* ivp = vtkSMInputProperty::SafeDownCast(
    refProxy->GetProperty("Input"));
  int output_port = 0;
  if (ivp && ivp->GetNumberOfProxies())
    {
    vtkSMProxy* pxy = ivp->GetProxy(0);
    input = vtkSMSourceProxy::SafeDownCast(pxy);
    output_port =ivp->GetOutputPortForConnection(0);
    }
  else
    {
    // reference proxy has no input. This generally happens when the widget is
    // controlling properties of a source. In that case, if the source has been
    // "created", simply use the source's bounds.
    input = vtkSMSourceProxy::SafeDownCast(refProxy);
    }

  if(input)
    {
    input->GetDataInformation(output_port)->GetBounds(bounds);
    return (bounds[1] >= bounds[0] && bounds[3] >= bounds[2] && bounds[5] >=
      bounds[4]) ? 1 : 0;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void pq3DWidget::updateWidgetVisibility()
{
  const bool widget_visible = this->Internal->Selected
    && this->Internal->WidgetVisible;
    
  const bool widget_enabled = widget_visible;
  
  this->updateWidgetState(widget_visible, widget_enabled);
}

//-----------------------------------------------------------------------------
void pq3DWidget::updateWidgetState(bool widget_visible, bool widget_enabled)
{
  if (this->Internal->WidgetProxy && this->renderView())
    {
    if(vtkSMIntVectorProperty* const visibility =
      vtkSMIntVectorProperty::SafeDownCast(
        this->Internal->WidgetProxy->GetProperty("Visibility")))
      {
      visibility->SetElement(0, widget_visible);
      }

    if(vtkSMIntVectorProperty* const enabled =
      vtkSMIntVectorProperty::SafeDownCast(
        this->Internal->WidgetProxy->GetProperty("Enabled")))
      {
      enabled->SetElement(0, widget_enabled);
      }

    this->Internal->WidgetProxy->UpdateVTKObjects();

    // need to render since the widget visibility state changed.
    this->render();
    }
  this->updatePickShortcut();
}

//-----------------------------------------------------------------------------
void pq3DWidget::updatePickShortcut()
{
  bool pickable = (this->Internal->Selected
    && this->Internal->WidgetVisible &&
    this->Internal->WidgetProxy && this->renderView());

  this->updatePickShortcut(pickable);
}

//-----------------------------------------------------------------------------
void pq3DWidget::updatePickShortcut(bool pickable)
{
  if (this->Internal->PickShortcut)
    {
    this->Internal->PickShortcut->setEnabled(pickable);
    }
}

//-----------------------------------------------------------------------------
void pq3DWidget::resetBounds()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if (!widget)
    {
    return;
    }

  double input_bounds[6];
  if (this->UseSelectionDataBounds)
    {
    vtkSMProxySelectionModel* selModel = getSelectionModel(widget);

    if (!selModel->GetSelectionDataBounds(input_bounds))
      {
      return;
      }
    }
  else if (!this->getReferenceInputBounds(input_bounds))
    {
    return;
    }
  this->resetBounds(input_bounds);
  this->setModified();
  this->render();
}
//-----------------------------------------------------------------------------
void pq3DWidget::updateMasterEnableState(bool I_am_the_Master)
{
  this->Internal->IsMaster = I_am_the_Master;
  if(I_am_the_Master)
    {
    this->setWidgetVisible(this->Internal->LastWidgetVisibilityGoal);
    }
  else
    {
    this->hideWidget();
    }
}
//-----------------------------------------------------------------------------
void pq3DWidget::handleSourceNotification(pqPipelineSource* source,char* msg)
{
  if(source->getProxy() == this->Internal->ReferenceProxy.GetPointer() && msg)
    {
    if(!strcmp("HideWidget", msg))
      {
      this->hideWidget();
      }
    else if(!strcmp("ShowWidget",msg))
      {
      this->showWidget();
      }
    }
}
