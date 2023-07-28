// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqVRAddStyleDialog_h
#define pqVRAddStyleDialog_h

#include <QDialog>

class vtkSMVRInteractorStyleProxy;

class pqVRAddStyleDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqVRAddStyleDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  virtual ~pqVRAddStyleDialog();

  void setInteractorStyle(vtkSMVRInteractorStyleProxy*, const QString& name);
  void updateInteractorStyle();

  // Returns true if there are any user-configurable options.
  bool isConfigurable();

private Q_SLOTS:

private:
  Q_DISABLE_COPY(pqVRAddStyleDialog)

  class pqInternals;
  pqInternals* Internals;
};

#endif
