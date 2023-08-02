// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqVRAddConnectionDialog_h
#define pqVRAddConnectionDialog_h

#include <QDialog>

#include "vtkPVVRConfig.h"

#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
class pqVRPNConnection;
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
class pqVRUIConnection;
#endif

class pqVRAddConnectionDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqVRAddConnectionDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  virtual ~pqVRAddConnectionDialog();

#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
  void setConnection(pqVRPNConnection* conn);
  pqVRPNConnection* getVRPNConnection();
  bool isVRPN();
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
  void setConnection(pqVRUIConnection* conn);
  pqVRUIConnection* getVRUIConnection();
  bool isVRUI();
#endif

  void updateConnection();

public Q_SLOTS:
  void accept();

protected:
  void keyPressEvent(QKeyEvent*);

private Q_SLOTS:
  void addInput();
  void removeInput();

  void connectionTypeChanged();

private:
  Q_DISABLE_COPY(pqVRAddConnectionDialog)

  class pqInternals;
  pqInternals* Internals;
};

#endif
