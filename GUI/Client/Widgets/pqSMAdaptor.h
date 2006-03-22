/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

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

/// Translates server manager events into Qt-compatible slots and signals
class QTWIDGETS_EXPORT pqSMAdaptor : public QObject
{
  Q_OBJECT
  
public:
  pqSMAdaptor();
  ~pqSMAdaptor();


  /// Get the global adapter instance.
  static pqSMAdaptor* instance();

  /// Set the specified property of a proxy property index 0 is assumed if Value is not a QList
  void setProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, QVariant Value);
  /// Get the specified property of a proxy if property has more than one element, 
  /// a QList is returned or
  /// a QList<QList<QVariant>> ( a list of name/value pairs can be returned -- these names are omitted when setting the property )
  QVariant getProperty(vtkSMProxy* Proxy, vtkSMProperty* Property);
  
  /// Set the specified property of a proxy
  void setProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index, QVariant Value);
  /// Get the specified property of a proxy
  QVariant getProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index);

  /// Get the domain of a property
  QVariant getPropertyDomain(vtkSMProperty* Property);
  
  
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

