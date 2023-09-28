// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqConsoleWidget_h
#define pqConsoleWidget_h

#include "pqWidgetCompleter.h"
#include "pqWidgetsModule.h"

#include <QTextCharFormat>
#include <QWidget>

/**
  Qt widget that provides an interactive console - you can send text to the
  console by calling printString() and receive user input by connecting to the
  executeCommand() slot.

  \sa pqPythonShell
*/
class PQWIDGETS_EXPORT pqConsoleWidget : public QWidget
{
  Q_OBJECT

public:
  pqConsoleWidget(QWidget* parent = nullptr);
  ~pqConsoleWidget() override;

  /**
   * Returns the current formatting that will be used by printString
   */
  QTextCharFormat getFormat();
  /**
   * Sets formatting that will be used by printString
   */
  void setFormat(const QTextCharFormat& Format);

  /**
   * Set a completer for this console widget
   */
  void setCompleter(pqWidgetCompleter* completer);

  QPoint getCursorPosition();

  /**
   * Set the size of the font to use. Size is measured in points.
   */
  void setFontSize(int size);

Q_SIGNALS:
  /**
   * Signal emitted whenever the user enters a command
   */
  void executeCommand(const QString& Command);

  /**
   * Fired to indicate to the application that the console has focus.
   */
  void consoleFocusInEvent();

public Q_SLOTS:
  /**
   * Writes the supplied text to the console
   */
  void printString(const QString& Text);

  /**
   * Updates the current command. Unlike printString, this will affect the
   * current command being typed.
   */
  void printCommand(const QString& cmd);

  /**
   * Get the text in the console.
   */
  QString text();

  /**
   * Clears the contents of the console
   */
  void clear();

  /**
   * Puts out an input accepting prompt.
   * It is recommended that one uses prompt instead of printString() to print
   * an input prompt since this call ensures that the prompt is shown on a new
   * line.
   */
  void prompt(const QString& text);

  /**
   * Inserts the given completion string at the cursor.  This will replace
   * the current word that the cursor is touching with the given text.
   * Determines the word using QTextCursor::StartOfWord, EndOfWord.
   */
  void insertCompletion(const QString& text);

  /**
   * This method can be called to make the internal QTextEdit take focus.
   */
  void takeFocus();

private:
  pqConsoleWidget(const pqConsoleWidget&);
  pqConsoleWidget& operator=(const pqConsoleWidget&);

  void internalExecuteCommand(const QString& Command);

  class pqImplementation;
  pqImplementation* const Implementation;
  friend class pqImplementation;

  friend class pqConsoleWidgetEventPlayer;

  int FontSize;

  /**
   * Prints and executes the command. Used by pqConsoleWidgetEventPlayer for
   * text playback.
   */
  void printAndExecuteCommand(const QString& text);
};

#endif // pqConsoleWidget_h
