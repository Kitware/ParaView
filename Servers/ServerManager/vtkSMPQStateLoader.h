/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPQStateLoader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPQStateLoader - state loader with added functionality to reuse
// views. The views to be reused can be set by using AddPreferredView().
// .SECTION Description
// vtkSMPQStateLoader is a state loader with added functionality to reuse
// views. The views to be reused can be set by using AddPreferredView().
// OBSOLETE. To be removed when the pqLookmarkStateLoader is removed.
#ifndef __vtkSMPQStateLoader_h
#define __vtkSMPQStateLoader_h

#include "vtkSMStateLoader.h"

class vtkSMViewProxy;
//BTX
struct vtkSMPQStateLoaderInternals;
//ETX

class VTK_EXPORT vtkSMPQStateLoader : public vtkSMStateLoader
{
public:
  static vtkSMPQStateLoader* New();
  vtkTypeRevisionMacro(vtkSMPQStateLoader, vtkSMStateLoader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // For every request to create a render module, one from this list is used 
  // first, if possible
  virtual void AddPreferredView(vtkSMViewProxy*);
  void RemovePreferredView(vtkSMViewProxy*);
  void ClearPreferredViews();

protected:
  vtkSMPQStateLoader();
  ~vtkSMPQStateLoader();

  // Description:
  // Overridden so that 'preferred views' are used if possible.
  virtual vtkSMProxy* CreateProxy(
    const char* xmlgroup, const char* xmlname, vtkIdType cid);

  // Overridden to avoid registering the reused rendermodules twice.
  virtual void RegisterProxyInternal(const char* group, 
    const char* name, vtkSMProxy* proxy);

  vtkSMPQStateLoaderInternals *PQInternal;
private:
  vtkSMPQStateLoader(const vtkSMPQStateLoader&); // Not implemented.
  void operator=(const vtkSMPQStateLoader&); // Not implemented.
};

#endif

