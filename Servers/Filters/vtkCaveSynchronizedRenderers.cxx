/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCaveSynchronizedRenderers.h"

#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVServerOptions.h"
#include "vtkPerspectiveTransform.h"
#include "vtkProcessModule.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkUnsignedCharArray.h"

#include "assert.h"


vtkStandardNewMacro(vtkCaveSynchronizedRenderers);
//----------------------------------------------------------------------------
vtkCaveSynchronizedRenderers::vtkCaveSynchronizedRenderers()
{
  this->NumberOfDisplays = 0;
  this->Displays= 0;
  this->SetNumberOfDisplays(1);

  this->DisplayOrigin[0] = 0.0;
  this->DisplayOrigin[1] = 0.0;
  this->DisplayOrigin[2] = 0.0;
  this->DisplayOrigin[3] = 1.0;
  this->DisplayX[0] = 0.0;
  this->DisplayX[1] = 0.0;
  this->DisplayX[2] = 0.0;
  this->DisplayX[3] = 1.0;
  this->DisplayY[0] = 0.0;
  this->DisplayY[1] = 0.0;
  this->DisplayY[2] = 0.0;
  this->DisplayY[3] = 1.0;

  // Screen surface rotation matrix
  SurfaceRot = vtkMatrix4x4::New();
  once =1;
  this->SetParallelController(vtkMultiProcessController::GetGlobalController());

  // Initilize using pvx file specified on the command line options.
  vtkPVServerOptions* options = vtkPVServerOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());
  if (!options)
    {
    vtkErrorMacro("Are you sure vtkCaveSynchronizedRenderers is crated on "
      "an appropriate processes?");
    }
  else
    {
    this->SetNumberOfDisplays(options->GetNumberOfMachines());
    for (int cc=0; cc < this->NumberOfDisplays; cc++)
      {
      if (options->GetDisplayName(cc))
        {
        vtkProcessModule::GetProcessModule()->SetProcessEnvironmentVariable(
          cc, options->GetDisplayName(cc));
        }
      this->DefineDisplay(cc, options->GetLowerLeft(cc),
        options->GetLowerRight(cc), options->GetUpperRight(cc));
      }
    }
  this->SetDisplayConfig();
}

//----------------------------------------------------------------------------
vtkCaveSynchronizedRenderers::~vtkCaveSynchronizedRenderers()
{
  this->SetNumberOfDisplays(0);
  this->SurfaceRot->Delete();
}

//----------------------------------------------------------------------------
void vtkCaveSynchronizedRenderers::HandleStartRender()
{
  this->ImageReductionFactor = 1;
  this->Superclass::HandleStartRender();
  this->ComputeCamera(this->GetRenderer()->GetActiveCamera());
  this->GetRenderer()->ResetCameraClippingRange();
}

//-----------------------------------------------------------------------------
void vtkCaveSynchronizedRenderers::SetNumberOfDisplays(int numberOfDisplays)
{
  if (numberOfDisplays == this->NumberOfDisplays)
    {
    return;
    }
  double** newDisplays = 0;
  if (numberOfDisplays > 0)
    {
    newDisplays = new double*[numberOfDisplays];
    for (int i = 0; i < numberOfDisplays; ++i)
      {
      newDisplays[i] = new double[12];
      if (i < this->NumberOfDisplays)
        {

        memcpy(newDisplays[i], this->Displays[i], 12 * sizeof(double));
        }
      else
        {
        newDisplays[i][0] = -1.;
        newDisplays[i][1] = -1.;
        newDisplays[i][2] = -1.;
        newDisplays[i][3] = 1.0;

        newDisplays[i][4] = 1.0;
        newDisplays[i][5] = -1.0;
        newDisplays[i][6] = -1.0;
        newDisplays[i][7] = 1.0;

        newDisplays[i][8] = -1.0;
        newDisplays[i][9] = 1.0;
        newDisplays[i][10] = -1.0;
        newDisplays[i][11] = 1.0;
        }
      }
    }
  for (int i = 0; i < this->NumberOfDisplays; ++i)
    {
    delete [] this->Displays[i];
    }
  delete [] this->Displays;
  this->Displays = newDisplays;

  this->NumberOfDisplays = numberOfDisplays;
  this->Modified();
}

