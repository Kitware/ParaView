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


vtkCxxRevisionMacro(vtkDummyRenderWindow, "1.3");

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
  return new vtkDummyRenderWindow;
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

float *vtkDummyRenderWindow::GetZbufferData(int x1, int y1, int x2, int y2)
{
  float *buf;
  int num = (x2-x1+1)*(y2-y1+1);
  int i;
   
  buf = new float[num];
  for (i = 0; i < num; ++i)
    {
    buf[i] = 1.0;
    }

  return buf;
}

float *vtkDummyRenderWindow::GetRGBAPixelData(int x1,int y1,int x2,int y2,
                                              int vtkNotUsed(front))
{
  float *buf;
  int num = (x2-x1+1)*(y2-y1+1)*4;
  int i;
   
  buf = new float[num];
  for (i = 0; i < num; ++i)
    {
    buf[i] = 0.0;
    }

  return buf;
}

unsigned char* 
vtkDummyRenderWindow::GetRGBACharPixelData(int x1,int y1,
                                           int x2,int y2,
                                           int vtkNotUsed(front))
{
  unsigned char *buf;
  int num = (x2-x1+1)*(y2-y1+1)*4;
  int i;
   
  buf = new unsigned char[num];
  for (i = 0; i < num; ++i)
    {
    buf[i] = 0;
    }

  return buf;
}



