// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#ifndef pqMyRenderView_h
#define pqMyRenderView_h

#include <pqRenderView.h>

/// This example illustrates how to use your own pq classes for your proxies
class pqMyRenderView : public pqRenderView
{
  Q_OBJECT
  typedef pqRenderView Superclass;

public:
  pqMyRenderView(const QString& group, const QString& name, vtkSMProxy* viewModule,
    pqServer* server, QObject* parent = nullptr);
  ~pqMyRenderView() = default;
};

#endif
