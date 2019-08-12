/*=========================================================================

   Program: ParaView
   Module:  pqLightsInspector.cxx

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
#include "pqLightsInspector.h"
#include "ui_pqLightsInspector.h"

#include "vtkCamera.h"
#include "vtkPVLight.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqLightsEditor.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyWidget.h"
#include "pqProxyWidget.h"
#include "pqRenderView.h"
#include "pqUndoStack.h"
#include "pqView.h"

//=============================================================================
class pqLightsInspector::pqInternals
{
public:
  Ui::LightsInspector Ui;
  pqLightsInspector* self;
  int observerTag;
  vtkSMRenderViewProxy* rview;

  pqInternals(pqLightsInspector* inSelf)
    : self(inSelf)
    , observerTag(0)
    , rview(nullptr)
  {
    this->Ui.setupUi(inSelf);

    // update the panel when user picks a different view
    QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), inSelf,
      SLOT(setActiveView(pqView*)));

    // hook up the add light button
    connect(this->Ui.addLight, SIGNAL(pressed()), inSelf, SLOT(addLight()));

    // populate the area with controls for each light
    this->updateLightWidgets();
  }

  ~pqInternals()
  {
    if (this->rview && this->observerTag != 0 && this->rview->GetProperty("AdditionalLights"))
    {
      this->rview->GetProperty("AdditionalLights")->RemoveObserver(this->observerTag);
    }
    QLayoutItem* child;
    while ((child = this->Ui.verticalLayout_2->takeAt(0)) != 0)
    {
      if (child->widget())
      {
        delete child->widget();
      }
      delete child;
    }
  }

  vtkSMRenderViewProxy* getActiveView()
  {
    // returns nullptr if the active view is incompatible,
    // otherwise returns the corresponding SMViewProxy
    pqView* pv = pqActiveObjects::instance().activeView();
    if (pv == nullptr)
    {
      return nullptr;
    }
    vtkSMRenderViewProxy* view = vtkSMRenderViewProxy::SafeDownCast(pv->getViewProxy());
    return view;
  }

  void updateLightWidgets()
  {
    // we populate the qt panel with child widgets for each light here

    // clear current contents
    QLayoutItem* child;
    child = this->Ui.verticalLayout->itemAt(0);
    // is this a lightkit widget?
    if (child->widget() && child->widget()->objectName() == "LightsEditor")
    {
      child = this->Ui.verticalLayout->takeAt(0);
      delete child->widget();
      delete child;
    }
    while ((child = this->Ui.verticalLayout_2->takeAt(0)) != 0)
    {
      if (child->widget())
      {
        delete child->widget();
      }
      delete child;
    }

    if (this->rview && this->observerTag != 0)
    {
      this->rview->GetProperty("AdditionalLights")->RemoveObserver(this->observerTag);
      this->observerTag = 0;
    }
    vtkSMRenderViewProxy* view = this->getActiveView();
    if (!view || !view->GetProperty("AdditionalLights"))
    {
      // disable the 'add light' button
      this->Ui.addLight->setEnabled(false);
      this->rview = nullptr;
      return;
    }
    this->Ui.addLight->setEnabled(true);

    pqView* pv = pqActiveObjects::instance().activeView();
    // add new light kit for this view.
    vtkSMPropertyGroup* lightkitGroup = nullptr;
    size_t ngroups = view->GetNumberOfPropertyGroups();
    for (size_t j = 0; j < ngroups; ++j)
    {
      vtkSMPropertyGroup* smGroup = view->GetPropertyGroup(j);
      if (smGroup->GetXMLLabel() == vtkStdString("Lights"))
      {
        lightkitGroup = smGroup;
        break;
      }
    }
    if (lightkitGroup)
    {
      pqLightsEditor* lightsEditor = new pqLightsEditor(view, lightkitGroup, self);
      this->Ui.verticalLayout->insertWidget(0, lightsEditor);
      // whenever a property changes, do a render.
      self->connect(lightsEditor, SIGNAL(changeFinished()), SLOT(render()));
    }
    // add new contents
    unsigned int nlights = vtkSMPropertyHelper(view, "AdditionalLights").GetNumberOfElements();
    for (unsigned int i = 0; i < nlights; ++i)
    {
      vtkSMProxy* light = vtkSMPropertyHelper(view, "AdditionalLights").GetAsProxy(i);

      // add a container widget - title, two buttons, proxy widget
      QWidget* widget = new QWidget(self);
      widget->setObjectName(QString("LightContainer%1").arg(i));
      QVBoxLayout* vbox = new QVBoxLayout(widget);
      vbox->setContentsMargins(0, pqPropertiesPanel::suggestedVerticalSpacing(), 0, 0);
      vbox->setSpacing(0);
      vbox->addWidget(pqProxyWidget::newGroupLabelWidget(QString("Light %1").arg(i), widget));

      // some buttons.
      QWidget* buttonWidget = new QWidget(widget);
      buttonWidget->setObjectName("ButtonContainer");
      QHBoxLayout* hbox = new QHBoxLayout(buttonWidget);
      hbox->setContentsMargins(0, 0, 0, 0);
      hbox->setSpacing(0);
      vbox->addWidget(buttonWidget);

      // Add a button to sync this light with the camera.
      QPushButton* syncButton = new QPushButton("Move to Camera");
      syncButton->setObjectName("MoveToCamera");
      // which light does this button affect?
      syncButton->setProperty("LightIndex", i);
      syncButton->setToolTip("Match this light's position and focal point to the camera.");
      connect(syncButton, SIGNAL(clicked()), self, SLOT(syncLightToCamera()));
      hbox->addWidget(syncButton);
      this->updateMoveToCamera(i, light);

      // Add a button to reset this light.
      QPushButton* resetButton = new QPushButton("Reset Light");
      resetButton->setObjectName("ResetLight");
      // which light does this button affect?
      resetButton->setProperty("LightIndex", i);
      resetButton->setToolTip("Reset this light parameters to default");
      connect(resetButton, SIGNAL(clicked()), self, SLOT(resetLight()));
      hbox->addWidget(resetButton);

      // Add a button to remove this light.
      QPushButton* removeButton = new QPushButton("Remove Light");
      removeButton->setObjectName("RemoveLight");
      // which light does this button affect?
      removeButton->setProperty("LightIndex", i);
      removeButton->setToolTip("Remove this light.");
      connect(removeButton, SIGNAL(clicked()), self, SLOT(removeLight()));
      hbox->addWidget(removeButton);

      // todo: we want to customize this to make sure controls are
      // consistent with the light type
      pqProxyWidget* lightWidget = new pqProxyWidget(light, widget);
      lightWidget->setApplyChangesImmediately(true);
      lightWidget->setView(pv);
      // which light does this widget affect?
      lightWidget->setProperty("LightIndex", i);
      lightWidget->updatePanel();
      vbox->addWidget(lightWidget);
      // whenever a property changes, do a render.
      self->connect(lightWidget, SIGNAL(changeFinished()), SLOT(updateAndRender()));

      // add it to this->Ui.scrollArea
      this->Ui.verticalLayout_2->addWidget(widget);
      this->Ui.verticalLayout_2->addSpacing(20);
    }
    this->Ui.verticalLayout_2->addStretch(1);
    // add observer to track when it changes.
    vtkSMProperty* property = view->GetProperty("AdditionalLights");
    this->observerTag = property->AddObserver(
      vtkCommand::ModifiedEvent, this, &pqLightsInspector::pqInternals::updateLightWidgets);
    this->rview = view;
  }

  void updateMoveToCamera(int lightIndex, vtkSMProxy* lightProxy)
  {
    int type = 0;
    vtkSMPropertyHelper(lightProxy, "LightType").Get(&type);
    bool visible = (type == VTK_LIGHT_TYPE_CAMERA_LIGHT || type == VTK_LIGHT_TYPE_SCENE_LIGHT);

    QWidget* container = self->findChild<QWidget*>(QString("LightContainer%1").arg(lightIndex));
    QPushButton* button = container->findChild<QPushButton*>(QString("MoveToCamera"));
    button->setVisible(visible);
  }
};

//-----------------------------------------------------------------------------
pqLightsInspector::pqLightsInspector(
  QWidget* parentObject, Qt::WindowFlags f, bool /* arg_autotracking */)
  : Superclass(parentObject, f)
  , Internals(new pqLightsInspector::pqInternals(this))
{
  // pqInternals& internals = (*this->Internals);
}

