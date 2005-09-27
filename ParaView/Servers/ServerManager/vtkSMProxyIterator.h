/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyIterator - iterates over all registered proxies (and groups)
// .SECTION Description
// vtkSMProxyIterator iterates over all proxies registered with the
// proxy manager. It can also iterate over groups.
// .SECTION See Also
// vtkSMProxy vtkSMProxyManager

#ifndef __vtkSMProxyIterator_h
#define __vtkSMProxyIterator_h

#include "vtkSMObject.h"

//BTX
struct vtkSMProxyIteratorInternals;
//ETX

class vtkSMProxyManager;
class vtkSMProxy;

class VTK_EXPORT vtkSMProxyIterator : public vtkSMObject
{
public:
  static vtkSMProxyIterator* New();
  vtkTypeRevisionMacro(vtkSMProxyIterator, vtkSMObject);
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
  // Get the proxy at the current iterator location.
  vtkSMProxy* GetProxy();

  // Description:
  // The traversal mode for the iterator. If the traversal mode is
  // set to GROUPS, each Next() will move to the next group, in
  // ONE_GROUP mode, all proxies in one group are visited and finally
  // in ALL mode, all proxies are visited.
  vtkSetMacro(Mode, int);
  vtkGetMacro(Mode, int);

//BTX
  enum TraversalMode
  {
    GROUPS_ONLY=0,
    ONE_GROUP=1,
    ALL=2
  };
//ETX

protected:
  vtkSMProxyIterator();
  ~vtkSMProxyIterator();

  int Mode;

  virtual void SaveState(const char*, ostream*, vtkIndent) {}

private:
  vtkSMProxyIteratorInternals* Internals;

  vtkSMProxyIterator(const vtkSMProxyIterator&); // Not implemented
  void operator=(const vtkSMProxyIterator&); // Not implemented
};

#endif
