// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqBlotDialog.h

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

#ifndef pqBlotDialog_h
#define pqBlotDialog_h

#include <QDialog>

class pqBlotShell;
class pqServer;

/**
   Qt dialog that embeds an instance of pqBlotShell, providing the user
   with an interactive pvblot console.

   \sa pqPythonDialog
*/

class pqBlotDialog : public QDialog
{
  Q_OBJECT;

public:
  pqBlotDialog(QWidget* p);
  ~pqBlotDialog();

  virtual pqServer* activeServer() const;
  virtual void setActiveServer(pqServer* server);

public Q_SLOTS:
  virtual void open();
  virtual void open(const QString& filename);
  virtual void runScript();
  virtual void runScript(const QStringList& files);

protected Q_SLOTS:
  virtual void open(const QStringList& filenames);

private:
  Q_DISABLE_COPY(pqBlotDialog)

  class UI;
  UI* ui;
};

/**
   Internal class for converting action signals to slots that execute commands.
*/
class pqBlotDialogExecuteAction : QObject
{
  Q_OBJECT;

public:
  pqBlotDialogExecuteAction(QObject* parent, const QString& command);

  static pqBlotDialogExecuteAction* connect(QAction* action, pqBlotShell* shell);

public Q_SLOTS:
  virtual void trigger();

Q_SIGNALS:
  void triggered(const QString& command);

protected:
  QString Command;
};

#endif // pqBlotDialog_h
