/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqTests.h"

#include <vtkstd/string>

#include <QAbstractButton>
#include <QAction>
#include <QObject>
#include <QWidget>
#include <QtTest>
#include <QtTest/qttest_gui.h>

namespace
{

template<typename T>
T* pqLookupObject(QObject& Object, const char* Name)
{
  const vtkstd::string name = Name ? Name : "";
  const vtkstd::string part = name.substr(0, name.find("/"));
  const vtkstd::string remaining = (part.size() + 1) < name.size() ? name.substr(part.size() + 1) : "";
  
  if(remaining.empty())
  {
    QList<T*> children = Object.findChildren<T*>(part.c_str());
    if(children.size() != 1)
      return 0;
      
    return children[0];
  }
  
  QList<QObject*> children = Object.findChildren<QObject*>(part.c_str());
  if(children.size() != 1)
    return 0;

  return pqLookupObject<T>(*children[0], remaining.c_str());
}

bool pqActivate(QAction* Action)
{
  if(Action)
    Action->activate(QAction::Trigger);
    
  return Action ? true : false;
}

bool pqActivate(QAbstractButton* Button)
{
  if(Button)
    Button->click();
    
  return Button ? true : false;
}


template<typename TestT>
void pqRunRegressionTest()
{
  TestT test;
  QtTest::exec(&test);
}

template<typename TestT>
void pqRunRegressionTest(QWidget& RootWidget)
{
  TestT test(RootWidget);
  QtTest::exec(&test);
}

} // namespace



void pqTestTestingFramework::testSuccess()
{
  COMPARE(true, true);
}
  
void pqTestTestingFramework::testFailure()
{
  EXPECT_FAIL("", "Deliberate failure", Continue);
  COMPARE(true, false);
}

void pqTestFileMenu::testFileMenu()
{
  VERIFY(pqLookupObject<QWidget>(rootWidget, "menuBar/fileMenu"));
}
  
void pqTestFileMenu::testFileOpen()
{
  VERIFY(pqActivate(pqLookupObject<QAction>(rootWidget, "fileOpenAction")));
  VERIFY(pqLookupObject<QWidget>(rootWidget, "fileOpenBrowser"));
  VERIFY(pqActivate(pqLookupObject<QAbstractButton>(rootWidget, "fileOpenBrowser/buttonCancel")));
}

void pqRunRegressionTests()
{
  pqRunRegressionTest<pqTestTestingFramework>();
}

void pqRunRegressionTests(QWidget& RootWidget)
{
  pqRunRegressionTest<pqTestFileMenu>(RootWidget);
}

