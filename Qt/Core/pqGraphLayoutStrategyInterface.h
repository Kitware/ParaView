// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqGraphLayoutStrategyInterface_h
#define pqGraphLayoutStrategyInterface_h

#include "pqCoreModule.h"
#include <QStringList>
#include <QtPlugin>

class vtkGraphLayoutStrategy;

/**
 * interface class for plugins that create view modules
 */
class PQCORE_EXPORT pqGraphLayoutStrategyInterface
{
public:
  /**
   * destructor
   */
  virtual ~pqGraphLayoutStrategyInterface();

  /**
   * Return a list of layout strategies supported by this interface
   */
  virtual QStringList graphLayoutStrategies() const = 0;

  virtual vtkGraphLayoutStrategy* getGraphLayoutStrategy(const QString& layoutStrategy) = 0;
};

Q_DECLARE_INTERFACE(pqGraphLayoutStrategyInterface, "com.kitware/paraview/graphLayoutStrategy")

#endif
