/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxy -
// .SECTION Description

#ifndef __vtkSMProxy_h
#define __vtkSMProxy_h

#include "vtkSMObject.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

//BTX
struct vtkSMProxyInternals;
//ETX
class vtkSMProperty;

class VTK_EXPORT vtkSMProxy : public vtkSMObject
{
public:
  static vtkSMProxy* New();
  vtkTypeRevisionMacro(vtkSMProxy, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  void AddProperty(const char* name, vtkSMProperty* prop);

  // Description:
  vtkSMProperty* GetProperty(const char* name);

  // Description:
  virtual void UpdateVTKObjects();

  // Description:
  vtkSetStringMacro(VTKClassName);
  vtkGetStringMacro(VTKClassName);

protected:
  vtkSMProxy();
  ~vtkSMProxy();

//BTX
  friend class vtkSMProxyProperty;
  friend class vtkSMDisplayerProxy;
  friend class vtkSMDisplayWindowProxy;
//ETX

  // Description:
  virtual void CreateVTKObjects(int numObjects);

  // Description:
  int GetNumberOfIDs();

  // Description:
  vtkClientServerID GetID(int idx);

  // Description:
  unsigned int GetIDAsInt(int idx) { return this->GetID(idx).ID; }

  // Description:
  int GetNumberOfServerIDs();

  // Description:
  int GetServerID(int i);

  // Description:
  void ClearServerIDs();

  // Description:
  void AddServerID(int id);

  void AddVTKObject(vtkClientServerID id);
  void UnRegisterVTKObjects();

//BTX
  friend class vtkSMProxyObserver;

  // This is a convenience method that pushes the value of one property
  // to one server alone. This is most commonly used by sub-classes
  // to make calls on the server manager through the stream interface.
  // This method does not change the modified flag of the property.
  // If possible, use UpdateVTKObjects() instead of this.
  void PushProperty(const char* name, 
                    vtkClientServerID id, 
                    int serverid);
//ETX
  void RemoveAllObservers();
  void AddProperty(const char* name, 
                   vtkSMProperty* prop, 
                   int addObserver, 
                   int doUpdate);
  void SetPropertyModifiedFlag(const char* name, int flag);

  int* GetServerIDs();

  char* VTKClassName;
  int ObjectsCreated;

private:
  vtkSMProxyInternals* Internals;

  vtkSMProxy(const vtkSMProxy&); // Not implemented
  void operator=(const vtkSMProxy&); // Not implemented
};

#endif
