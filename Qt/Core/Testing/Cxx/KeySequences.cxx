// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "KeySequences.h"

#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QShortcut>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <pqKeySequences.h>
#include <pqModalShortcut.h>

#include <iostream>

KeySequencesWidget::KeySequencesWidget(const std::string& label, const std::string& buttonLabel,
  const QKeySequence& shortcut, QWidget* parent)
  : QFrame(parent, Qt::WindowFlags())
  , m_pressed(false)
{
  m_activeColor = this->palette().link().color();
  this->setFrameStyle(QFrame::Box | QFrame::Plain);
  this->setLineWidth(2);
  this->setObjectName("ksw");
  this->setStyleSheet(QString("QFrame#ksw {color: rgb(%1, %2, %3); }")
                        .arg(m_activeColor.red())
                        .arg(m_activeColor.green())
                        .arg(m_activeColor.blue()));
  m_action = new QAction(this);
  m_action->setText(buttonLabel.c_str());

  m_button = new QToolButton(this);
  m_button->setDefaultAction(m_action);
  m_button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
  m_label = new QLabel(label.c_str(), this);
  auto* hl = new QHBoxLayout(this);
  hl->addWidget(m_label);
  hl->addWidget(m_button);
  hl->setContentsMargins(1, 1, 1, 1);
  m_shortcut = pqKeySequences::instance().addModalShortcut(shortcut, m_action, this);
  m_shortcut->setObjectName(label.c_str());
  QObject::connect(
    m_shortcut, &pqModalShortcut::enabled, this, &KeySequencesWidget::onShortcutEnabled);
  QObject::connect(
    m_shortcut, &pqModalShortcut::disabled, this, &KeySequencesWidget::onShortcutDisabled);
  QObject::connect(m_action, &QAction::triggered, this, &KeySequencesWidget::demo);
}

