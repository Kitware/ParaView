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
// This class has dual role: 
// \li Firstly, it needs to know how to create
// undo elements based on their state xmls and load the element states.
// All undo elements must be registered with the state loader so that the 
// loader can create the correct type when loading the state.
// \li Secondly, it needs to provide the undo elements with proxies 
// they may request based on the proxy id. The loader may have to create new 
// proxies based on the state, if none already exists. The vtkSMUndoElement
// subclasses ask the loader to provide it with proxies they need
// to access.
//
// One can register vtkSMUndoElement subclassess with the state loader.
// The loader creates the most recently registered element which can 
// load the current state.
// .SECTION See Also
// vtkSMUndoStack vtkUndoElement vtkUndoSet

#ifndef __vtkSMUndoRedoStateLoader_h
#define __vtkSMUndoRedoStateLoader_h

#include "vtkSMDefaultStateLoader.h"

class vtkPVXMLElement;
class vtkSMUndoElement;
class vtkSMUndoRedoStateLoaderVector;
class vtkUndoElement;
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

  // Description:
  // The loader creates a vtkSMUndoElement subclass for every XML entry
  // in the state. Application can register undo elements that should be
  // created when loading state. The most recently registered element
  // is given higher priority for handling the state.
  // Returns the index for the register element.
  // By default, vtkSMProxyRegisterUndoElement, 
  // vtkSMProxyUnRegisterUndoElement, and vtkSMPropertyModificationUndoElement
  // are registered.
  unsigned int RegisterElement(vtkSMUndoElement*);

  // Description:
  // Unregister an undo element at an index.
  void UnRegisterElement(unsigned int index);

  // Description:
  // Get the registered element at an index.
  vtkSMUndoElement* GetRegisteredElement(unsigned int index);

  // Description:
  // Get number of registered elements.
  unsigned int GetNumberOfRegisteredElements();

  // Description:
  // Unhiding superclass implementation.
  virtual vtkSMProxy* NewProxy(int id)
    { return this->Superclass::NewProxy(id); }

protected:
  vtkSMUndoRedoStateLoader();
  ~vtkSMUndoRedoStateLoader();

  virtual vtkUndoElement* HandleTag(vtkPVXMLElement* root);

  // Either create a new proxy or returns one from the map
  // of existing properties. Newly created proxies are stored
  // in the map with the id as the key. The root is the xml
  // element under which the proxy definitions are stored.
  virtual vtkSMProxy* NewProxy(vtkPVXMLElement* root, int id);

private:
  vtkSMUndoRedoStateLoader(const vtkSMUndoRedoStateLoader&); // Not implemented.
  void operator=(const vtkSMUndoRedoStateLoader&); // Not implemented.

  vtkSMUndoRedoStateLoaderVector* RegisteredElements;
};

#endif

