// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#ifndef pqViewManager_h
#define pqViewManager_h

#include "pqTabbedMultiViewWidget.h"

/**
 * pqViewManager is deprecated. Use pqTabbedMultiViewWidget.
 */
class PQCOMPONENTS_EXPORT pqViewManager : public pqTabbedMultiViewWidget
{
  Q_OBJECT
  typedef pqTabbedMultiViewWidget Superclass;

public:
  pqViewManager(QWidget* parentW = nullptr);
  ~pqViewManager() override;

private:
  Q_DISABLE_COPY(pqViewManager)
};

#endif
