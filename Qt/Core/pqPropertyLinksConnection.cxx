// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPropertyLinksConnection.h"

#include "pqCoreUtilities.h"
#include "pqSMAdaptor.h"
#include "vtkCommand.h"
#include "vtkSMTrace.h"

#include <QStringList>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqPropertyLinksConnection::pqPropertyLinksConnection(QObject* qobject, const char* qproperty,
  const char* qsignal, vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex,
  bool use_unchecked_modified_event, QObject* parentObject)
  : Superclass(parentObject)
  , ObjectQt(qobject)
  , PropertyQt(qproperty)
  , SignalQt(qsignal)
  , ProxySM(smproxy)
  , PropertySM(smproperty)
  , IndexSM(smindex)
  , TraceChanges(false)
{
  setUseUncheckedProperties(use_unchecked_modified_event);

  if (this->ObjectQt && !this->SignalQt.isEmpty())
  {
    QObject::connect(
      this->ObjectQt, this->SignalQt.toUtf8().data(), this, SIGNAL(qtpropertyModified()));
  }
}

//-----------------------------------------------------------------------------
pqPropertyLinksConnection::~pqPropertyLinksConnection() = default;

//-----------------------------------------------------------------------------
void pqPropertyLinksConnection::setUseUncheckedProperties(bool useUnchecked)
{
  this->VTKConnector->Disconnect();

  if (this->PropertySM)
  {
    if (useUnchecked)
    {
      this->VTKConnector->Connect(this->PropertySM, vtkCommand::UncheckedPropertyModifiedEvent,
        this, SIGNAL(smpropertyModified()));
    }
    else
    {
      this->VTKConnector->Connect(
        this->PropertySM, vtkCommand::ModifiedEvent, this, SIGNAL(smpropertyModified()));
    }
  }
}

//-----------------------------------------------------------------------------
bool pqPropertyLinksConnection::operator==(const pqPropertyLinksConnection& other) const
{
  return this->ObjectQt == other.ObjectQt && this->PropertyQt == other.PropertyQt &&
    this->SignalQt == other.SignalQt && this->ProxySM == other.ProxySM &&
    this->PropertySM == other.PropertySM && this->IndexSM == other.IndexSM;
}

//-----------------------------------------------------------------------------
QVariant pqPropertyLinksConnection::currentQtValue() const
{
  return this->ObjectQt->property(this->PropertyQt.toUtf8().data());
}

//-----------------------------------------------------------------------------
QVariant pqPropertyLinksConnection::currentServerManagerValue(bool use_unchecked) const
{
  // FIXME: I'd like to directly use vtkSMPropertyHelper here. But we'll have to
  // figure things out before making that change. So we use the same code that
  // was using by older version of pqPropertyLinks.

  QVariant currentSMValue;
  pqSMAdaptor::PropertyValueType value_type =
    use_unchecked ? pqSMAdaptor::UNCHECKED : pqSMAdaptor::CHECKED;
  switch (pqSMAdaptor::getPropertyType(this->PropertySM))
  {
    case pqSMAdaptor::PROXY:
    case pqSMAdaptor::PROXYSELECTION:
    {
      pqSMProxy smproxy = pqSMAdaptor::getProxyProperty(this->PropertySM, value_type);
      currentSMValue.setValue(smproxy);
    }
    break;
    case pqSMAdaptor::PROXYLIST:
    {
      currentSMValue = pqSMAdaptor::getProxyListProperty(this->PropertySM);
    }
    break;

    case pqSMAdaptor::ENUMERATION:
      currentSMValue = pqSMAdaptor::getEnumerationProperty(this->PropertySM, value_type);
      break;

    case pqSMAdaptor::SINGLE_ELEMENT:
      currentSMValue = pqSMAdaptor::getElementProperty(this->PropertySM, value_type);
      break;

    case pqSMAdaptor::FILE_LIST:
      currentSMValue = pqSMAdaptor::getFileListProperty(this->PropertySM, value_type);
      break;

    case pqSMAdaptor::SELECTION:
      if (this->IndexSM == -1)
      {
        QList<QList<QVariant>> newVal =
          pqSMAdaptor::getSelectionProperty(this->PropertySM, value_type);
        currentSMValue.setValue(newVal);
      }
      else
      {
        QList<QVariant> sel;
        sel = pqSMAdaptor::getSelectionProperty(this->PropertySM, this->IndexSM, value_type);
        if (sel.size() == 2)
        {
          currentSMValue = sel[1];
        }
      }
      break;

    case pqSMAdaptor::MULTIPLE_ELEMENTS:
    case pqSMAdaptor::COMPOSITE_TREE:
      if (this->IndexSM == -1)
      {
        currentSMValue = pqSMAdaptor::getMultipleElementProperty(this->PropertySM, value_type);
      }
      else
      {
        currentSMValue =
          pqSMAdaptor::getMultipleElementProperty(this->PropertySM, this->IndexSM, value_type);
      }
      break;

    case pqSMAdaptor::UNKNOWN:
      break;
  }

  return currentSMValue;
}

