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
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyAdaptor.h"

#include <QtDebug>
#include <QPointer>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QGridLayout>

#include "pqAnimationCue.h"
#include "pqComboBoxDomain.h"
#include "pqPropertyLinks.h"
#include "pqSampleScalarWidget.h"
#include "pqSMAdaptor.h"

class pqSignalAdaptorKeyFrameValue::pqInternals : 
  public Ui::SignalAdaptorKeyFrameValue
{
public:
  enum WidgetType
    {
    NONE,
    LINEEDIT,
    COMBOBOX,
    CHECKBOX,
    SAMPLESCALARS
    };
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  pqPropertyLinks Links;
  QPointer<pqAnimationCue> Cue;
  QPointer<QWidget> ActiveWidget;
  WidgetType ActiveWidgetType;
  QPointer<QWidget> Parent;
  QPointer<pqComboBoxDomain> ComboBoxDomain;
  QPointer<QWidget> LargeParent;
  QPointer<pqSampleScalarWidget> SampleScalarWidget;
  QVariant MinValue;
  QVariant MaxValue;

  pqInternals()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->ActiveWidgetType = NONE;
    }
};

//-----------------------------------------------------------------------------
pqSignalAdaptorKeyFrameValue::pqSignalAdaptorKeyFrameValue(QWidget* lparent, 
  QWidget* _parent)
: QObject(_parent)
{
  this->Internals = new pqInternals;
  this->Internals->setupUi(_parent);
  this->Internals->lineEdit->hide();
  this->Internals->comboBox->hide();
  this->Internals->minButton->hide();
  this->Internals->maxButton->hide();
  this->Internals->Parent = _parent;
  this->Internals->LargeParent = lparent;

  lparent->hide();
  this->Internals->SampleScalarWidget = new pqSampleScalarWidget(true, lparent);
  QGridLayout* llayout = new QGridLayout(lparent);
  llayout->addWidget(this->Internals->SampleScalarWidget, 0, 0);
  llayout->setSpacing(0);
  this->Internals->SampleScalarWidget->layout()->setMargin(0);

  QObject::connect(this->Internals->minButton, SIGNAL(clicked(bool)),
    this, SLOT(onMin()));
  QObject::connect(this->Internals->maxButton, SIGNAL(clicked(bool)),
    this, SLOT(onMax()));
}

//-----------------------------------------------------------------------------
pqSignalAdaptorKeyFrameValue::~pqSignalAdaptorKeyFrameValue()
{
  delete this->Internals->SampleScalarWidget;
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


  QList<QVariant> currentValue = this->values();
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

  if (index == -1)
    {
    if (elem_type == vtkSMPropertyAdaptor::DOUBLE)
      {
      this->Internals->LargeParent->show();
      this->Internals->SampleScalarWidget->setDataSources(
       cue->getAnimatedProxy(),vtkSMDoubleVectorProperty::SafeDownCast(prop));
      QObject::connect(this->Internals->SampleScalarWidget,
        SIGNAL(samplesChanged()), this, SIGNAL(valueChanged()));
      activeWidget = this->Internals->SampleScalarWidget;
      this->Internals->ActiveWidgetType = pqInternals::SAMPLESCALARS;
      }
    }
  else
    {
    this->Internals->LargeParent->hide();

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
  if (currentValue.size() > 0)
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
  else if (this->Internals->ActiveWidgetType == pqInternals::SAMPLESCALARS)
    {
    // pqSampleScalarWidget automatically updates the domain.
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameValue::onMin()
{
  if (this->Internals->ActiveWidgetType == pqInternals::LINEEDIT
    && this->Internals->MinValue.isValid())
    {
    this->setValue(this->Internals->MinValue);
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameValue::onMax()
{
  if (this->Internals->ActiveWidgetType == pqInternals::LINEEDIT
    && this->Internals->MaxValue.isValid())
    {
    this->setValue(this->Internals->MaxValue);
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
    QVariant curValue = pqSMAdaptor::getMultipleElementProperty(prop, index);
    if (curValue.isValid())
      {
      this->setValue(curValue);
      }
    }
  else
    {
    QList<QVariant> curValues = pqSMAdaptor::getMultipleElementProperty(prop);
    this->setValue(curValues);
    }
}

//-----------------------------------------------------------------------------
QVariant pqSignalAdaptorKeyFrameValue::value() const
{
  QList<QVariant> list = this->values();
  if (list.size()> 0)
    {
    return list[0];
    }
  return QVariant();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSignalAdaptorKeyFrameValue::values() const
{
  QList<QVariant> values;
  switch (this->Internals->ActiveWidgetType)
    {
  case pqInternals::LINEEDIT:
    values << this->Internals->lineEdit->text();
    break;

  case pqInternals::CHECKBOX:
    values << (this->Internals->checkBox->checkState() == Qt::Checked);
    break;

  case pqInternals::COMBOBOX:
    values << this->Internals->comboBox->currentText();
    break;

  case pqInternals::SAMPLESCALARS:
    values = this->Internals->SampleScalarWidget->samples();
    break;

  default:
    break;
    }

  return values;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameValue::setValue(QVariant value)
{
  QList<QVariant> list;
  list.push_back(value);
  this->setValue(list);
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameValue::setValue(const QList<QVariant>& new_value)
{
  switch (this->Internals->ActiveWidgetType)
    {
  case pqInternals::LINEEDIT:
    if (new_value.size() > 0)
      {
      this->Internals->lineEdit->setText(new_value[0].toString());
      }
    break;

  case pqInternals::CHECKBOX:
      {
      QVariant v = new_value[0];
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
      int idx = combo->findText(new_value[0].toString());
      combo->setCurrentIndex(idx);
      if(idx == -1 && combo->count() > 0)
        {
        combo->setCurrentIndex(0);
        }
      }
    break;

  case pqInternals::SAMPLESCALARS:
    this->Internals->SampleScalarWidget->setSamples(new_value);
    break;

  default:
    break;
    }
}
