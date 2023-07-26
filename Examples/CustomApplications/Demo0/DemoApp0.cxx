// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include <pqApplicationCore.h>

#include <QApplication>
#include <QMainWindow>

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  pqApplicationCore appCore(argc, argv);
  QMainWindow window;
  window.show();
  return app.exec();
}
