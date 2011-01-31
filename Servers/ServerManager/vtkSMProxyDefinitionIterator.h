/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyDefinitionIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyDefinitionIterator - iterates over all proxy definitions
// that the proxy manager can create new proxies from. It can also iterate
// over registered compound proxy definitions.
// .SECTION Description
// vtkSMProxyDefinitionIterator iterates over all proxy definitions known to
// the proxy manager. The iterator defines mode in which it can be made to
// iterate over definitions of a particular group alone. This iterator can
// also be used to iterate over compound proxy definitions.
// .SECTION See Also
// vtkSMProxyManager

#ifndef __vtkSMProxyDefinitionIterator_h
#define __vtkSMProxyDefinitionIterator_h

#include "vtkSMObject.h"

class vtkPVXMLElement;
class vtkSMProxyDefinitionManager;

class VTK_EXPORT vtkSMProxyDefinitionIterator : public vtkSMObject
{
public:
  vtkTypeMacro(vtkSMProxyDefinitionIterator, vtkSMObject);
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

protected:
  vtkSMProxyDefinitionIterator();
  ~vtkSMProxyDefinitionIterator();

private:
  vtkSMProxyDefinitionIterator(const vtkSMProxyDefinitionIterator&); // Not implemented.
  void operator=(const vtkSMProxyDefinitionIterator&); // Not implemented.
};


#endif
