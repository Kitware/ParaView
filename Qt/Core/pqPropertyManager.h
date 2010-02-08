/*=========================================================================

   Program: ParaView
   Module:    pqPropertyManager.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#ifndef _pqPropertyManager_h
#define _pqPropertyManager_h

#include "pqCoreExport.h"
#include <QObject>
#include <QVariant>
#include <QPointer>

class vtkSMProxy;
class vtkSMProperty;

/// Manages links between Qt properties and unchecked proxy properties
/// This is useful if more than one QWidget exposes a single proxy property
/// In which case the server manager will not keep the widgets synchronized
/// Also provides a mechanims for accepting or rejecting changes for unchecked
/// properties
class PQCORE_EXPORT pqPropertyManager : public QObject
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
  void unregisterLink(
    QObject* qObject, const char* qProperty, const char* signal,
    vtkSMProxy* Proxy, vtkSMProperty* Property, int Index=-1);

  /// returns whether there are modified properties to send to 
  /// the server manager
  bool isModified() const;

  // Call this method to un-links all property links 
  // maintained by this object.
  void removeAllLinks();

signals:
  /// Signal emitted when there are possible properties to send down to
  /// the server manager
  void modified();

  /// Signal emitted when the user has accepted changes
  void aboutToAccept();
  void accepted();
  /// Signal emitted when the user has rejected changes
  void rejected();

public slots:
  /// accept property changes by pushing them all down to the server manager
  void accept();
  /// reject property changes and revert all QObject properties
  void reject();
  /// Called whenever a property changes from the GUI side
  void propertyChanged();
  
protected:
  class pqInternal;
  pqInternal* Internal;

};

class pqPropertyManagerProperty;

class PQCORE_EXPORT pqPropertyManagerPropertyLink : public QObject
{
  Q_OBJECT
public:
  pqPropertyManagerPropertyLink(pqPropertyManagerProperty* p, 
                                QObject* o, 
                                const char* property, 
                                const char* signal);
  QObject* object() const;
  QByteArray property() const;
private slots:
  void guiPropertyChanged();
  void managerPropertyChanged();
private:
  QPointer<QObject> QtObject;
  QByteArray QtProperty;
  int Block;
};

class PQCORE_EXPORT pqPropertyManagerProperty : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QVariant value READ value WRITE setValue)
  friend class pqPropertyManager;
  friend class pqPropertyManagerPropertyLink;
public:
  pqPropertyManagerProperty(QObject* p);
  ~pqPropertyManagerProperty();
  QVariant value() const;
  void setValue(const QVariant&);
  void addLink(QObject* o, const char* property, const char* signal);
  void removeLink(QObject* o, const char* property, const char* signal);
  void removeAllLinks();
  int numberOfLinks() const { return Links.size(); }
signals:
  void propertyChanged();
  void guiPropertyChanged();
  void flushProperty();
private:
  QVariant Value;
  QList<pqPropertyManagerPropertyLink*> Links;
};

#endif // !_pqPropertyManager_h

