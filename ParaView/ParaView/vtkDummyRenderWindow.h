/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDummyRenderWindow.h
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
// .NAME vtkDummyRenderWindow - Updates actor pipelines but does not render.
// .SECTION Description
// vtkDummyRenderWindow  is meant to be used with a filter that moves all the data
// to a subset of the processes.  The processes with no data do not need to 
// create a window and render, but they do need to update the pipeline.

// .SECTION see also
// vtkDummyRenderer 

#ifndef __vtkDummyRenderWindow_h
#define __vtkDummyRenderWindow_h

#include "vtkRenderWindow.h"
#include <stdio.h>
#include "vtkGraphicsFactory.h"



class VTK_EXPORT vtkDummyRenderWindow : public vtkRenderWindow
{
public:
  vtkTypeRevisionMacro(vtkDummyRenderWindow,vtkRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct an instance of  vtkDummyRenderWindow with its screen size 
  // set to 300x300, borders turned on, positioned at (0,0), double 
  // buffering turned on.
  static vtkDummyRenderWindow *New();

  // Description:
  // Ask each renderer owned by this RenderWindow to render its image and 
  // synchronize this process.
  virtual void Render();


protected:
  vtkDummyRenderWindow();
  ~vtkDummyRenderWindow();

private:
  vtkDummyRenderWindow(const vtkDummyRenderWindow&);  // Not implemented.
  void operator=(const vtkDummyRenderWindow&);  // Not implemented.
};



#endif


