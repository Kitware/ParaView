// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  // XXX(gcc-4.8): This is a workaround for a bug in gcc-4.8.0.
  QObject::connect(m_shortcut, SIGNAL(activated()), this, SIGNAL(activated()));
  if (m_action)
  {
    // XXX(gcc-4.8): This is a workaround for a bug in gcc-4.8.0.
    QObject::connect(m_shortcut, SIGNAL(activated()), m_action, SLOT(trigger()));
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
    if (qobject_cast<QWidget*>(m_shortcut->parent()) == contextWidget)
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
    // XXX(gcc-4.8): This is a workaround for a bug in gcc-4.8.0.
    QObject::connect(m_shortcut, SIGNAL(activated()), this, SIGNAL(activated()));
    if (m_action)
    {
      // XXX(gcc-4.8): This is a workaround for a bug in gcc-4.8.0.
      QObject::connect(m_shortcut, SIGNAL(activated()), m_action, SLOT(trigger()));
    }
  }
}

bool pqModalShortcut::isEnabled() const
{
  return m_shortcut ? m_shortcut->isEnabled() : false;
}

void pqModalShortcut::setEnabled(bool enable, bool changeFocus)
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
      // Now if we have a context widget with window context, give it focus
      // so that users can immediately use the key.
      auto ctxt = m_shortcut->context();
      if ((ctxt == Qt::WidgetShortcut || ctxt == Qt::WidgetWithChildrenShortcut) && changeFocus)
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