//-----------------------------------------------------------------------------
pqLightsInspector::~pqLightsInspector()
{
}

//-----------------------------------------------------------------------------
void pqLightsInspector::setActiveView(pqView*)
{
  this->Internals->updateLightWidgets();
}

//-----------------------------------------------------------------------------
void pqLightsInspector::addLight()
{
  vtkSMRenderViewProxy* view = this->Internals->getActiveView();
  if (!view)
  {
    return;
  }

  // tell undo/redo and state about the new light
  BEGIN_UNDO_SET("Add Light");
  vtkSMSessionProxyManager* pxm = view->GetSessionProxyManager();
  vtkSMProxy* light = pxm->NewProxy("additional_lights", "Light");

  // standard application level logic
  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->InitializeProxy(light);
  // registration, python tracing handled here:
  controller->RegisterLightProxy(light, view);

  light->Delete();

  // call vtkPVRenderView::AddLight - light saved in "AdditionalLights" list
  vtkSMPropertyHelper(view, "AdditionalLights").Add(light);

  // modification of AdditionalLights already calls this:
  // this->Internals->updateLightWidgets();

  // make it so...
  view->UpdateVTKObjects();
  END_UNDO_SET();

  this->render();
}

//-----------------------------------------------------------------------------
void pqLightsInspector::render()
{
  pqRenderView* renderView = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (!renderView)
  {
    return;
  }
  renderView->render();
}

