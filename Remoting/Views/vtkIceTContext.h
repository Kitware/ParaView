/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTContext.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   vtkIceTContext
 *
 *
 *
 * This is a helper class for vtkIceTRenderManager and vtkOpenGLIceTRenderer.
 * Most users will never need this class.
 *
 * This class was conceived to pass IceT contexts between vtkIceTRenderManager
 * and vtkOpenGLIceTRenderer without having to include the IceT header file in
 * either class.  Along the way, some functionality was added.
 *
 *
 * @bug
 * If you set the communicator to nullptr and then to a valid value, the IceT state
 * will be lost.
 *
 * @sa
 * vtkIceTRenderManager
*/

#ifndef vtkIceTContext_h
#define vtkIceTContext_h

#include "vtkObject.h"
#include "vtkRemotingViewsModule.h" // needed for export macro

class vtkMultiProcessController;

class vtkIceTContextOpaqueHandle;

class VTKREMOTINGVIEWS_EXPORT vtkIceTContext : public vtkObject
{
public:
  vtkTypeMacro(vtkIceTContext, vtkObject);
  static vtkIceTContext* New();
  virtual void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Associate the context with the given controller.  Currently, this must
   * be a vtkMPIController.  The context is not valid until a controller is
   * set.
   */
  virtual void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  /**
   * Make this context the current one.
   */
  virtual void MakeCurrent();

  //@{
  /**
   * Turn this on to enable the OpenGL layer in IceT.  By default this is off.
   * Unless you explicitly plan to use the OpenGL layer, it should probably
   * remain off to ensure that you don't accidentally use a feature you did not
   * intend to use.
   */
  vtkGetMacro(UseOpenGL, int);
  virtual void SetUseOpenGL(int flag);
  vtkBooleanMacro(UseOpenGL, int);
  //@}

  /**
   * Copy the state from the given context to this context.
   */
  virtual void CopyState(vtkIceTContext* src);

  /**
   * Returns true if the current state is valid.
   */
  virtual int IsValid();

protected:
  vtkIceTContext();
  ~vtkIceTContext();

  vtkMultiProcessController* Controller;

  int UseOpenGL;

private:
  vtkIceTContext(const vtkIceTContext&) = delete;
  void operator=(const vtkIceTContext&) = delete;

  vtkIceTContextOpaqueHandle* Context;
};

#endif // vtkIceTContext_h
