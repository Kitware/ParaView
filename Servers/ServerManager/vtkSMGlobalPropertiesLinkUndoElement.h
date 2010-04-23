/*=========================================================================

  Program:   ParaView
  Module:    vtkSMGlobalPropertiesLinkUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMGlobalPropertiesLinkUndoElement
// .SECTION Description
//

#ifndef __vtkSMGlobalPropertiesLinkUndoElement_h
#define __vtkSMGlobalPropertiesLinkUndoElement_h

#include "vtkSMUndoElement.h"

class vtkSMProxy;

class VTK_EXPORT vtkSMGlobalPropertiesLinkUndoElement : public vtkSMUndoElement
{
public:
  static vtkSMGlobalPropertiesLinkUndoElement* New();
  vtkTypeMacro(vtkSMGlobalPropertiesLinkUndoElement, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Undo the operation encapsulated by this element.
  virtual int Undo();

  // Description:
  // Redo the operation encaspsulated by this element.
  virtual int Redo();

  // Description:
  // Returns if this element can load the xml state for the given element.
  virtual bool CanLoadState(vtkPVXMLElement*);

  void LinkAdded(const char* mgrname,
    const char* globalpropname,
    vtkSMProxy* proxy, const char* propname);

  void LinkRemoved(const char* mgrname,
    const char* globalpropname,
    vtkSMProxy* proxy, const char* propname);

//BTX
protected:
  vtkSMGlobalPropertiesLinkUndoElement();
  ~vtkSMGlobalPropertiesLinkUndoElement();

  int UndoRedoInternal(bool undo);
private:
  vtkSMGlobalPropertiesLinkUndoElement(const vtkSMGlobalPropertiesLinkUndoElement&); // Not implemented
  void operator=(const vtkSMGlobalPropertiesLinkUndoElement&); // Not implemented
//ETX
};

#endif

