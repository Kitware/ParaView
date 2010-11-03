/*=========================================================================

  Program:   ParaView
  Module:    vtkPMSILProperty

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMSILProperty
// .SECTION Description
// PMProperty that deals with SIL data extraction to get the property value

#ifndef __vtkPMSILProperty_h
#define __vtkPMSILProperty_h

#include "vtkPMProperty.h"
#include "vtkSMMessage.h"
#include <vtkstd/set>

class vtkGraph;

class VTK_EXPORT vtkPMSILProperty : public vtkPMProperty
{
public:
  static vtkPMSILProperty* New();
  vtkTypeMacro(vtkPMSILProperty, vtkPMProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMSILProperty();
  ~vtkPMSILProperty();

  friend class vtkPMProxy;

  // Description:
  // Parse the xml for the property and specially the "subtree" extra attribute
  // and the "output_port" if this one is different than the default one which
  // is 0.
  virtual bool ReadXMLAttributes(vtkPMProxy* proxyhelper, vtkPVXMLElement* element);

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

  vtkSetStringMacro(SubTree);

  static void GetLeaves( vtkGraph *sil, vtkIdType vertexid,
                         vtkstd::set<vtkIdType>& list,
                         bool traverse_cross_edges);

private:
  vtkPMSILProperty(const vtkPMSILProperty&); // Not implemented
  void operator=(const vtkPMSILProperty&); // Not implemented

  char* SubTree;
  int OutputPort;
//ETX
};

#endif
