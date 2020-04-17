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
#ifndef pqPropertyLinks_h
#define pqPropertyLinks_h

#include <QObject>
#include <QtDebug>

#include "pqCoreModule.h"
#include "pqPropertyLinksConnection.h" // needed for pqPropertyLinksConnection.

class vtkSMProperty;
class vtkSMProxy;

/**
* pqPropertyLinks is used to connect vtkSMProperty and subclasses to
* properties on QObject instances. pqPropertyLinks enables setting up a link
* between QWidgets and vtkSMProperty's so that whenever one of them changes,
* the other is updated as well.
*
* vtkSMProperty has two types of values, checked and unchecked. This class by
* default uses checked values, but it can be told to use unchecked values using
* setUseUncheckedProperties(). When setUseUncheckedProperties() is set to
* true, one can use accept()/reset() to accept or discard any changes
* to the vtkSMProperty.
*
* When setUseUncheckedProperties() is set to false (default), one can set
* setAutoUpdateVTKObjects() to true (default false) to ensure that
* vtkSMProxy::UpdateVTKObjects() is called whenever the property changes.
*
* This class uses weak-references to both the Qt and ServerManager objects
* hence it's safe to destroy either of those without updating the
* pqPropertyLinks instance.
*
* Also, pqPropertyLinks never uses any delayed timers or connections.
*/
class PQCORE_EXPORT pqPropertyLinks : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPropertyLinks(QObject* parent = 0);
  ~pqPropertyLinks() override;

  /**
  * Setup a link between a Qt property and vtkSMProperty on a vtkSMProxy
  * instance. The QObject is updated using the vtkSMProperty's current value
  * immediately.
  * Arguments:
  * \li \c qobject :- the Qt object
  * \li \c qproperty :- the Qt property name on \c qobject that can be used to
  *                     get/set the object's value(s).
  * \li \c qsignal :- signal fired by the \c qobject whenever it's property
  *                   value changes.
  * \li \c smproxy :- the vtkSMProxy instance.
  * \li \c smproperty :- the vtkSMProperty from the \c proxy.
  * \li \c smindex :- for multi-element properties, specify a non-negative
  *                 value to link to a particular element of the
  *                 vtkSMProperty..
  * Returns false if link adding fails for some reason.
  */
  bool addPropertyLink(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex = -1);

  /**
   * Like addPropertyLink, but also sets up properties to be Python traceable.
   */
  bool addTraceablePropertyLink(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex = -1);

  /**
   * Like addPropertyLink, but used to create a custom subclass of pqPropertyLinksConnection.
   */
  template <class ConnectionType>
  bool addPropertyLink(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex = -1,
    ConnectionType* notused = NULL);

  /**
   * Like addPropertyLink, but used to create a custom subclass of pqPropertyLinksConnection
   * and flag it as traceable. Note that subclasses of pqPropertyLinksConnection may ignore
   * the TraceChanges flag set on it with this method.
   */
  template <class ConnectionType>
  bool addTraceablePropertyLink(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex = -1,
    ConnectionType* notused = NULL);

  /**
  * Remove a particular link.
  */
  bool removePropertyLink(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex = -1);

  bool autoUpdateVTKObjects() const { return this->AutoUpdateVTKObjects; }
  bool useUncheckedProperties() const { return this->UseUncheckedProperties; }

public Q_SLOTS:
  /**
  * Remove all links.
  */
  void removeAllPropertyLinks() { this->clear(); }
  void clear();

  /**
  * When UseUncheckedProperties == true, the smproperty values are not changed
  * whenever the qobject is modified. Use this method to change the smproperty
  * values using the current qobject values.
  */
  void accept();

  /**
  * Set the qobject values using the smproperty values. This also clears any
  * unchecked values that may be set on the smproperty.
  */
  void reset();

  /**
  * set whether to use unchecked properties on the server manager
  * one may get/set unchecked properties to get domain updates before an
  * accept is done
  */
  void setUseUncheckedProperties(bool val);

  /**
  * set whether UpdateVTKObjects is called automatically when needed
  */
  void setAutoUpdateVTKObjects(bool val) { this->AutoUpdateVTKObjects = val; }

Q_SIGNALS:
  void qtWidgetChanged();
  void smPropertyChanged();

private Q_SLOTS:
  /**
  * slots called when pqPropertyLinksConnection indicates that a Qt or SM
  * property was changed.
  */
  void onQtPropertyModified();
  void onSMPropertyModified();

private:
  bool addNewConnection(pqPropertyLinksConnection*);

  template <class ConnectionType>
  ConnectionType* addPropertyLinkInternal(QObject* qobject, const char* qproperty,
    const char* qsignal, vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex = -1);

private:
  Q_DISABLE_COPY(pqPropertyLinks)

  class pqInternals;
  pqInternals* Internals;
  bool UseUncheckedProperties;
  bool AutoUpdateVTKObjects;
};

//-----------------------------------------------------------------------------
template <class ConnectionType>
ConnectionType* pqPropertyLinks::addPropertyLinkInternal(QObject* qobject, const char* qproperty,
  const char* qsignal, vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex)
{
  if (!qobject || !qproperty || !qsignal || !smproxy || !smproperty)
  {
    qCritical() << "Invalid parameters to pqPropertyLinks::addPropertyLinkInternal";
    qDebug() << "(" << qobject << ", " << qproperty << ", " << qsignal << ") <==> ("
             << (smproxy ? smproxy->GetXMLName() : "(none)") << ","
             << (smproperty ? smproperty->GetXMLLabel() : "(none)") << smindex << ")";
    return nullptr;
  }

  ConnectionType* connection = new ConnectionType(qobject, qproperty, qsignal, smproxy, smproperty,
    smindex, this->useUncheckedProperties(), this);
  this->addNewConnection(connection);

  return connection;
}

//-----------------------------------------------------------------------------
template <class ConnectionType>
bool pqPropertyLinks::addPropertyLink(QObject* qobject, const char* qproperty, const char* qsignal,
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex, ConnectionType*)
{
  auto connection = this->addPropertyLinkInternal<ConnectionType>(
    qobject, qproperty, qsignal, smproxy, smproperty, smindex);
  return connection != nullptr;
}

//-----------------------------------------------------------------------------
template <class ConnectionType>
bool pqPropertyLinks::addTraceablePropertyLink(QObject* qobject, const char* qproperty,
  const char* qsignal, vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex, ConnectionType*)
{
  auto connection = this->addPropertyLinkInternal<ConnectionType>(
    qobject, qproperty, qsignal, smproxy, smproperty, smindex);
  if (connection)
  {
    connection->setTraceChanges(true);
    return true;
  }
  return false;
}

#endif
