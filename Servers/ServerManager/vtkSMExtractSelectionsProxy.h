/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractSelectionsProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMExtractSelectionsProxy - proxy for extract (point/cell) selection
// filters. 
// .SECTION Description
// vtkSMExtractSelectionsProxy has a subproxy which is the proxy for the 
// selection.

#ifndef __vtkSMExtractSelectionsProxy_h
#define __vtkSMExtractSelectionsProxy_h

#include "vtkSMSourceProxy.h"

class VTK_EXPORT vtkSMExtractSelectionsProxy : public vtkSMSourceProxy
{
public:
  static vtkSMExtractSelectionsProxy* New();
  vtkTypeRevisionMacro(vtkSMExtractSelectionsProxy, vtkSMSourceProxy);
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

  // Description:
  // Get the selection field type.
  vtkGetMacro(SelectionFieldType, int);

  // Description:
  // Add an index to the selection.
  void AddIndex(vtkIdType piece, vtkIdType id);
  void RemoveAllIndices();

  // Description:
  // Add a global id to the selection.
  void AddGlobalID(vtkIdType id);
  void RemoveAllGlobalIDs();

  void CopySelectionSource(vtkSMSourceProxy* selSource);
//BTX
protected:
  vtkSMExtractSelectionsProxy();
  ~vtkSMExtractSelectionsProxy();

  virtual void CreateVTKObjects();

  // Description:
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);

  int UseGlobalIDs;

  int SelectionFieldType;
private:
  vtkSMExtractSelectionsProxy(const vtkSMExtractSelectionsProxy&); // Not implemented.
  void operator=(const vtkSMExtractSelectionsProxy&); // Not implemented.

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif

