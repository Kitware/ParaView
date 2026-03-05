// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   pqAbortTesting
 * @brief   Autostart plugin to trigger abort
 *
 * A simple autostart plugin that trigger abort if a progress signal
 * superior to 25 is received. For testing purposes only.
 */

#ifndef pqAbortTesting_h
#define pqAbortTesting_h

#include <QObject>

class pqServer;
class vtkObject;
class pqAbortTesting : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqAbortTesting(QObject* parent = nullptr);
  ~pqAbortTesting() override;

  /**
   * Connect pqProgressManager::progress to onProgress slot
   */
  void onStartup();

  /**
   * Empty, needed for plugin
   */
  void onShutdown(){};

private:
  Q_DISABLE_COPY(pqAbortTesting)

private Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Trigger abort if progress > 25
   */
  void onProgress(const QString& message, int progress);
};

#endif
