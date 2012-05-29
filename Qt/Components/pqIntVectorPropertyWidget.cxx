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
#include "pqSignalAdaptorCompositeTreeWidget.h"

#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>

pqIntVectorPropertyWidget::pqIntVectorPropertyWidget(vtkSMProperty *smproperty,
                                                     vtkSMProxy *proxy,
                                                     QWidget *parent)
  : pqPropertyWidget(proxy, parent)
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(smproperty);
  if(!ivp)
    {
    return;
    }
  
  // find the domain
  vtkSMDomain *domain = 0;
  vtkSMDomainIterator *domainIter = ivp->NewDomainIterator();
  for(domainIter->Begin(); !domainIter->IsAtEnd(); domainIter->Next())
    {
    domain = domainIter->GetDomain();
    }
  domainIter->Delete();

  QHBoxLayout *layout = new QHBoxLayout;
  layout->setMargin(0);

  if(vtkSMBooleanDomain::SafeDownCast(domain))
    {
    QCheckBox *checkBox = new QCheckBox(QString(), this);
    checkBox->setObjectName("CheckBox");
    this->addPropertyLink(checkBox, "checked", SIGNAL(toggled(bool)), ivp);
    layout->addWidget(checkBox);
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

    layout->addWidget(comboBox);
    }
  else if(vtkSMCompositeTreeDomain::SafeDownCast(domain))
    {
    pqTreeWidget *treeWidget = new pqTreeWidget(this);

    treeWidget->setObjectName("TreeWidget");
    treeWidget->setHeaderLabel(smproperty->GetXMLLabel());

    pqSignalAdaptorCompositeTreeWidget *adaptor =
      new pqSignalAdaptorCompositeTreeWidget(treeWidget, ivp);

    this->addPropertyLink(adaptor, "values", SIGNAL(valuesChanged()), ivp);

    layout->addWidget(treeWidget);
    this->setShowLabel(false);
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
      this->addPropertyLink(widget, "value", SIGNAL(valueChanged(int)), ivp);
      layout->addWidget(widget);
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
          lineEdit->setObjectName("LineEdit" + QString::number(i*2+0));
          lineEdit->setText(QString::number(vtkSMPropertyHelper(smproperty).GetAsInt(i*2+0)));
          gridLayout->addWidget(lineEdit, i, 0);
          this->addPropertyLink(lineEdit, "text", SIGNAL(textChanged(QString)), ivp, i*2+0);

          lineEdit = new QLineEdit(this);
          lineEdit->setObjectName("LineEdit" + QString::number(i*2+1));
          lineEdit->setText(QString::number(vtkSMPropertyHelper(smproperty).GetAsInt(i*2+1)));
          gridLayout->addWidget(lineEdit, i, 1);
          this->addPropertyLink(lineEdit, "text", SIGNAL(textChanged(QString)), ivp, i*2+1);
          }

        layout->addLayout(gridLayout);
        }
      else
        {
        for(int i = 0; i < ivp->GetNumberOfElements(); i++)
          {
          QLineEdit *lineEdit = new QLineEdit;
          lineEdit->setObjectName("LineEdit" + QString::number(i));
          lineEdit->setText(QString::number(vtkSMPropertyHelper(smproperty).GetAsInt(i)));
          layout->addWidget(lineEdit);
          this->addPropertyLink(lineEdit, "text", SIGNAL(textChanged(QString)), ivp, i);
          }
        }
      }
    }

  this->setLayout(layout);
}
