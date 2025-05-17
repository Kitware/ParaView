// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqPythonTextArea.h"

#include <QTabBar>

#include <iostream>

//-----------------------------------------------------------------------------
template <>
inline void pqPythonTabWidget::linkTo<QTextEdit>(QTextEdit* obj)
{
  const auto FindLinkedWidget = [this](const QObject* localObj) -> pqPythonTextArea*
  {
    for (int i = 0; i < this->count() - 1; ++i)
    {
      pqPythonTextArea* widget = this->getWidget<pqPythonTextArea>(i);
      if (widget && widget->isLinkedTo(localObj))
      {
        return widget;
      }
    }

    return nullptr;
  };

  pqPythonTextArea* linkedWidget = FindLinkedWidget(obj);
  if (!linkedWidget)
  {
    linkedWidget = this->getCurrentTextArea();
    if (!linkedWidget->isEmpty() || linkedWidget->isLinked())
    {
      this->createNewEmptyTab();
      linkedWidget = this->getCurrentTextArea();
    }

    linkedWidget->linkTo(obj);
    const QString txt = obj->toPlainText();
    if (!txt.isEmpty())
    {
      linkedWidget->setText(txt);
    }

    // We set a callback on the QTextEdit in the associated filter gets destroyed
    this->connect(obj, &QObject::destroyed,
      [this, FindLinkedWidget](QObject* object)
      {
        pqPythonTextArea* destroyedWidget = FindLinkedWidget(object);
        if (destroyedWidget)
        {
          destroyedWidget->unlink();
          if (destroyedWidget->isEmpty())
          {
            this->tabCloseRequested(this->indexOf(destroyedWidget));
          }
          else
          {
            this->updateTab(destroyedWidget);
          }
        }
      });
  }

  this->tabBar()->setCurrentIndex(this->indexOf(linkedWidget));
  this->updateTab(linkedWidget);
}
