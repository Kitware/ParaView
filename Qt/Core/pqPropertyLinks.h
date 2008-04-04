/*=========================================================================

   Program: ParaView
   Module:    pqPropertyLinks.h

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

#ifndef _pqPropertyLinks_h
#define _pqPropertyLinks_h

#include "pqCoreExport.h"
#include <QObject>

class vtkObject;
class vtkSMProxy;
class vtkSMProperty;

/// provides direct links between Qt widgets and server manager properties
/// changing the value of a widget automatically updates the server manager
/// a change in the server manager automatically updates the widget
class PQCORE_EXPORT pqPropertyLinks : public QObject
{
  Q_OBJECT
  
public:
  /// constructor creates a property link object
  /// using unchecked properties is off by default
  pqPropertyLinks(QObject* p=0);
  /// destructor
  ~pqPropertyLinks();

  /// link a property
  void addPropertyLink(
    QObject* qObject, const char* qProperty, const char* signal,
    vtkSMProxy* Proxy, vtkSMProperty* Property, int Index=-1);
  
  /// un-link a property
  void removePropertyLink(
    QObject* qObject, const char* qProperty, const char* signal,
    vtkSMProxy* Proxy, vtkSMProperty* Property, int Index=-1);

  // Call this method to un-links all property links 
  // maintained by this object.
  void removeAllPropertyLinks();

signals:
  /// signals fired when a link is updated.
  void qtWidgetChanged();
  void smPropertyChanged();

public slots:
  /// accept the changes and push them to the server manager
  /// regardless of the whether we're using unchecked properties
  void accept();
  
  /// reject any changes and update the QObject's properties to reflect the
  /// server manager properties
  void reset();

  /// set whether to use unchecked properties on the server manager
  /// one may get/set unchecked properties to get domain updates before an
  /// accept is done
  void setUseUncheckedProperties(bool);
  
  /// set whether UpdateVTKObjects is called automatically when needed
  void setAutoUpdateVTKObjects(bool);

public:

  /// get whether unchecked properties are used
  bool useUncheckedProperties();

  /// get whether UpdateVTKObjects is called automatically when needed
  bool autoUpdateVTKObjects();

protected:
  
  class pqInternal;
  pqInternal* Internal;

};


/// pqPropertyLinks private helper class
class PQCORE_EXPORT pqPropertyLinksConnection : public QObject
{
  Q_OBJECT
  friend class pqPropertyLinks;
public:
  pqPropertyLinksConnection(QObject*parent, 
    vtkSMProxy* proxy, vtkSMProperty* property, int idx,
    QObject* qobject, const char* qproperty);
  ~pqPropertyLinksConnection();

  void setUseUncheckedProperties(bool) const;
  bool useUncheckedProperties() const;
  void setAutoUpdateVTKObjects(bool) const;
  bool autoUpdateVTKObjects() const;

  bool getOutOfSync() const;
  void clearOutOfSync() const;

  bool isEqual(vtkSMProxy* proxy, vtkSMProperty* property, int idx,
    QObject* qObject, const char* qproperty) const;
signals: 
  void qtWidgetChanged();
  void smPropertyChanged();

private slots:
  void triggerDelayedSMLinkedPropertyChanged();

  void smLinkedPropertyChanged();
  void qtLinkedPropertyChanged();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif // !_pqPropertyLinks_h

