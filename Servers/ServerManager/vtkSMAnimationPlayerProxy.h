/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationPlayerProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAnimationPlayerProxy - proxy for vtkAnimationPlayer and
// subclasses.
// .SECTION Description
// This is proxy for vtkAnimationPlayer and subclasses.

#ifndef __vtkSMAnimationPlayerProxy_h
#define __vtkSMAnimationPlayerProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMAnimationPlayerProxy : public vtkSMProxy
{
public:
  static vtkSMAnimationPlayerProxy* New();
  vtkTypeMacro(vtkSMAnimationPlayerProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns if currently playing.
  int IsInPlay();

//BTX
protected:
  vtkSMAnimationPlayerProxy();
  ~vtkSMAnimationPlayerProxy();

  virtual void CreateVTKObjects();
private:
  vtkSMAnimationPlayerProxy(const vtkSMAnimationPlayerProxy&); // Not implemented
  void operator=(const vtkSMAnimationPlayerProxy&); // Not implemented

  class vtkObserver;
  vtkObserver* Observer;
//ETX
};

#endif

