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

  virtual float *GetZbufferData(int x1, int y1, int x2, int y2);
  virtual float *GetRGBAPixelData(int x1,int y1,int x2,int y2,int front);
  virtual unsigned char* GetRGBACharPixelData(int x1,int y1,int x2,int y2,
                                    int front);

  virtual void SetMultiSamples(int i) {i = i;}

  // Description:
  // The following methods have to be defined since they
  // are pure virtual in super-class.
  virtual void SetDisplayId(void *) {};
  virtual void SetWindowId(void *)  {};
  virtual void SetParentId(void *)  {};
  virtual void *GetGenericDisplayId() {return 0;};
  virtual void *GetGenericWindowId()  {return 0;};
  virtual void *GetGenericParentId()  {return 0;};
  virtual void *GetGenericContext()   {return 0;};
  virtual void *GetGenericDrawable()  {return 0;};
  virtual void SetWindowInfo(char *)  {};
  virtual void SetParentInfo(char *)  {};
  virtual void Start() {};
  virtual void Frame() {};
  virtual void HideCursor() {};
  virtual void ShowCursor() {};
  virtual void SetFullScreen(int) {};
  virtual void WindowRemap() {};
  virtual int GetEventPending() {return 0;};
  virtual void MakeCurrent() {};
  virtual int GetDepthBufferSize() {return -1;};
  virtual unsigned char *GetPixelData(int, int, int, int, int) 
    {
      return 0;
    }
  virtual int GetPixelData(int ,int ,int ,int , int,
			   vtkUnsignedCharArray*) 
    {
      return VTK_ERROR;
    }
  virtual int SetPixelData(int, int, int, int, unsigned char *,int)
    {
      return VTK_ERROR;
    }
  virtual int SetPixelData(int, int, int, int, vtkUnsignedCharArray*,
			   int )
    {
      return VTK_ERROR;
    }
  virtual int GetRGBAPixelData(int, int, int, int, int, vtkFloatArray* )
    {
      return VTK_ERROR;
    }
  virtual int SetRGBAPixelData(int ,int ,int ,int ,float *,int,
			       int vtkNotUsed(blend)=0) 
    {
      return VTK_ERROR;
    }
  virtual int SetRGBAPixelData(int, int, int, int, vtkFloatArray*,
			       int, int vtkNotUsed(blend)=0) 
    {
      return VTK_ERROR;
    }
  virtual int GetRGBACharPixelData(int ,int, int, int, int,
				   vtkUnsignedCharArray*)
    {
      return VTK_ERROR;
    }
  virtual int SetRGBACharPixelData(int ,int ,int ,int ,unsigned char *, int,
				   int vtkNotUsed(blend)=0) 
    {
      return VTK_ERROR;
    }
  virtual int SetRGBACharPixelData(int, int, int, int,
				   vtkUnsignedCharArray *,
				   int, int vtkNotUsed(blend)=0) 
    {
      return VTK_ERROR;
    }
  virtual int GetZbufferData( int, int, int, int, vtkFloatArray*)
    {
      return VTK_ERROR;
    }
  virtual int SetZbufferData(int, int, int, int, float *)
    {
      return VTK_ERROR;
    }
  virtual int SetZbufferData( int, int, int, int, vtkFloatArray * )
    {
      return VTK_ERROR;
    }


protected:
  vtkDummyRenderWindow();
  ~vtkDummyRenderWindow();

private:
  vtkDummyRenderWindow(const vtkDummyRenderWindow&);  // Not implemented.
  void operator=(const vtkDummyRenderWindow&);  // Not implemented.
};



#endif


