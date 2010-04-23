/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRepresentationAnimationHelperProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRepresentationAnimationHelperProxy - helper proxy used to animate
// properties on the representations for any source. 
// .SECTION Description
// vtkSMRepresentationAnimationHelperProxy is helper proxy used to animate
// properties on the representations for any source. This makes is possible to
// set up an animation cue that will affect properties on all representations
// for a source without directly referring to the representation proxies.

#ifndef __vtkSMRepresentationAnimationHelperProxy_h
#define __vtkSMRepresentationAnimationHelperProxy_h

#include "vtkSMProxy.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class VTK_EXPORT vtkSMRepresentationAnimationHelperProxy : public vtkSMProxy
{
public:
  static vtkSMRepresentationAnimationHelperProxy* New();
  vtkTypeMacro(vtkSMRepresentationAnimationHelperProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Don't use directly. Use the corresponding properties intstead.
  void SetVisibility(int);
  void SetOpacity(double);
  void SetSourceProxy(vtkSMProxy* proxy);

//BTX
protected:
  vtkSMRepresentationAnimationHelperProxy();
  ~vtkSMRepresentationAnimationHelperProxy();

  vtkWeakPointer<vtkSMProxy> SourceProxy;
private:
  vtkSMRepresentationAnimationHelperProxy(const vtkSMRepresentationAnimationHelperProxy&); // Not implemented
  void operator=(const vtkSMRepresentationAnimationHelperProxy&); // Not implemented
//ETX
};

#endif

