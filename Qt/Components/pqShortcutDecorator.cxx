// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqShortcutDecorator.h"

#include "pqKeySequences.h"
#include "pqModalShortcut.h"
#include "pqPropertyWidget.h"

#include <QEvent>
#include <QKeySequence>
#include <QMetaObject>

pqShortcutDecorator::pqShortcutDecorator(pqPropertyWidget* parent)
  : Superclass(nullptr, parent)
  , m_pressed(false)
  , m_silent(false)
  , m_allowRefocus(false)
{
  parent->setLineWidth(2);
  parent->setFrameStyle(QFrame::NoFrame | QFrame::Plain);
  this->markFrame(false, QColor(0, 0, 0, 0));

  parent->installEventFilter(this);

  // We can't cast to pqInteractivePropertyWidget because that
  // class is in a separate library. Instead see if the slot we
  // care about exists.
  if (parent->metaObject()->indexOfMethod(
        QMetaObject::normalizedSignature("widgetVisibilityUpdated(bool)")) >= 0)
  {
    QObject::connect(parent, SIGNAL(widgetVisibilityUpdated(bool)), this, SLOT(setEnabled(bool)));
  }
}

pqShortcutDecorator::pqShortcutDecorator(vtkPVXMLElement* xml, pqPropertyWidget* parent)
  : Superclass(xml, parent)
  , m_pressed(false)
  , m_silent(false)
{
  parent->setFrameStyle(QFrame::Box | QFrame::Plain);
  parent->setLineWidth(2);
  parent->installEventFilter(this);
}

void pqShortcutDecorator::addShortcut(pqModalShortcut* shortcut)
{
  m_shortcuts.push_back(shortcut);
  this->onShortcutEnabled(); // make border active; color it. enable all shortcuts for this widget

  // m_shortcut = pqKeySequences::instance().addModalShortcut(shortcut, m_action, this);
  // m_shortcut->setObjectName(label.c_str());
  QObject::connect(
    shortcut, &pqModalShortcut::enabled, this, &pqShortcutDecorator::onShortcutEnabled);
  QObject::connect(
    shortcut, &pqModalShortcut::disabled, this, &pqShortcutDecorator::onShortcutDisabled);
}

bool pqShortcutDecorator::isEnabled() const
{
  // All attached shortcuts should have the same state, so just take the first (if any).
  if (m_shortcuts.empty())
  {
    return false;
  }

  for (auto& shortcut : m_shortcuts)
  {
    if (shortcut)
    {
      return shortcut->isEnabled();
    }
  }

  return false;
}

void pqShortcutDecorator::onShortcutEnabled()
{
  if (m_silent)
  {
    return;
  }
  // One shortcut was just enabled... but to mark ourselves
  // as active, we must activate all of them.
  m_silent = true;
  for (auto& shortcut : m_shortcuts)
  {
    if (shortcut)
    {
      shortcut->setEnabled(true, m_allowRefocus);
    }
  }
  m_silent = false;
  m_allowRefocus = false; // Always reset after use.

  // Set the visual style
  auto* propWidget = this->propertyWidget();
  this->markFrame(true, propWidget->palette().link().color());
}

void pqShortcutDecorator::onShortcutDisabled()
{
  if (m_silent)
  {
    return;
  }
  // One shortcut was just disabled... but to mark ourselves
  // as inactive, we must deactivate all of them.
  m_silent = true;
  for (auto& shortcut : m_shortcuts)
  {
    if (shortcut)
    {
      shortcut->setEnabled(false);
    }
  }
  m_silent = false;

  // Set the visual style
  this->markFrame(false, QColor(0, 0, 0, 0));
}

void pqShortcutDecorator::setEnabled(bool enable, bool refocusWhenEnabling)
{
  if (enable)
  {
    m_allowRefocus = refocusWhenEnabling; // This will be reset inside onShortcutEnabled.
    // This has the effect of turning all shortcuts on.
    this->onShortcutEnabled();
  }
  else
  {
    // This has the effect of turning all shortcuts on.
    this->onShortcutDisabled();
  }
}

pqPropertyWidget* pqShortcutDecorator::propertyWidget() const
{
  return qobject_cast<pqPropertyWidget*>(this->parent());
}

bool pqShortcutDecorator::eventFilter(QObject* obj, QEvent* event)
{
  if (obj != this)
  {
    switch (event->type())
    {
      case QEvent::Enter:
        this->markFrame(true, this->propertyWidget()->palette().link().color().darker());
        break;
      case QEvent::Leave:
      {
        auto* propWidget = this->propertyWidget();
        bool ena = this->isEnabled();
        this->markFrame(ena, ena ? propWidget->palette().link().color() : QColor(0, 0, 0, 0));
        m_pressed = false;
      }
      break;
      case QEvent::MouseButtonPress:
        m_pressed = true;
        break;
      case QEvent::MouseButtonRelease:
        if (m_pressed)
        {
          m_pressed = false;
          // Reorder how shortcuts will cycle so that the previous
          // sibling is this shortcut's "next".
          for (auto& shortcut : m_shortcuts)
          {
            if (shortcut)
            {
              pqKeySequences::instance().reorder(shortcut);
            }
          }
          this->setEnabled(!this->isEnabled(), true);
          // Eat this mouse event:
          return true;
        }
        break;
      default:
        // do nothing
        break;
    }
  }
  return Superclass::eventFilter(obj, event);
}

void pqShortcutDecorator::markFrame(bool active, const QColor& frameColor)
{
  (void)active; // This can be used to modulate line width, frame shape.
  auto* propWidget = this->propertyWidget();
  propWidget->setFrameShape(QFrame::Box);
  propWidget->setLineWidth(2);
  propWidget->setStyleSheet(QString("QFrame#%1 {color: rgba(%2, %3, %4, %5); }")
                              .arg(propWidget->objectName())
                              .arg(frameColor.red())
                              .arg(frameColor.green())
                              .arg(frameColor.blue())
                              .arg(frameColor.alpha()));
}
