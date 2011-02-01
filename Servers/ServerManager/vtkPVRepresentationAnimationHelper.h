/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRepresentationAnimationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRepresentationAnimationHelper - helper proxy used to animate
// properties on the representations for any source.
// .SECTION Description
// vtkPVRepresentationAnimationHelper is helper proxy used to animate
// properties on the representations for any source. This makes is possible to
// set up an animation cue that will affect properties on all representations
// for a source without directly referring to the representation proxies.

#ifndef __vtkPVRepresentationAnimationHelper_h
#define __vtkPVRepresentationAnimationHelper_h

#include "vtkSMProxy.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class VTK_EXPORT vtkPVRepresentationAnimationHelper : public vtkSMProxy
{
public:
  static vtkPVRepresentationAnimationHelper* New();
  vtkTypeMacro(vtkPVRepresentationAnimationHelper, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Don't use directly. Use the corresponding properties intstead.
  void SetVisibility(int);
  void SetOpacity(double);
  void SetSourceProxy(vtkSMProxy* proxy);

//BTX
protected:
  vtkPVRepresentationAnimationHelper();
  ~vtkPVRepresentationAnimationHelper();

  vtkWeakPointer<vtkSMProxy> SourceProxy;
private:
  vtkPVRepresentationAnimationHelper(const vtkPVRepresentationAnimationHelper&); // Not implemented
  void operator=(const vtkPVRepresentationAnimationHelper&); // Not implemented
//ETX
};

#endif
