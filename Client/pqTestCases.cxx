/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqEventPlayer.h"
#include "pqTestCases.h"

#include <vtkstd/string>

#include <QAbstractButton>
#include <QAction>
#include <QObject>
#include <QWidget>
#include <QtTest/qttest.h>
#include <QtTest/qttest_gui.h>

namespace
{

/// Given a Qt object, lookup a child object by name, treating the name as a hierarchical "path"
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

} // namespace

/////////////////////////////////////////////////////////////////////////
// pqTestCases

pqTestCases::pqTestCases(QWidget* const RootWidget) :
  rootWidget(RootWidget)
{
}

void pqTestCases::testSuccess()
{
  COMPARE(true, true);
}

void pqTestCases::testFailure()
{
  EXPECT_FAIL("", "Deliberate failure", Continue);
  COMPARE(true, false);
}

void pqTestCases::testFileNew()
{
  VERIFY(rootWidget);
  
  pqEventPlayer player(*rootWidget);
  VERIFY(player.playEvent("fileNewAction", "trigger_action", ""));
}

void pqTestCases::testFileMenu()
{
  VERIFY(rootWidget);  
  VERIFY(pqLookupObject<QWidget>(*rootWidget, "menuBar/fileMenu"));
}

/*
void pqTestCases::testFileOpen()
{
  VERIFY(rootWidget);
  VERIFY(pqActivate(pqLookupObject<QAction>(*rootWidget, "debugOpenLocalFilesAction")));
  VERIFY(pqLookupObject<QWidget>(*rootWidget, "fileOpenDialog"));
  VERIFY(pqActivate(pqLookupObject<QAbstractButton>(*rootWidget, "fileOpenDialog/buttonCancel")));
}
*/

void pqTestCases::testSlider()
{
  VERIFY(rootWidget);
  
  pqEventPlayer player(*rootWidget);
  VERIFY(player.playEvent("Resolution", "set_int", "8"));
}
