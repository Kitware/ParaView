/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkMantaRenderWindow.cxx

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkManta.h"
#include "vtkMantaRenderer.h"
#include "vtkMantaRenderWindow.h"

#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

#include <Image/SimpleImage.h>
#include <Engine/Control/RTRT.h>
#include <Engine/Display/SyncDisplay.h>

vtkCxxRevisionMacro(vtkMantaRenderWindow, "1.8");
vtkStandardNewMacro(vtkMantaRenderWindow);

//----------------------------------------------------------------------------
vtkMantaRenderWindow::vtkMantaRenderWindow() : ColorBuffer(0), DepthBuffer(0)
{
  cerr << "CREATE MANTA RENDER WINDOW " << this << endl;

  if ( this->WindowName )
    {
    delete [] this->WindowName;
    this->WindowName = NULL;
    }

  this->WindowName = new char[ strlen("Visualization Toolkit - Manta") + 1 ];
  strcpy( this->WindowName, "Visualization Toolkit - Manta" );
}

//----------------------------------------------------------------------------
vtkMantaRenderWindow::~vtkMantaRenderWindow()
{
  cerr << "DESTROY MANTA RENDER WINDOW " << this << endl;

  vtkRenderer * ren = NULL;
  this->Renderers->InitTraversal();
  for ( ren  = this->Renderers->GetNextItem();
        ren != NULL;
        ren  = this->Renderers->GetNextItem() )
    {
    // currently a call to vtkMantaActor::ReleaseGraphicsResources(vtkWindow*)
    // causes a segfault problem upon the exit of ParaView UNLESS the user
    // explicitly deletes all objects via panel 'Pipeline Browser'. Thus here
    // we only allow the renderer on layer #1, i.e., the vtkOpenGLRenderer
    // object associated with the orientation axes widget, to call vtkRenderer
    // ::SetRenderWindow(vtkWindow *) that asks each vtkProp / vtkActor to
    // call vtkProp / vtkActor::ReleaseGraphicsResources(vtkWindow *) to
    // release OpenGL-specific resources used to render the orientation axes.
    //
    // Once the aforemention segfault problem is fixed, each vtkMantaRenderer
    // object will be asked below to call vtkMantaRenderer::SetRenderWindow(
    // vtkWindow *). However, an if-statement is still needed below to decide
    // which function, vtkRenderer::SetRenderWindow(...) or vtkMantaRenderer::
    // SetRenderWindow(... to be added ...), is actually called here.
    //
    // The lack of this if-statement would cause a segfault problem upon exit,
    // either for VTK examples or ParaView.
    if ( ( this->GetNumberOfLayers() >  1 && ren->GetLayer() == 1 ) ||
         ( this->GetNumberOfLayers() <= 1 ) )
      {
      ren->SetRenderWindow( NULL );
      }
    }
  ren = NULL;

  delete [] this->ColorBuffer;
  this->ColorBuffer = NULL;
  delete [] this->DepthBuffer;
  this->DepthBuffer = NULL;
}

//------------------------------------------------------------------------------
int* vtkMantaRenderWindow::GetSize( )
{
  int tempSize[2];
  tempSize[0] = this->Size[0];
  tempSize[1] = this->Size[1];

  int *ret = this->Superclass::GetSize();

  if (this->Size[0] != tempSize[0] || this->Size[1] != tempSize[1])
    {
    //on a change, let Manta know
    this->InternalSetSize(this->Size[0], this->Size[1]);
    }

  return ret;
}

//------------------------------------------------------------------------------
void vtkMantaRenderWindow::SetSize( int width, int height )
{
  if (this->Size[0] == width && this->Size[1] == height)
    {
    return;
    }
  this->InternalSetSize(width, height);
}

