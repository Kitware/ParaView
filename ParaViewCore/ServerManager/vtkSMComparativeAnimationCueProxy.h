/*=========================================================================

  Program:   ParaView
  Module:    vtkSMComparativeAnimationCueProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMComparativeAnimationCueProxy - cue used for parameter animation by
// the comparative view.
// .SECTION Description
// vtkSMComparativeAnimationCueProxy is a animation cue used for parameter
// animation by the vtkSMComparativeViewProxy. It provides a non-conventional
// API i.e. without using properties to allow the user to setup parameter
// values over the comparative grid.

#ifndef __vtkSMComparativeAnimationCueProxy_h
#define __vtkSMComparativeAnimationCueProxy_h

#include "vtkSMProxy.h"

class vtkPVComparativeAnimationCue;

class VTK_EXPORT vtkSMComparativeAnimationCueProxy : public vtkSMProxy
{
public:
  static vtkSMComparativeAnimationCueProxy* New();
  vtkTypeMacro(vtkSMComparativeAnimationCueProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the client-side vtkPVComparativeAnimationCue instance. Same as
  // vtkPVComparativeAnimationCue::SafeDownCast(this->GetClientSideObject());
  vtkPVComparativeAnimationCue* GetCue();

  // Description:
  // Saves the state of the proxy. This state can be reloaded
  // to create a new proxy that is identical the present state of this proxy.
  // The resulting proxy's XML hieratchy is returned, in addition if the root
  // argument is not NULL then it's also inserted as a nested element.
  // This call saves all a proxy's properties, including exposed properties
  // and sub-proxies. More control is provided by the following overload.
  virtual vtkPVXMLElement* SaveXMLState(vtkPVXMLElement* root)
    { return this->Superclass::SaveXMLState(root); }
  // Description:
  // The iterator is use to filter the property available on the given proxy
  virtual vtkPVXMLElement* SaveXMLState(vtkPVXMLElement* root, vtkSMPropertyIterator* iter);

  // Description:
  // Loads the proxy state from the XML element. Returns 0 on failure.
  // \c locator is used to locate other proxies that may be referred to in the
  // state XML (which happens in case of properties of type vtkSMProxyProperty
  // or subclasses). If locator is NULL, then such properties are left
  // unchanged.
  virtual int LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator);

//BTX
protected:
  vtkSMComparativeAnimationCueProxy();
  ~vtkSMComparativeAnimationCueProxy();

  // Description:
  // Given a class name (by setting VTKClassName) and server ids (by
  // setting ServerIDs), this methods instantiates the objects on the
  // server(s)
  virtual void CreateVTKObjects();

  // Method used to simplify the access to the concreate VTK class underneath
  friend class vtkSMComparativeAnimationCueUndoElement;
  vtkPVComparativeAnimationCue* GetComparativeAnimationCue();

private:
  vtkSMComparativeAnimationCueProxy(const vtkSMComparativeAnimationCueProxy&); // Not implemented
  void operator=(const vtkSMComparativeAnimationCueProxy&); // Not implemented

  class vtkInternal;
  vtkInternal* Internals;
//ETX
};

#endif
