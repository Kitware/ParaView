#include "vtkConeSource.h"
#include "vtkSphereSource.h"
#include "vtkCylinderSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"

// this program tests toggling visibility of objects

//----------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
  int     second = 1;
  int     objRes = 12;
  double  objRad = 0.075;

  int     objVis[15][3] =
  { 
    { 1, 1, 1 }, { 0, 1, 1 }, { 1, 1, 1 }, { 1, 0, 1 }, { 1, 1, 1 }, 
    { 1, 1, 0 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 1, 1 }, { 0, 1, 0 },
    { 1, 1, 1 }, { 0, 0, 1 }, { 1, 1, 1 }, { 0, 0, 0 }, { 1, 1, 1 }
  };

  // cone
  vtkConeSource * cone= vtkConeSource::New();
  cone->SetRadius( objRad );
  cone->SetHeight( objRad * 2 );
  cone->SetResolution( objRes );

  vtkPolyDataMapper * coneMapper = vtkPolyDataMapper::New();
  coneMapper->SetInputConnection( cone->GetOutputPort() );
  
  vtkActor * coneActor = vtkActor::New();
  coneActor->SetMapper( coneMapper );
  coneActor->AddPosition( 0.0, objRad * 2.0, 0.0 );
  coneActor->RotateZ( 90.0 );
    
  // sphere
  vtkSphereSource * sphere = vtkSphereSource::New();
  sphere->SetCenter( 0.0, 0.0, 0.0 );
  sphere->SetRadius( objRad );
  sphere->SetThetaResolution( objRes );
  sphere->SetPhiResolution  ( objRes );

  vtkPolyDataMapper * sphereMapper = vtkPolyDataMapper::New();
  sphereMapper->SetInputConnection( sphere->GetOutputPort() );
  
  vtkActor * sphereActor = vtkActor::New();
  sphereActor->SetMapper( sphereMapper );
  
  // cylinder
  vtkCylinderSource * cylinder = vtkCylinderSource::New();
  cylinder->SetCenter( 0.0, -objRad * 2, 0.0 );
  cylinder->SetRadius( objRad );
  cylinder->SetHeight( objRad * 2 );
  cylinder->SetResolution( objRes );

  vtkPolyDataMapper * cylinderMapper = vtkPolyDataMapper::New();
  cylinderMapper->SetInputConnection( cylinder->GetOutputPort() );
  
  vtkActor * cylinderActor = vtkActor::New();
  cylinderActor->SetMapper( cylinderMapper );
  
  vtkRenderer * renderer = vtkRenderer::New();
  renderer->SetBackground( 0.0, 0.0, 1.0 );
  renderer->AddActor( coneActor );
  renderer->AddActor( sphereActor );
  renderer->AddActor( cylinderActor );

  vtkRenderWindow * renWin = vtkRenderWindow::New();
  renWin->AddRenderer( renderer );
  renWin->SetSize( 400, 400 );

  vtkRenderWindowInteractor * interactor = vtkRenderWindowInteractor::New();
  interactor->SetRenderWindow( renWin );

  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    interactor->Start();
    }
  else
    {
    for ( int i = 0; i < 15; i ++ )
      {
      coneActor->SetVisibility( objVis[i][0] );
      sphereActor->SetVisibility( objVis[i][1] );
      cylinderActor->SetVisibility( objVis[i][2] );
      renWin->Render();
      if ( i == 3 )
        {
        retVal = vtkRegressionTestImage( renWin );
        }
      usleep( second * 1000000 );
      }
    }

  cone->Delete();
  coneMapper->Delete();
  coneActor->Delete();
  
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();

  cylinder->Delete();
  cylinderMapper->Delete();
  cylinderActor->Delete();

  renderer->Delete();
  renWin->Delete();
  interactor->Delete();

  return !retVal;
}
