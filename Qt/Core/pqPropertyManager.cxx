// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

// self include
#include "pqPropertyManager.h"

// Qt includes
#include <QApplication>
#include <QByteArray>
#include <QMultiMap>

// ParaView includes
#include "pqPropertyLinks.h"

//-----------------------------------------------------------------------------
pqPropertyManager::pqPropertyManager(QObject* p)
  : QObject(p)
{
  this->Links = new pqPropertyLinks();
  this->Links->setUseUncheckedProperties(true);
  this->Links->setAutoUpdateVTKObjects(false);
  QObject::connect(this->Links, SIGNAL(qtWidgetChanged()), this, SLOT(propertyChanged()));
  this->Modified = false;
}

//-----------------------------------------------------------------------------
pqPropertyManager::~pqPropertyManager()
{
  delete this->Links;
  this->Links = nullptr;
}

//-----------------------------------------------------------------------------
void pqPropertyManager::registerLink(QObject* qObject, const char* qProperty, const char* signal,
  vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  this->Links->addPropertyLink(qObject, qProperty, signal, Proxy, Property, Index);
}

//-----------------------------------------------------------------------------
void pqPropertyManager::unregisterLink(QObject* qObject, const char* qProperty, const char* signal,
  vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  this->Links->removePropertyLink(qObject, qProperty, signal, Proxy, Property, Index);
}

//-----------------------------------------------------------------------------
void pqPropertyManager::removeAllLinks()
{
  this->Links->removeAllPropertyLinks();
}

//-----------------------------------------------------------------------------
void pqPropertyManager::accept()
{
  Q_EMIT this->aboutToAccept();
  this->Links->accept();
  Q_EMIT this->accepted();
  this->Modified = false;
}

//-----------------------------------------------------------------------------
void pqPropertyManager::reject()
{
  this->Links->reset();
  Q_EMIT this->rejected();
  this->Modified = false;
}

//-----------------------------------------------------------------------------
void pqPropertyManager::propertyChanged()
{
  this->Modified = true;
  Q_EMIT this->modified();
}

//-----------------------------------------------------------------------------
bool pqPropertyManager::isModified() const
{
  return this->Modified;
}
