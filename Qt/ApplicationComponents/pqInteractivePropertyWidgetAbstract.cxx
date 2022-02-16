/*=========================================================================

   Program: ParaView
   Module:  pqInteractivePropertyWidgetAbstract.cxx

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
#include "pqInteractivePropertyWidgetAbstract.h"

#include "pqCoreUtilities.h"
#include "pqPropertyLinks.h"
#include "pqServer.h"
#include "pqView.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMNewWidgetRepresentationProxyAbstract.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <cassert>
#include <string>

struct pqInteractivePropertyWidgetAbstract::pqInternals
{
  vtkWeakPointer<vtkSMProxy> DataSource;
  vtkSmartPointer<vtkSMPropertyGroup> SMGroup;
  unsigned long UserEventObserverId = 0;
};

//-----------------------------------------------------------------------------
pqInteractivePropertyWidgetAbstract::pqInteractivePropertyWidgetAbstract(
  const char* /* widget_smgroup */, const char* /* widget_smname */, vtkSMProxy* smproxy,
  vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqInteractivePropertyWidgetAbstract::pqInternals())
{
  assert(smproxy);
  assert(smgroup);
  this->Internals->SMGroup = smgroup;

  auto* hints = smgroup->GetHints();
  vtkPVXMLElement* visLink = nullptr;
  if (hints && (visLink = hints->FindNestedElementByName("WidgetVisibilityLink")))
  {
    if (const char* portIndex = visLink->GetAttribute("port"))
    {
      try
      {
        this->LinkedPortIndex = std::stoi(portIndex);
      }
      catch (const std::exception&)
      {
        this->LinkedPortIndex = -1;
      }
    }
  }
}