void KeySequencesWidget::demo()
{
  m_shortcut->setEnabled(!m_shortcut->isEnabled());
  pqKeySequences::instance().dumpShortcuts(m_shortcut->keySequence());
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void KeySequencesWidget::enterEvent(QEvent* e)
#else
void KeySequencesWidget::enterEvent(QEnterEvent* e)
#endif
{
  this->setFrameShape(QFrame::Box);
  if (!m_shortcut->isEnabled())
  {
    QColor darker = m_activeColor.darker();
    this->setStyleSheet(QString("QFrame#ksw {color: rgb(%1, %2, %3); }")
                          .arg(darker.red())
                          .arg(darker.green())
                          .arg(darker.blue()));
  }
  this->Superclass::enterEvent(e);
}

void KeySequencesWidget::leaveEvent(QEvent* e)
{
  m_pressed = false;
  if (m_shortcut->isEnabled())
  {
    this->setFrameShape(QFrame::Box);
    this->setStyleSheet(QString("QFrame#ksw {color: rgb(%1, %2, %3); }")
                          .arg(m_activeColor.red())
                          .arg(m_activeColor.green())
                          .arg(m_activeColor.blue()));
  }
  else
  {
    this->setFrameShape(QFrame::NoFrame);
    QColor defColor = this->palette().windowText().color();
    this->setStyleSheet(QString("QFrame#ksw {color: rgb(%1, %2, %3); }")
                          .arg(defColor.red())
                          .arg(defColor.green())
                          .arg(defColor.blue()));
  }
  this->Superclass::leaveEvent(e);
}

void KeySequencesWidget::mousePressEvent(QMouseEvent* e)
{
  m_pressed = true;
  this->Superclass::mousePressEvent(e);
}

void KeySequencesWidget::mouseReleaseEvent(QMouseEvent* e)
{
  if (m_pressed)
  {
    // Reorder how shortcuts will cycle so that the previous
    // sibling is this shortcut's "next".
    pqKeySequences::instance().reorder(m_shortcut);
    m_shortcut->setEnabled(!m_shortcut->isEnabled());
  }
  this->Superclass::mouseReleaseEvent(e);
}

void KeySequencesWidget::onShortcutEnabled()
{
  this->setFrameShape(QFrame::Box);
  this->setStyleSheet(QString("QFrame#ksw {color: rgb(%1, %2, %3); }")
                        .arg(m_activeColor.red())
                        .arg(m_activeColor.green())
                        .arg(m_activeColor.blue()));
}

void KeySequencesWidget::onShortcutDisabled()
{
  this->setFrameShape(QFrame::NoFrame);
  QColor defColor = this->palette().windowText().color();
  this->setStyleSheet(QString("QFrame#ksw {color: rgb(%1, %2, %3); }")
                        .arg(defColor.red())
                        .arg(defColor.green())
                        .arg(defColor.blue()));
}

void KeySequencesTester::a1()
{
  ++m_counts[1];
  std::cout << "1\n";
}

void KeySequencesTester::a2()
{
  ++m_counts[2];
  std::cout << "2\n";
}

void KeySequencesTester::a3()
{
  ++m_counts[3];
  std::cout << "3\n";
}

void KeySequencesTester::a4()
{
  ++m_counts[4];
  std::cout << "4\n";
}

void KeySequencesTester::b()
{
  if (m_widgets.empty())
  {
    return;
  }
  QWidget* w = *m_widgets.begin();
  m_widgets.erase(m_widgets.begin());
  ++m_counts[0];
  // m_layout->removeAt(w);
  delete w;
}

void KeySequencesTester::basic()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
  auto* window = new QMainWindow;
  auto* widget = new QWidget(window);
  window->setCentralWidget(widget);
  auto* layout = new QVBoxLayout(widget);
  layout->setContentsMargins(1, 1, 1, 1);
  layout->setSpacing(1);

  auto* w1 = new KeySequencesWidget("Thing 1", "1", QKeySequence(tr("Ctrl+A")), widget);
  auto* w2 = new KeySequencesWidget("Thing 2", "2", QKeySequence(tr("Ctrl+A")), widget);
  auto* w3 = new KeySequencesWidget("Thing 3", "3", QKeySequence(tr("Ctrl+A")), widget);
  auto* w4 = new KeySequencesWidget("Thing 4", "4", QKeySequence(tr("Ctrl+A")), widget);
  layout->addWidget(w1);
  layout->addWidget(w2);
  layout->addWidget(w3);
  layout->addWidget(w4);
  QObject::connect(w1->action(), &QAction::triggered, this, &KeySequencesTester::a1);
  QObject::connect(w2->action(), &QAction::triggered, this, &KeySequencesTester::a2);
  QObject::connect(w3->action(), &QAction::triggered, this, &KeySequencesTester::a3);
  QObject::connect(w4->action(), &QAction::triggered, this, &KeySequencesTester::a4);

  m_layout = layout;
  m_widgets.push_back(w1);
  m_widgets.push_back(w2);
  m_widgets.push_back(w3);
  m_widgets.push_back(w4);
  auto* ab = new QAction(widget);
  pqKeySequences::instance().addModalShortcut(QKeySequence(tr("Ctrl+B")), ab, widget);
  QObject::connect(ab, &QAction::triggered, this, &KeySequencesTester::b);

  window->show();

  QVERIFY(QTest::qWaitForWindowActive(window));
  QTest::keySequence(window, QKeySequence("Ctrl+B")); // deletes w1
  QTest::keySequence(window, QKeySequence("Ctrl+A")); // calls a4()
  QTest::keySequence(window, QKeySequence("Ctrl+A")); // no effect
  QTest::mouseClick(w2, Qt::LeftButton);              // enable w2
  QTest::keySequence(window, QKeySequence("Ctrl+A")); // calls a2()
  QTest::keySequence(window, QKeySequence("Ctrl+A")); // no effect
  QTest::keySequence(window, QKeySequence("Ctrl+B")); // deletes w2

  if (!qgetenv("DEBUG_TEST").isEmpty())
  {
    qApp->exec();
  }
  std::cout << "  b  " << m_counts[0] << "\n";
  for (int ii = 1; ii < 5; ++ii)
  {
    std::cout << "  a" << ii << " " << m_counts[ii] << "\n";
  }
  QVERIFY2(m_counts[0] == 2, "Should have activated b twice.");
  QVERIFY2(m_counts[1] == 0, "Improperly activated a1.");
  QVERIFY2(m_counts[2] == 1, "Never activated a2.");
  QVERIFY2(m_counts[3] == 0, "Improperly activated a3.");
  QVERIFY2(m_counts[4] == 1, "Never activated a4.");
#endif // QT >= 5.10.0
}

int KeySequences(int argc, char* argv[])
{
  QApplication app(argc, argv);
  KeySequencesTester tester;
  return QTest::qExec(&tester, argc, argv);
}
