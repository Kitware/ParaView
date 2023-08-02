// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

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
  int type = vtkPVCompositeKeyFrame::GetTypeFromString(text.toUtf8().data());
  if (type == vtkPVCompositeKeyFrame::NONE)
  {
    qDebug() << "Unknown type chosen in the combox: " << text;
    return;
  }

  if (type == vtkPVCompositeKeyFrame::SINUSOID && this->Internals->ValueLabel)
  {
    this->Internals->ValueLabel->setText(tr("Amplitude"));
  }
  else if (this->Internals->ValueLabel)
  {
    this->Internals->ValueLabel->setText(tr("Value"));
  }
}
