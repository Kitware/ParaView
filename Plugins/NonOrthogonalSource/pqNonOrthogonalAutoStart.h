// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqNonOrthogonalAutoStart_h
#define pqNonOrthogonalAutoStart_h

#include <QObject>

class pqNonOrthogonalAutoStart : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqNonOrthogonalAutoStart(QObject* parent = nullptr);
  ~pqNonOrthogonalAutoStart() override;

  void startup();
  void shutdown();

private:
  Q_DISABLE_COPY(pqNonOrthogonalAutoStart)
};

#endif
