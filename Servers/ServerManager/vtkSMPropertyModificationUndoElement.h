/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyModificationUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPropertyModificationUndoElement
// .SECTION Description
// This is the concrete implementation for the Undo element for a property
// modification event. 
// The undo action sets the property to the value that was pushed on
// to the server previous to the modification.
// The redo action sets the property to the modified value.

#ifndef __vtkSMPropertyModificationUndoElement_h
#define __vtkSMPropertyModificationUndoElement_h

#include "vtkSMUndoElement.h"
class vtkSMProxy;

class VTK_EXPORT vtkSMPropertyModificationUndoElement : public vtkSMUndoElement
{
public:
  static vtkSMPropertyModificationUndoElement* New();
  vtkTypeRevisionMacro(vtkSMPropertyModificationUndoElement, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Undo the operation encapsulated by this element.
  virtual int Undo();

  // Description:
  // Redo the operation encaspsulated by this element.
  virtual int Redo();

  // Description:
  // Set the property/proxy that was modified. 
  void ModifiedProperty(vtkSMProxy* proxy, const char* propertyname);
protected:
  vtkSMPropertyModificationUndoElement();
  ~vtkSMPropertyModificationUndoElement();

  // Description:
  // Overridden to save state specific to the class.
  // \arg \c element <Element /> representing this object.
  virtual void SaveStateInternal(vtkPVXMLElement* root);

  virtual void LoadStateInternal(vtkPVXMLElement* element);

  vtkPVXMLElement* XMLElement;
  void SetXMLElement(vtkPVXMLElement*);


private:
  vtkSMPropertyModificationUndoElement(const vtkSMPropertyModificationUndoElement&); // Not implemented.
  void operator=(const vtkSMPropertyModificationUndoElement&); // Not implemented.
};

#endif