//-------------------------------------------------------------------------
void vtkCaveSynchronizedRenderers::DefineDisplay(
  int idx, double origin[3], double x[3], double y[3])
{
  if (idx >= this->NumberOfDisplays)
    {
    vtkErrorMacro("idx is too high !");
    return;
    }
  memcpy(&this->Displays[idx][0], origin, 3 * sizeof(double));
  memcpy(&this->Displays[idx][4], x, 3 * sizeof(double));
  memcpy(&this->Displays[idx][8], y, 3 * sizeof(double));
  if (idx == this->GetParallelController()->GetLocalProcessId())
    {
    memcpy(this->DisplayOrigin, origin, 3 * sizeof(double));
    memcpy(this->DisplayX, x, 3 * sizeof(double));
    memcpy(this->DisplayY, y, 3 * sizeof(double));
    }
  this->Modified();
}

//-------------------------------------------------------------------------
// Room camera is a camera in room coordinates that points at the display.
// Client camera is the camera on the client.  The out camera is the
// combination of the two used for the final cave display.
// It is the room camera transformed by the world camera.
void vtkCaveSynchronizedRenderers::ComputeCamera(vtkCamera* cam)
{
  if(once)
    {
      cam->SetHeadTracked( true );
      cam->SetConfigParams( this->O2Screen, this->O2Right,
                            this->O2Left, this->O2Top, this->O2Bottom,
                            0.065, 1.0, this->SurfaceRot);
      once =0;
    }
}

//------------------------------------------------------------------HeadTracked
// This enables the display config for head tracking
void vtkCaveSynchronizedRenderers::SetDisplayConfig()
{
  // Base coordinates of the screen
  double xBase[3], yBase[3], zBase[3];
  for (int i = 0; i < 3; ++i)
    {
    xBase[i] = this->DisplayX[i]-this->DisplayOrigin[i];
    yBase[i] = this->DisplayY[i]-this->DisplayX[i];
    }
  vtkMath::Cross( xBase, yBase, zBase );

  this->SetSurfaceRotation( xBase, yBase, zBase);

  // Get the new DisplayOrigin, DisplayX and DisplayY after transfromation
  this->SurfaceRot->MultiplyPoint( this->DisplayOrigin, this->DisplayOrigin );
  this->SurfaceRot->MultiplyPoint( this->DisplayX, this->DisplayX );
  this->SurfaceRot->MultiplyPoint( this->DisplayY, this->DisplayY );

  // Set O2Screen, O2Right, O2Left, O2Bottom, O2Top
  this->O2Screen = - this->DisplayOrigin[2];
  this->O2Right  =   this->DisplayX[0];
  this->O2Left   = - this->DisplayOrigin[0];
  this->O2Top    =   this->DisplayY[1];
  this->O2Bottom = - this->DisplayX[1];
}

void vtkCaveSynchronizedRenderers::SetSurfaceRotation( double xBase[3],
                                                       double yBase[3],
                                                       double zBase[3])
{
  vtkMath::Normalize( xBase );
  vtkMath::Normalize( yBase );
  vtkMath::Normalize( zBase );

  this->SurfaceRot->SetElement( 0, 0, xBase[0] );
  this->SurfaceRot->SetElement( 0, 1, xBase[1] );
  this->SurfaceRot->SetElement( 0, 2, xBase[2] );

  this->SurfaceRot->SetElement( 1, 0, yBase[0] );
  this->SurfaceRot->SetElement( 1, 1, yBase[1] );
  this->SurfaceRot->SetElement( 1, 2, yBase[2] );

  this->SurfaceRot->SetElement( 2, 0, zBase[0]);
  this->SurfaceRot->SetElement( 2, 1, zBase[1]);
  this->SurfaceRot->SetElement( 2, 2, zBase[2]);
}

//----------------------------------------------------------------------------
void vtkCaveSynchronizedRenderers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfDisplays: "
    << this->NumberOfDisplays << endl;
  vtkIndent rankIndent = indent.GetNextIndent();
  for (int i = 0; i < this->NumberOfDisplays; ++i)
    {
    os << rankIndent;
    for (int j = 0; j < 12; ++j)
      {
      os << this->Displays[i][j] << " ";
      }
    os << endl;
    }
  os << indent << "Origin: "
     << this->DisplayOrigin[0] << " "
     << this->DisplayOrigin[1] << " "
     << this->DisplayOrigin[2] << " "
     << this->DisplayOrigin[3] << endl;
  os << indent << "X: "
     << this->DisplayX[0] << " "
     << this->DisplayX[1] << " "
     << this->DisplayX[2] << " "
     << this->DisplayX[3] << endl;
  os << indent << "Y: "
     << this->DisplayY[0] << " "
     << this->DisplayY[1] << " "
     << this->DisplayY[2] << " "
     << this->DisplayY[3] << endl;
}
