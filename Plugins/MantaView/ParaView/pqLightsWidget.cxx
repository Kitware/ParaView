/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqLightsWidget.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqInterfaceTracker.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyWidgetInterface.h"
#include "pqProxyEditorPropertyWidget.h"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

#include "vtkSMMantaViewProxy.h"
#include "vtkSMPropertyGroup.h"

//-----------------------------------------------------------------------------
pqLightsWidget::pqLightsWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  this->NumberOfLights = 1;
  this->CurrentLight = 0;

  this->setShowLabel(true);

  QGridLayout* gridLayout = new QGridLayout(this);
  gridLayout->setMargin(pqPropertiesPanel::suggestedMargin());
  gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());

  QPushButton* add = new QPushButton("Add Light", this);
  QObject::connect(add, SIGNAL(released()), this, SLOT(onAdd()));
  gridLayout->addWidget(add, 0, 0);

  QHBoxLayout* boxLayout = new QHBoxLayout();
  gridLayout->addLayout(boxLayout, 1, 0);

  QPushButton* previous = new QPushButton("<", this);
  QObject::connect(previous, SIGNAL(released()), this, SLOT(onPrevious()));
  boxLayout->addWidget(previous);

  vtkSMProperty* clight = smgroup->GetProperty("CurrentLight");
  pqProxyEditorPropertyWidget* editor = new pqProxyEditorPropertyWidget(smproxy, clight, this);
  editor->setProperty(clight);
  editor->setObjectName("light_id");
  boxLayout->addWidget(editor);
  this->UpdateLabel();
  QObject::connect(editor, SIGNAL(changeAvailable()), this, SLOT(forceRender()));

  QPushButton* next = new QPushButton(">", this);
  QObject::connect(next, SIGNAL(released()), this, SLOT(onNext()));
  boxLayout->addWidget(next);
}

//-----------------------------------------------------------------------------
pqLightsWidget::~pqLightsWidget()
{
}

//-----------------------------------------------------------------------------
void pqLightsWidget::forceRender()
{
  vtkSMMantaViewProxy* rvproxy =
    vtkSMMantaViewProxy::SafeDownCast(pqActiveObjects::instance().activeView()->getViewProxy());
  if (!rvproxy)
  {
    return;
  }
  rvproxy->StillRender();
  pqActiveObjects::instance().activeView()->render();
}

//-----------------------------------------------------------------------------
void pqLightsWidget::onAdd()
{
  vtkSMMantaViewProxy* rvproxy =
    vtkSMMantaViewProxy::SafeDownCast(pqActiveObjects::instance().activeView()->getViewProxy());
  if (!rvproxy)
  {
    return;
  }
  this->NumberOfLights++;
  rvproxy->MakeLight();
  this->forceRender();
}

//-----------------------------------------------------------------------------
void pqLightsWidget::onPrevious()
{
  vtkSMMantaViewProxy* rvproxy =
    vtkSMMantaViewProxy::SafeDownCast(pqActiveObjects::instance().activeView()->getViewProxy());
  if (!rvproxy)
  {
    return;
  }
  this->CurrentLight--;
  if (this->CurrentLight < 0)
  {
    this->CurrentLight = 0;
  }
  this->UpdateLabel();
  rvproxy->PreviousLight();
}

//-----------------------------------------------------------------------------
void pqLightsWidget::onNext()
{
  vtkSMMantaViewProxy* rvproxy =
    vtkSMMantaViewProxy::SafeDownCast(pqActiveObjects::instance().activeView()->getViewProxy());
  if (!rvproxy)
  {
    return;
  }
  this->CurrentLight++;
  if (this->CurrentLight == this->NumberOfLights)
  {
    this->CurrentLight = this->NumberOfLights - 1;
  }
  this->UpdateLabel();
  rvproxy->NextLight();
}

//-----------------------------------------------------------------------------
void pqLightsWidget::UpdateLabel()
{
  pqProxyEditorPropertyWidget* editor = this->findChild<pqProxyEditorPropertyWidget*>("light_id");
  QPushButton* button = editor->findChild<QPushButton*>("PushButton");
  button->setText("Edit Light " + QString::number(this->CurrentLight));
}
