/*=========================================================================

   Program: ParaView
   Module:    pqSMSignalAdaptors.cxx

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

// self includes
#include "pqSMSignalAdaptors.h"

// Qt includes
#include <QLayout>
#include <QMap>
#include <QPointer>
#include <QtDebug>
#include <QWidget>

// SM includes
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyManager.h"

// ParaView Server Manager includes
#include "pq3DWidget.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqSMProxy.h"

pqSignalAdaptorProxy::pqSignalAdaptorProxy(QObject* p, 
              const char* colorProperty, const char* signal)
  : QObject(p), PropertyName(colorProperty)
{
  QObject::connect(p, signal,
                   this, SLOT(handleProxyChanged()));
}

QVariant pqSignalAdaptorProxy::proxy() const
{
  QString str = this->parent()->property(this->PropertyName).toString();
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  pqSMProxy p = pm->GetProxy(str.toAscii().data());
  QVariant ret;
  ret.setValue(p);
  return ret;
}

void pqSignalAdaptorProxy::setProxy(const QVariant& var)
{
  if(var != this->proxy())
    {
    pqSMProxy p = var.value<pqSMProxy>();
    /*
    // TODO:  would like to use vtkSMProxyManager
    vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
    QString name = pm->GetProxyName(p);
    */
    pqPipelineSource* o = pqServerManagerModel::instance()->getPQSource(p);
    if(o)
      {
      QString name = o->getProxyName();
      this->parent()->setProperty(this->PropertyName, QVariant(name));
      }
    }
}

void pqSignalAdaptorProxy::handleProxyChanged()
{
  QVariant p = this->proxy();
  emit this->proxyChanged(p);
}

//*****************************************************************************
class pqSignalAdaptorProxyListInternal
{
public:
  vtkSmartPointer<vtkSMProxyListDomain> Domain;
  QPointer<pqProxy> ReferenceProxy;
  QPointer<QWidget> WidgetFrame;

  typedef QMap<vtkSMProxy*, QPointer<pq3DWidget> > MapOf3DWidgets;
  MapOf3DWidgets Widgets;
  QPointer<pq3DWidget> ActiveWidget;

  pqSignalAdaptorProxyListInternal()
    {
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
//-----------------------------------------------------------------------------
pqSignalAdaptorProxyList::pqSignalAdaptorProxyList(QObject* p, 
              const char* colorProperty, const char* signal)
  : QObject(p), PropertyName(colorProperty)
{
  this->Internal = new pqSignalAdaptorProxyListInternal();

  QObject::connect(p, signal, this, SLOT(handleProxyChanged()));
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
void pqSignalAdaptorProxyList::setReferenceProxy(pqProxy* _proxy)
{
  this->Internal->ReferenceProxy = _proxy;
  this->initialize3DWidget();
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorProxyList::setProperty(vtkSMProperty* property)
{
  this->Internal->Domain = vtkSMProxyListDomain::SafeDownCast(
    property->GetDomain("proxy_list"));
  this->initialize3DWidget();
}

//-----------------------------------------------------------------------------
QVariant pqSignalAdaptorProxyList::proxy() const
{
  int index = this->parent()->property(this->PropertyName).toInt();
  pqSMProxy p = this->Internal->Domain->GetProxy(index); 
  QVariant ret;
  ret.setValue(p);
  return ret;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorProxyList::setProxy(const QVariant& var)
{
  if(var != this->proxy())
    {
    pqSMProxy p = var.value<pqSMProxy>();
    int index = this->Internal->Domain->GetIndex(p);
    if (index < 0)
      {
      index = 0;
      }
    this->parent()->setProperty(this->PropertyName, QVariant(index));
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
  vtkPVXMLElement* hints = smProxy->GetHints();

  if (!hints)
    {
    // No hints, no 3D widget.
    this->Internal->WidgetFrame->hide();
    if (this->Internal->ActiveWidget)
      {
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
          this->Internal->ActiveWidget->deselect();
          this->Internal->ActiveWidget = 0;
          }
      return;
      }
    this->Internal->Widgets.insert(smProxy, widget3D);
    this->Internal->WidgetFrame->layout()->addWidget(widget3D);
    widget3D->setReferenceProxy(this->Internal->ReferenceProxy);
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
    }

  this->Internal->ActiveWidget = widget3D;
  this->Internal->ActiveWidget->select();
  this->Internal->WidgetFrame->setProperty("title", 
    QString(smProxy->GetXMLName()) + " Widget");
  this->Internal->WidgetFrame->show();
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
  return widgets[0];
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorProxyList::select()
{
  if (this->Internal->ActiveWidget)
    {
    this->Internal->ActiveWidget->select();
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorProxyList::deselect()
{
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

//*****************************************************************************
pqSignalAdaptorDouble::pqSignalAdaptorDouble(QObject* _parent,
  const char* _property, const char* signal): QObject(_parent)
{
  this->PropertyName = _property;
  QObject::connect(_parent, signal, this, SLOT(objectSignalFired()));
}

//-----------------------------------------------------------------------------
pqSignalAdaptorDouble::~pqSignalAdaptorDouble()
{
}

//-----------------------------------------------------------------------------
QString pqSignalAdaptorDouble::value()
{
  bool convertible = false;
  double dValue = 
    QVariant(this->parent()->property(
        this->PropertyName.toStdString().c_str())).toDouble(&convertible);
  return convertible? QString::number(dValue, 'g', 3): "nan";
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorDouble::setValue(const QString &string)
{
  QVariant sValue(string);
  bool convertible = false;
  double dValue = sValue.toDouble(&convertible);
  this->parent()->setProperty(this->PropertyName.toStdString().c_str(),
    (convertible)?QString::number(dValue, 'g', 3): "nan");
}
//-----------------------------------------------------------------------------
void pqSignalAdaptorDouble::objectSignalFired()
{
  QString val = this->value();
  emit this->valueChanged(val);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
