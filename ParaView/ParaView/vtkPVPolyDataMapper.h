/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyDataMapper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPolyDataMapper - Optimized verion of vtkOpenGLPolyDataMapper
// .SECTION Description
// vtkPVPolyDataMapper is an  old version of vtkOpenGLPolyDataMapper.
// I could not optimize the macro version as well as this version.
// I could not get the last 20% performance improvement.  This in only 
// temporary.  I plan to have a mapper that uses trinagle strips
// to get better performance in the future.

#ifndef __vtkPVPolyDataMapper_h
#define __vtkPVPolyDataMapper_h

#include "vtkPolyDataMapper.h"

class vtkProperty;
class vtkRenderWindow;
class vtkOpenGLRenderer;

class VTK_EXPORT vtkPVPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkPVPolyDataMapper *New();
  vtkTypeRevisionMacro(vtkPVPolyDataMapper,vtkPolyDataMapper);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement superclass render method.
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *a);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Draw method for OpenGL.
  virtual int Draw(vtkRenderer *ren, vtkActor *a);
  
protected:
  vtkPVPolyDataMapper();
  ~vtkPVPolyDataMapper();

  int ListId;
private:
  vtkPVPolyDataMapper(const vtkPVPolyDataMapper&);  // Not implemented.
  void operator=(const vtkPVPolyDataMapper&);  // Not implemented.
};

#endif
