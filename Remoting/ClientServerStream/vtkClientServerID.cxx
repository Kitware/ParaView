// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkClientServerID.h"

#include "vtkIOStream.h"

ostream& operator<<(ostream& os, const vtkClientServerID& id)
{
  return os << id.ID;
}

vtkOStreamWrapper& operator<<(vtkOStreamWrapper& os, const vtkClientServerID& id)
{
  return os << id.ID;
}
