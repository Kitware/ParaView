// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqVRDockPanel_h
#define pqVRDockPanel_h

#include <QDockWidget>

class pqView;
class QListWidgetItem;
class vtkObject;
class vtkSMProxy;

class pqVRDockPanel : public QDockWidget
{
  Q_OBJECT
  typedef QDockWidget Superclass;

public:
  pqVRDockPanel(const QString& t, QWidget* p = nullptr, Qt::WindowFlags f = Qt::WindowFlags{})
    : Superclass(t, p, f)
  {
    this->constructor();
  }
  pqVRDockPanel(QWidget* p = nullptr, Qt::WindowFlags f = Qt::WindowFlags{})
    : Superclass(p, f)
  {
    this->constructor();
  }
  virtual ~pqVRDockPanel();

private Q_SLOTS:
  void addConnection();
  void removeConnection();
  void updateConnections();
  void editConnection(QListWidgetItem* item = nullptr);
  void updateConnectionButtons(int row);

  void addStyle();
  void removeStyle();
  void initStyles();
  void updateStyles();
  void editStyle(QListWidgetItem* item = nullptr);
  void configureStyle(vtkObject*, long unsigned int, void*);
  void updateStyleButtons(int row);

  void proxyChanged(vtkSMProxy*);
  void styleComboChanged(const QString& name);
  void setActiveView(pqView*);

  void saveState();
  void restoreState();

  void disableConnectionButtons();
  void enableConnectionButtons();

  void updateStartStopButtonStates();
  void start();
  void stop();

private:
  Q_DISABLE_COPY(pqVRDockPanel)

  void constructor();

  class pqInternals;
  pqInternals* Internals;
};

#endif
