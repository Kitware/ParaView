/*=========================================================================

   Program: ParaView
   Module:    pqAnimatablePropertiesComboBox.cxx

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

========================================================================*/
#include "pqAnimatablePropertiesComboBox.h"

// Server Manager Includes.
#include "vtkEventQtSlotConnect.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMVectorProperty.h"
#include "vtkSmartPointer.h"

// Qt Includes.
#include <QtDebug>

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqSMAdaptor.h"
#include "pqServerManagerModel.h"

//-----------------------------------------------------------------------------
class pqAnimatablePropertiesComboBox::pqInternal
{
public:
  pqInternal()
  {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->CollapseVectors = false;
    this->VectorSizeFilter = -1;
  }

  vtkSmartPointer<vtkSMProxy> Source;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  bool CollapseVectors;
  int VectorSizeFilter;

  struct PropertyInfo
  {
    vtkSmartPointer<vtkSMProxy> Proxy;
    QString Name;
    int Index;
    bool IsDisplayProperty;
    unsigned int DisplayPort;
    PropertyInfo()
    {
      this->Index = 0;
      this->IsDisplayProperty = false;
      this->DisplayPort = 0;
    }
  };
};

Q_DECLARE_METATYPE(pqAnimatablePropertiesComboBox::pqInternal::PropertyInfo);

//-----------------------------------------------------------------------------
pqAnimatablePropertiesComboBox::pqAnimatablePropertiesComboBox(QWidget* _parent)
  : Superclass(_parent)
{
  this->Internal = new pqInternal();
  this->UseBlankEntry = false;
}

//-----------------------------------------------------------------------------
pqAnimatablePropertiesComboBox::~pqAnimatablePropertiesComboBox()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqAnimatablePropertiesComboBox::setSource(vtkSMProxy* proxy)
{
  if (this->Internal->Source == proxy)
  {
    return;
  }

  this->Internal->VTKConnect->Disconnect();
  this->setEnabled(proxy != nullptr);
  this->Internal->Source = proxy;
  this->buildPropertyList();
}

//-----------------------------------------------------------------------------
void pqAnimatablePropertiesComboBox::setSourceWithoutProperties(vtkSMProxy* proxy)
{
  if (this->Internal->Source == proxy)
  {
    return;
  }

  this->Internal->VTKConnect->Disconnect();
  this->setEnabled(proxy != nullptr);
  this->Internal->Source = proxy;
  this->clear();
}

//-----------------------------------------------------------------------------
void pqAnimatablePropertiesComboBox::setCollapseVectors(bool val)
{
  if (this->Internal->CollapseVectors != val)
  {
    this->Internal->CollapseVectors = val;
    this->buildPropertyList();
  }
}

//-----------------------------------------------------------------------------
void pqAnimatablePropertiesComboBox::setVectorSizeFilter(int new_size)
{
  if (this->Internal->VectorSizeFilter != new_size)
  {
    this->Internal->VectorSizeFilter = new_size;
    this->buildPropertyList();
  }
}

//-----------------------------------------------------------------------------
void pqAnimatablePropertiesComboBox::buildPropertyList()
{
  this->clear();
  if (!this->Internal->Source.GetPointer())
  {
    return;
  }
  if (this->UseBlankEntry)
  {
    this->addSMPropertyInternal("<select>", nullptr, QString(), -1);
  }
  this->buildPropertyListInternal(this->Internal->Source, QString());
  this->addDisplayProperties(this->Internal->Source);
}

