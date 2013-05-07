/*=========================================================================

   Program: ParaView
   Module: pqDoubleVectorPropertyWidget.cxx

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

#include "pqDoubleVectorPropertyWidget.h"

#include "vtkSMArrayRangeDomain.h"
#include "vtkSMBoundsDomain.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include "pqSignalAdaptors.h"
#include "pqDoubleRangeWidget.h"
#include "pqScalarValueListPropertyWidget.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include "pqLineEdit.h"

pqDoubleVectorPropertyWidget::pqDoubleVectorPropertyWidget(vtkSMProperty *smProperty,
                                                           vtkSMProxy *smProxy,
                                                           QWidget *parentObject)
  : pqPropertyWidget(smProxy, parentObject)
{
  this->setChangeAvailableAsChangeFinished(false);

  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(smProperty);
  if(!dvp)
    {
    return;
    }
  
  // find the domain
  vtkSMDoubleRangeDomain *defaultDomain = NULL;

  vtkSMDomain *domain = 0;
  vtkSMDomainIterator *domainIter = dvp->NewDomainIterator();
  for(domainIter->Begin(); !domainIter->IsAtEnd(); domainIter->Next())
    {
    domain = domainIter->GetDomain();
    }
  domainIter->Delete();

  if(!domain)
    {
    defaultDomain = vtkSMDoubleRangeDomain::New();
    domain = defaultDomain;
    }

  QHBoxLayout *layoutLocal = new QHBoxLayout;
  layoutLocal->setMargin(0);
  layoutLocal->setSpacing(0);

  if(vtkSMDoubleRangeDomain *range = vtkSMDoubleRangeDomain::SafeDownCast(domain))
    {
    if((vtkSMBoundsDomain::SafeDownCast(range) ||
      vtkSMArrayRangeDomain::SafeDownCast(range))
      && smProperty->GetRepeatable())
      {
      pqScalarValueListPropertyWidget *widget =
        new pqScalarValueListPropertyWidget(smProperty, smProxy, this);
      widget->setObjectName("ScalarValueList");
      widget->setRangeDomain(range);
      this->addPropertyLink(widget, "scalars", SIGNAL(scalarsChanged()), smProperty);

      this->setChangeAvailableAsChangeFinished(true);
      layoutLocal->addWidget(widget);
      this->setShowLabel(false);

      PV_DEBUG_PANELS() << "pqScalarValueListPropertyWidget for a repeatable "
                    << "DoubleVectorProperty with a BoundsDomain ("
                    << pqPropertyWidget::getXMLName(range) << ") ";
      }
    else if(dvp->GetNumberOfElements() == 1 &&
            range->GetMinimumExists(0) &&
            range->GetMaximumExists(0))
      {
      // bounded ranges are represented with a slider and a spin box
      pqDoubleRangeWidget *widget = new pqDoubleRangeWidget(this);
      widget->setObjectName("DoubleRangeWidget");
      widget->setMinimum(range->GetMinimum(0));
      widget->setMaximum(range->GetMaximum(0));

      this->addPropertyLink(widget, "value", SIGNAL(valueChanged(double)), smProperty);
      this->connect(widget, SIGNAL(valueEdited(double)),
                    this, SIGNAL(changeFinished()));

      layoutLocal->setSpacing(4);
      layoutLocal->addWidget(widget);

      PV_DEBUG_PANELS() << "pqDoubleRangeWidget for an DoubleVectorProperty "
                    << "with a single element and a "
                    << "DoubleRangeDomain (" << pqPropertyWidget::getXMLName(range) << ") "
                    << "with a minimum and a maximum";
      }
    else
      {
      // unbounded ranges are represented with a line edit
      int elementCount = dvp->GetNumberOfElements();

      if(elementCount == 6)
        {
        QGridLayout *gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(0);
        gridLayout->setVerticalSpacing(2);

        for(int i = 0; i < 3; i++)
          {
          pqLineEdit *lineEdit = new pqLineEdit(this);
          lineEdit->setValidator(new QDoubleValidator(lineEdit));
          lineEdit->setObjectName(QString("LineEdit%1").arg(i));
          lineEdit->setTextAndResetCursor(
            QVariant(vtkSMPropertyHelper(smProperty).GetAsDouble(i)).toString());
          gridLayout->addWidget(lineEdit, i, 0);
          this->addPropertyLink(lineEdit, "text2",
                                SIGNAL(textChanged(const QString&)), dvp, i);
          this->connect(lineEdit, SIGNAL(textChangedAndEditingFinished()),
                        this, SIGNAL(changeFinished()));

          lineEdit = new pqLineEdit(this);
          lineEdit->setValidator(new QDoubleValidator(lineEdit));
          lineEdit->setObjectName(QString("LineEdit%1").arg(i+3));
          lineEdit->setTextAndResetCursor(
            QVariant(vtkSMPropertyHelper(smProperty).GetAsDouble(i + 3)).toString());
          gridLayout->addWidget(lineEdit, i, 1);
          this->addPropertyLink(lineEdit, "text2",
                                SIGNAL(textChanged(const QString&)), dvp, i + 3);
          this->connect(lineEdit, SIGNAL(textChangedAndEditingFinished()),
                        this, SIGNAL(changeFinished()));
          }

        layoutLocal->addLayout(gridLayout);

        PV_DEBUG_PANELS() << "3x2 grid of QLineEdit's for an DoubleVectorProperty "
                      << "with an "
                      << "DoubleRangeDomain (" << pqPropertyWidget::getXMLName(range) << ") "
                      << "and 6 elements";
        }
      else
        {
        for(unsigned int i = 0; i < dvp->GetNumberOfElements(); i++)
          {
          pqLineEdit *lineEdit = new pqLineEdit(this);
          lineEdit->setValidator(new QDoubleValidator(lineEdit));
          lineEdit->setObjectName(QString("LineEdit%1").arg(i));
          lineEdit->setTextAndResetCursor(
            QVariant(vtkSMPropertyHelper(smProperty).GetAsDouble(i)).toString());
          layoutLocal->addWidget(lineEdit);
          this->addPropertyLink(lineEdit, "text2",
                                SIGNAL(textChanged(const QString&)), dvp, i);
          this->connect(lineEdit, SIGNAL(textChangedAndEditingFinished()),
                        this, SIGNAL(changeFinished()));
          }

        PV_DEBUG_PANELS() << "List of QLineEdit's for an DoubleVectorProperty "
                      << "with an "
                      << "DoubleRangeDomain (" << pqPropertyWidget::getXMLName(range) << ") "
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
