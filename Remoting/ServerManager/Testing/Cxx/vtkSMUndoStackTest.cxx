/*=========================================================================

Program:   ParaView
Module:    vtkSMUndoStackTest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMUndoStackTest.h"

#include "vtkSMMessage.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyModificationUndoElement.h"
#include "vtkSMProxy.h"
#include "vtkSMRemoteObjectUpdateUndoElement.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMUndoStack.h"
#include "vtkUndoSet.h"

void vtkSMUndoStackTest::UndoRedo()
{
  vtkSMSession* session = vtkSMSession::New();
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

  vtkSMProxy* sphere = pxm->NewProxy("sources", "SphereSource");
  sphere->UpdateVTKObjects();
  QVERIFY(sphere != NULL);
  QCOMPARE(vtkSMPropertyHelper(sphere, "Radius").GetAsDouble(), 0.5);

  vtkSMUndoStack* undoStack = vtkSMUndoStack::New();
  vtkUndoSet* undoSet = vtkUndoSet::New();
  vtkSMRemoteObjectUpdateUndoElement* undoElement = vtkSMRemoteObjectUpdateUndoElement::New();
  undoElement->SetSession(session);

  vtkSMMessage before;
  before.CopyFrom(*sphere->GetFullState());
  vtkSMPropertyHelper(sphere, "Radius").Set(1.2);
  sphere->UpdateVTKObjects();
  vtkSMMessage after;
  after.CopyFrom(*sphere->GetFullState());
  undoElement->SetUndoRedoState(&before, &after);

  undoSet->AddElement(undoElement);
  undoElement->Delete();
  undoStack->Push("ChangeRadius", undoSet);
  undoSet->Delete();

  QVERIFY(static_cast<bool>(undoStack->CanUndo()) == true);
  undoStack->Undo();
  QVERIFY(static_cast<bool>(undoStack->CanUndo()) == false);
  sphere->UpdateVTKObjects();
  QCOMPARE(vtkSMPropertyHelper(sphere, "Radius").GetAsDouble(), 0.5);

  QVERIFY(static_cast<bool>(undoStack->CanRedo()) == true);
  undoStack->Redo();
  sphere->UpdateVTKObjects();
  QCOMPARE(vtkSMPropertyHelper(sphere, "Radius").GetAsDouble(), 1.2);
  QVERIFY(static_cast<bool>(undoStack->CanRedo()) == false);

  undoStack->Delete();

  sphere->Delete();
  session->Delete();
}

void vtkSMUndoStackTest::StackDepth()
{
  vtkSMUndoStack* stack = vtkSMUndoStack::New();
  QCOMPARE(stack->GetStackDepth(), 10);
  stack->Delete();
}
