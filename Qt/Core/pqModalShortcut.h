// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqModalShortcut_h
#define pqModalShortcut_h

#include "pqProxy.h"
#include <QKeySequence>
#include <QObject>

class QAction;
class QShortcut;
class pqModalShortcut;

/**\brief Manage an action and/or widget's responsivity to a shortcut.
 *
 * This object will add and remove a connection between
 * a widget/action and a QShortcut as required by the pqKeySequences
 * manager to prevent any ambiguous activations.
 */
class PQCORE_EXPORT pqModalShortcut : public QObject
{
  Q_OBJECT;

public:
  using Superclass = QObject;
  ~pqModalShortcut() override;

  /// If the shortcut should be restricted to a particular widget
  /// (such as a pqView), use this method to set and update the widget
  /// during the life of the pqModalShortcut.
  void setContextWidget(
    QWidget* contextWidget, Qt::ShortcutContext contextArea = Qt::WindowShortcut);

  bool isEnabled() const;
  void setEnabled(bool shouldEnable, bool changeFocus = true);

  QKeySequence keySequence() const;

Q_SIGNALS:
  /// Called from setEnabled() whenever it is passed true.
  ///
  /// This is used by pqKeySequences to disable any sibling shortcuts
  /// with the same keysequence.
  ///
  /// This may also be used by widgets to update their visual state, indicating
  /// they are now accepting shortcuts.
  void enabled();

  /// Called from setEnabled() whenever it is passed false.
  ///
  /// This may be used by widgets to update their visual state, indicating
  /// they are no longer accepting shortcuts.
  void disabled();

  /// Called from the destructor.
  ///
  /// This is used by pqKeySequences to clean its records.
  void unregister();

  /// Invoked when the key sequence is pressed.
  void activated();

protected:
  friend class pqKeySequences;
  pqModalShortcut(const QKeySequence& key, QAction* action = nullptr, QWidget* parent = nullptr);

  QKeySequence m_key;
  QPointer<QShortcut> m_shortcut;
  QPointer<QAction> m_action;
};

#endif
