/*=========================================================================

   Program: ParaView
   Module:  pqModalShortcut.cxx

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
#include "pqModalShortcut.h"

#include "pqProxy.h"
#include "pqQVTKWidget.h"

#include <QAction>
#include <QShortcut>
#include <QWidget>

namespace
{
// Special case to handle pqQVTKWidget. pqQVTKWidget creates an internal widget
// that doesn't bubble out events and hence we need to monitor shortcut events on
// that internal widget.
QWidget* sanitizedContext(QWidget* parent)
{
  if (auto qvtkwidget = qobject_cast<pqQVTKWidget*>(parent))
  {
    return qvtkwidget->renderWidget();
  }
  return parent;
}
}
pqModalShortcut::pqModalShortcut(const QKeySequence& key, QAction* action, QWidget* parent)
  : Superclass(parent)
  , m_key(key)
  , m_action(action)
{
  m_shortcut = new QShortcut(key, ::sanitizedContext(parent));
  // if parent, look for Q_SLOTS to auto-connect
  // if action, connect shortcut.
  QObject::connect(m_shortcut, &QShortcut::activated, this, &pqModalShortcut::activated);
  if (m_action)
  {
    QObject::connect(m_shortcut, &QShortcut::activated, m_action, &QAction::trigger);
  }
}

pqModalShortcut::~pqModalShortcut()
{
  Q_EMIT unregister();
  delete m_shortcut;
}

void pqModalShortcut::setContextWidget(QWidget* contextWidget, Qt::ShortcutContext contextArea)
{
  contextWidget = ::sanitizedContext(contextWidget);

  bool enabled = this->isEnabled();
  if (m_shortcut)
  {
    if (m_shortcut->parentWidget() == contextWidget)
    {
      if (m_shortcut->context() != contextArea)
      {
        m_shortcut->setContext(contextArea);
      }
      return;
    }
  }

  // To change parents, it's best to start over.
  delete m_shortcut;
  if (!contextWidget && contextArea != Qt::ApplicationShortcut)
  {
    // We need to keep a shortcut around, but don't pay attention
    // to it since the context widget is null.
    m_shortcut = nullptr;
  }
  else
  {
    m_shortcut = new QShortcut(m_key, contextWidget);
    m_shortcut->setEnabled(enabled);
    m_shortcut->setContext(contextArea);
    QObject::connect(m_shortcut, &QShortcut::activated, this, &pqModalShortcut::activated);
    if (m_action)
    {
      QObject::connect(m_shortcut, &QShortcut::activated, m_action, &QAction::trigger);
    }
  }
}

bool pqModalShortcut::isEnabled() const
{
  return m_shortcut ? m_shortcut->isEnabled() : false;
}

void pqModalShortcut::setEnabled(bool enable)
{
  if (!m_shortcut)
  {
    // View was destroyed.
    return;
  }

  if (enable)
  {
    if (!m_shortcut->isEnabled())
    {
      Q_EMIT enabled();
      m_shortcut->setEnabled(true);
      // Now if we have a context widget, give it focus
      // so that users can immediately use the key.
      auto ctxt = m_shortcut->context();
      if (ctxt == Qt::WidgetShortcut || ctxt == Qt::WidgetWithChildrenShortcut)
      {
        auto* parent = dynamic_cast<QWidget*>(m_shortcut->parent());
        if (parent)
        {
          parent->setFocus(Qt::OtherFocusReason);
        }
      }
    }
  }
  else
  {
    if (m_shortcut->isEnabled())
    {
      m_shortcut->setEnabled(false);
      Q_EMIT disabled();
    }
  }
}

QKeySequence pqModalShortcut::keySequence() const
{
  return m_key;
}
