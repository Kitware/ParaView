/*=========================================================================

   Program: ParaView
   Module:    pqSMSignalAdaptors.cxx

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

// self includes
#include "pqSMSignalAdaptors.h"

// Qt includes
#include <QLayout>
#include <QMap>
#include <QPointer>
#include <QWidget>
#include <QtDebug>

// SM includes
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"

// ParaView Server Manager includes
#include "pqApplicationCore.h"
#include "pqPipelineFilter.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqSMProxy.h"
#include "pqServerManagerModel.h"

pqSignalAdaptorProxy::pqSignalAdaptorProxy(
  QObject* p, const char* colorProperty, const char* signal)
  : QObject(p)
  , PropertyName(colorProperty)
{
  QObject::connect(p, signal, this, SLOT(handleProxyChanged()));
}

QVariant pqSignalAdaptorProxy::proxy() const
{
  QString str = this->parent()->property(this->PropertyName).toString();
  vtkSMSessionProxyManager* pm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  pqSMProxy p = pm->GetProxy(str.toLocal8Bit().data());
  QVariant ret;
  ret.setValue(p);
  return ret;
}

void pqSignalAdaptorProxy::setProxy(const QVariant& var)
{
  if (var != this->proxy())
  {
    pqSMProxy p = var.value<pqSMProxy>();
    if (p)
    {
      pqPipelineSource* o =
        pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>(p);
      if (o)
      {
        QString name = o->getSMName();
        this->parent()->setProperty(this->PropertyName, QVariant(name));
      }
    }
  }
}

void pqSignalAdaptorProxy::handleProxyChanged()
{
  QVariant p = this->proxy();
  Q_EMIT this->proxyChanged(p);
}
