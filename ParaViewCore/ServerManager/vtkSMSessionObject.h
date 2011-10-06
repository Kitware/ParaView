/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSessionObject.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSessionObject - superclass for any server manager classes
//                            that are related to a session
// .SECTION Description
// vtkSMSessionObject provides methods to set and get the relative session

#ifndef __vtkSMSessionObject_h
#define __vtkSMSessionObject_h

#include "vtkSMObject.h"
#include <vtkWeakPointer.h> // Needed to keep track of the session

class vtkSMSession;
class vtkSMSessionProxyManager;

class VTK_EXPORT vtkSMSessionObject : public vtkSMObject
{
public:
  static vtkSMSessionObject* New();
  vtkTypeMacro(vtkSMSessionObject, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the session on wihch this object exists.
  // Note that session is not reference counted.
  virtual void SetSession(vtkSMSession*);
  virtual vtkSMSession* GetSession();

  // Description:
  // Return the corresponding ProxyManager if any.
  virtual vtkSMSessionProxyManager* GetSessionProxyManager();

protected:
  vtkSMSessionObject();
  ~vtkSMSessionObject();

  // Identifies the session id to which this object is related.
  vtkWeakPointer<vtkSMSession> Session;

private:
  vtkSMSessionObject(const vtkSMSessionObject&); // Not implemented
  void operator=(const vtkSMSessionObject&); // Not implemented
};

#endif
