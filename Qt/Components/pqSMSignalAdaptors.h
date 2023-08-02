// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqSMSignalAdaptors_h
#define pqSMSignalAdaptors_h

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

protected: // NOLINT(readability-redundant-access-specifiers)
  QByteArray PropertyName;
};

#endif
