/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballMoveActor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTrackballMoveActor.h"

#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVSource.h"
#include "vtkPVDataInformation.h"
#include "vtkPVWindow.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

vtkCxxRevisionMacro(vtkPVTrackballMoveActor, "1.13");
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
void vtkPVTrackballMoveActor::OnButtonDown(int, int, vtkRenderer *,
                                           vtkRenderWindowInteractor *)
{
}


//-------------------------------------------------------------------------
void vtkPVTrackballMoveActor::OnButtonUp(int, int, vtkRenderer *,
                                         vtkRenderWindowInteractor *)
{
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
  vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->GetApplication());
  if ( !app )
    {
    return;
    }
  vtkPVWindow *window = app->GetMainWindow();
  vtkPVSource* pvs = window->GetCurrentPVSource();
  if ( pvs )
    {
    double bounds[6];
    double center[4];
    double dpoint1[3];
    double startpoint[4];
    double endpoint[4];
    int cc;

    // Get bounds
    pvs->GetDataInformation()->GetBounds(bounds);

    // Calculate center of bounds.
    for ( cc = 0; cc < 3; cc ++ )
      {
      center[cc] = (bounds[cc *2] + bounds[cc *2 + 1])/2;
      }
    center[3] = 1;

    // Convert the center of bounds to display coordinate
    ren->SetWorldPoint(center);
    ren->WorldToDisplay();
    ren->GetDisplayPoint(dpoint1);

    // Convert start point to world coordinate
    ren->SetDisplayPoint(rwi->GetLastEventPosition()[0],
                         rwi->GetLastEventPosition()[1], dpoint1[2]);
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

    double move[3];
    pvs->GetDisplayGUI()->GetActorTranslate(move);
    
    for ( cc = 0; cc < 3; cc ++ )
      {
      move[cc] += endpoint[cc] - startpoint[cc];
      }
    
    pvs->GetDisplayGUI()->SetActorTranslate(move);

    ren->ResetCameraClippingRange();
    rwi->Render();
    }
}

//-------------------------------------------------------------------------
void vtkPVTrackballMoveActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}






