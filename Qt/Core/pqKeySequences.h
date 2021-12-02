/*=========================================================================

   Program: ParaView
   Module:  pqKeySequences.h

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

protected:
  pqKeySequences(QObject* parent);
  ~pqKeySequences() override = default;

  bool m_silence; // Set true in slot implementations to avoid signal/slot recursion.
};

#endif
