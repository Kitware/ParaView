/*=========================================================================

   Program: ParaView
   Module:  pqShortcutDecorator.h

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

#ifndef pqShortcutDecorator_h
#define pqShortcutDecorator_h

#include "pqPropertyWidgetDecorator.h"

class pqModalShortcut;

class QColor;

/**\brief Decorate a property widget by highlighting its frame
 *        when keyboard shortcuts are active.
 *
 * This also monitors enter/exit/mouse events to let users
 * activate/deactivate the widget's keyboard shortcuts.
 */
class PQCOMPONENTS_EXPORT pqShortcutDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT;

public:
  using Superclass = pqPropertyWidgetDecorator;
  pqShortcutDecorator(pqPropertyWidget* parent);
  pqShortcutDecorator(vtkPVXMLElement*, pqPropertyWidget* parent);

  /**
   * Add a shortcut the parent widget should respond to.
   */
  void addShortcut(pqModalShortcut* shortcut);

  bool isEnabled() const;

protected Q_SLOTS:
  /**
   * Called when any shortcut is enabled (and will enable them all and mark the widget).
   *
   * When new shortcuts are added to a property widget, this ensures any existing ones
   * are also activated so the state is consistent.
   */
  virtual void onShortcutEnabled();
  /**
   * Called when any shortcut is disabled (and will disable them all and mark the widget).
   *
   * When an individual shortcut is disabled (usually because a separate widget has
   * taken possession of it), this ensures any other shortcuts for _this_ widget
   * are also deactivated so the state is consistent.
   */
  virtual void onShortcutDisabled();
  /**
   * Called to force all shortcuts for the attached property widget to enable/disable.
   *
   * This is tied internally to pqPropertyWidget::widgetVisibilityUpdated, which passes
   * a boolean state so that shortcuts are disabled when the widget is not visible and
   * the widget grabs shortcuts when it becomes visible.
   */
  virtual void setEnabled(bool enable);

protected:
  /**
   * The parent property widget (returned as a pqPropertyWidget, not QWidget).
   */
  pqPropertyWidget* propertyWidget() const;
  /**
   * Monitor mouse events allowing users to enable/disable the parent-widget's shortcuts.
   */
  bool eventFilter(QObject* obj, QEvent* event) override;
  /**
   * Show/hide and color the parent widget's frame.
   */
  void markFrame(bool active, const QColor& frameColor);

  // All the shortcuts that decorate the property widget.
  // These will all be enabled/disabled en banc.
  QList<QPointer<pqModalShortcut>> m_shortcuts;
  // Note when the user has pressed the mouse inside the widget and not released it.
  bool m_pressed;
  // Prevent recursive signaling inside onShortcutEnabled/onShortcutDisabled.
  bool m_silent;
};

#endif // pqShortcutDecorator_h
