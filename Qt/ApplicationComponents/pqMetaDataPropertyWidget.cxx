/*=========================================================================

   Program: ParaView
   Module: pqMetaDataPropertyWidget.cxx

   Copyright (c) 2005-2022 Sandia Corporation, Kitware Inc.
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
#include "pqMetaDataPropertyWidget.h"

#include "pqLabel.h"
#include "pqProxyWidget.h"

#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkSMCompoundSourceProxy.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMSettings.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMVectorProperty.h"

#include <QScopedValueRollback>
#include <QVBoxLayout>
#include <QWidget>

class pqMetaDataPropertyWidget::pqInternals
{
public:
  bool InUpdate = false; // if inside UpdatePropertyWithUncheckedValues

  // copy property's unchecked values into checked values. if copied, return true, else false
  bool UpdatePropertyWithUncheckedValues(vtkSMProperty* smproperty)
  {
    vtkLogScopeFunction(TRACE);
    if (this->InUpdate)
    { // In Copy, vtkSMPropertyHelper::SetElements calls ClearUncheckedElements, which might invoke
      // UncheckedPropertyModifiedEvent
      vtkLog(TRACE, "Already inside " << __func__);
      return false;
    }

    QScopedValueRollback<bool> rollback(this->InUpdate, true);
    vtkSMPropertyHelper uncheckedHelper(smproperty);
    uncheckedHelper.SetUseUnchecked(true);
    bool needsUpdate = true;

    vtkLogF(TRACE, "unchecked values | checked values");
    for (unsigned int i = 0; i < uncheckedHelper.GetNumberOfElements(); ++i)
    {
      vtkLog(TRACE, << uncheckedHelper.GetAsVariant(i) << " | "
                    << vtkSMPropertyHelper(smproperty).GetAsVariant(i));
      needsUpdate |=
        (uncheckedHelper.GetAsVariant(i) != vtkSMPropertyHelper(smproperty).GetAsVariant(i));
    };
    return needsUpdate ? vtkSMPropertyHelper(smproperty).Copy(uncheckedHelper) : false;
  }
};

//-----------------------------------------------------------------------------
pqMetaDataPropertyWidget::pqMetaDataPropertyWidget(
  vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent)
  : Superclass(proxy, parent)
  , Internals(std::unique_ptr<pqInternals>(new pqInternals()))
{
  auto layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  this->setLayout(layout);
  bool useDocumentationForLabels = pqProxyWidget::useDocumentationForLabels(proxy);

  for (unsigned int i = 0; i < smgroup->GetNumberOfProperties(); ++i)
  {
    auto smproperty = smgroup->GetProperty(i);
    const char* smkey = smgroup->GetPropertyName(i);
    auto layoutLocal = new QHBoxLayout(this);
    auto widget = pqProxyWidget::createWidgetForProperty(smproperty, proxy, this);

    widget->setObjectName(QString(smkey).remove(' '));
    layout->addLayout(layoutLocal);
    smproperty->AddObserver(
      vtkCommand::UncheckedPropertyModifiedEvent, this, &pqMetaDataPropertyWidget::updateMetadata);
    this->connect(widget, SIGNAL(changeAvailable()), this, SIGNAL(changeAvailable()));

    if (!widget->showLabel())
    { // widget is already showing a label (usual suspects are QCheckbox)
      layoutLocal->addWidget(widget);
      continue;
    }
    const bool isCompoundProxy = vtkSMCompoundSourceProxy::SafeDownCast(proxy) != nullptr;
    const char* xmllabel =
      (smproperty->GetXMLLabel() && !isCompoundProxy) ? smproperty->GetXMLLabel() : smkey;

    const QString xmlDocumentation = pqProxyWidget::documentationText(smproperty);
    const QString itemLabel = useDocumentationForLabels
      ? QString("<p><b>%1</b>: %2</p>").arg(xmllabel).arg(xmlDocumentation)
      : QString(xmllabel);

    pqLabel* label = new pqLabel(itemLabel, this);
    label->setObjectName(
      QString("%1Label%2").arg(widget->metaObject()->className()).arg(widget->objectName()));
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    layoutLocal->addWidget(label, /*stretch*/ 0);
    layoutLocal->addWidget(widget, /*stretch*/ 1);
  }
}

//-----------------------------------------------------------------------------
pqMetaDataPropertyWidget::~pqMetaDataPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqMetaDataPropertyWidget::updateMetadata(
  vtkObject* obj, unsigned long vtkNotUsed(eventId), void* vtkNotUsed(callData))
{
  vtkLogScopeFunction(TRACE);
  vtkSMSourceProxy* source = nullptr;
  vtkSMProperty* smproperty = nullptr;
  if ((source = vtkSMSourceProxy::SafeDownCast(this->proxy())) == nullptr)
  {
    return;
  }
  if ((smproperty = vtkSMProperty::SafeDownCast(obj)) == nullptr)
  {
    return;
  }
  vtkLog(TRACE, << "Property modified: " << smproperty->GetXMLName());
  if (!this->Internals->UpdatePropertyWithUncheckedValues(smproperty))
  {
    vtkLog(TRACE, << "Metadata update unnecessary");
    return;
  }

  // Save this property into application user settings.
  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  QString settingName = QString("%1.%2.%3")
                          .arg(source->GetXMLGroup())
                          .arg(source->GetXMLName())
                          .arg(smproperty->GetXMLName());
  if (auto vecProp = vtkSMVectorProperty::SafeDownCast(smproperty))
  {
    unsigned int max = vecProp->GetNumberOfElements();
    for (unsigned int i = 0; i < vecProp->GetNumberOfElements(); ++i)
    {
      QString settingKey = settingName;
      if (max > 1)
      {
        settingKey.append(QString("[%1]").arg(i));
      }
      if (vtkSMIntVectorProperty::SafeDownCast(vecProp) != nullptr)
      {
        settings->SetSetting(
          settingKey.toStdString().c_str(), vtkSMPropertyHelper(smproperty).GetAsInt(i));
      }
      else if (vtkSMDoubleVectorProperty::SafeDownCast(vecProp) != nullptr)
      {
        settings->SetSetting(
          settingKey.toStdString().c_str(), vtkSMPropertyHelper(smproperty).GetAsDouble(i));
      }
      else if (vtkSMStringVectorProperty::SafeDownCast(vecProp) != nullptr)
      {
        settings->SetSetting(
          settingKey.toStdString().c_str(), vtkSMPropertyHelper(smproperty).GetAsString(i));
      }
      else if (vtkSMIdTypeVectorProperty::SafeDownCast(vecProp) != nullptr)
      {
        settings->SetSetting(settingKey.toStdString().c_str(),
          static_cast<int>(vtkSMPropertyHelper(smproperty).GetAsIdType(i)));
      }
    }
  }

  source->UpdateProperty(smproperty->GetXMLName());
  source->UpdatePipelineInformation();
  vtkSmartPointer<vtkSMPropertyIterator> iter;
  iter.TakeReference(source->NewPropertyIterator());

  // if vtkSMArraySelectionDomain is present, ResetToDomainDefaults will
  // query the information property for values.
  vtkLogScopeF(TRACE, "Reset to domain defaults");
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    auto property = vtkSMVectorProperty::SafeDownCast(iter->GetProperty());
    if (property == nullptr)
    {
      continue;
    }
    if (property->FindDomain("vtkSMArraySelectionDomain") != nullptr)
    {
      vtkLog(TRACE, << "Reload " << property->GetXMLName());
      property->ResetToDomainDefaults();
    }
  }
}
