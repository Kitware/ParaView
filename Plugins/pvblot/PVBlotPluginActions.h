// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PVBlotPluginActions.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
