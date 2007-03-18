/*=========================================================================

   Program:   ParaView
   Module:    pqSignalAdaptorProxyList.cxx

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
#include "pqSignalAdaptorProxyList.h"

// Qt includes
#include <QLayout>
#include <QMap>
#include <QPointer>
#include <QtDebug>
#include <QWidget>
#include <QComboBox>

// SM includes
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyManager.h"

// ParaView Server Manager includes
#include "pq3DWidget.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPipelineModel.h"
#include "pqRenderViewModule.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqSMAdaptor.h"
#include "pqSMProxy.h"

class pqSignalAdaptorProxyListInternal
{
public:
  vtkSmartPointer<vtkSMProxyListDomain> Domain;
  QString SMPropertyName;
  QPointer<pqProxy> ReferenceProxy;
  QPointer<QWidget> WidgetFrame;
  QPointer<QComboBox> ComboBox;
  QPointer<pqRenderViewModule> RenderModule;

  typedef QMap<vtkSMProxy*, QPointer<pq3DWidget> > MapOf3DWidgets;
  MapOf3DWidgets Widgets;
  QPointer<pq3DWidget> ActiveWidget;

  bool Selected;

  pqSignalAdaptorProxyListInternal()
    {
    this->Selected = false;
    }

  ~pqSignalAdaptorProxyListInternal()
    {
    this->ActiveWidget = 0;
    foreach (pq3DWidget* widget, this->Widgets)
      {
      delete widget;
      }
    }
};

//*****************************************************************************
pqSignalAdaptorProxyList::pqSignalAdaptorProxyList(QComboBox* combobox, 
              pqProxy* ref_proxy, const QString& smproperty_name)
  : QObject(combobox) 
{
  this->Internal = new pqSignalAdaptorProxyListInternal();
  this->Internal->ComboBox = combobox;
  QObject::connect(combobox, SIGNAL(currentIndexChanged(const QString&)), 
    this, SLOT(handleProxyChanged()));

  this->Internal->ReferenceProxy = ref_proxy;
  vtkSMProperty* prop;
  prop = ref_proxy->getProxy()->GetProperty(smproperty_name.toAscii().data());
  if (prop)
    {
    this->Internal->Domain = vtkSMProxyListDomain::SafeDownCast(
      prop->GetDomain("proxy_list"));
    }
  this->Internal->SMPropertyName = smproperty_name;

  this->createProxyList();
  this->initialize3DWidget();
}

//-----------------------------------------------------------------------------
pqSignalAdaptorProxyList::~pqSignalAdaptorProxyList()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorProxyList::setWidgetFrame(QWidget* frame)
{
  this->Internal->WidgetFrame = frame;
  this->initialize3DWidget();
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorProxyList::createProxyList()
{
  if (!this->Internal->Domain)
    {
    qDebug() << "pqSignalAdaptorProxyList cannot be used on a property without a "
      "proxy list domain.";
    return;
    }

  this->Internal->ComboBox->blockSignals(true);
  this->Internal->ComboBox->clear();

  QList<pqSMProxy> proxies = pqSMAdaptor::getProxyPropertyDomain(
    this->Internal->ReferenceProxy->getProxy()->GetProperty(
      this->Internal->SMPropertyName.toAscii().data()));
  foreach(vtkSMProxy* _proxy, proxies)
    {
    this->Internal->ComboBox->addItem(_proxy->GetXMLLabel());
    }
  this->Internal->ComboBox->blockSignals(false);
  this->Internal->ComboBox->setCurrentIndex(-1);
}

//-----------------------------------------------------------------------------
QVariant pqSignalAdaptorProxyList::proxy() const
{
  QList<pqSMProxy> proxies = pqSMAdaptor::getProxyPropertyDomain(
    this->Internal->ReferenceProxy->getProxy()->GetProperty(
      this->Internal->SMPropertyName.toAscii().data()));

  int index = this->Internal->ComboBox->currentIndex();
  if (index < 0 || index >= proxies.size())
    {
    return QVariant(); 
    }

  pqSMProxy selected_proxy = proxies[index];
  QVariant ret;
  ret.setValue(selected_proxy);
  return ret;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorProxyList::setProxy(const QVariant& var)
{
  if(var != this->proxy())
    {
    pqSMProxy p = var.value<pqSMProxy>();
    int index = -1;
    if (p.GetPointer())
      {
      QList<pqSMProxy> proxies = pqSMAdaptor::getProxyPropertyDomain(
        this->Internal->ReferenceProxy->getProxy()->GetProperty(
          this->Internal->SMPropertyName.toAscii().data()));
      index = proxies.indexOf(p.GetPointer());
      if (index < 0)
        {
        qDebug() << "Selected proxy value not in the list: " << p.GetPointer();
        return;
        }
      }
    this->Internal->ComboBox->blockSignals(true);
    this->Internal->ComboBox->setCurrentIndex(index);
    this->initialize3DWidget();
    this->Internal->ComboBox->blockSignals(false);
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorProxyList::setRenderModule(pqRenderViewModule* renModule)
{
  this->Internal->RenderModule = renModule;
  if (this->Internal->ActiveWidget)
    {
    this->Internal->ActiveWidget->setRenderModule(renModule);
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorProxyList::initialize3DWidget()
{
  if (!this->Internal->WidgetFrame || !this->Internal->ReferenceProxy ||
    !this->Internal->Domain)
    {
    return;
    }

  vtkSMProxy* smProxy = this->proxy().value<pqSMProxy>();

  vtkPVXMLElement* hints = (smProxy)? smProxy->GetHints() : NULL;

  if (!hints)
    {
    // No hints, no 3D widget.
    this->Internal->WidgetFrame->hide();
    if (this->Internal->ActiveWidget)
      {
      this->Internal->ActiveWidget->hide();
      this->Internal->ActiveWidget->setRenderModule(0);
      this->Internal->ActiveWidget->deselect();
      this->Internal->ActiveWidget = 0;
      }
    return;
    }

  if (!this->Internal->Widgets.contains(smProxy))
    {
    // We need to create a widget for the proxy.
    pq3DWidget* widget3D = this->new3DWidget(smProxy);
    if (!widget3D)
      {
      this->Internal->WidgetFrame->hide();
        if (this->Internal->ActiveWidget)
          {
          this->Internal->ActiveWidget->hide();
          this->Internal->ActiveWidget->setRenderModule(0);
          this->Internal->ActiveWidget->deselect();
          this->Internal->ActiveWidget = 0;
          }
      return;
      }
    this->Internal->Widgets.insert(smProxy, widget3D);
    this->Internal->WidgetFrame->layout()->addWidget(widget3D);
    QObject::connect(widget3D, SIGNAL(widgetChanged()), 
      this, SIGNAL(modified()));
    }

  pq3DWidget* widget3D = this->Internal->Widgets.value(smProxy);
  if (widget3D == this->Internal->ActiveWidget)
    {
    return;
    }
  else if (this->Internal->ActiveWidget)
    {
    this->Internal->ActiveWidget->deselect();
    this->Internal->ActiveWidget->hide();
    this->Internal->ActiveWidget->setRenderModule(0);
    this->Internal->ActiveWidget = 0;
    }

  this->Internal->ActiveWidget = widget3D;
  this->Internal->ActiveWidget->setRenderModule(this->Internal->RenderModule);
  this->Internal->WidgetFrame->setProperty("title", 
    QString(smProxy->GetXMLName()) + " Widget");
  this->Internal->WidgetFrame->show();
  this->Internal->ActiveWidget->show();
  if (this->Internal->Selected)
    {
    this->Internal->ActiveWidget->select();
    }
  else
    {
    this->Internal->ActiveWidget->deselect();
    }
}

//-----------------------------------------------------------------------------
pq3DWidget* pqSignalAdaptorProxyList::new3DWidget(vtkSMProxy* smProxy)
{
  QList<pq3DWidget*> widgets =
    pq3DWidget::createWidgets(smProxy);
  if (widgets.size() == 0)
    {
    return 0;
    }
  if (widgets.size() > 1)
    {
    qDebug() << "pqSignalAdaptorProxyList currently only supports one "
      " 3D widget per proxy.";
    }
  for (int cc=1; cc < widgets.size(); cc++)
    {
    delete widgets[cc];
    }

  widgets[0]->setParent(this->Internal->WidgetFrame);
  widgets[0]->setReferenceProxy(this->Internal->ReferenceProxy);
  // We implicitly reset the widget bounds the first time the widget is shown.
  widgets[0]->resetBounds();
  widgets[0]->reset();
  return widgets[0];
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorProxyList::select()
{
  this->Internal->Selected = true;
  if (this->Internal->ActiveWidget)
    {
    this->Internal->ActiveWidget->select();
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorProxyList::deselect()
{
  this->Internal->Selected = false;
  if (this->Internal->ActiveWidget)
    {
    this->Internal->ActiveWidget->deselect();
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorProxyList::accept()
{
  if (this->Internal->ActiveWidget)
    {
    this->Internal->ActiveWidget->accept();
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorProxyList::reset()
{
  if (this->Internal->ActiveWidget)
    {
    this->Internal->ActiveWidget->reset();
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorProxyList::handleProxyChanged()
{
  QVariant p = this->proxy();
  this->initialize3DWidget();
  emit this->proxyChanged(p);
  emit this->modified();
}
