/*=========================================================================

   Program: ParaView
   Module:  pqRemoveLightWidget.cxx

   Copyright (c) 2017 Sandia Corporation, Kitware Inc.
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
#include "pqRemoveLightWidget.h"

#include "vtkPVProxyDefinitionIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"

#include <QPushButton>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
pqRemoveLightWidget::pqRemoveLightWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  PV_DEBUG_PANELS() << "pqRemoveLightWidget for a property with "
                       "the panel_widget=\"remove_light_button\" attribute.";
  // vtkSMSessionProxyManager* pxm = smproxy->GetSessionProxyManager();

  QPushButton* button = new QPushButton(smproperty->GetXMLLabel(), this);
  button->setObjectName("PushButton");
  // this is over-written by pqProxyWidget to this value:
  // this->setObjectName("RemoveLight");

  this->PushButton = button;

  this->connect(button, SIGNAL(pressed()), SLOT(buttonPressed()));

  QHBoxLayout* layoutLocal = new QHBoxLayout(this);
  layoutLocal->setMargin(0);
  layoutLocal->addWidget(button);
  layoutLocal->addStretch();

  this->setShowLabel(false);
}

//-----------------------------------------------------------------------------
pqRemoveLightWidget::~pqRemoveLightWidget()
{
}

void pqRemoveLightWidget::buttonPressed()
{
  // pass the signal along to the listening pqLightInspector,
  // with our proxy so we can be identified.
  emit removeLight(this->proxy());
}