//------------------------------------------------------------------------------
void vtkMantaRenderWindow::InternalSetSize( int width, int height )
{
  // reallocate pixel buffers
  delete [] this->ColorBuffer;
  this->ColorBuffer = new uint32[width*height];
  delete [] this->DepthBuffer;
  this->DepthBuffer = new float[width*height];

  this->Renderers->InitTraversal();
  for ( vtkRenderer *ren  = this->Renderers->GetNextItem();
        ren != NULL;
        ren  = this->Renderers->GetNextItem() )
    {
    vtkMantaRenderer * mantaRenderer = vtkMantaRenderer::SafeDownCast(ren);
    if (!mantaRenderer)
      {
      //TODO: Handle GL layers
      }
    else
      {
      //TODO: The renderer should resize itself based on our size

      // compute an up-to-date size for the renderer and do NOT always trust
      // vtkRenderer::GetSize() that may return an obsolete size upon
      // window shrinking. For example when the size of layer #1 is proportional
      // to that of the render window through vtkRenderer::SetViewport() upon
      // the initialization of the renderer (as in the "multiRen" example)
      double * renViewport = mantaRenderer->GetViewport();
      int renderSize[2];
      renderSize[0] = int((renViewport[2] - renViewport[0]) * width  + 0.5f);
      renderSize[1] = int((renViewport[3] - renViewport[1]) * height + 0.5f);      
      if (mantaRenderer->GetSyncDisplay())
        {
#if 0
        //just for debugging
        cerr << "VSIZE NOW " << renderSize[0] << "x" << renderSize[1] << endl;
        bool dummy;
        int mantaSize[2];
        mantaRenderer->GetSyncDisplay()->getCurrentImage()->
          getResolution( dummy, mantaSize[0], mantaSize[1] );
        cerr << "0 MSIZE NOW " << mantaSize[0] << "x" << mantaSize[1] << endl;
#endif
        mantaRenderer->GetMantaEngine()-> addTransaction
          ("resize",
           Manta::Callback::create
           (mantaRenderer->GetMantaEngine(),
            &Manta::MantaInterface::changeResolution,
            mantaRenderer->GetChannelId(), renderSize[0], renderSize[1],
            true));
        
        //Wait for manta to: receive the above transaction (after end of current frame),
        //render with that, and for the old sized images to drain from the pipeline
        mantaRenderer->GetSyncDisplay()->waitOnFrameReady();
        mantaRenderer->GetSyncDisplay()->doneRendering();
        mantaRenderer->GetSyncDisplay()->waitOnFrameReady();
        mantaRenderer->GetSyncDisplay()->doneRendering();
        //next wait (inside vtkMantaRenderer) is guaranteed to get the right size,
        //but not before then (first wait above might happen before transaction handled)
        }
      }
    }

  this->Superclass::SetSize(width, height);
}

