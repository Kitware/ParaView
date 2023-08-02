// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "myMainWindow.h"

#include <pqPVApplicationCore.h>

#include <QApplication>

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  pqPVApplicationCore appCore(argc, argv);
  myMainWindow window;
  window.show();
  return app.exec();
}
