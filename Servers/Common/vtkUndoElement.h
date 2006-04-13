/*=========================================================================

  Program:   ParaView
  Module:    vtkUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUndoElement - unit undo-redo-able operation.
// .SECTION Description
// This is an abstract class that defines the API for an undo-redo-able operation.
// One or more vtkUndoElement objects can define
// a single undo-redo step. Every concrete implementation of this class
// must know how to undo as well as redo the operation, and save and load the
// state as an XML.
//
// vtkUndoElement, vtkUndoSet and vtkUndoStack form the undo/redo framework core.
// .SECTION See Also
// vtkUndoStack vtkUndoSet

#ifndef __vtkUndoElement_h
#define __vtkUndoElement_h

#include "vtkObject.h"

class vtkPVXMLElement;

class VTK_EXPORT vtkUndoElement : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkUndoElement, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Undo the operation encapsulated by this element.
  // \return the status of the operation, 1 on success, 0 otherwise.
  virtual int Undo() = 0;

  // Description:
  // Redo the operation encaspsulated by this element.
  // \return the status of the operation, 1 on success, 0 otherwise.
  virtual int Redo() = 0;

  // Description:
  // Saves the state of the element in an xml. Subclasses must override
  // SaveStateInternal() to add any subclass specific state information.
  // This method merely ensure that root is valid.
  // \arg \c root parent element under which the state xml element for this
  // object is to be added.
  void SaveState(vtkPVXMLElement* root);

  // Description:
  // Loads the state of this element from the XML node.
  // Subclasses must override LoadStateInternal() to add any subclass specific 
  // state loading. This method merely ensure that element is valid.
  // \arg \c element is the XML element for this object. 
  void LoadState(vtkPVXMLElement* element);
//BTX
protected:
  vtkUndoElement();
  ~vtkUndoElement();

  // Description:
  // Overridden by subclasses to save state specific to the class.
  // \arg \c parent element. 
  virtual void SaveStateInternal(vtkPVXMLElement* root)=0;

  // Description:
  // Overridden by subclasses to load state specific to the class.
  // \arg \c parent element. 
  virtual void LoadStateInternal(vtkPVXMLElement* element) =0;

private:
  vtkUndoElement(const vtkUndoElement&); // Not implemented.
  void operator=(const vtkUndoElement&); // Not implemented.
//ETX
};

#endif

