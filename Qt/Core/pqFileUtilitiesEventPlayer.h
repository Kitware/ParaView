// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqFileUtilitiesEventPlayer_h
#define pqFileUtilitiesEventPlayer_h

#include "pqCoreModule.h" // for exports.
#include <pqWidgetEventPlayer.h>

/**
 * @class pqFileUtilitiesEventPlayer
 * @brief adds support for miscellaneous file-system utilities for test playback
 *
 * pqFileUtilitiesEventPlayer is a pqWidgetEventPlayer subclass that adds
 * support for a set of commands that help with file system checks and changes
 * e.g. testing for file, removing file, copying file etc.
 */
class PQCORE_EXPORT pqFileUtilitiesEventPlayer : public pqWidgetEventPlayer
{
  Q_OBJECT
  typedef pqWidgetEventPlayer Superclass;

public:
  pqFileUtilitiesEventPlayer(QObject* parent = nullptr);

  using Superclass::playEvent;
  bool playEvent(
    QObject* qobject, const QString& command, const QString& args, bool& errorFlag) override;

private:
  Q_DISABLE_COPY(pqFileUtilitiesEventPlayer);
};

#endif
