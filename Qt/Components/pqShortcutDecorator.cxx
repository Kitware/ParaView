/*=========================================================================

   Program: ParaView
   Module:  pqShortcutDecorator.cxx

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
      shortcut->setEnabled(true);
    }
  }
  m_silent = false;

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

void pqShortcutDecorator::setEnabled(bool enable)
{
  if (enable)
  {
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
          this->setEnabled(!this->isEnabled());
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
