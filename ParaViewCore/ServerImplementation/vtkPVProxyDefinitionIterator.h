/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProxyDefinitionIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVProxyDefinitionIterator - iterates over all proxy definitions
// that the proxy manager can create new proxies from. It can also iterate
// over registered compound proxy definitions.
// .SECTION Description
// vtkPVProxyDefinitionIterator iterates over all proxy definitions known to
// the proxy manager. The iterator defines mode in which it can be made to
// iterate over definitions of a particular group alone. This iterator can
// also be used to iterate over compound proxy definitions.
// .SECTION See Also
// vtkSMProxyManager

#ifndef __vtkPVProxyDefinitionIterator_h
#define __vtkPVProxyDefinitionIterator_h

#include "vtkObject.h"

class vtkPVXMLElement;
class vtkPVProxyDefinitionManager;

class VTK_EXPORT vtkPVProxyDefinitionIterator : public vtkObject
{
public:
  vtkTypeMacro(vtkPVProxyDefinitionIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // ********* Iterator Commom methods **********

  // Description:
  // Move the iterator to the beginning.
  virtual void GoToFirstItem();
  virtual void InitTraversal()
    { this->GoToFirstItem(); }

  // Description:
  // Move the iterator to the next item.
  virtual void GoToNextItem();

    // Description:
  // Move the iterator to the next group.
  virtual void GoToNextGroup();

  // Description:
  // Test whether the iterator is currently pointing to a valid
  // item.
  virtual bool IsDoneWithTraversal();


  // ********* Configuration methods **********

  virtual void AddTraversalGroupName(const char* groupName) = 0;

  // ********* Access methods **********

  // Access methods
  /// Return the current group name or NULL if Next() was never called.
  virtual const char* GetGroupName();
  /// Return the current proxy name or NULL if Next() was never called.
  virtual const char* GetProxyName();
  /// Return true if the current definition has been defined in the Custom scope
  virtual bool  IsCustom();
  /// Return the current XML proxy definition
  virtual vtkPVXMLElement* GetProxyDefinition();
  /// Return the current XML proxy hints
  virtual vtkPVXMLElement* GetProxyHints() = 0;

protected:
  vtkPVProxyDefinitionIterator();
  ~vtkPVProxyDefinitionIterator();

private:
  vtkPVProxyDefinitionIterator(const vtkPVProxyDefinitionIterator&); // Not implemented.
  void operator=(const vtkPVProxyDefinitionIterator&); // Not implemented.
};


#endif
