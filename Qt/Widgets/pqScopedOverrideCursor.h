// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqScopedOverrideCursor_h
#define pqScopedOverrideCursor_h

#include "pqWidgetsModule.h"

#include <QCursor>

/**
  RAII component that overrides the mouse cursor during an operation.
  The normal cursor is restored when the pqScopedOverrideCursor object goes out-of-scope:

  /code
  {
  pqScopedOverrideCursor cursor(Qt::WaitCursor);
  for(i = 0; i != really_big_number; ++i)
    {
    DoSomethingTimeConsuming();
    }
  }
  /endcode
*/

class PQWIDGETS_EXPORT pqScopedOverrideCursor
{
public:
  pqScopedOverrideCursor() = delete;
  pqScopedOverrideCursor(Qt::CursorShape cursorShape);
  virtual ~pqScopedOverrideCursor();
};

#endif
