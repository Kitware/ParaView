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
#include "pqYoungsMaterialInterfacePanel.h"

#include "pqComboBoxDomain.h"
#include "pqCoreUtilities.h"
#include "pqSMAdaptor.h"
#include "pqTreeWidget.h"
#include "vtkCommand.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <QGridLayout>
#include <QComboBox>
#include <QLabel>

//-----------------------------------------------------------------------------
pqYoungsMaterialInterfacePanel::pqYoungsMaterialInterfacePanel(
  pqProxy* pqproxy, QWidget* parentObject)
  : Superclass(pqproxy, parentObject), BlockUpdateOptions(false)
{
  vtkSMProxy* smproxy = this->proxy();
  Q_ASSERT(smproxy != NULL);
  Q_ASSERT(smproxy->GetProperty("VolumeFractionArrays"));
  Q_ASSERT(smproxy->GetProperty("OrderingArrays"));
  Q_ASSERT(smproxy->GetProperty("NormalArrays"));

  QTreeWidget* volumeFractionArraysWidget = this->findChild<QTreeWidget*>(
    "VolumeFractionArrays");
  Q_ASSERT(volumeFractionArraysWidget);
  this->VolumeFractionArrays = volumeFractionArraysWidget;
 
  QObject::connect(volumeFractionArraysWidget,
    SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
    this, SLOT(updateOptions()));

  //QObject::connect(volumeFractionArraysWidget,
  //  SIGNAL(itemSelectionChanged()), this, SLOT(updateOptions()));

  QLabel* label1 = new QLabel("Ordering Array:", this);
  QComboBox* box1 = new QComboBox(this);
  box1->setObjectName("OrderingArrays");

  pqComboBoxDomain* domain1 = new pqComboBoxDomain(
    box1, smproxy->GetProperty("OrderingArrays"), "array_list");
  domain1->setObjectName("OrderingArraysDomain");
  domain1->addString("None");

  QLabel* label2 = new QLabel("Normal Array:", this);
  QComboBox* box2 = new QComboBox(this);
  box2->setObjectName("NormalArrays");

  pqComboBoxDomain* domain2 = new pqComboBoxDomain(
    box2, smproxy->GetProperty("NormalArrays"), "array_list");
  domain2->setObjectName("NormalArraysDomain");
  domain2->addString("None");

  int row_index = this->PanelLayout->rowCount();
  this->PanelLayout->addWidget(label1, row_index, 0, 1, 1);
  this->PanelLayout->addWidget(box1, row_index, 1, 1, 1);

  row_index++;
  this->PanelLayout->addWidget(label2, row_index, 0, 1, 1);
  this->PanelLayout->addWidget(box2, row_index, 1, 1, 1);

  this->OrderingArrays = box1;
  this->NormalArrays = box2;

  QObject::connect(
    this->OrderingArrays, SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(orderingArraysChanged(const QString&)));
  QObject::connect(
    this->NormalArrays, SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(normalArraysChanged(const QString&)));

  // this ensures that on undo-redo, the combo-boxes are updated correctly.
  // FIXME: This doesn't work as expected since vtkSMVectorPropertyTemplate fires
  // ModifiedEvent before the unchecked property values are cleared. We need to
  // think and decide what should be the order of those two actions.
  pqCoreUtilities::connect(
    smproxy->GetProperty("NormalArrays"), vtkCommand::ModifiedEvent,
    this, SLOT(updateOptions()));
  pqCoreUtilities::connect(
    smproxy->GetProperty("OrderingArrays"), vtkCommand::ModifiedEvent,
    this, SLOT(updateOptions()));
  this->updateOptions();
}

//-----------------------------------------------------------------------------
pqYoungsMaterialInterfacePanel::~pqYoungsMaterialInterfacePanel()
{
}

