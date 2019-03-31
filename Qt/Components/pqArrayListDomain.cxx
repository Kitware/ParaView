/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqArrayListDomain.h"

#include "pqSMAdaptor.h"
#include "vtkCommand.h"
#include "vtkSMDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

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
  QList<QList<QVariant> > newVal =
    pqSMAdaptor::getSelectionProperty(this->Internals->SMProperty, pqSMAdaptor::UNCHECKED);
  QVariant variantVal;
  variantVal.setValue(newVal);

  this->parent()->setProperty(this->Internals->QProperty.toLocal8Bit().data(), variantVal);
}
