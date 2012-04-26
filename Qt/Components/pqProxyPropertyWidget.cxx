/*=========================================================================

   Program: ParaView
   Module: pqProxyPropertyWidget.cxx

   Copyright (c) 2005-2012 Sandia Corporation, Kitware Inc.
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

#include "pqProxyPropertyWidget.h"

#include <QVBoxLayout>

#include "vtkSMProperty.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMProxyListDomain.h"

#include "pqProxySelectionWidget.h"

pqProxyPropertyWidget::pqProxyPropertyWidget(vtkSMProperty *property,
                                             vtkSMProxy *proxy,
                                             QWidget *parent)
  : pqPropertyWidget(proxy, parent)
{
  QVBoxLayout *layout = new QVBoxLayout;

  // find the domain
  vtkSMDomain *domain = 0;
  vtkSMDomainIterator *domainIter = property->NewDomainIterator();
  for(domainIter->Begin(); !domainIter->IsAtEnd(); domainIter->Next())
    {
    domain = domainIter->GetDomain();
    }
  domainIter->Delete();

  if(vtkSMProxyListDomain::SafeDownCast(domain))
    {
    pqProxySelectionWidget *widget =
      new pqProxySelectionWidget(proxy,
                                 proxy->GetPropertyName(property),
                                 property->GetXMLLabel(),
                                 this);
    widget->select();
    this->addPropertyLink(widget,
                          proxy->GetPropertyName(property),
                          SIGNAL(proxyChanged(pqSMProxy)),
                          property);
    this->connect(widget, SIGNAL(modified()), this, SIGNAL(modified()));

    layout->addWidget(widget);
    }

  this->setLayout(layout);
}

bool pqProxyPropertyWidget::showLabel() const
{
  return false;
}
