/*=========================================================================

   Program: ParaView
   Module:  pqModalShortcut.h

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
    QWidget* contextWidget, Qt::ShortcutContext contextArea = Qt::WidgetShortcut);

  bool isEnabled() const;
  void setEnabled(bool);

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
