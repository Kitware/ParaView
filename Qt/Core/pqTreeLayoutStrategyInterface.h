// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqTreeLayoutStrategyInterface_h
#define pqTreeLayoutStrategyInterface_h

#include "pqCoreModule.h"
#include <QStringList>
#include <QtPlugin>

class vtkAreaLayoutStrategy;

/**
 * interface class for plugins that create view modules
 */
class PQCORE_EXPORT pqTreeLayoutStrategyInterface
{
public:
  /**
   * destructor
   */
  pqTreeLayoutStrategyInterface();
  virtual ~pqTreeLayoutStrategyInterface();

  /**
   * Return a list of layout strategies supported by this interface
   */
  virtual QStringList treeLayoutStrategies() const = 0;

  virtual vtkAreaLayoutStrategy* getTreeLayoutStrategy(const QString& layoutStrategy) = 0;
};

Q_DECLARE_INTERFACE(pqTreeLayoutStrategyInterface, "com.kitware/paraview/treeLayoutStrategy")

#endif
