/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoRedoStateLoader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUndoRedoStateLoader -  loader for a vtkUndoSet.
// .SECTION Description
// This is the state loader for loading undo/redo states.
// The GUI can subclass this to handle GUI specific elements.
// vtkSMUndoStack can be given an instance of appropriate subclass
// to handle undo/redo elements properly.
// .SECTION See Also
// vtkSMUndoStack vtkUndoElement vtkUndoSet

#ifndef __vtkSMUndoRedoStateLoader_h
#define __vtkSMUndoRedoStateLoader_h

#include "vtkSMDefaultStateLoader.h"

class vtkPVXMLElement;
class vtkUndoSet;

class VTK_EXPORT vtkSMUndoRedoStateLoader : public vtkSMDefaultStateLoader
{
public:
  static vtkSMUndoRedoStateLoader* New();
  vtkTypeRevisionMacro(vtkSMUndoRedoStateLoader, vtkSMDefaultStateLoader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Loads an Undo/Redo set state returns a new instance of vtkUndoSet.
  // This method allocates a new vtkUndoSet and it is the responsibility 
  // of the caller to \c Delete it.
  vtkUndoSet* LoadUndoRedoSet(vtkPVXMLElement* rootElement);

protected:
  vtkSMUndoRedoStateLoader();
  ~vtkSMUndoRedoStateLoader();

  virtual void HandleTag(const char* tagName, vtkPVXMLElement* root);

  void HandleProxyRegister(vtkPVXMLElement*);
  void HandleProxyUnRegister(vtkPVXMLElement*);
  void HandlePropertyModification(vtkPVXMLElement*);

  vtkUndoSet* UndoSet;
private:
  vtkSMUndoRedoStateLoader(const vtkSMUndoRedoStateLoader&); // Not implemented.
  void operator=(const vtkSMUndoRedoStateLoader&); // Not implemented.
};

#endif

