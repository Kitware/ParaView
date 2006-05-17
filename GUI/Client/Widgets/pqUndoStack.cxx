/*=========================================================================

   Program:   ParaQ
   Module:    pqUndoStack.cxx

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
#include "pqUndoStack.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkSmartPointer.h"
#include "vtkSMUndoStack.h"
#include "vtkSMProxyManager.h"


#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqServer.h"
//-----------------------------------------------------------------------------
class pqUndoStackImplementation
{
public:
  vtkSmartPointer<vtkSMUndoStack> UndoStack;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnector;
};

//-----------------------------------------------------------------------------
pqUndoStack::pqUndoStack(QObject* _parent/*=null*/) :QObject(_parent)
{
  this->Implementation = new pqUndoStackImplementation;
  this->Implementation->UndoStack = vtkSmartPointer<vtkSMUndoStack>::New();
  this->Implementation->VTKConnector = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Implementation->VTKConnector->Connect(this->Implementation->UndoStack,
    vtkCommand::ModifiedEvent, this, SLOT(onStackChanged(vtkObject*, 
        unsigned long, void*, void*, vtkCommand*)), NULL, 1.0);
}

//-----------------------------------------------------------------------------
pqUndoStack::~pqUndoStack()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqUndoStack::onStackChanged(vtkObject*, unsigned long, void*, 
    void*,  vtkCommand*)
{
  bool canUndo = false;
  bool canRedo = false;
  QString undoLabel;
  QString redoLabel;
  if (this->Implementation->UndoStack->CanUndo())
    {
    canUndo = true;
    undoLabel = this->Implementation->UndoStack->GetUndoSetLabel(0);
    }
  if (this->Implementation->UndoStack->CanRedo())
    {
    canRedo = true;
    redoLabel = this->Implementation->UndoStack->GetRedoSetLabel(0);
    }
  emit this->StackChanged(canUndo, undoLabel, canRedo, redoLabel);

}

//-----------------------------------------------------------------------------
void pqUndoStack::BeginOrContinueUndoSet(QString label)
{
  pqServer * server= pqApplicationCore::instance()->getActiveServer();
  if (!server)
    {
    qDebug()<< "no active server. cannot undo/redo.";
    return;
    }
  vtkIdType cid = server->GetConnectionID();
  this->Implementation->UndoStack->BeginOrContinueUndoSet(cid,
    label.toStdString().c_str());
}

//-----------------------------------------------------------------------------
void pqUndoStack::PauseUndoSet()
{
  this->Implementation->UndoStack->PauseUndoSet();
}

//-----------------------------------------------------------------------------
void pqUndoStack::EndUndoSet()
{
  this->Implementation->UndoStack->EndUndoSet();
}

//-----------------------------------------------------------------------------
void pqUndoStack::Accept()
{
  this->BeginOrContinueUndoSet("Accept");
}

//-----------------------------------------------------------------------------
void pqUndoStack::Reset()
{
  this->Implementation->UndoStack->CancelUndoSet();
}

//-----------------------------------------------------------------------------
void pqUndoStack::Undo()
{
  this->Implementation->UndoStack->Undo();
  // Update of proxies have to happen in order.
  vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies("sources", 1);
  vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies("displays", 1);
  vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies(1);
  pqApplicationCore::instance()->render();
}

//-----------------------------------------------------------------------------
void pqUndoStack::Redo()
{
  this->Implementation->UndoStack->Redo();
  // Update of proxies have to happen in order.
  vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies("sources", 1);
  vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies("displays", 1);
  vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies(1);
  pqApplicationCore::instance()->render();
}
