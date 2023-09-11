// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include <QAbstractItemView>
#include <QStringListModel>

#include "pqWidgetCompleter.h"

void pqWidgetCompleter::updateCompletionModel(const QString& prompt)
{
  // Start by clearing the model
  this->setModel(nullptr);

  QStringList completions = this->getCompletions(prompt);
  QString completionPrefix = this->getCompletionPrefix(prompt);

  // Initialize the completion model
  if (!completions.isEmpty())
  {
    this->setCompletionMode(QCompleter::PopupCompletion);
    this->setModel(new QStringListModel(completions, this));
    this->setCaseSensitivity(Qt::CaseInsensitive);
    this->setCompletionPrefix(completionPrefix.toLower());
    if (this->popup())
    {
      this->popup()->setCurrentIndex(this->completionModel()->index(0, 0));
    }
  }
};
