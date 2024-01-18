// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

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

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Called to force all shortcuts for the attached property widget to enable/disable.
   *
   * This is tied internally to pqPropertyWidget::widgetVisibilityUpdated, which passes
   * a boolean state so that shortcuts are disabled when the widget is not visible and
   * the widget grabs shortcuts when it becomes visible.
   *
   * If \a refocusWhenEnabling is true, and if an enabled shortcut has a context
   * widget, the keyboard focus will shift to that widget (so that users can
   * immediately use it). This parameter is false by default and should only be
   * enabled when direct user interaction with the decorated frame is what causes
   * the call to setEnabled().
   */
  virtual void setEnabled(bool enable, bool refocusWhenEnabling = false);

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

protected: // NOLINT(readability-redundant-access-specifiers)
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
  // Should shortcuts set the keyboard focus to their context widget?
  // This is set to true when users explicitly click on the widget frame
  // and false otherwise (so that programmatic changes to the widget made
  // in response to other events do not interrupt a user; for example,
  // using the arrow keys in the pipeline browser to change pipelines
  // should not move the keyboard focus away from the pipeline browser).
  // Note that this only applies when enabling shortcuts (since disabling
  // a shortcut would never require a refocus).
  bool m_allowRefocus;
};

#endif // pqShortcutDecorator_h
