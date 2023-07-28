// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#ifndef pqSLACDataLoadManager_h
#define pqSLACDataLoadManager_h

#include <QDialog>

class pqServer;

/// This dialog box provides an easy way to set up the readers in the pipeline
/// and to ready them for the rest of the SLAC tools.
class pqSLACDataLoadManager : public QDialog
{
  Q_OBJECT;

public:
  pqSLACDataLoadManager(QWidget* p, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqSLACDataLoadManager() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  virtual void checkInputValid();
  virtual void setupPipeline();

Q_SIGNALS:
  void createdPipeline();

protected:
  pqServer* Server;

private:
  Q_DISABLE_COPY(pqSLACDataLoadManager)

  class pqUI;
  pqUI* ui;
};

#endif // pqSLACDataLoadManager_h
