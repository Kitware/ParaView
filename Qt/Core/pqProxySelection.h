// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqProxySelection_h
#define pqProxySelection_h

#include "pqCoreModule.h"
#include <QList>

class pqServerManagerModelItem;
class vtkSMProxySelectionModel;

/**
 * pqProxySelection is used to specify a selection comprising proxies.
 * pqProxySelection provides methods to convert to and from
 * vtkSMProxySelectionModel.
 */
using pqProxySelection = QList<pqServerManagerModelItem*>;

class PQCORE_EXPORT pqProxySelectionUtilities
{
public:
  /**
   * copy values from vtkSMProxySelectionModel. All proxies in the
   * vtkSMProxySelectionModel must be known to pqServerManagerModel otherwise
   * it will be ignored. Returns true, if the selection was changed, otherwise
   * returns false.
   */
  static bool copy(vtkSMProxySelectionModel* source, pqProxySelection& dest);

  /**
   * copy values to vtkSMProxySelectionModel. Clears any existing selection.
   */
  static bool copy(const pqProxySelection& source, vtkSMProxySelectionModel* dest);

  /**
   * Selections can contains pqOutputPort, pqPipelineSource, or
   * pqExtractor instances. Use this method to filter out all
   * pqOutputPort and replace them with corresponding pqPipelineSource
   * instances.
   */
  static pqProxySelection getPipelineProxies(const pqProxySelection& sel);
};

#endif
