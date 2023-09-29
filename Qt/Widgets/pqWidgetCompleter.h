// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqWidgetCompleter_h
#define pqWidgetCompleter_h

#include "pqWidgetsModule.h"

#include <QCompleter>
#include <QWidget>

/**
 * Abstact class for widget completers. Concrete implementations must define functions that return
 * possible completions given a prompt, and the completion prefix corresponding to the incomplete
 * name.
 */
class PQWIDGETS_EXPORT pqWidgetCompleter : public QCompleter
{
public:
  pqWidgetCompleter(QWidget* parent) { this->setParent(parent); }

  /**
   * This method is called by the client to request an update on the internal completion model,
   * given a text prompt.
   */
  virtual void updateCompletionModel(const QString& prompt);

  ///@{
  /**
   * get/set CompleteEmptyPrompts, indicating whether or not the completer should show anything
   * if an empty prompt is given.
   */
  bool getCompleteEmptyPrompts() { return this->CompleteEmptyPrompts; };
  void setCompleteEmptyPrompts(bool newValue) { this->CompleteEmptyPrompts = newValue; };
  ///@}

protected:
  /**
   * Return a list of strings that could match the given prompt
   */
  virtual QStringList getCompletions(const QString& prompt) = 0;

  /**
   * Return the part of the prompt that can be completed.
   */
  virtual QString getCompletionPrefix(const QString& prompt) = 0;

private:
  bool CompleteEmptyPrompts = false;
};
#endif
