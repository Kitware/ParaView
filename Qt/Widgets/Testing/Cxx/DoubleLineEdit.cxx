// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "DoubleLineEdit.h"

#include <QApplication>
#include <QTest>
#include <QVBoxLayout>
#include <QWidget>
#include <pqDoubleLineEdit.h>

// NOLINTNEXTLINE(performance-no-int-to-ptr)
Q_DECLARE_METATYPE(pqDoubleLineEdit::RealNumberNotation);
void DoubleLineEditTester::basic()
{
  QWidget window;
  auto wl = new QVBoxLayout(&window);
  wl->setContentsMargins(10, 10, 10, 10);

  QWidget* container = new QWidget(&window);
  wl->addWidget(container);

  auto l = new QVBoxLayout(container);
  auto e = new pqDoubleLineEdit();
  e->setText("2.121");
  l->addWidget(new QLineEdit("text"));
  l->addWidget(e);

  window.show();
}

void DoubleLineEditTester::useGlobalPrecision_data()
{
  QTest::addColumn<int>("global_precision");
  QTest::addColumn<int>("local_precision");
  QTest::addColumn<pqDoubleLineEdit::RealNumberNotation>("global_notation");
  QTest::addColumn<pqDoubleLineEdit::RealNumberNotation>("local_notation");
  QTest::addColumn<QString>("text");
  QTest::addColumn<QString>("simplifiedText");

  QTest::newRow("case 1") << 3 << 1 << pqDoubleLineEdit::ScientificNotation
                          << pqDoubleLineEdit::FixedNotation << "3.98888888"
                          << "3.989e+00";

  QTest::newRow("case 2") << 5 << 1 << pqDoubleLineEdit::ScientificNotation
                          << pqDoubleLineEdit::MixedNotation << "3.98888888"
                          << "3.98889e+00";

  QTest::newRow("case 3") << 2 << 1 << pqDoubleLineEdit::FixedNotation
                          << pqDoubleLineEdit::MixedNotation << "3.98888888"
                          << "3.99";

  QTest::newRow("case 4") << 1 << 1 << pqDoubleLineEdit::FullNotation
                          << pqDoubleLineEdit::FixedNotation << "3.98888888"
                          << "3.98888888";

  QTest::newRow("case 5") << static_cast<int>(QLocale::FloatingPointShortest)
                          << static_cast<int>(QLocale::FloatingPointShortest)
                          << pqDoubleLineEdit::FixedNotation << pqDoubleLineEdit::FixedNotation
                          << "3.98888888"
                          << "3.98888888";
}

void DoubleLineEditTester::useGlobalPrecision()
{
  QFETCH(int, global_precision);
  QFETCH(int, local_precision);
  QFETCH(pqDoubleLineEdit::RealNumberNotation, global_notation);
  QFETCH(pqDoubleLineEdit::RealNumberNotation, local_notation);
  QFETCH(QString, text);
  QFETCH(QString, simplifiedText);

  // global changed before.
  pqDoubleLineEdit::setGlobalPrecisionAndNotation(global_precision, global_notation);

  pqDoubleLineEdit e;
  e.setUseGlobalPrecisionAndNotation(true);
  e.setPrecision(local_precision);
  e.setNotation(local_notation);
  e.setText(text);

  QCOMPARE(e.simplifiedText(), simplifiedText);

  // change global setting.
  pqDoubleLineEdit::setGlobalPrecisionAndNotation(local_precision, local_notation);

  // create new widget using local setting.
  pqDoubleLineEdit e1;
  e1.setUseGlobalPrecisionAndNotation(false);
  e1.setPrecision(local_precision);
  e1.setNotation(local_notation);
  e1.setText(text); // set text using local setting.
  e1.setUseGlobalPrecisionAndNotation(true);

  // global changed afterwords.
  pqDoubleLineEdit::setGlobalPrecisionAndNotation(global_precision, global_notation);

  QCOMPARE(e1.simplifiedText(), simplifiedText);
}

int DoubleLineEdit(int argc, char* argv[])
{
  QApplication app(argc, argv);
  DoubleLineEditTester tester;
  return QTest::qExec(&tester, argc, argv);
}
