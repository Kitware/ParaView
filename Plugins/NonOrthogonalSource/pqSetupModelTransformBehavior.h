// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#ifndef pqSetupModelTransformBehavior_h
#define pqSetupModelTransformBehavior_h

#include <QObject>

class pqSetupModelTransformBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqSetupModelTransformBehavior(QObject* parent = nullptr);
  ~pqSetupModelTransformBehavior() override;

private:
  Q_DISABLE_COPY(pqSetupModelTransformBehavior)
};

#endif
