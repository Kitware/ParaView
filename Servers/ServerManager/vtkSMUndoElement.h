/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUndoElement - abstract superclass for Server Manager undo 
// elements.
// .SECTION Description
// Abstract superclass for Server Manager undo elements. 
// This class keeps a ConnectionID, since every server manager undo/redo element
// has to have a connection id.

#ifndef __vtkSMUndoElement_h
#define __vtkSMUndoElement_h

#include "vtkUndoElement.h"

class vtkPVXMLElement;
class vtkSMStateLoaderBase;

class VTK_EXPORT vtkSMUndoElement : public vtkUndoElement
{
public:
  vtkTypeRevisionMacro(vtkSMUndoElement, vtkUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the connection ID.
  vtkGetMacro(ConnectionID, vtkIdType);
  vtkSetMacro(ConnectionID, vtkIdType);

  // Description:
  // Get/Set the state loader.
  // This must be set before the undo/redo actions are called.
  // The element may use the state loader to access
  // proxies. 
  // This is only valid within Undo()/Redo() calls.
  vtkGetObjectMacro(StateLoader, vtkSMStateLoaderBase);
  void SetStateLoader(vtkSMStateLoaderBase* loader);
protected:
  vtkSMUndoElement();
  ~vtkSMUndoElement();

  vtkIdType ConnectionID;

  // Description:
  // Access the XML element that is keeps the state 
  // to undo/redo.
  vtkPVXMLElement* XMLElement;
  void SetXMLElement(vtkPVXMLElement*);

  vtkSMStateLoaderBase* StateLoader;

  // Description:
  // Overridden to save state specific to the class.
  // \arg \c element <Element /> representing this object.
  virtual void SaveStateInternal(vtkPVXMLElement* root);

  virtual void LoadStateInternal(vtkPVXMLElement* element);
private:
  vtkSMUndoElement(const vtkSMUndoElement&); // Not implemented.
  void operator=(const vtkSMUndoElement&); // Not implemented.
};


#endif

