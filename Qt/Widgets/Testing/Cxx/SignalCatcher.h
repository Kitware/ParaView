// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef SignalCatcher_h
#define SignalCatcher_h

#include <QObject>
#include <iostream>

class SignalCatcher : public QObject
{
  Q_OBJECT

public:
  explicit SignalCatcher(QObject* _parent = nullptr)
    : QObject(_parent)
  {
  }

public Q_SLOTS:
  void onValuesChanged(int min, int max)
  {
    std::cout << "Integer, min: " << min << std::endl;
    std::cout << "Integer, max: " << max << std::endl;
  }

  void onValuesChanged(double min, double max)
  {
    std::cout << "Double, min: " << min << std::endl;
    std::cout << "Double, max: " << max << std::endl;
  }

private:
};

#endif
