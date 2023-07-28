// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCommandLineOptionsBehavior_h
#define pqCommandLineOptionsBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

/**
 * @ingroup Behaviors
 * pqCommandLineOptionsBehavior processes command-line options on startup and
 * performs relevant actions such as loading data files, connecting to server
 * etc.
 * This also handles test playback and image comparison.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCommandLineOptionsBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqCommandLineOptionsBehavior(QObject* parent = nullptr);

  /**
   * Used during testing to "initialize" application state as much as possible.
   */
  static void resetApplication();

  /**
   * Handle server connection options. Processes `--server` and `--server-url`
   * command line arguments. In none provided, connect to the builtin server.
   */
  static void processServerConnection();

  /**
   * Handle plugins.
   */
  static void processPlugins();

  /**
   * Handle open data file requests (i.e. `--data`).
   */
  static void processData();

  /**
   * Handle state loading request (i.e. `--state`).
   */
  static void processState();

  /**
   * Handle script loading request (i.e. `--script`).
   */
  static void processScript();

  /**
   * Handle Catalyst live session connection request (i.e. `--live`).
   */
  static void processLive();

  /**
   * Returns test playback status.
   */
  static bool processTests();

protected Q_SLOTS:
  void processCommandLineOptions();

private:
  Q_DISABLE_COPY(pqCommandLineOptionsBehavior)
};

#endif
