/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballMoveActor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVTrackballMoveActor.h"

#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVWindow.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

vtkCxxRevisionMacro(vtkPVTrackballMoveActor, "1.1");
vtkStandardNewMacro(vtkPVTrackballMoveActor);

//-------------------------------------------------------------------------
vtkPVTrackballMoveActor::vtkPVTrackballMoveActor()
{
}

//-------------------------------------------------------------------------
vtkPVTrackballMoveActor::~vtkPVTrackballMoveActor()
{
}

//-------------------------------------------------------------------------
void vtkPVTrackballMoveActor::OnButtonDown(int x, int y, vtkRenderer *,
                                     vtkRenderWindowInteractor *rwi)
{
  this->LastX = x;
  this->LastY = y;
  rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetDesiredUpdateRate());
}


//-------------------------------------------------------------------------
void vtkPVTrackballMoveActor::OnButtonUp(int x, int y, vtkRenderer *,
                                    vtkRenderWindowInteractor *rwi)
{
  this->LastX = x;
  this->LastY = y;

  rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetStillUpdateRate());
  rwi->Render();
}

//-------------------------------------------------------------------------
void vtkPVTrackballMoveActor::OnMouseMove(int x, int y, vtkRenderer *ren,
                                     vtkRenderWindowInteractor *rwi)
{
  if (ren == NULL)
    {
    return;
    }

  // These are different because y is flipped.
  vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->Application);
  if ( !app )
    {
    return;
    }
  vtkPVWindow *window = app->GetMainWindow();
  vtkPVData* data = window->GetCurrentPVData();
  if (data )
    {
    float bounds[6];
    float center[3];
    float dpoint1[3];
    float startpoint[4];
    float endpoint[4];
    int cc;

    // Get bounds
    data->GetBounds(bounds);

    // Calculate center of bounds.
    for ( cc = 0; cc < 3; cc ++ )
      {
      center[cc] = (bounds[cc *2] + bounds[cc *2 + 1])/2;
      }

    // Convert the center of bounds to display coordinate
    ren->SetWorldPoint(center);
    ren->WorldToDisplay();
    ren->GetDisplayPoint(dpoint1);

    // Convert start point to world coordinate
    ren->SetDisplayPoint(this->LastX, this->LastY, dpoint1[2]);
    ren->DisplayToWorld();
    ren->GetWorldPoint(startpoint);
    
    // Convert end point to world coordinate
    ren->SetDisplayPoint(x, y, dpoint1[2]);
    ren->DisplayToWorld();
    ren->GetWorldPoint(endpoint);

    for ( cc = 0; cc < 3; cc ++ )
      {
      startpoint[cc] /= startpoint[3];
      endpoint[cc]   /= endpoint[3];
      }

    float move[3];
    data->GetActorTranslate(move);
    
    for ( cc = 0; cc < 3; cc ++ )
      {
      move[cc] += endpoint[cc] - startpoint[cc];
      }
    
    data->SetActorTranslate(move);

    ren->ResetCameraClippingRange();
    rwi->Render();
    }

  this->LastX = x;
  this->LastY = y;
}

//-------------------------------------------------------------------------
void vtkPVTrackballMoveActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}






