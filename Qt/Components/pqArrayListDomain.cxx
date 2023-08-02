// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqArrayListDomain.h"

#include "pqSMAdaptor.h"
#include "vtkCommand.h"
#include "vtkSMDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"

#include <QWidget>
#include <QtDebug>

#include <cassert>

class pqArrayListDomain::pqInternals
{
public:
  QString QProperty;
  vtkWeakPointer<vtkSMProxy> SMProxy;
  vtkWeakPointer<vtkSMProperty> SMProperty;
  vtkWeakPointer<vtkSMDomain> SMDomain;
  unsigned long ObserverId;
  pqInternals()
    : ObserverId(0)
  {
  }
  ~pqInternals()
  {
    if (this->SMDomain && this->ObserverId)
    {
      this->SMDomain->RemoveObserver(this->ObserverId);
    }
    this->ObserverId = 0;
  }
};

//-----------------------------------------------------------------------------
pqArrayListDomain::pqArrayListDomain(QWidget* selectorWidget, const QString& qproperty,
  vtkSMProxy* proxy, vtkSMProperty* smproperty, vtkSMDomain* domain)
  : Superclass(selectorWidget)
  , Internals(new pqInternals())
{
  assert(selectorWidget && proxy && smproperty && domain);

  this->Internals->QProperty = qproperty;
  this->Internals->SMProxy = proxy;
  this->Internals->SMProperty = smproperty;
  this->Internals->SMDomain = domain;

  this->Internals->ObserverId =
    domain->AddObserver(vtkCommand::DomainModifiedEvent, this, &pqArrayListDomain::domainChanged);
}

//-----------------------------------------------------------------------------
pqArrayListDomain::~pqArrayListDomain()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqArrayListDomain::domainChanged()
{
  // reset the widget's value using the domain.
  QVariant variantVal;
  vtkSMStringVectorProperty* prop =
    vtkSMStringVectorProperty::SafeDownCast(this->Internals->SMProperty);

  unsigned int nbPerCommand = prop->GetNumberOfElementsPerCommand();

  // if property has multiple string elements
  if (prop && nbPerCommand > 1 && prop->GetElementType(1) == vtkSMStringVectorProperty::STRING)
  {
    QList<QVariant> newPropList = pqSMAdaptor::getStringListProperty(this->Internals->SMProperty);
    variantVal.setValue(newPropList);
  }
  else
  {
    QList<QList<QVariant>> newPropList =
      pqSMAdaptor::getSelectionProperty(this->Internals->SMProperty, pqSMAdaptor::UNCHECKED);
    variantVal.setValue(newPropList);
  }

  this->parent()->setProperty(this->Internals->QProperty.toUtf8().data(), variantVal);
}
