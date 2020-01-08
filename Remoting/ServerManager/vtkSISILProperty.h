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

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIProperty.h"

class vtkGraph;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSISILProperty : public vtkSIProperty
{
public:
  static vtkSISILProperty* New();
  vtkTypeMacro(vtkSISILProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSISILProperty();
  ~vtkSISILProperty() override;

  friend class vtkSIProxy;

  /**
   * Parse the xml for the property and specially the "subtree" extra attribute
   * and the "output_port" if this one is different than the default one which
   * is 0.
   */
  bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element) override;

  /**
   * Pull the current state of the underneath implementation
   */
  bool Pull(vtkSMMessage*) override;

  vtkSetStringMacro(SubTree);

  class vtkIdTypeSet;
  static void GetLeaves(
    vtkGraph* sil, vtkIdType vertexid, vtkIdTypeSet& list, bool traverse_cross_edges);

private:
  vtkSISILProperty(const vtkSISILProperty&) = delete;
  void operator=(const vtkSISILProperty&) = delete;

  char* SubTree;
  int OutputPort;
};

#endif
