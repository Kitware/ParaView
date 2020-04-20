/*=========================================================================

   Program:   ParaView
   Module:    pqSMSignalAdaptors.h

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

#ifndef pq_SMSignalAdaptors_h
#define pq_SMSignalAdaptors_h

#include "pqComponentsModule.h"
#include <QObject>
#include <QVariant>
class pqProxy;

/**
* signal adaptor to allow getting/setting/observing of a pseudo vtkSMProxy property
*/
class PQCOMPONENTS_EXPORT pqSignalAdaptorProxy : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QVariant proxy READ proxy WRITE setProxy)
public:
  /**
  * constructor requires a QObject, the name of the QString proxy name, and
  * a signal for property changes
  */
  pqSignalAdaptorProxy(QObject* p, const char* Property, const char* signal);
  /**
  * get the proxy
  */
  QVariant proxy() const;
Q_SIGNALS:
  /**
  * signal the proxy changed
  */
  void proxyChanged(const QVariant&);
public Q_SLOTS:
  /**
  * set the proxy
  */
  void setProxy(const QVariant&);
protected Q_SLOTS:
  void handleProxyChanged();

protected:
  QByteArray PropertyName;
};

#endif
