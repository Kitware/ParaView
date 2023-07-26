// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-CLAUSE
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

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
