/*=========================================================================

   Program: ParaView
   Module:  pqDisplayColor2Widget.cxx

   Copyright (c) 2005-2022 Sandia Corporation, Kitware Inc.
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
#include "pqDisplayColor2Widget.h"

#include "pqArraySelectorPropertyWidget.h"
#include "pqDataRepresentation.h"
#include "pqIntVectorPropertyWidget.h"
#include "pqPropertiesPanel.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QPointer>

#include <string>

class pqDisplayColor2Widget::pqInternals
{
public:
  QPointer<pqDataRepresentation> Representation;
  QPointer<QWidget> Variables;
  QPointer<QWidget> Components;

  // Locks in the opacity array name on the representation.
  void FinalizeColorArray2Name(vtkSMPVRepresentationProxy* reprProxy)
  {
    vtkSMPropertyHelper helper(reprProxy, "ColorArray2Name");
    helper.SetUseUnchecked(true);
    const char* name = helper.GetInputArrayNameToProcess();
    const int assoc = helper.GetInputArrayAssociation();
    // If the selected array has this name, then enable/disable that property on the representation
    if (std::string(name) == "Gradient Magnitude")
    {
      vtkSMPropertyHelper(reprProxy, "UseGradientForTransfer2D", true).Set(1);
    }
    else
    {
      vtkSMPropertyHelper(reprProxy, "UseGradientForTransfer2D", true).Set(0);
      helper.SetUseUnchecked(false);
      helper.SetInputArrayToProcess(assoc, name);
    }
  }
  // Locks in the opacity array component on the representation.
  void FinalizeColorArray2Component(vtkSMPVRepresentationProxy* reprProxy)
  {
    vtkSMPropertyHelper helper(reprProxy, "ColorArray2Component");
    helper.SetUseUnchecked(true);
    const int component = helper.GetAsInt();
    helper.SetUseUnchecked(false);
    helper.Set(component);
  }
};

pqDisplayColor2Widget::pqDisplayColor2Widget(QWidget* parent)
  : Superclass(parent)
  , Internals(std::unique_ptr<pqInternals>(new pqInternals()))
{
  auto hbox = new QHBoxLayout();
  hbox->setContentsMargins(0, 0, 0, 0);
  hbox->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  this->setLayout(hbox);
  // default, show empty comboboxes. populated when a valid representation is available.
  this->Internals->Variables = new QComboBox(this);
  this->Internals->Variables->setMinimumSize(QSize(150, 0));
  this->Internals->Components = new QComboBox(this);
  hbox->addWidget(this->Internals->Variables);
  hbox->addWidget(this->Internals->Components);
}

pqDisplayColor2Widget::~pqDisplayColor2Widget() = default;

void pqDisplayColor2Widget::setRepresentation(pqDataRepresentation* display)
{
  if (this->Internals->Representation != nullptr)
  {
    this->Internals->Representation = nullptr;
  }
  // gets rid of stale widgets from previous representation (if any)
  if (this->Internals->Variables != nullptr)
  {
    this->layout()->removeWidget(this->Internals->Variables);
    delete this->Internals->Variables;
    this->Internals->Variables = nullptr;
  }
  if (this->Internals->Components != nullptr)
  {
    this->layout()->removeWidget(this->Internals->Components);
    delete this->Internals->Components;
    this->Internals->Components = nullptr;
  }

  vtkSMProxy* reprProxy = nullptr;
  vtkSMProperty* varProp = nullptr;
  // valid representation? create array selector widget and an accompanying
  // int vector property widget for the array components.
  if (display && (reprProxy = display->getProxy()) &&
    (varProp = reprProxy->GetProperty("ColorArray2Name")))
  {
    this->Internals->Variables = new pqArraySelectorPropertyWidget(
      varProp, reprProxy, { qMakePair(0, QString("Gradient Magnitude")) }, this);
    this->Internals->Variables->setObjectName("Variables");
    this->Internals->Variables->setMinimumSize(QSize(150, 0));
    qobject_cast<pqArraySelectorPropertyWidget*>(this->Internals->Variables)
      ->setArray(0, "Gradient Magnitude");

    vtkSMProperty* componentProp = reprProxy->GetProperty("ColorArray2Component");
    this->Internals->Components = new pqIntVectorPropertyWidget(componentProp, reprProxy, this);
    this->Internals->Components->setObjectName("Components");

    this->Internals->Representation = display;

    QObject::connect(display, &pqDataRepresentation::useSeparateOpacityArrayModified, this,
      &pqDisplayColor2Widget::onArrayModified);
    QObject::connect(qobject_cast<pqArraySelectorPropertyWidget*>(this->Internals->Variables),
      &pqArraySelectorPropertyWidget::arrayChanged, this, &pqDisplayColor2Widget::onArrayModified);
    QObject::connect(qobject_cast<pqIntVectorPropertyWidget*>(this->Internals->Components),
      &pqIntVectorPropertyWidget::changeAvailable, this, &pqDisplayColor2Widget::onArrayModified);
  }
  else
  {
    // dummy combo boxes.
    this->Internals->Variables = new QComboBox(this);
    this->Internals->Variables->setMinimumSize(QSize(150, 0));
    this->Internals->Components = new QComboBox(this);
  }
  this->layout()->addWidget(this->Internals->Variables);
  this->layout()->addWidget(this->Internals->Components);
}

void pqDisplayColor2Widget::onArrayModified()
{
  if (this->Internals->Representation == nullptr)
  {
    return;
  }
  vtkSMPVRepresentationProxy* reprProxy =
    vtkSMPVRepresentationProxy::SafeDownCast(this->Internals->Representation->getProxy());
  if (reprProxy == nullptr)
  {
    return;
  }
  // need to lock in the array name and component before VTK objects are updated.
  this->Internals->FinalizeColorArray2Name(reprProxy);
  this->Internals->FinalizeColorArray2Component(reprProxy);
  reprProxy->UpdateVTKObjects();
  this->Internals->Representation->renderViewEventually();
}
