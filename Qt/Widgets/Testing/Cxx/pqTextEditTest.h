// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqTextEditTest_h
#define pqTextEditTest_h

#include <QObject>

class QSignalSpy;
class pqTextEdit;

class pqTextEditTester : public QObject
{
  Q_OBJECT

public:
  enum SpyType
  {
    TextChanged = 0,
    EditingFinished,
    TextChangedAndEditingFinished,
  };

private Q_SLOTS:
  void init();
  void cleanup();

  void testSets();
  void testSets_data();
  void testTypeText();
  void testTypeText_data();
  void testFocus();
  void testFocus_data();
  void testReTypeText();
  void testReTypeText_data();

private: // NOLINT(readability-redundant-access-specifiers)
  pqTextEdit* TextEdit;
  QSignalSpy* TextChangedSpy;
  QSignalSpy* EditingFinishedSpy;
  QSignalSpy* TextChangedAndEditingFinishedSpy;

  QSignalSpy* spy(int spy);
};

#endif
