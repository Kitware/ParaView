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
#ifndef __pqPropertyLinksConnection_h
#define __pqPropertyLinksConnection_h

#include <QObject>
#include <QPointer>

#include "vtkNew.h"
#include "vtkWeakPointer.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkEventQtSlotConnect.h"

class pqPropertyLinks;

/// *** THIS CLASS IS NOT DESIGNED FOR PUBLIC USE ***
/// pqPropertyLinksConnection is an internal class used by
/// pqPropertyLinks.
/// *** THIS CLASS IS NOT DESIGNED FOR PUBLIC USE ***
class pqPropertyLinksConnection : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  /// This also sets up observers to monitor any changes. This does not change
  /// any values on either items.
  pqPropertyLinksConnection(
    QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex,
    bool use_unchecked_modified_event,
    QObject* parentObject=0);

  virtual ~pqPropertyLinksConnection();

  vtkSMProxy* proxy() const
    { return this->ProxySM; }

  void setUseUncheckedProperties(bool useUnchecked);

  /// Comparison operator
  bool operator==(const pqPropertyLinksConnection& other) const;

public slots:
  /// Copy values from ServerManager to Qt. If use_unchecked is true, unchecked
  /// SMProperty values are used.
  void copyValuesFromServerManagerToQt(bool use_unchecked);
 
  /// Copy values from Qt to ServerManager. If use_unchecked is true, unchecked
  /// values for SMProperty are updated.
  void copyValuesFromQtToServerManager(bool use_unchecked);

signals:
  /// Fired whenever the Qt widget changes (except in during a call to
  /// copyValuesFromServerManagerToQt()).
  void qtpropertyModified();

  /// Fired whenever the ServerManager property changes (except in during a call to
  /// copyValuesFromQtToServerManager()).
  void smpropertyModified();

private:
  Q_DISABLE_COPY(pqPropertyLinksConnection)
  vtkNew<vtkEventQtSlotConnect> VTKConnector;

  QPointer<QObject> ObjectQt;
  QString PropertyQt;
  QString SignalQt;

  vtkWeakPointer<vtkSMProxy> ProxySM;
  vtkWeakPointer<vtkSMProperty> PropertySM;
  int IndexSM;
};

#endif
