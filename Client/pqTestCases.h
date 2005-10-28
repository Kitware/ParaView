/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqTestCases_h
#define _pqTestCases_h

#include <QObject>

class QWidget;

/// Defines a set of built-in test-cases implemented in C++
/** \todo Support execution of individual test-cases from the command-line */
class pqTestCases :
  public QObject
{
  Q_OBJECT
  
public:
  pqTestCases(QWidget* RootWidget);

private:
  QWidget* const rootWidget;
  
private slots:
  void testSuccess();
  void testFailure();
  void testFileMenu();
  void testFileOpen();
};

#endif // !_pqTestCases_h

