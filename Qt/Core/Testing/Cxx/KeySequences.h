// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef KeySequences_h
#define KeySequences_h

#include <QFrame>
#include <QList>

#include <array>

class QLabel;
class QLayout;
class QToolButton;

class pqModalShortcut;

class KeySequencesWidget : public QFrame
{
  Q_OBJECT;

public:
  using Superclass = QFrame;
  KeySequencesWidget(const std::string& label, const std::string& buttonLabel,
    const QKeySequence& shortcut, QWidget* parent = nullptr);
  ~KeySequencesWidget() override = default;

  QAction* action() const { return m_action; }

protected Q_SLOTS:
  virtual void onShortcutEnabled();
  virtual void onShortcutDisabled();
  virtual void demo();

protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEvent*) override;
#else
  void enterEvent(QEnterEvent*) override;
#endif
  void leaveEvent(QEvent*) override;
  void mousePressEvent(QMouseEvent*) override;
  void mouseReleaseEvent(QMouseEvent*) override;
  // TODO: Handle keyboard focus to enable/disable shortcuts.
  // void focusInEvent(QFocusEvent*) override;
  // void focusOutEvent(QFocusEvent*) override;

  QColor m_activeColor;
  QLabel* m_label;
  QAction* m_action;
  QToolButton* m_button;
  pqModalShortcut* m_shortcut;
  bool m_pressed;
};

class KeySequencesTester : public QObject
{
  Q_OBJECT;

public:
  KeySequencesTester()
    : m_counts{ 0, 0, 0, 0, 0 }
  {
  }
public Q_SLOTS:
  void a1();
  void a2();
  void a3();
  void a4();
  void b();
private Q_SLOTS:
  void basic();

protected:
  std::array<int, 5> m_counts;
  QLayout* m_layout;
  QList<QWidget*> m_widgets;
};
#endif
