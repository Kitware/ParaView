// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDefaultMainWindow.h"
#include "ui_pqDefaultMainWindow.h"

class pqDefaultMainWindow::pqInternals : public Ui::MainWindow
{
};

//-----------------------------------------------------------------------------
pqDefaultMainWindow::pqDefaultMainWindow(QWidget* parentObject, Qt::WindowFlags winFlags)
  : Superclass(parentObject, winFlags)
{
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);
}

//-----------------------------------------------------------------------------
pqDefaultMainWindow::~pqDefaultMainWindow()
{
  delete this->Internals;
  this->Internals = nullptr;
}
