/*=========================================================================

  Module:    vtkPVWindowToImageFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVWindowToImageFilter - PV version of vtkWindowToImageFilter
// .SECTION Description
// vtkPVWindowToImageFilter is a simplified version of vtkWindowToImageFilter
// that takes a vtkPVRenderModule as input instead of a vtkRenderWindow.
// The output image is the same size as the render window; magnification is
// not supported.  This version calls StillRender on the render module, to
// take care of the cases where the things that happen in the render module's
// render are not triggered off of the call to Render on the render window.

#ifndef __vtkPVWindowToImageFilter_h
#define __vtkPVWindowToImageFilter_h

#include "vtkImageSource.h"

class vtkPVRenderModule;

class VTK_EXPORT vtkPVWindowToImageFilter : public vtkImageSource
{
public:
  static vtkPVWindowToImageFilter* New();
  vtkTypeRevisionMacro(vtkPVWindowToImageFilter, vtkImageSource);
  void PrintSelf(ostream &os, vtkIndent indent);
  
  // Description:
  // Indicates where to get the pixel data from.
  void SetInput(vtkPVRenderModule *input);
  
  // Description:
  // Set/Get the flag that determines which buffer to read from.
  // The default is to read from the front buffer.   
  vtkBooleanMacro(ReadFrontBuffer, int);
  vtkGetMacro(ReadFrontBuffer, int);
  vtkSetMacro(ReadFrontBuffer, int);

  // Description:
  // Set/get the flag for whether the filter should render or just grab the
  // color buffer.
  vtkBooleanMacro(ShouldRender, int);
  vtkGetMacro(ShouldRender, int);
  vtkSetMacro(ShouldRender, int);
  
protected:
  vtkPVWindowToImageFilter();
  ~vtkPVWindowToImageFilter();
  
  // vtkPVRenderModule is not a vtkDataObject, so we need our own ivar.
  vtkPVRenderModule *Input;
  
  int ReadFrontBuffer;
  int ShouldRender;
  
  void ExecuteInformation();
  void ExecuteData(vtkDataObject *data);
  
private:
  vtkPVWindowToImageFilter(const vtkPVWindowToImageFilter&); // Not implemented
  void operator=(const vtkPVWindowToImageFilter&); // Not implemented
};

#endif