//-----------------------------------------------------------------------------
void pqAnimatablePropertiesComboBox::buildPropertyListInternal(
  vtkSMProxy* proxy, const QString& labelPrefix)
{
  this->Internal->VTKConnect->Disconnect();
  vtkSmartPointer<vtkSMPropertyIterator> iter;
  iter.TakeReference(proxy->NewPropertyIterator());
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    vtkSMVectorProperty* smproperty = vtkSMVectorProperty::SafeDownCast(iter->GetProperty());
    if (!smproperty || !smproperty->GetAnimateable() || smproperty->GetInformationOnly())
    {
      continue;
    }
    unsigned int num_elems = smproperty->GetNumberOfElements();
    if (this->Internal->VectorSizeFilter >= 0 &&
      static_cast<int>(num_elems) != this->Internal->VectorSizeFilter)
    {
      continue;
    }

    bool collapseVectors = this->Internal->CollapseVectors || smproperty->GetRepeatCommand();

    if (collapseVectors)
    {
      num_elems = 1;
    }
    for (unsigned int cc = 0; cc < num_elems; cc++)
    {
      int index = collapseVectors ? -1 : static_cast<int>(cc);
      QString label = labelPrefix.isEmpty() ? "" : labelPrefix + " - ";
      label += iter->GetProperty()->GetXMLLabel();
      label = (num_elems > 1) ? label + " (" + QString::number(cc) + ")" : label;

      this->addSMPropertyInternal(label, proxy, iter->GetKey(), index);
    }
  }

  // Now add properties of all proxies in properties that have
  // proxy lists.
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    vtkSMProxyProperty* smproperty = vtkSMProxyProperty::SafeDownCast(iter->GetProperty());
    if (smproperty && pqSMAdaptor::getPropertyType(smproperty) == pqSMAdaptor::PROXYSELECTION)
    {
      vtkSMProxy* child_proxy = pqSMAdaptor::getProxyProperty(smproperty);
      if (child_proxy)
      {
        QString newPrefix = labelPrefix.isEmpty() ? "" : labelPrefix + ":";
        newPrefix += smproperty->GetXMLLabel();
        this->buildPropertyListInternal(child_proxy, newPrefix);

        // if this property's value changes, we'll have to rebuild
        // the property names menu.
        this->Internal->VTKConnect->Connect(smproperty, vtkCommand::ModifiedEvent, this,
          SLOT(buildPropertyList()), nullptr, 0, Qt::QueuedConnection);
      }
    }
  }
}
//-----------------------------------------------------------------------------
void pqAnimatablePropertiesComboBox::addDisplayProperties(vtkSMProxy* proxy)
{
  // if the vector size is set and not set to 1, we should not add Visibility and Opacity
  // which have only 1 component
  if (!proxy || !proxy->IsA("vtkSMSourceProxy") || this->Internal->VectorSizeFilter > 1)
  {
    return;
  }

  vtkSMSourceProxy* sourceProxy = static_cast<vtkSMSourceProxy*>(proxy);
  unsigned int numports = sourceProxy->GetNumberOfOutputPorts();
  for (unsigned int kk = 0; kk < numports; kk++)
  {
    QString suffix;
    if (numports > 1)
    {
      suffix = QString(" [%1]").arg(sourceProxy->GetOutputPortName(kk));
    }

    this->addSMPropertyInternal(
      QString("%1%2").arg("Visibility").arg(suffix), proxy, "Visibility", 0, true, kk);

    this->addSMPropertyInternal(
      QString("%1%2").arg("Opacity").arg(suffix), proxy, "Opacity", 0, true, kk);
  }
}

//-----------------------------------------------------------------------------
void pqAnimatablePropertiesComboBox::addSMProperty(
  const QString& label, const QString& propertyname, int index)
{
  if (!this->Internal->Source)
  {
    qCritical() << "Source must be set before adding properties.";
    return;
  }

  this->addSMPropertyInternal(label, this->Internal->Source, propertyname, index);
}

//-----------------------------------------------------------------------------
void pqAnimatablePropertiesComboBox::addSMPropertyInternal(const QString& label, vtkSMProxy* proxy,
  const QString& propertyname, int index, bool is_display_property /*=false*/,
  unsigned int display_port /*=0*/)
{
  pqInternal::PropertyInfo info;
  info.Proxy = proxy;
  info.Name = propertyname;
  info.Index = index;
  info.IsDisplayProperty = is_display_property;
  info.DisplayPort = display_port;
  this->addItem(label, QVariant::fromValue(info));
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimatablePropertiesComboBox::getCurrentProxy() const
{
  int index = this->currentIndex();
  if (index != -1)
  {
    QVariant _data = this->itemData(index);
    pqInternal::PropertyInfo info = _data.value<pqInternal::PropertyInfo>();
    if (info.IsDisplayProperty)
    {
      pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
      pqPipelineSource* src = smmodel->findItem<pqPipelineSource*>(info.Proxy);
      QList<vtkSMProxy*> helperProxies = src->getHelperProxies("RepresentationAnimationHelper");
      if (static_cast<unsigned int>(helperProxies.size()) > info.DisplayPort)
      {
        return helperProxies[info.DisplayPort];
      }
      else
      {
        return nullptr;
      }
    }

    return info.Proxy;
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
QString pqAnimatablePropertiesComboBox::getCurrentPropertyName() const
{
  int index = this->currentIndex();
  if (index != -1)
  {
    QVariant _data = this->itemData(index);
    pqInternal::PropertyInfo info = _data.value<pqInternal::PropertyInfo>();
    return info.Name;
  }
  return QString();
}

//-----------------------------------------------------------------------------
int pqAnimatablePropertiesComboBox::getCurrentIndex() const
{
  int index = this->currentIndex();
  if (index != -1)
  {
    QVariant _data = this->itemData(index);
    pqInternal::PropertyInfo info = _data.value<pqInternal::PropertyInfo>();
    return info.Index;
  }
  return 0;
}

//-----------------------------------------------------------------------------
bool pqAnimatablePropertiesComboBox::getCollapseVectors() const
{
  return this->Internal->CollapseVectors;
}

//-----------------------------------------------------------------------------
int pqAnimatablePropertiesComboBox::getVectorSizeFilter() const
{
  return this->Internal->VectorSizeFilter;
}