//-----------------------------------------------------------------------------
void pqPropertyLinksConnection::setQtValue(const QVariant& value)
{
  this->ObjectQt->setProperty(this->PropertyQt.toUtf8().data(), value);
}

//-----------------------------------------------------------------------------
void pqPropertyLinksConnection::setServerManagerValue(bool use_unchecked, const QVariant& value)
{
  // FIXME: I'd like to directly use vtkSMPropertyHelper here. But we'll have to
  // figure things out before making that change. So we use the same code that
  // was using by older version of pqPropertyLinks.

  pqSMAdaptor::PropertyValueType value_type =
    use_unchecked ? pqSMAdaptor::UNCHECKED : pqSMAdaptor::CHECKED;
  switch (pqSMAdaptor::getPropertyType(this->PropertySM))
  {
    case pqSMAdaptor::PROXY:
    case pqSMAdaptor::PROXYSELECTION:
      if (use_unchecked)
      {
        pqSMAdaptor::setUncheckedProxyProperty(this->PropertySM, value.value<pqSMProxy>());
      }
      else
      {
        pqSMAdaptor::setProxyProperty(this->PropertySM, value.value<pqSMProxy>());
      }
      break;

    case pqSMAdaptor::ENUMERATION:
      pqSMAdaptor::setEnumerationProperty(this->PropertySM, value, value_type);
      break;

    case pqSMAdaptor::SINGLE_ELEMENT:
      pqSMAdaptor::setElementProperty(this->PropertySM, value, value_type);
      break;

    case pqSMAdaptor::FILE_LIST:
      if (!value.canConvert<QStringList>())
      {
        qWarning() << "File list is not a list.";
      }
      else
      {
        pqSMAdaptor::setFileListProperty(this->PropertySM, value.value<QStringList>(), value_type);
      }
      break;

    case pqSMAdaptor::SELECTION:
      if (this->IndexSM == -1)
      {
        QList<QList<QVariant>> theProp = value.value<QList<QList<QVariant>>>();
        pqSMAdaptor::setSelectionProperty(this->PropertySM, theProp, value_type);
      }
      else
      {
        QList<QVariant> domain;
        domain = pqSMAdaptor::getSelectionPropertyDomain(this->PropertySM);
        QList<QVariant> selection;
        selection.append(domain[this->IndexSM]);
        selection.append(value);

        pqSMAdaptor::setSelectionProperty(this->PropertySM, selection, value_type);
      }
      break;

    case pqSMAdaptor::MULTIPLE_ELEMENTS:
    case pqSMAdaptor::COMPOSITE_TREE:
      if (this->IndexSM == -1)
      {
        pqSMAdaptor::setMultipleElementProperty(this->PropertySM, value.toList(), value_type);
      }
      else
      {
        pqSMAdaptor::setMultipleElementProperty(this->PropertySM, this->IndexSM, value, value_type);
      }
      break;
    case pqSMAdaptor::PROXYLIST:
    {
      pqSMAdaptor::setProxyListProperty(this->PropertySM, value.toList());
    }
    break;

    case pqSMAdaptor::UNKNOWN:
      break;
  }
}

//-----------------------------------------------------------------------------
void pqPropertyLinksConnection::copyValuesFromServerManagerToQt(bool use_unchecked)
{
  if (!this->ObjectQt || !this->ProxySM || !this->PropertySM)
  {
    return;
  }

  bool prev = this->blockSignals(true);
  QVariant qtValue = this->currentQtValue();
  QVariant smValue = this->currentServerManagerValue(use_unchecked);
  if (qtValue != smValue)
  {
    if (static_cast<QMetaType::Type>(smValue.type()) == QMetaType::Double)
    {
      // QVariant is able to convert double to string but we need to be able
      // to specify how it should be formatted using settings, which
      // is provided by pqCoreUtilities
      double doubleVal = smValue.toDouble();
      this->setQtValue(pqCoreUtilities::formatFullNumber(doubleVal));
    }
    else
    {
      this->setQtValue(smValue);
    }
  }
  this->blockSignals(prev);
}

//-----------------------------------------------------------------------------
void pqPropertyLinksConnection::copyValuesFromQtToServerManager(bool use_unchecked)
{
  if (!this->ObjectQt || !this->ProxySM || !this->PropertySM)
  {
    return;
  }

  bool prev = this->blockSignals(true);

  QVariant qtValue = this->currentQtValue();

  if (this->traceChanges())
  {
    SM_SCOPED_TRACE(PropertiesModified).arg("proxy", this->proxySM());
    this->setServerManagerValue(use_unchecked, qtValue);
  }
  else
  {
    this->setServerManagerValue(use_unchecked, qtValue);
  }

  this->blockSignals(prev);
}
