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

class vtkSMProxyManager;
class vtkSMProxyDefinitionIteratorInternals;
class vtkPVXMLElement;

class VTK_EXPORT vtkSMProxyDefinitionIterator : public vtkSMObject
{
public:
  static vtkSMProxyDefinitionIterator* New();
  vtkTypeMacro(vtkSMProxyDefinitionIterator, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Go to the beginning of the collection.
  void Begin();

  // Description:
  // Go to the beginning of one group.
  void Begin(const char* groupName);

  // Description:
  // Is the iterator pointing past the last element?
  int IsAtEnd();

  // Description:
  // Move to the next property.
  void Next();

  // Description:
  // Get the group at the current iterator location.
  const char* GetGroup();

  // Description:
  // Get the key (proxy name) at the current iterator location.
  const char* GetKey();

  // Description:
  // Returns the XML element defining the proxy.
  vtkPVXMLElement* GetDefinition();

  // Description:
  // Returns if the current definition is a custom proxy definition.
  bool IsCustom();

  // Description:
  // The traversal mode for the iterator. It is not advisable to change the mode
  // during an iteration i.e. after changing mode, one must call Begin() before
  // using the iterator.
  // If the traversal mode is
  // set to GROUPS, each Next() will move to the next group, in
  // ONE_GROUP mode, all proxy definitions in one group are visited,
  // In CUSTOM_ONLY mode, the iterator
  // iterates over custom proxy definitions. 
  // in ALL mode, all proxy definitions are visited. 
  vtkSetMacro(Mode, int);
  vtkGetMacro(Mode, int);
  void SetModeToGroupsOnly() { this->SetMode(GROUPS_ONLY); }
  void SetModeToOneGroup() { this->SetMode(ONE_GROUP); }
  void SetModeToAll() { this->SetMode(ALL); }
  void SetModeToCustomOnly() { this->SetMode(CUSTOM_ONLY); }
//BTX
  enum TraversalMode
  {
    GROUPS_ONLY=0,
    ONE_GROUP=1,
    CUSTOM_ONLY=2,
    ALL=3
  };
//ETX
protected:
  vtkSMProxyDefinitionIterator();
  ~vtkSMProxyDefinitionIterator();

  // Description:
  // If current item is not a custom proxy definition, then 
  // simply keeps on calling NextInternal() until the end is reached or a custom
  // proxy definition is encountered. Used in CUSTOM_ONLY mode.
  void MoveTillCustom();

  // Description:
  // Implementation for Next().
  void NextInternal();
  int Mode;
private:
  vtkSMProxyDefinitionIterator(const vtkSMProxyDefinitionIterator&); // Not implemented.
  void operator=(const vtkSMProxyDefinitionIterator&); // Not implemented.

  vtkSMProxyDefinitionIteratorInternals* Internals;
};


#endif
