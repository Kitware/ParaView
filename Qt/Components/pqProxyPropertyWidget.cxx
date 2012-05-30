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

#include "pqProxySelectionWidget.h"
#include "pqSelectionInputWidget.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyListDomain.h"

pqProxyPropertyWidget::pqProxyPropertyWidget(vtkSMProperty *smproperty,
                                             vtkSMProxy *proxy,
                                             QWidget *parent)
  : pqPropertyWidget(proxy, parent)
{
  QVBoxLayout *vbox = new QVBoxLayout;

  bool selection_input = (smproperty->GetHints() &&
    smproperty->GetHints()->FindNestedElementByName("SelectionInput"));
  if (selection_input)
    {
    pqSelectionInputWidget* siw = new pqSelectionInputWidget(this);
    siw->setObjectName(proxy->GetPropertyName(smproperty));
    vbox->addWidget(siw);

    this->SelectionInputWidget = siw;

    this->addPropertyLink(siw, "selection",
      SIGNAL(selectionChanged(pqSMProxy)), smproperty);
    
    this->connect(siw, SIGNAL(selectionChanged(pqSMProxy)),
      this, SIGNAL(modified()));

    // don't show label for the proxy selection widget
    this->setShowLabel(false);
    }
  else
    {
    // find the domain
    vtkSMProxyListDomain *domain = 0;
    vtkSMDomainIterator *domainIter = smproperty->NewDomainIterator();
    for (domainIter->Begin(); !domainIter->IsAtEnd() && domain == NULL; domainIter->Next())
      {
      domain = vtkSMProxyListDomain::SafeDownCast(domainIter->GetDomain());
      }
    domainIter->Delete();

    if (domain)
      {
      pqProxySelectionWidget *widget = new pqProxySelectionWidget(proxy,
        proxy->GetPropertyName(smproperty),
        smproperty->GetXMLLabel(),
        this);
      widget->setView(this->view());
      widget->select();
      this->addPropertyLink(widget,
        proxy->GetPropertyName(smproperty),
        SIGNAL(proxyChanged(pqSMProxy)),
        smproperty);
      this->connect(widget, SIGNAL(modified()), this, SIGNAL(modified()));

      vbox->addWidget(widget);

      // store the proxy selection widget so that we can call
      // its accept() method when our apply() is called
      this->ProxySelectionWidget = widget;

      // don't show label for the proxy selection widget
      this->setShowLabel(false);
      }
    }

  this->setLayout(vbox);
}

void pqProxyPropertyWidget::apply()
{
  if (this->SelectionInputWidget)
    {
    this->SelectionInputWidget->preAccept();
    }
  this->Superclass::apply();

  // apply properties for the proxy selection widget
  if(this->ProxySelectionWidget)
    {
    this->ProxySelectionWidget->accept();
    }

  if (this->SelectionInputWidget)
    {
    this->SelectionInputWidget->postAccept();
    }
}
