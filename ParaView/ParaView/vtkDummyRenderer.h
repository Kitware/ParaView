/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDummyRenderer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDummyRenderer - Does nothing but update the pipelines.
// .SECTION Description
// vtkDummyRenderer is meant to be used with a filter that moves all the data
// to a subset of the processes.  The processes with no data do not need to 
// create a window and render, but they do need to update the pipeline.

// .SECTION See Also
// vtkRenderWindow

#ifndef __vtkDummyRenderer_h
#define __vtkDummyRenderer_h


#include "vtkRenderer.h"

class vtkRayCaster;
class vtkRenderWindow;
class vtkVolume;
class vtkCuller;

class VTK_EXPORT vtkDummyRenderer : public vtkRenderer
{
public:
  vtkTypeRevisionMacro(vtkDummyRenderer,vtkRenderer);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkDummyRenderer *New();

  // Description:
  // Create an image. This is a superclass method which will in turn 
  // call the DeviceRender method of Subclasses of vtkDummyRenderer
  virtual void Render();

  // Description:
  // Create an image. Subclasses of vtkDummyRenderer must implement this method.
  virtual void DeviceRender();
  
  // Description:
  // Render the overlay actors. This gets called from the RenderWindow
  // because it may need to be synchronized to happen after the
  // buffers have been swapped.
  void RenderOverlay();

  virtual float GetPickedZ();
  
protected:
  vtkDummyRenderer();
  ~vtkDummyRenderer();

//BTX
  virtual void DevicePickRender();
  virtual void StartPick(unsigned int pickFromSize);
  virtual void UpdatePickId();
  virtual void DonePick(); 
  virtual unsigned int GetPickedId();
//ETX

private:
  vtkDummyRenderer(const vtkDummyRenderer&);  // Not implemented.
  void operator=(const vtkDummyRenderer&);  // Not implemented.
};


#endif
