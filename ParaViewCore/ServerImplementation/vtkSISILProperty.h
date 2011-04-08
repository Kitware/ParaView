/*=========================================================================

  Program:   ParaView
  Module:    vtkSISILProperty

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSISILProperty
// .SECTION Description
// SIProperty that deals with SIL data extraction to get the property value

#ifndef __vtkSISILProperty_h
#define __vtkSISILProperty_h

#include "vtkSIProperty.h"

class vtkGraph;

class VTK_EXPORT vtkSISILProperty : public vtkSIProperty
{
public:
  static vtkSISILProperty* New();
  vtkTypeMacro(vtkSISILProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSISILProperty();
  ~vtkSISILProperty();

  friend class vtkSIProxy;

  // Description:
  // Parse the xml for the property and specially the "subtree" extra attribute
  // and the "output_port" if this one is different than the default one which
  // is 0.
  virtual bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element);

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

  vtkSetStringMacro(SubTree);

  class vtkIdTypeSet;
  static void GetLeaves( vtkGraph *sil, vtkIdType vertexid,
                         vtkIdTypeSet& list,
                         bool traverse_cross_edges);

private:
  vtkSISILProperty(const vtkSISILProperty&); // Not implemented
  void operator=(const vtkSISILProperty&); // Not implemented

  char* SubTree;
  int OutputPort;

//ETX
};

#endif
