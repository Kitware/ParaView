// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#ifndef pqSLACActionGroup_h
#define pqSLACActionGroup_h

#include <QActionGroup>

/// Adds actions that are helpful for setting up visualization of SLAC
/// simulation result files.
class pqSLACActionGroup : public QActionGroup
{
  Q_OBJECT;

public:
  pqSLACActionGroup(QObject* p);

private:
  Q_DISABLE_COPY(pqSLACActionGroup)
};

#endif // pqSLACActionGroup_h