//-----------------------------------------------------------------------------
void pqLightsInspector::updateAndRender()
{
  vtkSMRenderViewProxy* view = this->Internals->getActiveView();
  if (!view)
  {
    return;
  }

  auto proxyWidget = sender();
  int index = proxyWidget->property("LightIndex").toInt();

  vtkSMProxy* lightProxy = vtkSMPropertyHelper(view, "AdditionalLights").GetAsProxy(index);
  this->Internals->updateMoveToCamera(index, lightProxy);

  this->render();
}

//-----------------------------------------------------------------------------
void pqLightsInspector::removeLight(vtkSMProxy* lightProxy)
{
  vtkSMRenderViewProxy* view = this->Internals->getActiveView();
  if (!view)
  {
    return;
  }

  unsigned int nlights = vtkSMPropertyHelper(view, "AdditionalLights").GetNumberOfElements();
  if (nlights < 1)
  {
    return;
  }

  if (!lightProxy)
  {
    auto removeButton = sender();
    int index = removeButton->property("LightIndex").toInt();

    lightProxy = vtkSMPropertyHelper(view, "AdditionalLights").GetAsProxy(index);
  }
  // tell python trace to remove the light
  SM_SCOPED_TRACE(CallFunction)
    .arg("RemoveLight")
    .arg("light", lightProxy)
    .arg("comment", "remove light added to the view");

  // tell undo/redo and state about the new light
  BEGIN_UNDO_SET("Remove Light");

  vtkSMPropertyHelper(view, "AdditionalLights").Remove(lightProxy);

  // this prevents undo of the remove
  // vtkNew<vtkSMParaViewPipelineController> controller;
  // controller->UnRegisterProxy(lightProxy);

  // modification of AdditionalLights already calls this:
  // this->Internals->updateLightWidgets();

  // make it so...
  view->UpdateVTKObjects();
  END_UNDO_SET();

  this->render();
}

//-----------------------------------------------------------------------------
void pqLightsInspector::resetLight(vtkSMProxy* lightProxy)
{
  vtkSMRenderViewProxy* view = this->Internals->getActiveView();
  if (!view)
  {
    return;
  }
  if (!lightProxy)
  {
    auto syncButton = sender();
    int index = syncButton->property("LightIndex").toInt();

    lightProxy = vtkSMPropertyHelper(view, "AdditionalLights").GetAsProxy(index);
  }

  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", lightProxy).arg("comment", "reset a light");
  lightProxy->ResetPropertiesToDefault();
  lightProxy->UpdateVTKObjects();
  this->render();
}

//-----------------------------------------------------------------------------
void pqLightsInspector::syncLightToCamera(vtkSMProxy* lightProxy)
{
  vtkSMRenderViewProxy* view = this->Internals->getActiveView();
  if (!view)
  {
    return;
  }
  if (!lightProxy)
  {
    auto resetButton = sender();
    int index = resetButton->property("LightIndex").toInt();

    lightProxy = vtkSMPropertyHelper(view, "AdditionalLights").GetAsProxy(index);
  }

  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", lightProxy).arg("comment", "update a light");

  int type = 0;
  vtkSMPropertyHelper(lightProxy, "LightType").Get(&type);

  // default camera light params.
  double position[3] = { 0, 0, 1 };
  double focal[3] = { 0, 0, 0 };
  // nothing to do for headlight or ambient light
  if (type == VTK_LIGHT_TYPE_CAMERA_LIGHT)
  {
    // camera light is relative the camera already, this button is just a 'reset' to defaults.
    vtkSMPropertyHelper(lightProxy, "LightPosition").Modified().Set(position, 3);
    vtkSMPropertyHelper(lightProxy, "FocalPoint").Modified().Set(focal, 3);
  }
  else if (type == VTK_LIGHT_TYPE_SCENE_LIGHT)
  {
    vtkCamera* camera = view->GetActiveCamera();
    camera->GetPosition(position);
    camera->GetFocalPoint(focal);

    vtkSMPropertyHelper(lightProxy, "LightPosition").Modified().Set(position, 3);
    vtkSMPropertyHelper(lightProxy, "FocalPoint").Modified().Set(focal, 3);
  }
  lightProxy->UpdateVTKObjects();
  this->render();
}