//------------------------------------------------------------------------------
#define USE_SSE 0
#if USE_SSE
#include "ColorKey/ColorKeyCPYSSE.h"
#include "ColorKey/ColorKeySSE.h"
#include "ZKey/ZKeyRangeSSE.h"
#endif
void vtkMantaRenderWindow::CopyResultFrame(void)
{
  // When this function is called, Render() has already been called on each of
  // the renderers in this->Renderers. The RGBA and Z buffer data should also
  // be ready by now.
  int numPixels = this->Size[0]*this->Size[1];

  vtkTimerLog::MarkStartEvent("Color-key");

  // TODO: move this function back to LayerRender() for vtkMantaRenderer layers
  // to make it single buffering rather than double buffering, it will save us
  // 5ms per frame on the 8 cores machine
  this->Renderers->InitTraversal();
  for ( vtkRenderer *ren  = this->Renderers->GetNextItem();
        ren != NULL;
        ren  = this->Renderers->GetNextItem() )
    {
    if (ren->GetLayer() != 0 &&
        ren->GetActors()->GetNumberOfItems() == 0)
      {
      // skip image composition if we are not Layer 0 and nothing is rendered
      // in this layer.
      // cerr << "empty layer: " << ren->GetLayer() << endl;
      continue;
      }

    int    * renderSize  = ren->GetSize();
    double * renViewport = ren->GetViewport();
    int    * renWinSize  = this->GetActualSize();
    int      renderPos[2];
    renderPos[0] = int( renViewport[0] * renWinSize[0] + 0.5f );
    renderPos[1] = int( renViewport[1] * renWinSize[1] + 0.5f );
    
    vtkMantaRenderer *mantaRenderer = vtkMantaRenderer::SafeDownCast(ren);
    if ( mantaRenderer  != 0 )
      {
      bool mStereo;
      int mXres;
      int mYres;
      //make sure we don't go off bounds since manta is asynch and lags
      mantaRenderer->GetMantaEngine()->getResolution(0, mStereo, mXres, mYres);

      //cerr << "Manta layer: " << ren->GetLayer() << endl;
      const uint32 *srcRGB = reinterpret_cast<uint32 *>(mantaRenderer->GetColorBuffer());
      const float *srcZ = mantaRenderer->GetDepthBuffer();
      uint32 *dstRGB = this->ColorBuffer + renWinSize[0] * renderPos[1] + renderPos[0];
      float *dstZ = this->DepthBuffer + renWinSize[0] * renderPos[1] + renderPos[0];

      //cout << "diff: " << (renWinSize[0] - renderSize[0]) << endl;

      if (mantaRenderer->GetLayer() == 0)
        {
#if !USE_SSE
        // unconditionally overwrite old buffer data if it is layer 0
        for (int j = 0; j < renderSize[1] && j < mYres; j++)
          {
          for (int i = 0; i < renderSize[0] && i < mXres; i++)
            {
              *(dstRGB++) = *(srcRGB++);
              *(dstZ++)   = *(srcZ++);
            }
          dstRGB += (renWinSize[0] - renderSize[0]);
          dstZ   += (renWinSize[0] - renderSize[0]);
          }
#else
        ColorKeyCPYSSE fObj(0, renderSize[0]);
        for (int i = 0; i < renderSize[1]; i++)
          {
           fObj(srcRGB, dstRGB);
           fObj(reinterpret_cast<const uint32 *> (srcZ), reinterpret_cast<uint32 *>(dstZ));
           srcRGB += renderSize[0], dstRGB += renWinSize[0];
           srcZ   += renderSize[0], dstZ   += renWinSize[0];
          }
#endif
        } // end of first layer case
      else
        {
        // perform color-key onto my own RGBA buffer
        double *bgColor = ren->GetBackground();
        unsigned char bgPixel[4];
        for (int i = 0; i < 3; i++) {
          bgPixel[i] = (unsigned char) (bgColor[i] * 255.f);
         }
        bgPixel[3] = 0;

#if !USE_SSE
        for (int j = 0; j < renderSize[1]; j++)
          {
          for (int i = 0; i < renderSize[0]; i++)
            {
            if ( *srcRGB != *(reinterpret_cast<uint32 *>(bgPixel)) )
              {
              // foreground pixel
              *(dstRGB) = *(srcRGB);
              }
              srcRGB++, dstRGB++;
            }
          dstRGB += (renWinSize[0] - renderSize[0]);
          }
#else
        ColorKeySSE fObj(*(reinterpret_cast<uint32 *>(bgPixel)), renderSize[0]);
        for (int i = 0; i < renderSize[1]; i++)
        {
            fObj(srcRGB, dstRGB);
            srcRGB += renderSize[0], dstRGB += renWinSize[0];
            srcZ   += renderSize[0], dstZ   += renWinSize[0];
        }
#endif
        } // end of overlay layer case
      } // end of MantaRenderer case
    else
      {
      // read RGBA from OpenGL buffer and do color-key operation
      // cerr << "OpenGL layer: " << ren->GetLayer() << endl;

      const uint32 *srcRGB =
          reinterpret_cast<uint32 *> (this->vtkOpenGLRenderWindow::GetRGBACharPixelData(
              renderPos[0], renderPos[1],
              renderPos[0] + renderSize[0] - 1,
              renderPos[1] + renderSize[1] - 1, 0 /* back buffer */));
      // TODO: make it color key, we don't have to read back Z buffer if it is
      // truly color-key. The PROBLEM is we have to use the background color of
      // mantaRenderer, not the background color of OpenGLRenderer. WHY???
      // TODO: why doesn't virtual function works here?
      float *srcZ          =  new float[this->Size[1]*this->Size[0]];
      this->vtkOpenGLRenderWindow::GetZbufferData(
          renderPos[0], renderPos[1],
          renderPos[0] + renderSize[0] - 1,
          renderPos[1] + renderSize[1] - 1, srcZ);
      uint32 *dstRGB       = this->ColorBuffer  + renWinSize[0] * renderPos[1] + renderPos[0];
      // srcRGB and srcZ are modified in the composition, we need to save them
      // to delete[] the buffers
      const uint32 *glRGB  = srcRGB;
      const float *glZ     = srcZ;

#if !USE_SSE
      for (int j = 0; j < renderSize[1]; j++)
        {
        for (int i = 0; i < renderSize[0]; i++)
          {
          // perform color (actually Z)-key onto my own RGBA buffer
          // TODO: make it color key
          const float z = *srcZ;
          if (z > 0 && z < 1.0)
            {
            // foreground pixel
            *(dstRGB) = *(srcRGB);
            }
          srcRGB++, dstRGB++, srcZ++;
          }
        dstRGB += (renWinSize[0] - renderSize[0]);
        }
#else
      ZKeyRangeSSE fObj(0.0f, 1.0f, renderSize[0]);
      for (int i = 0; i < renderSize[1]; i++)
        {
         fObj(reinterpret_cast<const float *>(srcRGB), srcZ,
           reinterpret_cast<float *>(dstRGB) );
         srcRGB += renderSize[0], dstRGB += renWinSize[0];
         srcZ   += renderSize[0];// dstZ   += renWinSize[0];
        }
#endif

      delete [] glRGB;
      delete [] glZ;

      } // end of OpenGLRenderer case
    } // end of for() each renderer

  vtkTimerLog::MarkEndEvent("Color-key");

  if (!this->OffScreenRendering)
    {
    vtkTimerLog::MarkStartEvent("DrawRGBAPixel");
    // on screen rendering, draw RGBA data to hardware buffer
    this->vtkOpenGLRenderWindow::SetRGBACharPixelData(
        0, 0, this->Size[0] - 1, this->Size[1] - 1,
        reinterpret_cast<unsigned char *>(this->ColorBuffer),
        !this->GetDoubleBuffer(), 0 /* no alpha blending */ );
    // this calls glFlush()/glXSwapBuffers()
    this->Frame();
    vtkTimerLog::MarkEndEvent("DrawRGBAPixel");
    }
}

