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

#ifdef FIXME_COLLABORATION
  // Description:
  // Saves the state of the proxy. 
  virtual vtkPVXMLElement* SaveState(vtkPVXMLElement* root)
    { return this->Superclass::SaveState(root); }

  // Description:
  // Saves the state of the proxy.
  // Overridden to add state for this proxy.
  virtual vtkPVXMLElement* SaveState(
    vtkPVXMLElement* root, vtkSMPropertyIterator *iter, int saveSubProxies);

  // Description:
  // Loads the proxy state from the XML element. Returns 0 on failure.
  // \c locator is used to locate other proxies that may be referred to in the
  // state XML (which happens in case of properties of type vtkSMProxyProperty
  // or subclasses). If locator is NULL, then such properties are left
  // unchanged.
  virtual int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* locator);

  // Description:
  // Same as LoadState except that the proxy will try to undo the changes
  // recorded in the state. 
  virtual int RevertState(vtkPVXMLElement* element, vtkSMProxyLocator* locator);
#endif

//BTX
protected:
  vtkSMComparativeAnimationCueProxy();
  ~vtkSMComparativeAnimationCueProxy();

private:
  vtkSMComparativeAnimationCueProxy(const vtkSMComparativeAnimationCueProxy&); // Not implemented
  void operator=(const vtkSMComparativeAnimationCueProxy&); // Not implemented
//ETX
};

#endif
