/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractSelectionProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMExtractSelectionProxy - proxy for extract (point/cell) selection
// filters. 
// .SECTION Description
// vtkSMExtractSelectionProxy has a subproxy which is the proxy for the 
// selection.

#ifndef __vtkSMExtractSelectionProxy_h
#define __vtkSMExtractSelectionProxy_h

#include "vtkSMSourceProxy.h"

class VTK_EXPORT vtkSMExtractSelectionProxy : public vtkSMSourceProxy
{
public:
  static vtkSMExtractSelectionProxy* New();
  vtkTypeRevisionMacro(vtkSMExtractSelectionProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Update the VTK object on the server by pushing the values of
  // all modifed properties (un-modified properties are ignored).
  // If the object has not been created, it will be created first.
  virtual void UpdateVTKObjects();

  // Description:
  // Set if the "GlobalIDs" property values are used or 
  // the "Indices" property values are used as the selection.
  vtkSetMacro(UseGlobalIDs, int);
  vtkGetMacro(UseGlobalIDs, int);
  vtkBooleanMacro(UseGlobalIDs, int);

  void AddIndex(vtkIdType piece, vtkIdType id);
  void RemoveAllIndices();

  void AddGlobalID(vtkIdType id);
  void RemoveAllGlobalIDs();

//BTX
protected:
  vtkSMExtractSelectionProxy();
  ~vtkSMExtractSelectionProxy();

  virtual void CreateVTKObjects(int numObjects);

  // Description:
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);

  int UseGlobalIDs;

  int SelectionFieldType;
private:
  vtkSMExtractSelectionProxy(const vtkSMExtractSelectionProxy&); // Not implemented.
  void operator=(const vtkSMExtractSelectionProxy&); // Not implemented.

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif

