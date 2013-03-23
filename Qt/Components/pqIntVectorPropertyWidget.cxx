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

#include "vtkSMDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMBooleanDomain.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMEnumerationDomain.h"

#include "pqTreeWidget.h"
#include "pqIntRangeWidget.h"
#include "pqSignalAdaptors.h"
#include "pqTreeWidgetSelectionHelper.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLineEdit>

pqIntVectorPropertyWidget::pqIntVectorPropertyWidget(vtkSMProperty *smproperty,
                                                     vtkSMProxy *smProxy,
                                                     QWidget *parentObject)
  : pqPropertyWidget(smProxy, parentObject)
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(smproperty);
  if(!ivp)
    {
    return;
    }
  
  // find the domain
  vtkSMIntRangeDomain *defaultDomain = NULL;

  vtkSMDomain *domain = 0;
  vtkSMDomainIterator *domainIter = ivp->NewDomainIterator();
  for(domainIter->Begin(); !domainIter->IsAtEnd(); domainIter->Next())
    {
    domain = domainIter->GetDomain();
    }
  domainIter->Delete();

  if(!domain)
    {
    defaultDomain = vtkSMIntRangeDomain::New();
    domain = defaultDomain;
    }

  QHBoxLayout *layoutLocal = new QHBoxLayout;
  layoutLocal->setMargin(0);

  if(vtkSMBooleanDomain::SafeDownCast(domain))
    {
    QCheckBox *checkBox = new QCheckBox(smproperty->GetXMLLabel(), this);
    checkBox->setObjectName("CheckBox");
    this->addPropertyLink(checkBox, "checked", SIGNAL(toggled(bool)), ivp);
    this->connect(checkBox, SIGNAL(toggled(bool)),
                  this, SIGNAL(editingFinished()));
    layoutLocal->addWidget(checkBox);

    this->setShowLabel(false);

    this->setReason() << "QCheckBox for an IntVectorProperty with a BooleanDomain";
    }
  else if(vtkSMEnumerationDomain *ed = vtkSMEnumerationDomain::SafeDownCast(domain))
    {
    QComboBox *comboBox = new QComboBox(this);
    comboBox->setObjectName("ComboBox");

    for(unsigned int i = 0; i < ed->GetNumberOfEntries(); i++)
      {
      comboBox->addItem(ed->GetEntryText(i));
      }

    pqSignalAdaptorComboBox *adaptor = new pqSignalAdaptorComboBox(comboBox);

    this->addPropertyLink(adaptor,
                          "currentText",
                          SIGNAL(currentTextChanged(QString)),
                          smproperty);

    this->connect(adaptor, SIGNAL(currentTextChanged(QString)),
                  this, SIGNAL(editingFinished()));

    layoutLocal->addWidget(comboBox);

    this->setReason() << "QComboBox for an IntVectorProperty with a "
                      << "EnumerationDomain (" << pqPropertyWidget::getXMLName(ed) << ")";
    }
  else if(vtkSMCompositeTreeDomain::SafeDownCast(domain))
    {
    pqTreeWidget *treeWidget = new pqTreeWidget(this);

    treeWidget->setObjectName("TreeWidget");
    treeWidget->setHeaderLabel(smproperty->GetXMLLabel());

    pqSignalAdaptorCompositeTreeWidget *adaptor =
      new pqSignalAdaptorCompositeTreeWidget(treeWidget, ivp);
    adaptor->setObjectName("CompositeTreeAdaptor");

    pqTreeWidgetSelectionHelper* helper =
      new pqTreeWidgetSelectionHelper(treeWidget);
    helper->setObjectName("CompositeTreeSelectionHelper");

    this->addPropertyLink(adaptor, "values", SIGNAL(valuesChanged()), ivp);

    this->connect(adaptor, SIGNAL(valuesChanged()),
                  this, SIGNAL(editingFinished()));

    layoutLocal->addWidget(treeWidget);
    this->setShowLabel(false);

    this->setReason() << "pqTreeWidget for an IntVectorPropertyWidget with a "
                      << "CompositeTreeDomain";
    }
  else if(vtkSMIntRangeDomain *range = vtkSMIntRangeDomain::SafeDownCast(domain))
    {
    int elementCount = ivp->GetNumberOfElements();

    if(elementCount == 1 &&
       range->GetMinimumExists(0) &&
       range->GetMaximumExists(0))
      {
      // slider + spin box
      pqIntRangeWidget *widget = new pqIntRangeWidget(this);
      widget->setObjectName("IntRangeWidget");
      widget->setMinimum(range->GetMinimum(0));
      widget->setMaximum(range->GetMaximum(0));
      widget->setDomain(range);
      this->addPropertyLink(widget, "value", SIGNAL(valueChanged(int)), ivp);
      this->connect(widget, SIGNAL(valueChanged(int)),
                    this, SIGNAL(editingFinished()));
      layoutLocal->addWidget(widget);

      this->setReason() << "pqIntRangeWidget for an IntVectorProperty with a "
                        << "single element and an "
                        << "IntRangeDomain (" << pqPropertyWidget::getXMLName(range) << ") "
                        << "with a minimum and a maximum";
      }
    else
      {
      if(elementCount == 6)
        {
        QGridLayout *gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(0);
        gridLayout->setVerticalSpacing(2);

        for(int i = 0; i < 3; i++)
          {
          QLineEdit *lineEdit = new QLineEdit(this);
          lineEdit->setValidator(new QIntValidator(lineEdit));
          lineEdit->setObjectName("LineEdit" + QString::number(i*2+0));
          lineEdit->setText(QString::number(vtkSMPropertyHelper(smproperty).GetAsInt(i*2+0)));
          gridLayout->addWidget(lineEdit, i, 0);
          this->addPropertyLink(lineEdit, "text", SIGNAL(textChanged(QString)), ivp, i*2+0);
          this->connect(lineEdit, SIGNAL(editingFinished()),
                        this, SIGNAL(editingFinished()));

          lineEdit = new QLineEdit(this);
          lineEdit->setValidator(new QIntValidator(lineEdit));
          lineEdit->setObjectName("LineEdit" + QString::number(i*2+1));
          lineEdit->setText(QString::number(vtkSMPropertyHelper(smproperty).GetAsInt(i*2+1)));
          gridLayout->addWidget(lineEdit, i, 1);
          this->addPropertyLink(lineEdit, "text", SIGNAL(textChanged(QString)), ivp, i*2+1);
          this->connect(lineEdit, SIGNAL(editingFinished()),
                        this, SIGNAL(editingFinished()));
          }

        layoutLocal->addLayout(gridLayout);

        this->setReason() << "3x2 grid of QLineEdit's for an IntVectorProperty "
                          << "with an "
                          << "IntRangeDomain (" << pqPropertyWidget::getXMLName(range) << ") "
                          << "and 6 elements";
        }
      else
        {
        for(unsigned int i = 0; i < ivp->GetNumberOfElements(); i++)
          {
          QLineEdit *lineEdit = new QLineEdit(this);
          lineEdit->setValidator(new QIntValidator(lineEdit));
          lineEdit->setObjectName("LineEdit" + QString::number(i));
          lineEdit->setText(QString::number(vtkSMPropertyHelper(smproperty).GetAsInt(i)));
          layoutLocal->addWidget(lineEdit);
          this->addPropertyLink(lineEdit, "text", SIGNAL(textChanged(QString)), ivp, i);
          this->connect(lineEdit, SIGNAL(editingFinished()),
                        this, SIGNAL(editingFinished()));
          }

        this->setReason() << "List of QLineEdit's for an IntVectorProperty "
                          << "with an "
                          << "IntRangeDomain (" << pqPropertyWidget::getXMLName(range) << ") "
                          << "and more than one element";
        }
      }
    }

  this->setLayout(layoutLocal);

  if (defaultDomain)
    {
    defaultDomain->Delete();
    }
}
