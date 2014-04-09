/*=========================================================================

Program:   ParaView
Module:    vtkSMContextViewProxy.h

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPlotMatrixViewProxy - Proxy class for plot matrix view

#ifndef __vtkSMPlotMatrixViewProxy_h
#define __vtkSMPlotMatrixViewProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMContextViewProxy.h"
#include "vtkClientServerStream.h" // For CS stream methods.

class vtkAbstractContextItem;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMPlotMatrixViewProxy : public vtkSMContextViewProxy
{
public:
  static vtkSMPlotMatrixViewProxy* New();
  vtkTypeMacro(vtkSMPlotMatrixViewProxy, vtkSMContextViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provides access to the vtk plot matrix.
  virtual vtkAbstractContextItem* GetContextItem();
//BTX
protected:
  virtual void CreateVTKObjects();
  void ActivePlotChanged();

  void PostRender(bool);

  bool ActiveChanged;

  vtkSMPlotMatrixViewProxy();
  ~vtkSMPlotMatrixViewProxy();
  void SendAnimationPath();
  void AnimationTickEvent();

private:
  vtkSMPlotMatrixViewProxy(const vtkSMPlotMatrixViewProxy&); // Not implemented
  void operator=(const vtkSMPlotMatrixViewProxy&); // Not implemented
//ETX
};

#endif
