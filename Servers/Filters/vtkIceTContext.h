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

// .NAME vtkIceTContext
//
// .SECTION Description
//
// This is a helper class for vtkIceTRenderManager and vtkIceTRenderer.  Most
// users will never need this class.
//
// This class was conceived to pass IceT contexts between vtkIceTRenderManager
// and vtkIceTRenderer without having to include the IceT header file in either
// class.  Allong the way, some functionality was added.
//
// .SECTION Bugs
//
// If you set the communicator to NULL and then to a valid value, the IceT state
// will be lost.
//
// .SECTION See Also
// vtkIceTRenderManager

#ifndef _vtkIceTContext_h
#define _vtkIceTContext_h

#include "vtkObject.h"

class vtkMultiProcessController;

class vtkIceTContextOpaqueHandle;

class VTK_EXPORT vtkIceTContext : public vtkObject
{
public:
  vtkTypeMacro(vtkIceTContext, vtkObject);
  static vtkIceTContext *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Associate the context with the given controller.  Currently, this must
  // be a vtkMPIController.  The context is not valid until a controller is
  // set.
  virtual void SetController(vtkMultiProcessController *controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Make this context the current one.
  virtual void MakeCurrent();

  // Description:
  // Copy the state from the given context to this context.
  virtual void CopyState(vtkIceTContext *src);

  // Description:
  // Returns true if the current state is valid.
  virtual int IsValid();

protected:
  vtkIceTContext();
  ~vtkIceTContext();

  vtkMultiProcessController *Controller;

private:
  vtkIceTContext(const vtkIceTContext &);       // Not implemented.
  void operator=(const vtkIceTContext &);       // Not implemented.

  vtkIceTContextOpaqueHandle *Context;
};

#endif //_vtkIceTContext_h

