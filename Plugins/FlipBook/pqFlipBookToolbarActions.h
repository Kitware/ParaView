// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqFlipBookToolbarActions_h
#define pqFlipBookToolbarActions_h

#include <QToolBar>

class pqFlipBookToolbarActions : public QToolBar
{
  Q_OBJECT

public:
  pqFlipBookToolbarActions(QWidget* p);

private:
  Q_DISABLE_COPY(pqFlipBookToolbarActions)
};

#endif
