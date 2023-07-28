// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqVariableType_h
#define pqVariableType_h

/**
 * Provides a standard enumeration of variables that can be displayed for a dataset
 */
enum pqVariableType
{
  /**
   * Used to represent an empty selection
   */
  VARIABLE_TYPE_NONE = 0,
  VARIABLE_TYPE_CELL = 1,
  VARIABLE_TYPE_NODE = 2,
};

#endif