//-----------------------------------------------------------------------------
void pqYoungsMaterialInterfacePanel::updateOptions()
{
  if (this->BlockUpdateOptions)
    {
    return;
    }

  // determine the volume fraction array currently selected.
  QTreeWidgetItem* currentItem = this->VolumeFractionArrays->currentItem();
  if (!currentItem)
    {
    this->OrderingArrays->setEnabled(false);
    this->NormalArrays->setEnabled(false);
    return;
    }

  this->OrderingArrays->setEnabled(true);
  this->NormalArrays->setEnabled(true);

  QString label = currentItem->text(0);
  
  // check if there's a normal and ordering array already defined for this
  // volume-fraction array. If so, show it.
  const char* ordering_array = vtkSMUncheckedPropertyHelper(
    this->proxy(), "OrderingArrays").GetStatus(label.toLatin1().data(), "");

  const char* normal_array = vtkSMUncheckedPropertyHelper(
    this->proxy(), "NormalArrays").GetStatus(label.toLatin1().data(), "");

  if (ordering_array == NULL || strlen(ordering_array) == 0)
    {
    ordering_array = "None";
    }
  if (normal_array == NULL || strlen(normal_array) == 0)
    {
    normal_array = "None";
    }

  bool prev = this->OrderingArrays->blockSignals(true);
  this->OrderingArrays->setCurrentIndex(
    this->OrderingArrays->findText(ordering_array));
  this->OrderingArrays->blockSignals(prev);

  prev = this->NormalArrays->blockSignals(true);
  this->NormalArrays->setCurrentIndex(
    this->NormalArrays->findText(normal_array));
  this->NormalArrays->blockSignals(prev);
}

//-----------------------------------------------------------------------------
void pqYoungsMaterialInterfacePanel::orderingArraysChanged(const QString& text)
{
  QTreeWidgetItem* currentItem = this->VolumeFractionArrays->currentItem();
  if (currentItem)
    {
    QString label = currentItem->text(0);
    vtkSMUncheckedPropertyHelper(this->proxy(), "OrderingArrays").SetStatus(
      label.toLatin1().data(),
      text == "None"? "" : text.toLatin1().data());
    this->setModified();
    }
}

//-----------------------------------------------------------------------------
void pqYoungsMaterialInterfacePanel::normalArraysChanged(const QString& text)
{
  QTreeWidgetItem* currentItem = this->VolumeFractionArrays->currentItem();
  if (currentItem)
    {
    QString label = currentItem->text(0);
    vtkSMUncheckedPropertyHelper(this->proxy(), "NormalArrays").SetStatus(
      label.toLatin1().data(),
      text == "None"? "" : text.toLatin1().data());
    this->setModified();
    }
}

//-----------------------------------------------------------------------------
void pqYoungsMaterialInterfacePanel::accept()
{
  this->BlockUpdateOptions = true;
  vtkSMProxy* smproxy = this->proxy();
  pqSMAdaptor::setMultipleElementProperty(
    smproxy->GetProperty("OrderingArrays"),
    pqSMAdaptor::getMultipleElementProperty(
      smproxy->GetProperty("OrderingArrays"), pqSMAdaptor::UNCHECKED),
    pqSMAdaptor::CHECKED);
  pqSMAdaptor::setMultipleElementProperty(
    smproxy->GetProperty("NormalArrays"),
    pqSMAdaptor::getMultipleElementProperty(
      smproxy->GetProperty("NormalArrays"), pqSMAdaptor::UNCHECKED),
    pqSMAdaptor::CHECKED);
  this->Superclass::accept();
  this->BlockUpdateOptions = false;
  this->updateOptions();
}

//-----------------------------------------------------------------------------
void pqYoungsMaterialInterfacePanel::reset()
{
  this->BlockUpdateOptions = true;
  vtkSMStringVectorProperty::SafeDownCast(
    this->proxy()->GetProperty("OrderingArrays"))->ClearUncheckedElements();
  vtkSMStringVectorProperty::SafeDownCast(
    this->proxy()->GetProperty("NormalArrays"))->ClearUncheckedElements();

  this->Superclass::reset();
  this->BlockUpdateOptions = false;
  this->updateOptions();
}