//------------------------------------------------------------------------------
// TODO: why doesn't virtual function works here?
int vtkMantaRenderWindow::GetRGBACharPixelData(int x1, int y1,
                                               int x2, int y2,
                                               int front,
                                               vtkUnsignedCharArray* data)
{
  int y_low, y_hi;
  int x_low, x_hi;

  if (y1 < y2)
    {
    y_low = y1;
    y_hi = y2;
    }
  else
    {
    y_low = y2;
    y_hi = y1;
    }

  if (x1 < x2)
    {
    x_low = x1;
    x_hi = x2;
    }
  else
    {
    x_low = x2;
    x_hi = x1;
    }

  int width = abs(x_hi - x_low) + 1;
  int height = abs(y_hi - y_low) + 1;
  int size = 4* width * height;

  if (data->GetMaxId() + 1 != size)
    {
    vtkDebugMacro("Resizing array.");
    data->SetNumberOfComponents(4);
    data->SetNumberOfValues(size);
    }
  return this->GetRGBACharPixelData(x1, y1, x2, y2, front, data->GetPointer(0));
}

//------------------------------------------------------------------------------
// TODO: why doesn't virtual function works here?
int vtkMantaRenderWindow::GetRGBACharPixelData(int x1, int y1,
                                               int x2, int y2,
                                               int front,
                                               unsigned char* data)
{
  // TODO: this does not actually honor the parameters
  if (this->ColorBuffer == 0)
    {
    return VTK_ERROR;
    }
  else
    {
    int width, height;
    width = abs(x2 - x1) + 1;
    height = abs(y2 - y1) + 1;

    for (int i = 0; i < width * height; i++)
      {
      reinterpret_cast<uint32 *> (data)[i] = this->ColorBuffer[i];
      }

    return VTK_OK;
    }
}

//------------------------------------------------------------------------------
int vtkMantaRenderWindow::GetZbufferData(int x1, int y1, int x2, int y2,
                                         float* z_data)
{
  // TODO: this does not actually honor the parameters
  if ( this->DepthBuffer == 0 )
    {
    return VTK_ERROR;
    }
  else
    {
    int width, height;
    width  = abs(x2 - x1) + 1;
    height = abs(y2 - y1) + 1;

    for ( int i = 0; i < width * height; i ++ )
      {
      z_data[i] = this->DepthBuffer[i];
      }

    return VTK_OK;
    }
}

//----------------------------------------------------------------------------
void vtkMantaRenderWindow::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//----------------------------------------------------------------------------
void vtkMantaRenderWindow::AddRenderer( vtkRenderer *ren )
{
  vtkMantaRenderer *mren = vtkMantaRenderer::SafeDownCast(ren);
  if (mren)
    {
    cerr << "Adding a manta renderer " << mren << endl;
    }
  else
    {
    cerr << "Adding a non manta renderer " << ren << " " << ren->GetClassName() << endl;
    }
  this->Superclass::AddRenderer( ren );
}
