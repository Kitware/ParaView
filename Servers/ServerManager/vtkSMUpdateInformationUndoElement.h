/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUpdateInformationUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUpdateInformationUndoElement
// .SECTION 
// Undo element for UpdatePipelineInformation() or UpdatePropertyInformation()
// calls on the vtkSMSourceProxy/vtkSMProxy.
// On undo, they do nothing, but on redo, it calls the appropriate method
// on the proxy.

#ifndef __vtkSMUpdateInformationUndoElement_h
#define __vtkSMUpdateInformationUndoElement_h

#include "vtkSMUndoElement.h"

class vtkSMProxy;

class VTK_EXPORT vtkSMUpdateInformationUndoElement : public vtkSMUndoElement
{
public:
  static vtkSMUpdateInformationUndoElement* New();
  vtkTypeMacro(vtkSMUpdateInformationUndoElement, vtkSMUndoElement);
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

  // Description:
  // Set the information about the proxy that is getting registered.
  void Updated(vtkSMProxy* proxy);

protected:
  vtkSMUpdateInformationUndoElement();
  ~vtkSMUpdateInformationUndoElement();

private:
  vtkSMUpdateInformationUndoElement(const vtkSMUpdateInformationUndoElement&); // Not implemented.
  void operator=(const vtkSMUpdateInformationUndoElement&); // Not implemented.
};


#endif

