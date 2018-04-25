/*=========================================================================

   Program:   ParaView
   Module:    pqSignalAdaptorKeyFrameType.cxx

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

#include "pqSignalAdaptorKeyFrameType.h"

#include <QDebug>
#include <QLabel>
#include <QPointer>

#include "pqKeyFrameTypeWidget.h"
#include "pqPropertyLinks.h"
#include "vtkPVCompositeKeyFrame.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

class pqSignalAdaptorKeyFrameType::pqInternals
{
public:
  vtkSmartPointer<vtkSMProxy> KeyFrameProxy;
  QPointer<QLabel> ValueLabel;
  QPointer<pqPropertyLinks> Links;
  QPointer<pqKeyFrameTypeWidget> Widget;
};

//-----------------------------------------------------------------------------
pqSignalAdaptorKeyFrameType::pqSignalAdaptorKeyFrameType(
  pqKeyFrameTypeWidget* widget, pqPropertyLinks* links, QLabel* label)
  : pqSignalAdaptorComboBox(widget->typeComboBox())
{
  this->Internals = new pqInternals;
  this->Internals->Widget = widget;
  this->Internals->ValueLabel = label;
  this->Internals->Links = links;

  QObject::connect(widget, SIGNAL(typeChanged(const QString&)), this, SLOT(onTypeChanged()));
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

  if (!this->Internals->Links)
  {
    return;
  }

  this->Internals->Links->removeAllPropertyLinks();

  if (proxy && strcmp(proxy->GetXMLName(), "CompositeKeyFrame") == 0)
  {
    // connect the combo box
    this->Internals->Links->addPropertyLink(this->Internals->Widget, "type",
      SIGNAL(typeChanged(const QString&)), proxy, proxy->GetProperty("Type"));

    // Connect the GUI and the properties.
    this->Internals->Links->addPropertyLink(this->Internals->Widget, "base",
      SIGNAL(baseChanged(const QString&)), proxy, proxy->GetProperty("Base"));
    this->Internals->Links->addPropertyLink(this->Internals->Widget, "startPower",
      SIGNAL(startPowerChanged(const QString&)), proxy, proxy->GetProperty("StartPower"));
    this->Internals->Links->addPropertyLink(this->Internals->Widget, "endPower",
      SIGNAL(endPowerChanged(const QString&)), proxy, proxy->GetProperty("EndPower"));

    this->Internals->Links->addPropertyLink(this->Internals->Widget, "offset",
      SIGNAL(offsetChanged(const QString&)), proxy, proxy->GetProperty("Offset"));
    this->Internals->Links->addPropertyLink(this->Internals->Widget, "frequency",
      SIGNAL(frequencyChanged(const QString&)), proxy, proxy->GetProperty("Frequency"));
    this->Internals->Links->addPropertyLink(this->Internals->Widget, "phase",
      SIGNAL(phaseChanged(double)), proxy, proxy->GetProperty("Phase"));
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
  int type = vtkPVCompositeKeyFrame::GetTypeFromString(text.toLocal8Bit().data());
  if (type == vtkPVCompositeKeyFrame::NONE)
  {
    qDebug() << "Unknown type chosen in the combox: " << text;
    return;
  }

  if (type == vtkPVCompositeKeyFrame::SINUSOID && this->Internals->ValueLabel)
  {
    this->Internals->ValueLabel->setText("Amplitude");
  }
  else if (this->Internals->ValueLabel)
  {
    this->Internals->ValueLabel->setText("Value");
  }
}
