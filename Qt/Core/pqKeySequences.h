// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqKeySequences_h
#define pqKeySequences_h

#include "pqProxy.h"
#include <QWidget>

class QAction;
class pqModalShortcut;

/**\brief Manage key sequences used for shortcuts.
 *
 * Rather use Qt's approach for shortcuts (which deals with
 * multiple listeners for the same key-sequence by notifying
 * all of them of ambiguous activation), this class
 * deactivates all but one listener so there can be no
 * ambiguity. Listeners are expected to (1) signal when
 * the user has requested they become the single active
 * listener and (2) respond to activation/deactivation by
 * updating their appearance so users can discern which
 * listener will respond to a given key sequence.
 */
class PQCORE_EXPORT pqKeySequences : public QObject
{
  Q_OBJECT
public:
  static pqKeySequences& instance();

  /**
   * Return the active shortcut for a given key sequence (if any).
   */
  pqModalShortcut* active(const QKeySequence& keySequence) const;

  /**
   * Inform the manager of an \a action (and paired \a parent)
   * that should be activated for the \a keySequence.
   */
  pqModalShortcut* addModalShortcut(
    const QKeySequence& keySequence, QAction* action, QWidget* parent);

  /**
   * Ask the manager to reorder shortcuts so that the currently-active
   * one becomes the "next" in line to the passed \a target.
   *
   * ** NB: This method is currently a placeholder. **
   *
   * Widgets should call this method before invoking pqModalShortCut::setEnabled
   * in response to user input.
   *
   * This method has no effect if no sibling of \a target is active
   * at the time it is invoked.
   */
  void reorder(pqModalShortcut* target);

  /**
   * Dump a list of shortcuts registered for a given key sequence.
   */
  void dumpShortcuts(const QKeySequence& keySequence) const;

protected Q_SLOTS:
  /**
   * Called when a shortcut is enabled to ensure siblings are disabled.
   */
  virtual void disableSiblings();
  /**
   * Not currently used. Intended for use enabling next-most-recently-used shortcut.
   */
  virtual void enableNextSibling();
  /**
   * Called when shortcuts are deleted to disable and unregister them.
   */
  virtual void removeModalShortcut();

protected: // NOLINT(readability-redundant-access-specifiers)
  pqKeySequences(QObject* parent);
  ~pqKeySequences() override = default;

  bool m_silence; // Set true in slot implementations to avoid signal/slot recursion.
};

#endif
