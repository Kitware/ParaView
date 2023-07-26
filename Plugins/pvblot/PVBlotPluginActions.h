// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#ifndef PVBlotPluginActions_h
#define PVBlotPluginActions_h

#include <QActionGroup>

class pqBlotDialog;
class pqServer;

/// Adds an action for starting the PVBlot interpreter window.
class PVBlotPluginActions : public QActionGroup
{
  Q_OBJECT;

public:
  PVBlotPluginActions(QObject* p);

  virtual pqServer* activeServer();
  virtual QWidget* mainWindow();

public Q_SLOTS:
  virtual void startPVBlot();
  virtual void startPVBlot(const QString& filename);

protected Q_SLOTS:
  virtual void startPVBlot(const QStringList& filenames);

private:
  PVBlotPluginActions(const PVBlotPluginActions&) = delete;
  void operator=(const PVBlotPluginActions&) = delete;
};

#endif // PVBlotPluginActions_h
