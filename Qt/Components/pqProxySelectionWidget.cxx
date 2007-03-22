/*=========================================================================

   Program:   ParaView
   Module:    pqProxySelectionWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "pqProxySelectionWidget.h"

// Qt includes
#include <QComboBox>
#include <QDebug>
#include <QPointer>
#include <QLabel>
#include <QStackedLayout>

// VTK/Server Manager includes
#include "vtkSmartPointer.h"
#include "vtkSMProperty.h"
#include "vtkSMDomain.h"

// ParaView GUI includes
#include "pqProxy.h"
#include "pqComboBoxDomain.h"
#include "pqSMAdaptor.h"
#include "pqRenderViewModule.h"
#include "pq3DWidget.h"   // TEMP  need to support more sub panels than pq3DWidget

class pqProxySelectionWidget::pqInternal : public QObject
{
public:
  pqInternal(pqProxySelectionWidget* p) : QObject(p)
  {
  this->Combo = NULL;
  this->DomainObserver = NULL;
  this->Widget = NULL;
  this->Selected = false;
  }
  QComboBox* Combo;
  QPointer<pqProxy> ReferenceProxy;
  QString Property;
  pqComboBoxDomain* DomainObserver;
  pq3DWidget* Widget;
  QPointer<pqRenderViewModule> RenderModule;
  bool Selected;
};

pqProxySelectionWidget::pqProxySelectionWidget(pqProxy* ref_proxy, 
                                     const QString& prop,
                                     const QString& label,
                                     QWidget* p)
  : QWidget(p) 
{
  this->Internal = new pqInternal(this);
  QGridLayout* l = new QGridLayout(this);
  l->setMargin(0);
  this->Internal->Combo = new QComboBox(this);
  if(label.isNull())
    {
    l->addWidget(this->Internal->Combo, 0, 0, 1, 2);
    }
  else
    {
    QLabel* labelWidget = new QLabel(label, this);
    l->addWidget(labelWidget, 0, 0, 1, 1);
    l->addWidget(this->Internal->Combo, 0, 1, 1, 1);
    }

  QObject::connect(this->Internal->Combo,
                   SIGNAL(currentIndexChanged(int)), 
                   this, SLOT(handleProxyChanged()));

  this->Internal->ReferenceProxy = ref_proxy;
  this->Internal->Property = prop;

  this->Internal->DomainObserver = 
    new pqComboBoxDomain(this->Internal->Combo, 
             ref_proxy->getProxy()->GetProperty(prop.toAscii().data()),
             "proxy_list");

}

//-----------------------------------------------------------------------------
pqProxySelectionWidget::~pqProxySelectionWidget()
{
}

//-----------------------------------------------------------------------------
pqSMProxy pqProxySelectionWidget::proxy() const
{
  QList<pqSMProxy> proxies = pqSMAdaptor::getProxyPropertyDomain(
    this->Internal->ReferenceProxy->getProxy()->GetProperty(
      this->Internal->Property.toAscii().data()));

  int index = this->Internal->Combo->currentIndex();
  if (index < 0 || index >= proxies.size())
    {
    return NULL;
    }

  return proxies[index];
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::setProxy(pqSMProxy var)
{
  QList<pqSMProxy> proxies = pqSMAdaptor::getProxyPropertyDomain(
    this->Internal->ReferenceProxy->getProxy()->GetProperty(
      this->Internal->Property.toAscii().data()));

  int index = proxies.indexOf(var.GetPointer());

  if(var.GetPointer() && index != this->Internal->Combo->currentIndex())
    {
    this->Internal->Combo->setCurrentIndex(index);
    }
  else if(var.GetPointer() && index < 0)
    {
    qDebug() << "Selected proxy value not in the list: " << var->GetXMLLabel();
    }
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::handleProxyChanged()
{
  this->initialize3DWidget();
  pqSMProxy p = this->proxy();
  emit this->proxyChanged(p);
}

void pqProxySelectionWidget::initialize3DWidget()
{

  if(this->Internal->Widget)
    {
    delete this->Internal->Widget;
    this->Internal->Widget = NULL;
    }

  if (!this->Internal->ReferenceProxy)
    {
    return;
    }

  vtkSMProxy* smProxy = this->proxy();

  vtkPVXMLElement* hints = (smProxy) ? smProxy->GetHints() : NULL;

  if (!hints)
    {
    // No hints, no 3D widget.
    return;
    }

    // We need to create a widget for the proxy.
  QList<pq3DWidget*> widgets =
    pq3DWidget::createWidgets(this->Internal->ReferenceProxy, smProxy);
  if (widgets.size() > 1)
    {
    qDebug() << "pqProxySelectionWidget currently only supports one "
      " 3D widget per proxy.";
    }
  for (int cc=1; cc < widgets.size(); cc++)
    {
    delete widgets[cc];
    }

  if(widgets.isEmpty())
    {
    return;
    }

  this->Internal->Widget = widgets[0];
  this->Internal->Widget->resetBounds();
  this->Internal->Widget->reset();

  QGridLayout* l = qobject_cast<QGridLayout*>(this->layout());
  l->addWidget(this->Internal->Widget, 1, 0, 1, 2);

  QObject::connect(this->Internal->Widget, SIGNAL(widgetChanged()), 
    this, SIGNAL(modified()));

  this->Internal->Widget->setRenderModule(this->Internal->RenderModule);
  this->Internal->Widget->show();
  
  if (this->Internal->Selected)
    {
    this->Internal->Widget->select();
    }
  else
    {
    this->Internal->Widget->deselect();
    }
}

void pqProxySelectionWidget::select()
{
  this->Internal->Selected = true;
  if(this->Internal->Widget)
    {
    this->Internal->Widget->select();
    }
}

void pqProxySelectionWidget::deselect()
{
  this->Internal->Selected = false;
  if(this->Internal->Widget)
    {
    this->Internal->Widget->deselect();
    }
}

void pqProxySelectionWidget::accept()
{
  if(this->Internal->Widget)
    {
    this->Internal->Widget->accept();
    }
}

void pqProxySelectionWidget::reset()
{
  if(this->Internal->Widget)
    {
    this->Internal->Widget->reset();
    }
}

void pqProxySelectionWidget::setRenderModule(pqRenderViewModule* rm)
{
  this->Internal->RenderModule = rm;
  if(this->Internal->Widget)
    {
    this->Internal->Widget->setRenderModule(rm);
    }
}


