/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqPropertyLinks2.h"

#include "pqPropertyLinks2Connection.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

#include <QMap>
#include <QtDebug>
#include <set>

class pqPropertyLinks2::pqInternals
{
public:
  QList<QPointer<pqPropertyLinks2Connection> > Connections;
  pqInternals() { }
};

//-----------------------------------------------------------------------------
pqPropertyLinks2::pqPropertyLinks2(QObject* parentObject)
  : Superclass(parentObject),
  Internals(new pqPropertyLinks2::pqInternals()),
  UseUncheckedProperties(false),
  AutoUpdateVTKObjects(false)
{
}

//-----------------------------------------------------------------------------
pqPropertyLinks2::~pqPropertyLinks2()
{
  // this really isn't necessary, but no harm done.
  this->clear();

  delete this->Internals;
  this->Internals = 0;
}

//-----------------------------------------------------------------------------
bool pqPropertyLinks2::addPropertyLink(
  QObject* qobject, const char* qproperty, const char* qsignal,
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex)
{
  if (!qobject || !qproperty || !qsignal || !smproxy || !smproperty)
    {
    qWarning() << "Invalid parameters to pqPropertyLinks2::addPropertyLink";
    qWarning() << "(" << qobject << ", " << qproperty << ", " << qsignal
      << ") <==> ("
      << (smproxy? smproxy->GetXMLName() : "(none)")
      << "," << (smproperty? smproperty->GetXMLLabel() : "(none)")
      << smindex << ")";
    return false;
    }

  pqPropertyLinks2Connection* connection = new pqPropertyLinks2Connection(
    qobject, qproperty, qsignal, smproxy, smproperty, smindex, this);

  // Avoid adding duplicates.
  foreach (pqPropertyLinks2Connection* existing, this->Internals->Connections)
    {
    if (*existing == *connection)
      {
      qWarning() << "Skipping duplicate connection: "
        << "(" << qobject << ", " << qproperty << ", " << qsignal
        << ") <==> ("
        << smproxy << "," << (smproperty? smproperty->GetXMLLabel() : "(none)")
        << smindex;
      delete connection;
      return true;
      }
    }

  this->Internals->Connections.push_back(connection);

  // initialize the Qt widget using the SMProperty values.
  connection->copyValuesFromServerManagerToQt(this->useUncheckedProperties());

  QObject::connect(connection, SIGNAL(qtpropertyModified()),
    this, SLOT(onQtPropertyModified()));
  QObject::connect(connection, SIGNAL(smpropertyModified()),
    this, SLOT(onSMPropertyModified()));
  return true;
}

//-----------------------------------------------------------------------------
bool pqPropertyLinks2::removePropertyLink(
  QObject* qobject, const char* qproperty, const char* qsignal,
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex)
{
  if (!qobject || !qproperty || !qsignal || !smproxy || !smproperty)
    {
    qWarning() << "Invalid parameters to pqPropertyLinks2::addPropertyLink";
    qWarning() << "(" << qobject << ", " << qproperty << ", " << qsignal
      << ") <==> ("
      << (smproxy? smproxy->GetXMLName() : "(none)")
      << "," << (smproperty? smproperty->GetXMLLabel() : "(none)")
      << smindex << ")";
    return false;
    }

  pqPropertyLinks2Connection* connection = new pqPropertyLinks2Connection(
    qobject, qproperty, qsignal, smproxy, smproperty, smindex, this);

  // Avoid adding duplicates.
  foreach (pqPropertyLinks2Connection* existing, this->Internals->Connections)
    {
    if (*existing == *connection)
      {
      this->Internals->Connections.removeOne(existing);
      delete connection;
      return true;
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
void pqPropertyLinks2::clear()
{
  foreach (pqPropertyLinks2Connection* connection, this->Internals->Connections)
    {
    delete connection;
    }
  this->Internals->Connections.clear();
}

//-----------------------------------------------------------------------------
void pqPropertyLinks2::onQtPropertyModified()
{
  pqPropertyLinks2Connection* connection =
    qobject_cast<pqPropertyLinks2Connection*>(this->sender());
  if (connection)
    {
    connection->copyValuesFromQtToServerManager(
      this->useUncheckedProperties());
    if (this->autoUpdateVTKObjects() && connection->proxy())
      {
      connection->proxy()->UpdateVTKObjects();
      }
    emit this->qtWidgetChanged();
    }
}

//-----------------------------------------------------------------------------
void pqPropertyLinks2::onSMPropertyModified()
{
  pqPropertyLinks2Connection* connection =
    qobject_cast<pqPropertyLinks2Connection*>(this->sender());
  if (connection)
    {
    connection->copyValuesFromServerManagerToQt(
      this->useUncheckedProperties());
    emit this->smPropertyChanged();
    }
}

//-----------------------------------------------------------------------------
void pqPropertyLinks2::accept()
{
  std::set<vtkSMProxy*> proxies_to_update;
  foreach (pqPropertyLinks2Connection* connection, this->Internals->Connections)
    {
    if (connection && connection->proxy())
      {
      connection->copyValuesFromQtToServerManager(false);
      proxies_to_update.insert(connection->proxy());
      }
    }

  for (std::set<vtkSMProxy*>::iterator iter = proxies_to_update.begin();
    iter != proxies_to_update.end(); ++iter)
    {
    (*iter)->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void pqPropertyLinks2::reset()
{
  foreach (pqPropertyLinks2Connection* connection, this->Internals->Connections)
    {
    if (connection && connection->proxy())
      {
      connection->copyValuesFromServerManagerToQt(false);
      }
    }
}
