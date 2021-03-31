/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkZSpaceRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkZSpaceRenderWindowInteractor
 * @brief   implements zSpace specific functions
 * required by vtkRenderWindowInteractor.
 *
 */

#ifndef vtkZSpaceRenderWindowInteractor_h
#define vtkZSpaceRenderWindowInteractor_h

#include "vtkRenderWindowInteractor3D.h"
#include "vtkZSpaceViewModule.h" // For export macro

#include "vtkEventData.h"

class vtkCamera;
class VTKZSPACEVIEW_EXPORT vtkZSpaceRenderWindowInteractor : public vtkRenderWindowInteractor3D
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static vtkZSpaceRenderWindowInteractor* New();

  vtkTypeMacro(vtkZSpaceRenderWindowInteractor, vtkRenderWindowInteractor3D);

  /**
   * These methods correspond to the Exit, User and Pick
   * callbacks. They allow for the Style to invoke them.
   */
  virtual void ExitCallback();

  /**
   * Run the event loop and return. This is provided so that you can
   * implement your own event loop but yet use the vtk event handling as
   * well.
   */
  void ProcessEvents() override;

  /*
   * Return the pointer index as a device
   */
  vtkEventDataDevice GetPointerDevice();

  /**
   * Update WorldEventPosition and WorldEventOrientation, then
   * call event functions depending on the zSpace buttons states.
   */
  void HandleInteractions();

  //@{
  /**
   * LeftButton event function (invoke Button3DEvent)
   * Initiate a clip : choose a clipping plane origin
   * and normal with the stylus.
   */
  void OnLeftButtonDown(vtkEventDataDevice3D*);
  void OnLeftButtonUp(vtkEventDataDevice3D*);
  //@}

  //@{
  /**
   * MiddleButton event function (invoke Button3DEvent)
   * Allows to position a prop with the stylus.
   */
  void OnMiddleButtonDown(vtkEventDataDevice3D*);
  void OnMiddleButtonUp(vtkEventDataDevice3D*);
  //@}

  //@{
  /**
   * LeftButton event function (invoke Button3DEvent)
   * Perform an hardware picking with the stylus
   * and show picked data if ShowPickedData is true.
   */
  void OnRightButtonDown(vtkEventDataDevice3D*);
  void OnRightButtonUp(vtkEventDataDevice3D*);
  //@}

protected:
  vtkZSpaceRenderWindowInteractor();
  ~vtkZSpaceRenderWindowInteractor() override = default;

private:
  vtkZSpaceRenderWindowInteractor(const vtkZSpaceRenderWindowInteractor&) = delete;
  void operator=(const vtkZSpaceRenderWindowInteractor&) = delete;
};

#endif
