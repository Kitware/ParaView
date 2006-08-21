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
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
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
    if(p)
      {
      pqPipelineSource* o = pqServerManagerModel::instance()->getPQSource(p);
      if(o)
        {
        QString name = o->getProxyName();
        this->parent()->setProperty(this->PropertyName, QVariant(name));
        }
      }
    }
}

void pqSignalAdaptorProxy::handleProxyChanged()
{
  QVariant p = this->proxy();
  emit this->proxyChanged(p);
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
