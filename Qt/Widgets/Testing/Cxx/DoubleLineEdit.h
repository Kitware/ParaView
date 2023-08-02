// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef DoubleLineEdit_h
#define DoubleLineEdit_h

#include <QObject>

class DoubleLineEditTester : public QObject
{
  Q_OBJECT;
private Q_SLOTS:
  void basic();
  void useGlobalPrecision();
  void useGlobalPrecision_data();
};

#endif
