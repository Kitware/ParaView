/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVisibleLinesPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVisibleLinesPainter - this is more of a hidden lines remove painter.
// .SECTION Description
// vtkVisibleLinesPainter is a hidden lines removal painter that removes hidden
// lines. Once this painter is inserted in the painter chain, if the
// representation type is VTK_WIREFRAME, then it will automatically remove the
// hidden lines.

#ifndef vtkVisibleLinesPainter_h
#define vtkVisibleLinesPainter_h

#include "vtkOpenGLRepresentationPainter.h"

class VTK_EXPORT vtkVisibleLinesPainter : public vtkOpenGLRepresentationPainter
{
public:
  static vtkVisibleLinesPainter* New();
  vtkTypeMacro(vtkVisibleLinesPainter, vtkOpenGLRepresentationPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release. In this case, releases the display lists.
  virtual void ReleaseGraphicsResources(vtkWindow*);

protected:
  vtkVisibleLinesPainter();
  ~vtkVisibleLinesPainter();

  // Description:
  // Some subclasses may need to do some preprocessing
  // before the actual rendering can be done eg. build effecient
  // representation for the data etc. This should be done here.
  // This method get called after the ProcessInformation()
  // but before RenderInternal().
  virtual void PrepareForRendering(vtkRenderer*, vtkActor*);

  // Description:
  // Changes the polygon mode according to the representation.
  void RenderInternal(
    vtkRenderer* renderer, vtkActor* actor, unsigned long typeflags, bool forceCompileOnly);

  // Description:
  // Returns true when rendering is possible.
  bool CanRender(vtkRenderer*, vtkActor*);

private:
  vtkVisibleLinesPainter(const vtkVisibleLinesPainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVisibleLinesPainter&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
