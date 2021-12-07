/*=========================================================================

   Program: ParaView
   Module:  KeySequences.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

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
  void enterEvent(QEvent*) override;
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
