// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMyRenderView.h"

#include <vtkSMViewProxy.h>

//-----------------------------------------------------------------------------
pqMyRenderView::pqMyRenderView(
  const QString& group, const QString& name, vtkSMProxy* viewModule, pqServer* server, QObject* p)
  : Superclass(group, name, vtkSMViewProxy::SafeDownCast(viewModule), server, p)
{
  std::cout << "Using pqMyRenderView" << std::endl;
}
