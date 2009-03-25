/*=========================================================================

   Program:   ParaView
   Module:    pqProxySelectionWidget.cxx

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
#include "pqView.h"
#include "pq3DWidget.h"
#include "pqNamedWidgets.h"
#include "pqCollapsedGroup.h"
#include "pqNamedWidgets.h"

//-----------------------------------------------------------------------------
class pqProxySelectionWidget::pqInternal
{
public:
  pqInternal()
    {
    this->Combo = NULL;
    this->DomainObserver = NULL;
    this->Widget = NULL;
    this->Selected = false;
    }
  QComboBox* Combo;
  vtkSMProxy* ReferenceProxy;
  QString Property;
  pqComboBoxDomain* DomainObserver;
  pqProxyPanel* Widget;
  QPointer<pqView> View;
  bool Selected;

  typedef QMap<vtkSMProxy*, pqProxyPanel*> PanelMap;
  PanelMap Panels;
};

//-----------------------------------------------------------------------------
pqProxySelectionWidget::pqProxySelectionWidget(vtkSMProxy* ref_proxy, 
                                     const QString& prop,
                                     const QString& label,
                                     QWidget* p)
  : QWidget(p) 
{
  this->Internal = new pqInternal();
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

  this->Internal->DomainObserver = new pqComboBoxDomain(this->Internal->Combo,
    ref_proxy->GetProperty(prop.toAscii().data()), "proxy_list");
}

//-----------------------------------------------------------------------------
pqProxySelectionWidget::~pqProxySelectionWidget()
{
  foreach (pqProxyPanel* panel, this->Internal->Panels)
    {
    delete panel;
    }
  this->Internal->Panels.clear();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
pqSMProxy pqProxySelectionWidget::proxy() const
{
  QList<pqSMProxy> proxies = pqSMAdaptor::getProxyPropertyDomain(
    this->Internal->ReferenceProxy->GetProperty(
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
    this->Internal->ReferenceProxy->GetProperty(
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

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::initialize3DWidget()
{
  pqProxyPanel* panel = qobject_cast<pqProxyPanel*>(this->parentWidget());

  if (this->Internal->Widget)
    {
    this->Internal->Widget->deselect();
    this->Internal->Widget->setView(0);
    this->Internal->Widget->hide();
    QObject::disconnect(panel, 0, this->Internal->Widget, 0);
    this->Internal->Widget = NULL;
    }

  if (!this->Internal->ReferenceProxy)
    {
    return;
    }

  vtkSMProxy* smProxy = this->proxy();
  // If there's a sub-panel already created for this proxy, don't create a new
  // one.
  this->Internal->Widget = this->Internal->Panels[smProxy];
  if (this->Internal->Widget)
    {
    pq3DWidget* widget3D = qobject_cast<pq3DWidget*>(this->Internal->Widget);
    if (widget3D)
      {
      widget3D->resetBounds();
      widget3D->reset();
      }
    }
  else
    {
    vtkPVXMLElement* hints = (smProxy) ? smProxy->GetHints() : NULL;
    if (hints)
      {
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
      if(!widgets.isEmpty())
        {
        pq3DWidget* w = widgets[0];
        this->Internal->Widget = w;
        w->resetBounds();
        w->reset();

        QGridLayout* l = qobject_cast<QGridLayout*>(this->layout());
        l->addWidget(w, 1, 0, 1, 2);
        }
      }

    // auto generate one
    if(!this->Internal->Widget)
      {
      pqProxyPanel* sub_panel = new pqProxyPanel(smProxy, this);
      pqCollapsedGroup* group = new pqCollapsedGroup(sub_panel);
      QGridLayout* gl = new QGridLayout(sub_panel);
      gl->setMargin(0);
      gl->addWidget(group);
      gl = new QGridLayout(group);
      group->setTitle(smProxy->GetXMLLabel());
      gl->setMargin(2);
      pqNamedWidgets::createWidgets(gl, smProxy);
      if (gl->rowCount() <= 2)
        {
        // no widgets were added for the proxy...so don't show any sub_panel.
        delete sub_panel;
        }
      else
        {
        pqNamedWidgets::link(group, smProxy, sub_panel->propertyManager());
        QGridLayout* l = qobject_cast<QGridLayout*>(this->layout());
        this->Internal->Widget = sub_panel;
        l->addWidget(this->Internal->Widget, 1, 0, 1, 2);
        }
      }
    }
 
  if (!this->Internal->Widget)
    {
    return;
    }

  // Save the panel for later.
  this->Internal->Panels[smProxy] = this->Internal->Widget;
  
  QObject::connect(panel, SIGNAL(onselect()),
                   this->Internal->Widget, SLOT(select()));
  QObject::connect(panel, SIGNAL(ondeselect()),
                   this->Internal->Widget, SLOT(deselect()));
  QObject::connect(panel, SIGNAL(onaccept()),
                   this->Internal->Widget, SLOT(accept()));
  QObject::connect(panel, SIGNAL(onreset()),
                   this->Internal->Widget, SLOT(reset()));
  QObject::connect(this->Internal->Widget, SIGNAL(modified()), 
                   panel, SLOT(setModified()));
  QObject::connect(panel, SIGNAL(viewChanged(pqView*)),
                   this->Internal->Widget, SLOT(setView(pqView*)));
  
  this->Internal->Widget->setView(this->Internal->View);
  if (this->Internal->Selected)
    {
    this->Internal->Widget->select();
    }
  else
    {
    this->Internal->Widget->deselect();
    }
  this->Internal->Widget->show();
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::select()
{
  this->Internal->Selected = true;
  if(this->Internal->Widget)
    {
    this->Internal->Widget->select();
    }
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::deselect()
{
  this->Internal->Selected = false;
  if(this->Internal->Widget)
    {
    this->Internal->Widget->deselect();
    }
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::accept()
{
  if(this->Internal->Widget)
    {
    this->Internal->Widget->accept();
    }
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::reset()
{
  if(this->Internal->Widget)
    {
    this->Internal->Widget->reset();
    }
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::setView(pqView* rm)
{
  this->Internal->View = rm;
  if(this->Internal->Widget)
    {
    this->Internal->Widget->setView(rm);
    }
}


