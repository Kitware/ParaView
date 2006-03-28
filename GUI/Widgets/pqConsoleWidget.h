/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "QtWidgetsExport.h"

#include <QWidget>
#include <QTextCharFormat>

/// Qt widget that provides an interactive console - send text to the console by calling printString(), and connect to the executeCommand() slot to receive user input
class QTWIDGETS_EXPORT pqConsoleWidget :
  public QWidget
{
  Q_OBJECT
  
public:
  pqConsoleWidget(QWidget* Parent);
  virtual ~pqConsoleWidget();

public slots:
  /// Returns the current formatting that will be used by printString
  QTextCharFormat getFormat();
  /// Sets formatting that will be used by printString
  void setFormat(const QTextCharFormat& Format);
  /// Writes the supplied text to the console
  void printString(const QString& Text);

signals:
  /// Signal emitted whenever the user enters a command
  void executeCommand(const QString& Command);

private slots:
  void onCursorPositionChanged();
  void onSelectionChanged();

private:
  pqConsoleWidget(const pqConsoleWidget&);
  pqConsoleWidget& operator=(const pqConsoleWidget&);

  void internalExecuteCommand(const QString& Command);

  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqConsoleWidget_h

