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
/**
 * @class   vtkSMPlotMatrixViewProxy
 * @brief   Proxy class for plot matrix view
*/

#ifndef vtkSMPlotMatrixViewProxy_h
#define vtkSMPlotMatrixViewProxy_h

#include "vtkClientServerStream.h"             // For CS stream methods.
#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMContextViewProxy.h"

class vtkAbstractContextItem;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMPlotMatrixViewProxy : public vtkSMContextViewProxy
{
public:
  static vtkSMPlotMatrixViewProxy* New();
  vtkTypeMacro(vtkSMPlotMatrixViewProxy, vtkSMContextViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Provides access to the vtk plot matrix.
   */
  virtual vtkAbstractContextItem* GetContextItem();

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
  vtkSMPlotMatrixViewProxy(const vtkSMPlotMatrixViewProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMPlotMatrixViewProxy&) VTK_DELETE_FUNCTION;
};

#endif
