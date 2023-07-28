// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqWaitCursor_h
#define pqWaitCursor_h

#include "pqWidgetsModule.h"

/**
  RAII component that displays a wait cursor during a long operation.
  The normal cursor is restored when the pqWaitCursor object goes out-of-scope:

  /code
  {
  pqWaitCursor cursor;
  for(i = 0; i != really_big_number; ++i)
    {
    DoSomethingTimeConsuming();
    }
  }
  /endcode
*/

class PQWIDGETS_EXPORT pqWaitCursor
{
public:
  pqWaitCursor();
  ~pqWaitCursor();
};

#endif
