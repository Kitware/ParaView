/*=========================================================================

   Program: ParaView
   Module:  pqPropertyLinksConnection.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqPropertyLinksConnection.h"

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
      this->ObjectQt, this->SignalQt.toLocal8Bit().data(), this, SIGNAL(qtpropertyModified()));
  }
}

//-----------------------------------------------------------------------------
pqPropertyLinksConnection::~pqPropertyLinksConnection()
{
}

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
  return this->ObjectQt->property(this->PropertyQt.toLocal8Bit().data());
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
    case pqSMAdaptor::PROXYLIST:
    {
      pqSMProxy smproxy = pqSMAdaptor::getProxyProperty(this->PropertySM, value_type);
      currentSMValue.setValue(smproxy);
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
        QList<QList<QVariant> > newVal =
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
    case pqSMAdaptor::SIL:
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
  this->ObjectQt->setProperty(this->PropertyQt.toLocal8Bit().data(), value);
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
        QList<QList<QVariant> > theProp = value.value<QList<QList<QVariant> > >();
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

    case pqSMAdaptor::SIL:
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

    case pqSMAdaptor::UNKNOWN:
    case pqSMAdaptor::PROXYLIST:
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
    this->setQtValue(smValue);
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
