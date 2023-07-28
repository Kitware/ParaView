// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

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
  pqSMProxy p = pm->GetProxy(str.toUtf8().data());
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
