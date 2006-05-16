/*=========================================================================

   Program:   ParaQ
   Module:    pqPropertyManager.h

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

#ifndef _pqPropertyManager_h
#define _pqPropertyManager_h

#include "pqWidgetsExport.h"
#include <QObject>
#include <QVariant>
#include <QPointer>

class vtkSMProxy;
class vtkSMProperty;

/// manages queued links between Qt properties and server manager properties
/// Provides a mechanims for accepting or rejecting changes
class PQWIDGETS_EXPORT pqPropertyManager : public QObject
{
  Q_OBJECT
  
public:
  /// constructor
  pqPropertyManager(QObject* p=0);
  /// destructor
  ~pqPropertyManager();

  /// register a QObject property to link with the server manager
  void registerLink(QObject* qObject, const char* qProperty, const char* signal,
                       vtkSMProxy* Proxy, vtkSMProperty* Property, int Index=-1);
  
  /// unregister a QObject property to link with the server manager
  void unregisterLink(QObject* qObject, const char* qProperty, const char* signal,
                          vtkSMProxy* Proxy, vtkSMProperty* Property, int Index=-1);

signals:
  /// signal emitted whether there are possible properties to send down to the server manager
  void canAcceptOrReject(bool);

  /// emitted before accept.
  void preaccept();
  /// emitted on accept() after preaccept() but before postaccept()/
  void accepted();
  ///emitted after accept;
  void postaccept();

  /// emitted before reject.
  void prereject();
  /// emitted after reject.
  void postreject();

public slots:
  /// accept property changes by pushing them all down to the server manager
  void accept();
  /// reject property changes and revert all QObject properties
  void reject();

protected slots:
  void propertyChanged();
protected:
  
  class pqInternal;
  pqInternal* Internal;

};

class pqPropertyManagerProperty;

class PQWIDGETS_EXPORT pqPropertyManagerPropertyLink : public QObject
{
  Q_OBJECT
public:
  pqPropertyManagerPropertyLink(pqPropertyManagerProperty* p, QObject* o, const char* property, const char* signal);
  QObject* object() const;
  QByteArray property() const;
private slots:
  void guiPropertyChanged();
  void managerPropertyChanged();
private:
  QPointer<QObject> QtObject;
  QByteArray QtProperty;
};

class PQWIDGETS_EXPORT pqPropertyManagerProperty : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QVariant value READ value WRITE setValue)
  friend class pqPropertyManager;
public:
  pqPropertyManagerProperty(QObject* p);
  ~pqPropertyManagerProperty();
  QVariant value() const;
  void setValue(const QVariant&);
  void addLink(QObject* o, const char* property, const char* signal);
  void removeLink(QObject* o, const char* property, const char* signal);
  int numberOfLinks() const { return Links.size(); }
signals:
  void propertyChanged();
  void flushProperty();
private:
  QVariant Value;
  QList<pqPropertyManagerPropertyLink*> Links;
};

#endif // !_pqPropertyManager_h

