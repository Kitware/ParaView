/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoStack.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUndoStack
// .SECTION Description
// This is the undo/redo stack for the Server Manager. This provides a 
// unified face for undo/redo irrespective of number of connections, their
// type etc etc.
// 
// The class records Server Manager changes that are undo/redo able and
// collects them. To begin recording such changes one must call BeginOrContinueUndoSet().
// To end recording use EndUndoSet(). On EndUndoSet, the changes are collected
// into an XML which is then sent to the data server of the connection
// set when BeginOrContinueUndoSet() is called. For this to work properly the GUI
// must ensure that between BeginOrContinueUndoSet() and EndUndoSet() changes are
// only performed to the proxies with the connection id used to BeginOrContinueUndoSet().
// In other words, a undo-able unit can only be on the same connection. 
// 
// On every undo/redo, it fetches the XML state change from the server.
// vtkSMUndoRedoStateLoader is used to generate a vtkUndoSet object from
// the XML. GUI can subclass vtkSMUndoRedoStateLoader to handle GUI specific
// XML elements. The loader instance must be set before performing the undo,
// otherwise vtkSMUndoRedoStateLoader is used.
//
// This class also provides API to push any vtkUndoSet instance on to a 
// server. GUI can use this to push its own changes that is undoable across
// connections.
// 
// .SECTION TODO
// \li Compound proxies are not handled yet.
// \li Mutiple clients are not supported. For now, the server can connect to only
// one client.


#ifndef __vtkSMUndoStack_h
#define __vtkSMUndoStack_h

#include "vtkUndoStack.h"

class vtkPVXMLElement;
class vtkSMUndoRedoStateLoader;
class vtkSMUndoStackObserver;
class vtkUndoSet;

class VTK_EXPORT vtkSMUndoStack : public vtkUndoStack
{
public:
  static vtkSMUndoStack* New();
  vtkTypeRevisionMacro(vtkSMUndoStack, vtkUndoStack);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Begin monitoring the proxy manager for undoable operations.
  // This call resumes the undo set, if one has been suspended,
  // by calling PauseUndoSet otherwise it starts a new UndoSet 
  // with the given label. If the undo set was resumed, the label is
  // ignored, instead the original label is used.
  // All such operations are noted by creating UndoElements for each.
  // All the undo elements will be part of a vtkUndoSet with the given
  // label. For now, this call cannot be nested i.e. BeginOrContinueUndoSet 
  // call after a BeginOrContinueUndoSet but before EndUndoSet/PauseUndoSet 
  // is an error. 
  // For now, the GUI has to ensure
  // that separate undo sets are created for operations on different connections.
  // Since currently we support only 1 connection at a time. This is
  // fine, but support needs to be added to hide connection details
  // from GUI in future.
  void BeginOrContinueUndoSet(vtkIdType connectionid, const char* label);

  // Description:
  // Pause the undo set building. Note that EndUndoSet must be called before 
  // the undo set if packaged and pushed onto the undo stack.
  void PauseUndoSet();

  // Description:
  // End the undo set currently being generated. This must be called only after
  // BeginOrContinueUndoSet has been called. If any change was noted, EndUndoSet pushes the
  // vtkUndoSet on to the server's undo stack. 
  void EndUndoSet();

  // Description:
  // Discard the undo set being built currently.
  void CancelUndoSet();

  // Description:
  // It is possible to push any instance of vtkUndoSet on to the server
  // for a specific connection. Note however that once it is pushed on the server
  // every connected client will be able to undo it (evetually atleast).
  void Push(vtkIdType connectionid, const char* label, vtkUndoSet* set);

  // Description:
  // Performs an Undo using the set on the top of the undo stack. The set is poped from
  // the undo stack and pushed at the top of the redo stack. 
  // \returns the status of the operation.
  virtual int Undo();

  // Description:
  // Performs a Redo using the set on the top of the redo stack. The set is poped from
  // the redo stack and pushed at the top of the undo stack. 
  // \returns the status of the operation.
  virtual int Redo();

  // Description:
  // Set the state loader to be used to load the undo/redo set states.
  // By default vtkSMUndoRedoStateLoader is used.
  void SetStateLoader(vtkSMUndoRedoStateLoader*);
  vtkGetObjectMacro(StateLoader, vtkSMUndoRedoStateLoader);

//BTX
protected:
  vtkSMUndoStack();
  ~vtkSMUndoStack();

  // Description:
  // Don;t call Push directly on this class. Instead use 
  // BeginOrContinueUndoSet() and EndUndoSet().
  virtual void Push(const char* , vtkUndoSet* ) 
    {
    vtkErrorMacro("vtkSMUndoStack does not support calling Push directly."
      " Please use BeginOrContinueUndoSet()/EndUndoSet() instead.");
    return;
    }

  // Description:
  // The method updates the client side stack. Client side stack merely contains the labels
  // for the undo/redo states and which connection they are to be performed on.
  // TODO: Eventually this method will be called as an effect of the PM telling the client
  // that something has been pushed on the server side undo stack.
  // As a consequence each client will update their undo stack status. Note,
  // only the status is updated, the actual undo state is not sent to the client
  // until it requests it. Ofcourse, this part is still not implemnted. For now,
  // multiple clients are not supported.
  void PushUndoConnection(const char* label, vtkIdType id);

  void ExecuteEvent(vtkObject* caller, unsigned long eventid, void* data);
  friend class vtkSMUndoStackObserver;

  void OnRegisterProxy(void* data);
  void OnUnRegisterProxy(void* data);
  void OnPropertyModified(void* data);
  void OnConnectionClosed(vtkIdType cid);

  int BuildUndoSet;
  vtkIdType ActiveConnectionID;
  vtkUndoSet* ActiveUndoSet;
  char* Label;
  vtkSetStringMacro(Label);

  friend class vtkSMUndoStackUndoSet;
  int ProcessUndo(vtkIdType id, vtkPVXMLElement* root);
  int ProcessRedo(vtkIdType id, vtkPVXMLElement* root);

  vtkSMUndoRedoStateLoader* StateLoader;
private:
  vtkSMUndoStack(const vtkSMUndoStack&); // Not implemented.
  void operator=(const vtkSMUndoStack&); // Not implemented.
  
  vtkSMUndoStackObserver* Observer;
//ETX
};


#endif

