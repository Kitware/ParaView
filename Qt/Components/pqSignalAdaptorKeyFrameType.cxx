/*=========================================================================

   Program:   ParaView
   Module:    pqSignalAdaptorKeyFrameType.cxx

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
#include "pqSignalAdaptorKeyFrameType.h"
#include "ui_pqSignalAdaptorKeyFrameType.h"

#include "vtkSMCompositeKeyFrameProxy.h"
#include "vtkSmartPointer.h"

#include <QComboBox>
#include <QDoubleValidator>
#include <QLabel>
#include <QPointer>
#include <QtDebug>
#include <QWidget>

#include "pqPropertyLinks.h"

class pqSignalAdaptorKeyFrameType::pqInternals
{
public:
  Ui::pqSignalAdaptorKeyFrameType Ui;
  vtkSmartPointer<vtkSMProxy> KeyFrameProxy;
  QPointer<QWidget> Frame;
  QPointer<QLabel> ValueLabel;
  QPointer<pqPropertyLinks> Links;
};

//-----------------------------------------------------------------------------
pqSignalAdaptorKeyFrameType::pqSignalAdaptorKeyFrameType(QComboBox* combo,
  QLabel* label, QWidget* frame, pqPropertyLinks* links)
: pqSignalAdaptorComboBox(combo)
{
  this->Internals = new pqInternals;
  this->Internals->Frame = frame;
  this->Internals->ValueLabel = label;
  this->Internals->Ui.setupUi(frame);
  this->Internals->Links = links;

  this->Internals->Ui.exponentialGroup->hide();
  this->Internals->Ui.sinusoidGroup->hide();

  QDoubleValidator * validator = new QDoubleValidator(this);
  this->Internals->Ui.Base->setValidator(validator);
  this->Internals->Ui.StartPower->setValidator(validator);
  this->Internals->Ui.EndPower->setValidator(validator);
  this->Internals->Ui.Offset->setValidator(validator);
  this->Internals->Ui.Frequency->setValidator(validator);

  combo->blockSignals(true);
  combo->addItem(QIcon(":pqWidgets/Icons/pqRamp16.png"), "Ramp", "Ramp");
  combo->addItem(QIcon(":pqWidgets/Icons/pqExponential16.png"), "Exponential", 
    "Exponential");
  combo->addItem(QIcon(":pqWidgets/Icons/pqSinusoidal16.png"), "Sinusoid", 
    "Sinusoid");
  combo->addItem(QIcon(":pqWidgets/Icons/pqStep16.png"), "Step", "Boolean");
  combo->setCurrentIndex(-1);
  combo->blockSignals(false);

  QObject::connect(combo, SIGNAL(currentIndexChanged(int)),
    this, SLOT(onTypeChanged()));
  frame->hide();
}

//-----------------------------------------------------------------------------
pqSignalAdaptorKeyFrameType::~pqSignalAdaptorKeyFrameType()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameType::setKeyFrameProxy(vtkSMProxy* proxy)
{
  this->Internals->KeyFrameProxy = proxy;
  
  if(!this->Internals->Links)
    {
    return;
    }

  this->Internals->Links->removeAllPropertyLinks();

  if (proxy)
    {
    // connect the combo box
    this->Internals->Links->addPropertyLink(
      this, "currentData",
      SIGNAL(currentTextChanged(const QString&)),
      proxy, proxy->GetProperty("Type"));

    // Connect the GUI and the properties.
    this->Internals->Links->addPropertyLink(
      this->Internals->Ui.Base, "text", SIGNAL(textChanged(const QString&)),
      proxy, proxy->GetProperty("Base"));
    this->Internals->Links->addPropertyLink(
      this->Internals->Ui.StartPower, "text", SIGNAL(textChanged(const QString&)),
      proxy, proxy->GetProperty("StartPower"));
    this->Internals->Links->addPropertyLink(
      this->Internals->Ui.EndPower, "text", SIGNAL(textChanged(const QString&)),
      proxy, proxy->GetProperty("EndPower"));

    this->Internals->Links->addPropertyLink(
      this->Internals->Ui.Offset, "text", SIGNAL(textChanged(const QString&)),
      proxy, proxy->GetProperty("Offset"));
    this->Internals->Links->addPropertyLink(
      this->Internals->Ui.Frequency, "text", SIGNAL(textChanged(const QString&)),
      proxy, proxy->GetProperty("Frequency"));
    this->Internals->Links->addPropertyLink(
      this->Internals->Ui.Phase, "value", SIGNAL(valueChanged(const QString&)),
      proxy, proxy->GetProperty("Phase"));
    }
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqSignalAdaptorKeyFrameType::getKeyFrameProxy() const
{
  return this->Internals->KeyFrameProxy;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameType::onTypeChanged()
{
  QString text = this->currentData().toString();
  int type = vtkSMCompositeKeyFrameProxy::GetTypeFromString(text.toAscii().data());
  if (type == vtkSMCompositeKeyFrameProxy::NONE)
    {
    qDebug() << "Unknown type choosen in the combox: " << text;
    return;
    }

  // Hide all
  this->Internals->Frame->hide();
  this->Internals->Ui.exponentialGroup->hide();
  this->Internals->Ui.sinusoidGroup->hide();

  if (type == vtkSMCompositeKeyFrameProxy::EXPONENTIAL)
    {
    this->Internals->Ui.exponentialGroup->show();
    this->Internals->Frame->show();
    }
  else if (type == vtkSMCompositeKeyFrameProxy::SINUSOID)
    {
    this->Internals->Ui.sinusoidGroup->show();
    this->Internals->Frame->show();
    }

  if (type == vtkSMCompositeKeyFrameProxy::SINUSOID && this->Internals->ValueLabel)
    {
    this->Internals->ValueLabel->setText("Amplitude");
    }
  else if (this->Internals->ValueLabel)
    {
    this->Internals->ValueLabel->setText("Value");
    }
}

