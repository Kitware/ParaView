/*=========================================================================

   Program:   ParaView
   Module:    pqSignalAdaptorKeyFrameValue.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "pqSignalAdaptorKeyFrameValue.h"
#include "ui_pqSignalAdaptorKeyFrameValue.h"

#include "vtkSMCompositeKeyFrameProxy.h"
#include "vtkSmartPointer.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkCommand.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyAdaptor.h"

#include <QtDebug>
#include <QPointer>
#include <QDoubleValidator>
#include <QIntValidator>

#include "pqSMAdaptor.h"
#include "pqPropertyLinks.h"
#include "pqAnimationCue.h"
#include "pqComboBoxDomain.h"

class pqSignalAdaptorKeyFrameValue::pqInternals : 
  public Ui::SignalAdaptorKeyFrameValue
{
public:
  enum WidgetType
    {
    NONE,
    LINEEDIT,
    COMBOBOX,
    CHECKBOX
    };
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  pqPropertyLinks Links;
  QPointer<pqAnimationCue> Cue;
  QPointer<QWidget> ActiveWidget;
  WidgetType ActiveWidgetType;
  QPointer<QWidget> Parent;
  QPointer<pqComboBoxDomain> ComboBoxDomain;

  QVariant MinValue;
  QVariant MaxValue;

  pqInternals()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->ActiveWidgetType = NONE;
    }
};

//-----------------------------------------------------------------------------
pqSignalAdaptorKeyFrameValue::pqSignalAdaptorKeyFrameValue(QWidget* _parent)
: QObject(_parent)
{
  this->Internals = new pqInternals;
  this->Internals->setupUi(_parent);
  this->Internals->lineEdit->hide();
  this->Internals->comboBox->hide();
  this->Internals->minButton->hide();
  this->Internals->maxButton->hide();
  this->Internals->Parent = _parent;

  QObject::connect(this->Internals->minButton, SIGNAL(clicked(bool)),
    this, SLOT(onMin()));
  QObject::connect(this->Internals->maxButton, SIGNAL(clicked(bool)),
    this, SLOT(onMax()));
}

//-----------------------------------------------------------------------------
pqSignalAdaptorKeyFrameValue::~pqSignalAdaptorKeyFrameValue()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameValue::setAnimationCue(pqAnimationCue* cue)
{
  if (this->Internals->Cue == cue)
    {
    return;
    }
  if (this->Internals->Cue)
    {
    QObject::disconnect(this->Internals->Cue, 0, this, 0);
    }
  this->Internals->Cue = cue;
  if (cue)
    {
    QObject::connect(cue, SIGNAL(modified()),
      this, SLOT(onCueModified()));
    }

  this->onCueModified();
}

//-----------------------------------------------------------------------------
pqAnimationCue* pqSignalAdaptorKeyFrameValue::getAnimationCue() const
{
  return this->Internals->Cue;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameValue::onCueModified()
{
  this->Internals->Parent->setEnabled(false);
  delete this->Internals->ComboBoxDomain;
  if (!this->Internals->Cue)
    {
    return;
    }
  pqAnimationCue* cue = this->Internals->Cue;

  vtkSMProperty* prop = cue->getAnimatedProperty();
  int index = cue->getAnimatedPropertyIndex();
  if (!prop)
    {
    return;
    }


  QString currentValue = this->value();
  vtkSmartPointer<vtkSMPropertyAdaptor> adaptor = 
    vtkSmartPointer<vtkSMPropertyAdaptor>::New();
  adaptor->SetProperty(prop);
  int prop_type = adaptor->GetPropertyType();
  int elem_type = adaptor->GetElementType();

  QList<QString> domainsTypes = pqSMAdaptor::getDomainTypes(prop);
  QWidget* activeWidget = 0;
  this->Internals->ActiveWidgetType = pqInternals::NONE;

  if (this->Internals->ActiveWidget)
    {
    QObject::disconnect(this->Internals->ActiveWidget, 0, this, 0);
    this->Internals->ActiveWidget = 0;
    }

  if (prop_type == vtkSMPropertyAdaptor::ENUMERATION && 
    elem_type == vtkSMPropertyAdaptor::INT)
    {
    activeWidget = this->Internals->comboBox;
    this->Internals->ComboBoxDomain = new pqComboBoxDomain(
      this->Internals->comboBox, prop, index);
    this->Internals->ActiveWidgetType = pqInternals::COMBOBOX;
    QObject::connect(this->Internals->comboBox,
      SIGNAL(currentIndexChanged(int)), 
      this, SIGNAL(valueChanged()));
    }
  else if (prop_type == vtkSMPropertyAdaptor::ENUMERATION &&
    elem_type == vtkSMPropertyAdaptor::BOOLEAN)
    {
    activeWidget = this->Internals->checkBox;
    this->Internals->ActiveWidgetType = pqInternals::CHECKBOX;
    QObject::connect(this->Internals->checkBox, SIGNAL(stateChanged(int)),
      this, SIGNAL(valueChanged()));
    }
  else if (elem_type == vtkSMPropertyAdaptor::INT)
    {
    activeWidget = this->Internals->lineEdit;
    delete this->Internals->lineEdit->validator();
    this->Internals->lineEdit->setValidator(
      new QIntValidator(this));
    this->Internals->ActiveWidgetType = pqInternals::LINEEDIT;
    QObject::connect(this->Internals->lineEdit, SIGNAL(textChanged(const QString&)),
      this, SIGNAL(valueChanged()));
    }
  else if (elem_type == vtkSMPropertyAdaptor::DOUBLE)
    {
    activeWidget = this->Internals->lineEdit;
    delete this->Internals->lineEdit->validator();
    this->Internals->lineEdit->setValidator(
      new QDoubleValidator(this));
    this->Internals->ActiveWidgetType = pqInternals::LINEEDIT;
    QObject::connect(this->Internals->lineEdit, SIGNAL(textChanged(const QString&)),
      this, SIGNAL(valueChanged()));
    }

  this->Internals->lineEdit->hide();
  this->Internals->comboBox->hide();
  this->Internals->checkBox->hide();
  if (activeWidget)
    {
    this->Internals->ActiveWidget = activeWidget;
    activeWidget->show();
    }
  this->Internals->Parent->setEnabled(true);
  if (currentValue != QString("-empty-"))
    {
    this->setValue(currentValue);
    }

  this->onDomainChanged();
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameValue::onDomainChanged()
{
  pqAnimationCue* cue = this->Internals->Cue;
  vtkSMProperty* prop = cue->getAnimatedProperty();
  int index = cue->getAnimatedPropertyIndex();
  if (!prop)
    {
    return;
    }

  vtkSmartPointer<vtkSMPropertyAdaptor> adaptor = 
    vtkSmartPointer<vtkSMPropertyAdaptor>::New();
  adaptor->SetProperty(prop);

  if (this->Internals->ActiveWidgetType == pqInternals::LINEEDIT && index != -1)
    {
    this->Internals->MinValue.clear();
    this->Internals->MinValue.clear();

    // decide min/max visibility.
    bool min_visible = false;
    bool max_visible = false;
    
    const char* min_str = adaptor->GetRangeMinimum(index);
    const char* max_str = adaptor->GetRangeMaximum(index);
    if (min_str)
      {
      this->Internals->MinValue = QVariant(min_str);
      min_visible = true;
      }
    if (max_str)
      {
      this->Internals->MaxValue = QVariant(max_str);
      max_visible = true;
      }

    this->Internals->minButton->setVisible(min_visible);
    this->Internals->maxButton->setVisible(max_visible);
    }
  else if (this->Internals->ActiveWidgetType == pqInternals::COMBOBOX)
    {
    // Nothing to do, the pqComboBoxDomain already manages it.
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameValue::onMin()
{
  if (this->Internals->ActiveWidgetType == pqInternals::LINEEDIT
    && this->Internals->MinValue.isValid())
    {
    this->setValue(this->Internals->MinValue.toString());
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameValue::onMax()
{
  if (this->Internals->ActiveWidgetType == pqInternals::LINEEDIT
    && this->Internals->MaxValue.isValid())
    {
    this->setValue(this->Internals->MaxValue.toString());
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameValue::setValueToCurrent()
{
  pqAnimationCue* cue = this->Internals->Cue;
  vtkSMProperty* prop = cue->getAnimatedProperty();
  int index = cue->getAnimatedPropertyIndex();
  if (!prop)
    {
    return;
    }
  if (index != -1)
    {
    QVariant curValue = pqSMAdaptor::getMultipleElementProperty(prop, index).toString();
    if (curValue.isValid())
      {
      this->setValue(curValue.toString());
      }
    }
}

//-----------------------------------------------------------------------------
QString pqSignalAdaptorKeyFrameValue::value() const
{
  switch (this->Internals->ActiveWidgetType)
    {
  case pqInternals::LINEEDIT:
    return this->Internals->lineEdit->text();

  case pqInternals::CHECKBOX:
    return (this->Internals->checkBox->checkState() == Qt::Checked)? "1.0" : "0.0";

  case pqInternals::COMBOBOX:
    return this->Internals->comboBox->currentText();
  default:
    break;
    }

  return QString("-empty-");
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameValue::setValue(const QString& value)
{
  switch (this->Internals->ActiveWidgetType)
    {
  case pqInternals::LINEEDIT:
    this->Internals->lineEdit->setText(value);
    break;

  case pqInternals::CHECKBOX:
      {
      QVariant v(value);
      if (v.canConvert(QVariant::Int) && v.toInt() > 0)
        {
        this->Internals->checkBox->setCheckState(Qt::Checked);
        }
      else
        {
        this->Internals->checkBox->setCheckState(Qt::Unchecked);
        }
      }
    break;

  case pqInternals::COMBOBOX:
      {
      QComboBox* combo = this->Internals->comboBox;
      int idx = combo->findText(value);
      combo->setCurrentIndex(idx);
      if(idx == -1 && combo->count() > 0)
        {
        combo->setCurrentIndex(0);
        }
      }
    break;

  default:
    break;
    }
}
