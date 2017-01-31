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
/**
 * @class   vtkSISILProperty
 *
 * SIProperty that deals with SIL data extraction to get the property value
*/

#ifndef vtkSISILProperty_h
#define vtkSISILProperty_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSIProperty.h"

class vtkGraph;

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSISILProperty : public vtkSIProperty
{
public:
  static vtkSISILProperty* New();
  vtkTypeMacro(vtkSISILProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkSISILProperty();
  ~vtkSISILProperty();

  friend class vtkSIProxy;

  /**
   * Parse the xml for the property and specially the "subtree" extra attribute
   * and the "output_port" if this one is different than the default one which
   * is 0.
   */
  virtual bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element) VTK_OVERRIDE;

  /**
   * Pull the current state of the underneath implementation
   */
  virtual bool Pull(vtkSMMessage*) VTK_OVERRIDE;

  vtkSetStringMacro(SubTree);

  class vtkIdTypeSet;
  static void GetLeaves(
    vtkGraph* sil, vtkIdType vertexid, vtkIdTypeSet& list, bool traverse_cross_edges);

private:
  vtkSISILProperty(const vtkSISILProperty&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSISILProperty&) VTK_DELETE_FUNCTION;

  char* SubTree;
  int OutputPort;
};

#endif
