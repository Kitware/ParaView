// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLoadPaletteReaction_h
#define pqLoadPaletteReaction_h

#include "pqReaction.h"
#include <QPointer>

class QMenu;

/**
 * @ingroup Reactions
 * pqLoadPaletteReaction is used to setup an action that allows the user to
 * load a palette. It setups up menu on the parent action which is populated
 * with available palettes.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqLoadPaletteReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqLoadPaletteReaction(QAction* parent = nullptr);
  ~pqLoadPaletteReaction() override;

protected:
  void updateEnableState() override;

private Q_SLOTS:
  void populateMenu();
  void actionTriggered(QAction* actn);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqLoadPaletteReaction)
  QPointer<QMenu> Menu;
};

#endif
