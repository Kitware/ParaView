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
#include <QObject>

/// Translates server manager events into Qt-compatible slots and signals
class pqSMAdaptor : public QObject
{
  Q_OBJECT
  
public:
  pqSMAdaptor();
  ~pqSMAdaptor();


  /// Set the specified property of a proxy property index 0 is assumed if Value is not a QList
  void SetProperty(vtkSMProperty* Property, QVariant Value);
  /// Get the specified property of a proxy if property has more than one element, a QList is returned
  QVariant GetProperty(vtkSMProperty* Property);
  
  /// Set the specified property of a proxy
  void SetProperty(vtkSMProperty* Property, int Index, QVariant Value);
  /// Get the specified property of a proxy
  QVariant GetProperty(vtkSMProperty* Property, int Index);
  
  
  // Property Linking

  /// Link a property of a proxy to a property of a QObject. The QObject property follows the vtkSMProperty.
  void LinkPropertyTo(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index,
                         QObject* qObject, const char* qProperty);
  
  /// Unlink a property of a proxy from a property of a QObject.
  void UnlinkPropertyFrom(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index,
                            QObject* qObject, const char* qProperty);

  /// Link a property of a QObject to a property of a proxy. The vtkSMProperty follows the QObject's property.
  void LinkPropertyTo(QObject* qObject, const char* qProperty, const char* signal,
                      vtkSMProxy* Proxy, vtkSMProperty* Property, int Index);
  
  /// Unlink a property of a QObject from a property of a proxy.
  void UnlinkPropertyFrom(QObject* qObject, const char* qProperty, const char* signal,
                          vtkSMProxy* Proxy, vtkSMProperty* Property, int Index);


  /// Connect a domain modified event to a Qt slot Slot must be of signature "foo(vtkSMProperty*)"
  void ConnectDomain(vtkSMProperty* Property, QObject* qObject, const char* slot);
  
  /// Disconnect a domain modified event from a Qt slot
  void DisconnectDomain(vtkSMProperty* Property, QObject* qObject, const char* slot);

signals:
  /// Signal emitted when Server Manager creates an instance
  void ProxyCreated(vtkSMProxy* proxy);
  /// Signal emitted when Server Manager deletes an instance
  void ProxyDeleted(vtkSMProxy* proxy);

protected slots:
  void SMLinkedPropertyChanged(vtkObject*, unsigned long, void*);
  void QtLinkedPropertyChanged(QWidget*);
  void SMDomainChanged(vtkObject*, unsigned long, void*);

protected:
  pqSMAdaptorInternal* Internal;

};

#endif // !_pqSMAdaptor_h

