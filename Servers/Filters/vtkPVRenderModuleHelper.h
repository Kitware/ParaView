/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderModuleHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRenderModuleHelper
// .SECTION Description
// RenderModules don't have any server-side representation. However, it may be
// necessary to provide access to some render module information to the server 
// side objects (such as actors). This class is a provision for such information
// communication. Each vtkSMRenderModuleProxy creates a vtkPVRenderModuleHelper object
// on the server. For every server-side object that needs information from the
// render module, the rendermodule provides a pointer to this vtkPVRenderModuleHelper
// object. 

#ifndef __vtkPVRenderModuleHelper_h
#define __vtkPVRenderModuleHelper_h

#include "vtkObject.h"

class VTK_EXPORT vtkPVRenderModuleHelper : public vtkObject
{
public:
  static vtkPVRenderModuleHelper* New();
  vtkTypeRevisionMacro(vtkPVRenderModuleHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the LOD Rendering Decision made by the render module.
  vtkSetMacro(LODFlag, int);
  vtkGetMacro(LODFlag, int);
  vtkBooleanMacro(LODFlag, int);

  // Description:
  // Get/Set the RenderModule decision to use Triangle Strips.
  vtkSetMacro(UseTriangleStrips, int);
  vtkGetMacro(UseTriangleStrips, int);
  vtkBooleanMacro(UseTriangleStrips, int);

  // Description:
  // Get/Set the RenderModule decision to use ImmediateMode Rendering.
  vtkSetMacro(UseImmediateMode, int);
  vtkGetMacro(UseImmediateMode, int);
  vtkBooleanMacro(UseImmediateMode, int);
protected:
  vtkPVRenderModuleHelper();
  ~vtkPVRenderModuleHelper();
 
  int LODFlag;
  int UseTriangleStrips;
  int UseImmediateMode;

private:
  vtkPVRenderModuleHelper(const vtkPVRenderModuleHelper&); // Not implemented.
  void operator=(const vtkPVRenderModuleHelper&); // Not implemented.
};


#endif