//-----------------------------------------------------------------------------
pqInteractivePropertyWidgetAbstract::~pqInteractivePropertyWidgetAbstract()
{
  if (this->Internals->UserEventObserverId > 0 && this->proxy())
  {
    this->proxy()->RemoveObserver(this->Internals->UserEventObserverId);
    this->Internals->UserEventObserverId = 0;
  }
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidgetAbstract::setupConnections(
  vtkSMNewWidgetRepresentationProxyAbstract* widget, vtkSMPropertyGroup* smgroup,
  vtkSMProxy* smproxy)
{
  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->InitializeProxy(widget);

  // Setup links between the proxy that the widget is going to be controlling
  widget->LinkProperties(smproxy, smgroup);
  widget->UpdateVTKObjects();

  // Marking this as a prototype ensures that the undo/redo system doesn't track
  // changes to the widget.
  widget->PrototypeOn();

  pqCoreUtilities::connect(
    widget, vtkCommand::StartInteractionEvent, this, SIGNAL(startInteraction()));
  pqCoreUtilities::connect(
    widget, vtkCommand::StartInteractionEvent, this, SIGNAL(changeAvailable()));
  pqCoreUtilities::connect(widget, vtkCommand::InteractionEvent, this, SIGNAL(interaction()));
  pqCoreUtilities::connect(widget, vtkCommand::InteractionEvent, this, SIGNAL(changeAvailable()));
  pqCoreUtilities::connect(widget, vtkCommand::EndInteractionEvent, this, SIGNAL(endInteraction()));
  pqCoreUtilities::connect(widget, vtkCommand::EndInteractionEvent, this, SIGNAL(changeFinished()));

  if (vtkSMProperty* input = smgroup->GetProperty("Input"))
  {
    this->setDataSource(vtkSMPropertyHelper(input).GetAsProxy());
  }
  else
  {
    this->setDataSource(nullptr);
  }

  // This ensures that when the user changes the Qt widget, we re-render to show
  // the update widget.
  this->connect(&this->links(), SIGNAL(qtWidgetChanged()), SLOT(render()));
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidgetAbstract::setupUserObserver(vtkSMProxy* smproxy)
{
  this->Internals->UserEventObserverId = smproxy->AddObserver(
    vtkCommand::UserEvent, this, &pqInteractivePropertyWidgetAbstract::handleUserEvent);
}

//-----------------------------------------------------------------------------
vtkSMPropertyGroup* pqInteractivePropertyWidgetAbstract::propertyGroup() const
{
  return this->Internals->SMGroup;
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidgetAbstract::setView(pqView* pqview)
{
  vtkSMNewWidgetRepresentationProxyAbstract* widgetProxy = this->internalWidgetProxy();

  if (pqview != nullptr && pqview->getServer()->session() != widgetProxy->GetSession())
  {
    pqview = nullptr;
  }

  pqView* rview = qobject_cast<pqView*>(pqview);

  pqView* oldview = this->view();
  if (oldview == rview)
  {
    return;
  }

  // Use vtkSMPropertyHelper in quiet mode so incompatible views (spreadsheet
  // view for example) stops complaining
  if (oldview)
  {
    vtkSMPropertyHelper(oldview->getProxy(), "HiddenRepresentations", true).Remove(widgetProxy);
    oldview->getProxy()->UpdateVTKObjects();
  }
  this->Superclass::setView(rview);
  if (rview)
  {
    vtkSMPropertyHelper(rview->getProxy(), "HiddenRepresentations", true).Add(widgetProxy);
    rview->getProxy()->UpdateVTKObjects();
  }
  this->updateWidgetVisibility();
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidgetAbstract::select()
{
  this->Superclass::select();
  this->placeWidget();
  this->updateWidgetVisibility();
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidgetAbstract::selectPort(int portIndex)
{
  if (this->LinkedPortIndex < 0 || this->LinkedPortIndex == portIndex)
  {
    this->select();
  }
  else
  {
    this->deselect();
  }
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidgetAbstract::deselect()
{
  this->Superclass::deselect();
  this->updateWidgetVisibility();
}

//-----------------------------------------------------------------------------
bool pqInteractivePropertyWidgetAbstract::isWidgetVisible() const
{
  return this->WidgetVisibility;
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidgetAbstract::setDataSource(vtkSMProxy* dsource)
{
  this->Internals->DataSource = dsource;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqInteractivePropertyWidgetAbstract::dataSource() const
{
  return this->Internals->DataSource;
}

//-----------------------------------------------------------------------------
vtkBoundingBox pqInteractivePropertyWidgetAbstract::dataBounds() const
{
  if (vtkSMSourceProxy* dsrc = vtkSMSourceProxy::SafeDownCast(this->dataSource()))
  {
    // FIXME: we need to get the output port number correctly. For now, just use
    // 0.
    vtkPVDataInformation* dataInfo = dsrc->GetDataInformation(0);
    vtkBoundingBox bbox(dataInfo->GetBounds());
    return bbox;
  }
  else
  {
    vtkBoundingBox bbox;
    return bbox;
  }
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidgetAbstract::render()
{
  if (pqView* pqview = this->view())
  {
    pqview->render();
  }
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidgetAbstract::reset()
{
  this->Superclass::reset();
  this->render();
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidgetAbstract::setWidgetVisible(bool val)
{
  if (this->WidgetVisibility != val)
  {
    SM_SCOPED_TRACE(CallFunction)
      .arg(val ? "ShowInteractiveWidgets" : "HideInteractiveWidgets")
      .arg("proxy", this->proxy())
      .arg("comment", "toggle interactive widget visibility (only when running from the GUI)");

    this->WidgetVisibility = val;
    this->updateWidgetVisibility();
    Q_EMIT this->widgetVisibilityToggled(val);
  }
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidgetAbstract::handleUserEvent(
  vtkObject* caller, unsigned long eventid, void* calldata)
{
  Q_UNUSED(caller);
  Q_UNUSED(eventid);

  assert(caller == this->proxy());
  assert(eventid == vtkCommand::UserEvent);

  const char* message = reinterpret_cast<const char*>(calldata);
  if (message != nullptr && strcmp("HideWidget", message) == 0)
  {
    this->setWidgetVisible(false);
  }
  else if (message != nullptr && strcmp("ShowWidget", message) == 0)
  {
    this->setWidgetVisible(true);
  }
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidgetAbstract::hideEvent(QHideEvent*)
{
  vtkSMNewWidgetRepresentationProxyAbstract* widgetProxy = this->internalWidgetProxy();

  this->VisibleState = vtkSMPropertyHelper(widgetProxy, "Visibility").GetAsInt() != 0;
  this->setWidgetVisible(false);
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidgetAbstract::showEvent(QShowEvent*)
{
  this->setWidgetVisible(this->VisibleState);
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidgetAbstract::updateWidgetVisibility()
{
  bool visible = this->isSelected() && this->isWidgetVisible() && this->view();
  vtkSMProxy* wdgProxy = this->internalWidgetProxy();
  assert(wdgProxy);

  vtkSMPropertyHelper(wdgProxy, "Visibility", true).Set(visible);
  vtkSMPropertyHelper(wdgProxy, "Enabled", true).Set(visible);
  wdgProxy->UpdateVTKObjects();
  this->render();
  Q_EMIT this->widgetVisibilityUpdated(visible);
}
