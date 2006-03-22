/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

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

