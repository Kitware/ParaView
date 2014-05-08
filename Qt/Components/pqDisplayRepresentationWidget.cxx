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

#include "pqComboBoxDomain.h"
#include "pqDataRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqUndoStack.h"
#include "vtkSMRepresentationProxy.h"

#include <QPointer>

//=============================================================================
class pqDisplayRepresentationWidget::PropertyLinksConnection : public pqPropertyLinksConnection
{
  typedef pqPropertyLinksConnection Superclass;
public:
  PropertyLinksConnection(
    QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex,
    bool use_unchecked_modified_event,
    QObject* parentObject=0)
    : Superclass(qobject, qproperty, qsignal,
      smproxy, smproperty, smindex, use_unchecked_modified_event,
      parentObject)
    {
    }
  virtual ~PropertyLinksConnection()
    {
    }

protected:
  /// Called to update the ServerManager Property due to UI change.
  virtual void setServerManagerValue(bool use_unchecked, const QVariant& value)
    {
    Q_ASSERT(use_unchecked == false);

    BEGIN_UNDO_SET("Change representation type");
    vtkSMProxy* reprProxy = this->proxySM();
    vtkSMRepresentationProxy::SetRepresentationType(
      reprProxy, value.toString().toLatin1().data());
    END_UNDO_SET();
    }
private:
  Q_DISABLE_COPY(PropertyLinksConnection);
};


//=============================================================================
class pqDisplayRepresentationWidget::pqInternals :
  public Ui::displayRepresentationWidget
{
public:
  pqPropertyLinks Links;
  QPointer<pqComboBoxDomain> Domain;
  QPointer<pqDataRepresentation> PQRepr;
};

//-----------------------------------------------------------------------------
pqDisplayRepresentationWidget::pqDisplayRepresentationWidget(
  QWidget* _p): Superclass(_p)
{
  this->Internal = new pqDisplayRepresentationWidget::pqInternals();
  this->Internal->setupUi(this);
  this->connect(this->Internal->comboBox,
    SIGNAL(currentIndexChanged(const QString&)),
    SIGNAL(representationTextChanged(const QString&)));
}

//-----------------------------------------------------------------------------
pqDisplayRepresentationWidget::~pqDisplayRepresentationWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::setRepresentation(pqDataRepresentation* display)
{
  if (this->Internal->PQRepr)
    {
    this->Internal->PQRepr->disconnect(this);
    }
  vtkSMProxy* proxy = display? display->getProxy() : NULL;
  this->setRepresentation(proxy);
  this->Internal->PQRepr = display;
  if (display)
    {
    display->connect(this, SIGNAL(representationTextChanged(const QString&)),
      SLOT(renderViewEventually()));
    }
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::setRepresentation(vtkSMProxy* proxy)
{
  // break old links.
  this->Internal->Links.clear();
  delete this->Internal->Domain;
  bool prev = this->Internal->comboBox->blockSignals(true);
  this->Internal->comboBox->clear();
  vtkSMProperty* smproperty = proxy? proxy->GetProperty("Representation") : NULL;
  this->Internal->comboBox->setEnabled(smproperty != NULL);
  if (!smproperty)
    {
    this->Internal->comboBox->addItem("Representation");
    this->Internal->comboBox->blockSignals(prev);
    return;
    }

  this->Internal->Domain = new pqComboBoxDomain(this->Internal->comboBox, smproperty);
  this->Internal->Links.addPropertyLink<PropertyLinksConnection>(
    this, "representationText", SIGNAL(representationTextChanged(const QString&)),
    proxy, smproperty);
  this->Internal->comboBox->blockSignals(prev);
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::setRepresentationText(const QString& text)
{
  int idx = this->Internal->comboBox->findText(text);
  if (idx != -1)
    {
    this->Internal->comboBox->setCurrentIndex(idx);
    }
}

//-----------------------------------------------------------------------------
QString pqDisplayRepresentationWidget::representationText() const
{
  int idx = this->Internal->comboBox->isEnabled()?
    this->Internal->comboBox->currentIndex() : -1;
  return (idx != -1)? this->Internal->comboBox->currentText() : QString();
}

//=============================================================================
pqDisplayRepresentationPropertyWidget::pqDisplayRepresentationPropertyWidget(
  vtkSMProxy *smProxy, QWidget *parentObject)
  : pqPropertyWidget(smProxy, parentObject)
{
  QVBoxLayout *layoutLocal = new QVBoxLayout;
  layoutLocal->setMargin(0);
  this->Widget = new pqDisplayRepresentationWidget(this);
  layoutLocal->addWidget(this->Widget);
  setLayout(layoutLocal);
  this->Widget->setRepresentation(smProxy);

  this->connect(this->Widget, SIGNAL(representationTextChanged(const QString&)),
    SIGNAL(changeAvailable()));
}

//-----------------------------------------------------------------------------
pqDisplayRepresentationPropertyWidget::~pqDisplayRepresentationPropertyWidget()
{
}
