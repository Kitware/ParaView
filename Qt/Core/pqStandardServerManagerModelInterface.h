// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqStandardServerManagerModelInterface_h
#define pqStandardServerManagerModelInterface_h

#include "pqCoreModule.h"
#include "pqServerManagerModelInterface.h"
#include <QObject>

/**
 * This is standard implementation used by ParaView for creating different
 * pqProxy subclassess for every proxy registered.
 */
class PQCORE_EXPORT pqStandardServerManagerModelInterface
  : public QObject
  , public pqServerManagerModelInterface
{
  Q_OBJECT
  Q_INTERFACES(pqServerManagerModelInterface)
public:
  pqStandardServerManagerModelInterface(QObject* parent);
  ~pqStandardServerManagerModelInterface() override;

  /**
   * Creates a pqProxy subclass for the vtkSMProxy given the details for its
   * registration with the proxy manager.
   */
  pqProxy* createPQProxy(
    const QString& group, const QString& name, vtkSMProxy* proxy, pqServer* server) const override;
};

#endif
