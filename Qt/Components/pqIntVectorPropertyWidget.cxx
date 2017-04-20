/*=========================================================================

   Program: ParaView
   Module: pqIntVectorPropertyWidget.cxx

   Copyright (c) 2005-2012 Sandia Corporation, Kitware Inc.
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

#include "pqIntVectorPropertyWidget.h"

#include "vtkSMBooleanDomain.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMNumberOfComponentsDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSmartPointer.h"

#include "pqComboBoxDomain.h"
#include "pqCompositeTreePropertyWidget.h"
#include "pqIntRangeWidget.h"
#include "pqLabel.h"
#include "pqLineEdit.h"
#include "pqProxyWidget.h"
#include "pqScalarValueListPropertyWidget.h"
#include "pqSignalAdaptorSelectionTreeWidget.h"
#include "pqSignalAdaptors.h"
#include "pqTreeView.h"
#include "pqTreeWidget.h"
#include "pqTreeWidgetSelectionHelper.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QIntValidator>

//-----------------------------------------------------------------------------
pqIntVectorPropertyWidget::pqIntVectorPropertyWidget(
  vtkSMProperty* smproperty, vtkSMProxy* smProxy, QWidget* parentObject)
  : pqPropertyWidget(smProxy, parentObject)
{
  this->setChangeAvailableAsChangeFinished(false);

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(smproperty);
  if (!ivp)
  {
    return;
  }

  bool useDocumentationForLabels = pqProxyWidget::useDocumentationForLabels(smProxy);

  // find the domain
  vtkSmartPointer<vtkSMDomain> domain;
  vtkSMDomainIterator* domainIter = ivp->NewDomainIterator();
  for (domainIter->Begin(); !domainIter->IsAtEnd(); domainIter->Next())
  {
    domain = domainIter->GetDomain();
  }
  domainIter->Delete();

  if (!domain)
  {
    domain = vtkSmartPointer<vtkSMIntRangeDomain>::New();
  }

  QHBoxLayout* layoutLocal = new QHBoxLayout;
  layoutLocal->setMargin(0);

  if (vtkSMBooleanDomain::SafeDownCast(domain))
  {
    QCheckBox* checkBox =
      new QCheckBox(useDocumentationForLabels ? "" : smproperty->GetXMLLabel(), this);
    checkBox->setObjectName("CheckBox");
    this->addPropertyLink(checkBox, "checked", SIGNAL(toggled(bool)), ivp);
    this->setChangeAvailableAsChangeFinished(true);
    layoutLocal->addWidget(checkBox);

    if (useDocumentationForLabels)
    {
      pqLabel* label = new pqLabel(QString("<p><b>%1</b>: %2</p>")
                                     .arg(smproperty->GetXMLLabel())
                                     .arg(pqProxyWidget::documentationText(smproperty)));
      label->setObjectName("CheckBoxLabel");
      label->setWordWrap(true);
      label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
      label->connect(label, SIGNAL(clicked()), checkBox, SLOT(click()));
      layoutLocal->addWidget(label, /*stretch=*/1);
    }
    this->setShowLabel(false);

    PV_DEBUG_PANELS() << "QCheckBox for an IntVectorProperty with a BooleanDomain";
  }
  else if (vtkSMEnumerationDomain* ed = vtkSMEnumerationDomain::SafeDownCast(domain))
  {
    if (vtkSMVectorProperty::SafeDownCast(smproperty)->GetRepeatCommand())
    {
      // Some enumeration properties, we add
      // ability to select mutliple elements. This is the case if the
      // SM property has repeat command flag set.
      pqTreeWidget* treeWidget = new pqTreeWidget(this);
      treeWidget->setObjectName("TreeWidget");
      treeWidget->setColumnCount(1);
      treeWidget->setRootIsDecorated(false);
      treeWidget->setMaximumRowCountBeforeScrolling(smproperty);

      QTreeWidgetItem* header = new QTreeWidgetItem();
      header->setData(0, Qt::DisplayRole, smproperty->GetXMLLabel());
      treeWidget->setHeaderItem(header);

      // helper makes it easier to select multiple entries.
      pqTreeWidgetSelectionHelper* helper = new pqTreeWidgetSelectionHelper(treeWidget);
      helper->setObjectName("TreeWidgetSelectionHelper");

      // adaptor makes it possible to link with the smproperty.
      pqSignalAdaptorSelectionTreeWidget* adaptor =
        new pqSignalAdaptorSelectionTreeWidget(treeWidget, smproperty);
      adaptor->setObjectName("SelectionTreeWidgetAdaptor");
      this->addPropertyLink(adaptor, "values", SIGNAL(valuesChanged()), smproperty);

      layoutLocal->addWidget(treeWidget);
      this->setChangeAvailableAsChangeFinished(true);
      this->setShowLabel(false);

      PV_DEBUG_PANELS() << "TreeWidget for an IntVectorProperty with a "
                        << "EnumerationDomain (" << pqPropertyWidget::getXMLName(ed) << ")"
                        << "(with repeatable command)";
    }
    else
    {
      QComboBox* comboBox = new QComboBox(this);
      comboBox->setObjectName("ComboBox");
      for (unsigned int i = 0; i < ed->GetNumberOfEntries(); i++)
      {
        comboBox->addItem(ed->GetEntryText(i));
      }
      // vtkSMNumberOfComponentsDomain is a dynamic domain
      // hence we need to connect it to a pqComboBoxDomain
      // so the combobox will stay updated.
      if (vtkSMNumberOfComponentsDomain::SafeDownCast(domain))
      {
        new pqComboBoxDomain(comboBox, smproperty, "comps");
      }

      pqSignalAdaptorComboBox* adaptor = new pqSignalAdaptorComboBox(comboBox);
      this->addPropertyLink(
        adaptor, "currentText", SIGNAL(currentTextChanged(QString)), smproperty);
      this->setChangeAvailableAsChangeFinished(true);
      layoutLocal->addWidget(comboBox);

      PV_DEBUG_PANELS() << "QComboBox for an IntVectorProperty with a "
                        << "EnumerationDomain (" << pqPropertyWidget::getXMLName(ed) << ")"
                        << "(with non-repeatable command)";
    }
  }
  else if (vtkSMCompositeTreeDomain::SafeDownCast(domain))
  {
    // Should have been handled by pqCompositeTreePropertyWidget.
    abort();
  }
  else if (vtkSMIntRangeDomain* range = vtkSMIntRangeDomain::SafeDownCast(domain))
  {
    int elementCount = ivp->GetNumberOfElements();

    if (ivp->GetRepeatable())
    {
      pqScalarValueListPropertyWidget* widget =
        new pqScalarValueListPropertyWidget(smproperty, smProxy, this);
      widget->setObjectName("ScalarValueList");
      widget->setRangeDomain(range);
      this->addPropertyLink(widget, "scalars", SIGNAL(scalarsChanged()), smproperty);

      this->setChangeAvailableAsChangeFinished(true);
      layoutLocal->addWidget(widget);
      this->setShowLabel(false);

      PV_DEBUG_PANELS() << "pqScalarValueListPropertyWidget for IntVectorProperty "
                        << "that is repeatable.";
    }
    else if (elementCount == 1 && range->GetMinimumExists(0) && range->GetMaximumExists(0))
    {
      // slider + spin box
      pqIntRangeWidget* widget = new pqIntRangeWidget(this);
      widget->setObjectName("IntRangeWidget");
      widget->setMinimum(range->GetMinimum(0));
      widget->setMaximum(range->GetMaximum(0));
      widget->setDomain(range);
      this->addPropertyLink(widget, "value", SIGNAL(valueChanged(int)), ivp);
      this->connect(widget, SIGNAL(valueEdited(int)), this, SIGNAL(changeFinished()));
      layoutLocal->addWidget(widget);

      PV_DEBUG_PANELS() << "pqIntRangeWidget for an IntVectorProperty with a "
                        << "single element and an "
                        << "IntRangeDomain (" << pqPropertyWidget::getXMLName(range) << ") "
                        << "with a minimum and a maximum";
    }
    else
    {
      if (elementCount == 6)
      {
        QGridLayout* gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(0);
        gridLayout->setVerticalSpacing(2);

        for (int i = 0; i < 3; i++)
        {
          pqLineEdit* lineEdit = new pqLineEdit(this);
          lineEdit->setValidator(new QIntValidator(lineEdit));
          lineEdit->setObjectName("LineEdit" + QString::number(i * 2 + 0));
          lineEdit->setTextAndResetCursor(
            QString::number(vtkSMPropertyHelper(smproperty).GetAsInt(i * 2 + 0)));
          gridLayout->addWidget(lineEdit, i, 0);
          this->addPropertyLink(lineEdit, "text2", SIGNAL(textChanged(QString)), ivp, i * 2 + 0);
          this->connect(
            lineEdit, SIGNAL(textChangedAndEditingFinished()), this, SIGNAL(changeFinished()));

          lineEdit = new pqLineEdit(this);
          lineEdit->setValidator(new QIntValidator(lineEdit));
          lineEdit->setObjectName("LineEdit" + QString::number(i * 2 + 1));
          lineEdit->setTextAndResetCursor(
            QString::number(vtkSMPropertyHelper(smproperty).GetAsInt(i * 2 + 1)));
          gridLayout->addWidget(lineEdit, i, 1);
          this->addPropertyLink(lineEdit, "text2", SIGNAL(textChanged(QString)), ivp, i * 2 + 1);
          this->connect(
            lineEdit, SIGNAL(textChangedAndEditingFinished()), this, SIGNAL(changeFinished()));
        }

        layoutLocal->addLayout(gridLayout);

        PV_DEBUG_PANELS() << "3x2 grid of QLineEdit's for an IntVectorProperty "
                          << "with an "
                          << "IntRangeDomain (" << pqPropertyWidget::getXMLName(range) << ") "
                          << "and 6 elements";
      }
      else
      {
        for (unsigned int i = 0; i < ivp->GetNumberOfElements(); i++)
        {
          pqLineEdit* lineEdit = new pqLineEdit(this);
          lineEdit->setValidator(new QIntValidator(lineEdit));
          lineEdit->setObjectName("LineEdit" + QString::number(i));
          lineEdit->setTextAndResetCursor(
            QString::number(vtkSMPropertyHelper(smproperty).GetAsInt(i)));
          layoutLocal->addWidget(lineEdit);
          this->addPropertyLink(lineEdit, "text2", SIGNAL(textChanged(QString)), ivp, i);
          this->connect(
            lineEdit, SIGNAL(textChangedAndEditingFinished()), this, SIGNAL(changeFinished()));
        }

        PV_DEBUG_PANELS() << "List of QLineEdit's for an IntVectorProperty "
                          << "with an "
                          << "IntRangeDomain (" << pqPropertyWidget::getXMLName(range) << ") "
                          << "and more than one element";
      }
    }
  }

  this->setLayout(layoutLocal);
}

//-----------------------------------------------------------------------------
pqIntVectorPropertyWidget::~pqIntVectorPropertyWidget()
{
}

//-----------------------------------------------------------------------------
pqPropertyWidget* pqIntVectorPropertyWidget::createWidget(
  vtkSMIntVectorProperty* smproperty, vtkSMProxy* smproxy, QWidget* parent)
{
  if (smproperty != nullptr && smproperty->FindDomain("vtkSMCompositeTreeDomain") != nullptr)
  {
    return new pqCompositeTreePropertyWidget(smproperty, smproxy, parent);
  }

  return new pqIntVectorPropertyWidget(smproperty, smproxy, parent);
}
