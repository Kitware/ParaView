/*=========================================================================

   Program: ParaView
   Module:    pqConsoleWidget.h

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

#ifndef _pqConsoleWidget_h
#define _pqConsoleWidget_h

#include "pqWidgetsModule.h"

#include <QCompleter>
#include <QTextCharFormat>
#include <QWidget>

class pqConsoleWidgetCompleter;

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
  void setCompleter(pqConsoleWidgetCompleter* completer);

  QPoint getCursorPosition();

  /**
   * Set the size of the font to use. Size is measured in points.
   */
  void setFontSize(int size);

signals:
  /**
  * Signal emitted whenever the user enters a command
  */
  void executeCommand(const QString& Command);

  /**
   * Fired to indicate to the application that the console has focus.
   */
  void consoleFocusInEvent();

public slots:
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

class PQWIDGETS_EXPORT pqConsoleWidgetCompleter : public QCompleter
{
public:
  /**
  * Update the completion model given a string.  The given string
  * is the current console text between the cursor and the start of
  * the line.
  */
  virtual void updateCompletionModel(const QString& str) = 0;
};

#endif // !_pqConsoleWidget_h
