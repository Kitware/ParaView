/*=========================================================================

   Program: ParaView
   Module:    pqDisplayRepresentationWidget.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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
#include "pqDisplayRepresentationWidget.h"
#include "ui_pqDisplayRepresentationWidget.h"

#include "vtkSMIntVectorProperty.h"

#include<QPointer>

#include "pqServerManagerModel.h"
#include "pqApplicationCore.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"

class pqDisplayRepresentationWidgetInternal : 
  public Ui::displayRepresentationWidget
{
public:
  QPointer<pqPipelineRepresentation> Display;
  pqPropertyLinks Links;
  pqSignalAdaptorComboBox* Adaptor;
};

//-----------------------------------------------------------------------------
pqDisplayRepresentationWidget::pqDisplayRepresentationWidget(
  QWidget* _p): QWidget(_p)
{
  this->Internal = new pqDisplayRepresentationWidgetInternal;
  this->Internal->setupUi(this);
  this->Internal->Links.setUseUncheckedProperties(true);

  this->Internal->Adaptor = new pqSignalAdaptorComboBox(
    this->Internal->comboBox);
  this->Internal->Adaptor->setObjectName("adaptor");

  QObject::connect(this->Internal->Adaptor, 
    SIGNAL(currentTextChanged(const QString&)),
    this, SLOT(onCurrentTextChanged(const QString&)));

  QObject::connect(this->Internal->Adaptor, 
    SIGNAL(currentTextChanged(const QString&)),
    this, SIGNAL(currentTextChanged(const QString&)));

  QObject::connect(&this->Internal->Links,
    SIGNAL(qtWidgetChanged()),
    this, SLOT(onQtWidgetChanged()));

  this->updateLinks();
}

//-----------------------------------------------------------------------------
pqDisplayRepresentationWidget::~pqDisplayRepresentationWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::setRepresentation(pqDataRepresentation* display)
{
  if(!display || display != this->Internal->Display)
    {
    this->Internal->Display = qobject_cast<pqPipelineRepresentation*>(display);
    this->updateLinks();
    }
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::updateLinks()
{
  // break old links.
  this->Internal->Links.removeAllPropertyLinks();

  this->Internal->comboBox->setEnabled(this->Internal->Display != 0);
  this->Internal->comboBox->blockSignals(true);
  this->Internal->comboBox->clear();
  if (!this->Internal->Display)
    {
    this->Internal->comboBox->addItem("Representation");
    this->Internal->comboBox->blockSignals(false);
    return;
    }

  vtkSMProxy* displayProxy = this->Internal->Display->getProxy();
  vtkSMProperty* repProperty =
      this->Internal->Display->getProxy()->GetProperty("Representation");
  if (repProperty)
    {
    repProperty->UpdateDependentDomains();
    QList<QVariant> items = 
      pqSMAdaptor::getEnumerationPropertyDomain(repProperty);
    foreach(QVariant item, items)
      {
      this->Internal->comboBox->addItem(item.toString());
      }

    this->Internal->Links.addPropertyLink(
      this->Internal->Adaptor, "currentText",
      SIGNAL(currentTextChanged(const QString&)),
      displayProxy, repProperty);
    this->Internal->comboBox->setEnabled(true);
    }
  else
    {
    this->Internal->comboBox->setEnabled(false);
    }

  this->Internal->comboBox->blockSignals(false);

}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::reloadGUI()
{
  this->updateLinks();
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::onQtWidgetChanged()
{
  if ( !this->Internal->Display )
    {
    //
    return;
    }
  BEGIN_UNDO_SET("Changed 'Representation'");
  QString text = this->Internal->Adaptor->currentText();

  vtkSMProperty* repProperty =
      this->Internal->Display->getProxy()->GetProperty("Representation");
  QList<QVariant> domainStrings = 
    pqSMAdaptor::getEnumerationPropertyDomain(repProperty);

  int index = domainStrings.indexOf(text);
  if (index != -1)
    {
    this->Internal->Display->setRepresentation(text);
    this->Internal->Links.blockSignals(true);
    //this->Internal->Links.accept();
    this->Internal->Links.blockSignals(false);
    }
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::onCurrentTextChanged(const QString&)
{
  if (this->Internal->Display)
    {
    this->Internal->Display->renderViewEventually();
    }
}

//-----------------------------------------------------------------------------
pqDisplayRepresentationPropertyWidget::pqDisplayRepresentationPropertyWidget(
  vtkSMProxy *proxy, QWidget *parent)
  : pqPropertyWidget(proxy, parent)
{
  QVBoxLayout *layout = new QVBoxLayout;
  layout->setMargin(0);
  this->Widget = new pqDisplayRepresentationWidget;
  layout->addWidget(this->Widget);
  setLayout(layout);

  pqServerManagerModel *smm = pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineRepresentation *repr = smm->findItem<pqPipelineRepresentation *>(proxy);
  if(repr)
    {
    this->Widget->setRepresentation(repr);
    }
}

//-----------------------------------------------------------------------------
pqDisplayRepresentationPropertyWidget::~pqDisplayRepresentationPropertyWidget()
{
}
