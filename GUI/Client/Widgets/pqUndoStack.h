/*=========================================================================

   Program: ParaView
   Module:    pqUndoStack.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

========================================================================*/
#ifndef __pqUndoStack_h
#define __pqUndoStack_h

#include "pqWidgetsExport.h"
#include <QObject>

class pqUndoStackImplementation;
class vtkCommand;
class vtkObject;
class vtkSMUndoStack;
class vtkUndoElement;
class pqServer;

/// This is an adaptor for the vtkSMUndoStack.
/// This class updates the vtkSMUndoStack status on accept reset etc etc.
/// That way, the Accept/Reset mechanism doesn't explicitly have
/// to manage Undo/Redo states.
class PQWIDGETS_EXPORT pqUndoStack : public QObject
{
  Q_OBJECT
public:
  pqUndoStack(bool clientOnly, QObject* parent=NULL);
  virtual ~pqUndoStack();

  bool CanUndo();
  bool CanRedo();

  // Description:
  // One can add arbritary elements to the active undo set.
  // If no ActiveUndoSet is present, an error will be raised. It is essential
  // that the StateLoader can handle the arbritary undo elements.
  void AddToActiveUndoSet(vtkUndoElement* element);

public slots:
  /// NOTE: Notice that the BeginOrContinueUndoSet doesn;t take
  /// connection Id. This is for two reasons:
  /// 1)  For 1st release we are not supporting multiple connections
  ///     from teh client, hence we only have one connection to deal with.
  /// 2)  Once we start supporting multiple connections vtkSMUndoStack()
  ///     will support UndoSet with elements on multiple connections
  ///     transparently.
  void BeginOrContinueUndoSet(QString label);
  void PauseUndoSet();
  void EndUndoSet();
  void Accept();
  void Reset();
  void Undo();
  void Redo();
  void Clear();

signals:
  /// Fired to notify interested parites that the stack has changed.
  /// Has information to know the status of the top of the stack.
  void StackChanged(bool canUndo, QString undoLabel, 
    bool canRedo, QString redoLabel);
    
  void CanUndoChanged(bool);
  void CanRedoChanged(bool);
  void UndoLabelChanged(const QString&);
  void RedoLabelChanged(const QString&);
  
  // Fired after undo.
  void Undone();
  // Fired after redo.
  void Redone();
 
public slots: 
  void setActiveServer(pqServer* server);  // TODO remove this

private slots:
  void onStackChanged(vtkObject*, unsigned long, void*, 
    void*,  vtkCommand*);


private:
  pqUndoStackImplementation* Implementation;
  
};


#endif

