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
#include "pqPropertyLinks.h"

#include "pqPropertyLinksConnection.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

#include <QMap>
#include <QtDebug>

#include <cassert>
#include <set>

class pqPropertyLinks::pqInternals
{
public:
  QList<QPointer<pqPropertyLinksConnection> > Connections;
  bool IgnoreSMChanges;
  pqInternals()
    : IgnoreSMChanges(false)
  {
  }
};

//-----------------------------------------------------------------------------
pqPropertyLinks::pqPropertyLinks(QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqPropertyLinks::pqInternals())
  , UseUncheckedProperties(false)
  , AutoUpdateVTKObjects(true)
{
}

//-----------------------------------------------------------------------------
pqPropertyLinks::~pqPropertyLinks()
{
  // this really isn't necessary, but no harm done.
  this->clear();

  delete this->Internals;
  this->Internals = 0;
}

//-----------------------------------------------------------------------------
bool pqPropertyLinks::addPropertyLink(QObject* qobject, const char* qproperty, const char* qsignal,
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex)
{
  return this->addPropertyLink<pqPropertyLinksConnection>(
    qobject, qproperty, qsignal, smproxy, smproperty, smindex);
}

//-----------------------------------------------------------------------------
bool pqPropertyLinks::addTraceablePropertyLink(QObject* qobject, const char* qproperty,
  const char* qsignal, vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex)
{
  return this->addTraceablePropertyLink<pqPropertyLinksConnection>(
    qobject, qproperty, qsignal, smproxy, smproperty, smindex);
}

//-----------------------------------------------------------------------------
void pqPropertyLinks::setUseUncheckedProperties(bool val)
{
  if (val == this->UseUncheckedProperties)
  {
    // nothing to change
    return;
  }

  foreach (pqPropertyLinksConnection* connection, this->Internals->Connections)
  {
    connection->setUseUncheckedProperties(val);
  }

  this->UseUncheckedProperties = val;
}

//-----------------------------------------------------------------------------
bool pqPropertyLinks::addNewConnection(pqPropertyLinksConnection* connection)
{
  assert(connection);

  // Avoid adding duplicates.
  foreach (pqPropertyLinksConnection* existing, this->Internals->Connections)
  {
    if (*existing == *connection)
    {
      vtkSMProperty* smproperty = connection->propertySM();
      qDebug() << "Skipping duplicate connection: "
               << "(" << connection->objectQt() << ", " << connection->propertyQt() << ") <==> ("
               << connection->proxySM() << ","
               << (smproperty ? smproperty->GetXMLLabel() : "(none)") << connection->indexSM();
      delete connection;
      return true;
    }
  }

  this->Internals->Connections.push_back(connection);

  // initialize the Qt widget using the SMProperty values.
  connection->copyValuesFromServerManagerToQt(this->useUncheckedProperties());

  QObject::connect(connection, SIGNAL(qtpropertyModified()), this, SLOT(onQtPropertyModified()));
  QObject::connect(connection, SIGNAL(smpropertyModified()), this, SLOT(onSMPropertyModified()));
  return true;
}

//-----------------------------------------------------------------------------
bool pqPropertyLinks::removePropertyLink(QObject* qobject, const char* qproperty,
  const char* qsignal, vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex)
{
  // remove has to be a little flexible about input arguments. It accepts null
  // qobject and smproperty.

  pqPropertyLinksConnection connection(qobject, qproperty, qsignal, smproxy, smproperty, smindex,
    this->useUncheckedProperties(), this);

  // Avoid adding duplicates.
  foreach (pqPropertyLinksConnection* existing, this->Internals->Connections)
  {
    if (*existing == connection)
    {
      this->Internals->Connections.removeOne(existing);
      delete existing;
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqPropertyLinks::clear()
{
  foreach (pqPropertyLinksConnection* connection, this->Internals->Connections)
  {
    delete connection;
  }
  this->Internals->Connections.clear();
}

//-----------------------------------------------------------------------------
void pqPropertyLinks::onQtPropertyModified()
{
  pqPropertyLinksConnection* connection = qobject_cast<pqPropertyLinksConnection*>(this->sender());
  if (connection)
  {
    connection->copyValuesFromQtToServerManager(this->useUncheckedProperties());
    if (this->autoUpdateVTKObjects() && connection->proxy())
    {
      connection->proxy()->UpdateVTKObjects();
    }
    Q_EMIT this->qtWidgetChanged();

    // although unintuitive, several panels e.g. pqDisplayProxyEditor expect
    // smPropertyChanged() whenever the SMProperty is chnaged, either by the GUI
    // or from ServerManager itself.
    // pqPropertyLinksConnection::copyValuesFromQtToServerManager() doesn't fire
    // any smpropertyModified() signal. It is fired only when ServerManager
    // changes the SMProperty. To avoid breaking old code, we fire this signal
    // explicitly. I'd like not provide some deprecation mechanism for this
    // behavior.
    if (this->autoUpdateVTKObjects() && connection->proxy())
    {
      Q_EMIT this->smPropertyChanged();
    }
  }
}

//-----------------------------------------------------------------------------
void pqPropertyLinks::onSMPropertyModified()
{
  if (!this->Internals->IgnoreSMChanges)
  {
    pqPropertyLinksConnection* connection =
      qobject_cast<pqPropertyLinksConnection*>(this->sender());
    if (connection)
    {
      connection->copyValuesFromServerManagerToQt(this->useUncheckedProperties());
      Q_EMIT this->smPropertyChanged();
    }
  }
}

//-----------------------------------------------------------------------------
void pqPropertyLinks::accept()
{
  std::set<vtkSMProxy*> proxies_to_update;

  // In some cases, pqPropertyLinks is used to connect multiple elements of a
  // property to different widgets (e.g. the VOI widgets for ExtractVOI filter).
  // In such cases when we "accept" changes from Index 0, for example, the Index
  // 1 ends up resetting its value using the current value from the property.
  // To avoid such incorrect feedback when we are accepting all changes, we use
  // IgnoreSMChanges flag.
  this->Internals->IgnoreSMChanges = true;

  foreach (pqPropertyLinksConnection* connection, this->Internals->Connections)
  {
    if (connection && connection->proxy())
    {
      connection->copyValuesFromQtToServerManager(false);
      proxies_to_update.insert(connection->proxy());
    }
  }
  this->Internals->IgnoreSMChanges = false;

  for (std::set<vtkSMProxy*>::iterator iter = proxies_to_update.begin();
       iter != proxies_to_update.end(); ++iter)
  {
    (*iter)->UpdateVTKObjects();
  }
}

//-----------------------------------------------------------------------------
void pqPropertyLinks::reset()
{
  foreach (pqPropertyLinksConnection* connection, this->Internals->Connections)
  {
    if (connection && connection->proxy())
    {
      connection->copyValuesFromServerManagerToQt(false);
      // Ensures that on "reset" we clear out unchecked values for the property.
      if (vtkSMProperty* prop = connection->propertySM())
      {
        prop->ClearUncheckedElements();
      }
    }
  }
}
