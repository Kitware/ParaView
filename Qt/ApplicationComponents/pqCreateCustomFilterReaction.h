// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCreateCustomFilterReaction_h
#define pqCreateCustomFilterReaction_h

#include "pqReaction.h"

/**
 * pqCreateCustomFilterReaction popups the create-custom-filter wizard for the
 * active selection.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCreateCustomFilterReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqCreateCustomFilterReaction(QAction* parent);

  /**
   * Create custom filter.
   */
  static void createCustomFilter();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqCreateCustomFilterReaction::createCustomFilter(); }

private:
  Q_DISABLE_COPY(pqCreateCustomFilterReaction)
};

#endif
