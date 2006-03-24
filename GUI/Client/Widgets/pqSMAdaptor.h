/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#ifndef _pqSMAdaptor_h
#define _pqSMAdaptor_h

class pqSMAdaptorInternal;
class vtkSMProperty;
class vtkSMProxy;
class vtkObject;
class QWidget;

#include "QtWidgetsExport.h"
#include <QObject>
#include <QVariant>
#include <QList>
#include "pqSMProxy.h"

/// Translates server manager events into Qt-compatible slots and signals
class QTWIDGETS_EXPORT pqSMAdaptor : public QObject
{
  Q_OBJECT
  
public:
  pqSMAdaptor();
  ~pqSMAdaptor();

  /// enumeration for types of properties this class handles
  enum PropertyType
    {
    UNKNOWN,
    PROXY,
    PROXYLIST,
    SELECTION,
    ENUMERATION,
    SINGLE_ELEMENT,
    MULTIPLE_ELEMENTS
    };

  /// Get the global adapter instance.
  static pqSMAdaptor* instance();

  /// Get the type of the property
  static PropertyType getPropertyType(vtkSMProperty* Property);

  /// get the proxy for a property, for example, glyph filter accepts a source (proxy) to glyph with
  static pqSMProxy getProxyProperty(vtkSMProxy* Proxy, vtkSMProperty* Property);
  /// get the proxy for a property, for example, glyph filter accepts a source (proxy) to glyph with
  static void setProxyProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, pqSMProxy Value);
  
  /// get the list of proxies for a property, for example, append filter accepts a list of proxies
  static QList<pqSMProxy> getProxyListProperty(vtkSMProxy* Proxy, vtkSMProperty* Property);
  /// get the list of proxies for a property, for example, append filter accepts a list of proxies
  static void setProxyListProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, QList<pqSMProxy> Value);

  /// get the list of possible proxies for a property
  static QList<pqSMProxy> getProxyPropertyDomain(vtkSMProxy* Proxy, vtkSMProperty* Property);


  /// get the pairs of selections for a selection property
  static QList<QList<QVariant> > getSelectionProperty(vtkSMProxy* Proxy, vtkSMProperty* Property);
  /// set the pairs of selections for a selection property
  static void setSelectionProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, QList<QList<QVariant> > Value);
  /// get the possible names for the selection property
  static QList<QVariant> getSelectionPropertyDomain(vtkSMProperty* Property);
  
  /// get the enumeration for a property
  static QVariant getEnumerationProperty(vtkSMProxy* Proxy, vtkSMProperty* Property);
  /// set the enumeration for a property
  static void setEnumerationProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, QVariant Value);
  /// get the possible enumerations (string) for a property
  static QList<QVariant> getEnumerationPropertyDomain(vtkSMProperty* Property);

  /// get the single element of a property (integer, string, real, etc..)
  static QVariant getElementProperty(vtkSMProxy* Proxy, vtkSMProperty* Property);
  /// set the single element of a property (integer, string, real, etc..)
  static void setElementProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, QVariant Value);
  /// get the range of possible values to set the single element of a property
  static QList<QVariant> getElementPropertyDomain(vtkSMProperty* Property);
  
  /// get the multiple elements of a property (integer, string, real, etc..)
  static QList<QVariant> getMultipleElementProperty(vtkSMProxy* Proxy, vtkSMProperty* Property);
  /// set the multiple elements of a property (integer, string, real, etc..)
  static void setMultipleElementProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, QList<QVariant> Value);
  /// get the ranges of possible values to set the multiple elements of a property
  static QList<QList<QVariant> > getMultipleElementPropertyDomain(vtkSMProperty* Property);
  /// get one of the multiple elements of a property (integer, string, real, etc..)
  static QVariant getMultipleElementProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index);
  /// set one of the multiple elements of a property (integer, string, real, etc..)
  static void setMultipleElementProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index, QVariant Value);
  /// get one of the ranges of possible values to set the multiple elements of a property
  static QList<QVariant> getMultipleElementPropertyDomain(vtkSMProperty* Property, int Index);

private:
  void setProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, QVariant QtProperty);
  QVariant getProperty(vtkSMProxy* Proxy, vtkSMProperty* Property);
  void setProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index, QVariant QtProperty);
  QVariant getProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index);
  QVariant pqSMAdaptor::getPropertyDomain(vtkSMProperty* Property);
public:

  // Property Linking

  /// Link a property of a proxy to a property of a QObject. The QObject property follows the vtkSMProperty.
  void linkPropertyTo(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index,
                         QObject* qObject, const char* qProperty);
  
  /// Unlink a property of a proxy from a property of a QObject.
  void unlinkPropertyFrom(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index,
                            QObject* qObject, const char* qProperty);

  /// Link a property of a QObject to a property of a proxy. The vtkSMProperty follows the QObject's property.
  void linkPropertyTo(QObject* qObject, const char* qProperty, const char* signal,
                      vtkSMProxy* Proxy, vtkSMProperty* Property, int Index);
  
  /// Unlink a property of a QObject from a property of a proxy.
  void unlinkPropertyFrom(QObject* qObject, const char* qProperty, const char* signal,
                          vtkSMProxy* Proxy, vtkSMProperty* Property, int Index);


  /// Connect a domain modified event to a Qt slot Slot must be of signature "foo(vtkSMProperty*)"
  void connectDomain(vtkSMProperty* Property, QObject* qObject, const char* slot);
  
  /// Disconnect a domain modified event from a Qt slot
  void disconnectDomain(vtkSMProperty* Property, QObject* qObject, const char* slot);

signals:
  /// Signal emitted when Server Manager creates an instance
  void proxyCreated(vtkSMProxy* proxy);
  /// Signal emitted when Server Manager deletes an instance
  void proxyDeleted(vtkSMProxy* proxy);

protected slots:
  void smLinkedPropertyChanged(vtkObject*, unsigned long, void*);
  void qtLinkedPropertyChanged(QWidget*);
  void smDomainChanged(vtkObject*, unsigned long, void*);

protected:
  pqSMAdaptorInternal* Internal;

private:
  static pqSMAdaptor *Instance;
};

#endif // !_pqSMAdaptor_h

