/*=========================================================================

   Program: ParaView
   Module:    pqAddToFavoritesReaction.cxx

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
#include "pqAddToFavoritesReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqPipelineFilter.h"
#include "pqSettings.h"

#include "vtkSMProxy.h"

#include <QDebug>
//-----------------------------------------------------------------------------
pqAddToFavoritesReaction::pqAddToFavoritesReaction(QAction* parentObject, QVector<QString>& filters)
  : Superclass(parentObject)
{
  this->Filters.swap(filters);

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(updateEnableState()));

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqAddToFavoritesReaction::updateEnableState()
{
  pqPipelineFilter* filter =
    qobject_cast<pqPipelineFilter*>(pqActiveObjects::instance().activeSource());
  if (filter == NULL || filter->modifiedState() == pqProxy::UNINITIALIZED)
  {
    this->parentAction()->setEnabled(false);
    return;
  }

  this->parentAction()->setEnabled(!this->Filters.contains(filter->getProxy()->GetXMLName()));
}

//-----------------------------------------------------------------------------
void pqAddToFavoritesReaction::addToFavorites(QAction* parent)
{
  pqPipelineFilter* filter =
    qobject_cast<pqPipelineFilter*>(pqActiveObjects::instance().activeSource());

  if (!filter)
  {
    qCritical() << "No active filter.";
    return;
  }

  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString key = QString("favorites.%1/").arg("ParaViewFilters");

  vtkSMProxy* proxy = filter->getProxy();
  QString filterId = QString("%1;%2;%3;%4")
                       .arg(proxy->GetXMLGroup())
                       .arg(parent->data().toString())
                       .arg(proxy->GetXMLLabel())
                       .arg(proxy->GetXMLName());

  QString value;
  if (settings->contains(key))
  {
    value = settings->value(key).toString();
  }
  QString settingValue = settings->value(key).toString();
  QStringList bmList = settingValue.split("|", QString::SkipEmptyParts);
  for (QString bm : bmList)
  {
    if (bm == filterId)
    {
      return;
    }
  }
  value += (filterId + QString("|"));
  settings->setValue(key, value);
}
