/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDummyRenderWindow.cxx
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
#include "vtkDummyRenderWindow.h"
#include "vtkCommand.h"


vtkCxxRevisionMacro(vtkDummyRenderWindow, "1.1");

// Construct an instance of  vtkDummyRenderWindow with its screen size 
// set to 300x300, borders turned on, positioned at (0,0), double 
// buffering turned on, stereo capable off.
vtkDummyRenderWindow::vtkDummyRenderWindow()
{

}

vtkDummyRenderWindow::~vtkDummyRenderWindow()
{
}

// return the correct type of RenderWindow 
vtkDummyRenderWindow *vtkDummyRenderWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkDummyRenderWindow");
  return (vtkDummyRenderWindow*)ret;
}


// Ask each renderer owned by this RenderWindow to render its image and 
// synchronize this process.
void vtkDummyRenderWindow::Render()
{
  vtkRenderer *aren;

  // if we are in the middle of an abort check then return now
  if (this->InAbortCheck)
    {
    return;
    }

  // if we are in a render already from somewhere else abort now
  if (this->InRender)
    {
    return;
    }

  // reset the Abort flag
  this->AbortRender = 0;
  this->InRender = 1;

  vtkDebugMacro(<< "Starting Render Method.\n");
  this->InvokeEvent(vtkCommand::StartEvent,NULL);

  for (this->Renderers->InitTraversal(); 
       (aren = this->Renderers->GetNextItem()); )
    {
    aren->Render();
    }



  this->InRender = 0;
  this->InvokeEvent(vtkCommand::EndEvent,NULL);  
}



void vtkDummyRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}


     


