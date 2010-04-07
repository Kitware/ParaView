#include "vtkConeSource.h"
#include "vtkSphereSource.h"
#include "vtkCylinderSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"

#ifndef usleep
#define usleep(time)
#endif


// this program tests creating odd-width images

//----------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
  int     objRes = 12;
  double  objRad = 0.075;

  // cone
  vtkConeSource * cone= vtkConeSource::New();
  cone->SetRadius( objRad );
  cone->SetHeight( objRad * 2 );
  cone->SetResolution( objRes );

  vtkPolyDataMapper * coneMapper = vtkPolyDataMapper::New();
  coneMapper->SetInputConnection( cone->GetOutputPort() );
  
  vtkActor * coneActor = vtkActor::New();
  coneActor->SetMapper( coneMapper );
  coneActor->AddPosition( -objRad * 3.0, 0.0, 0.0 );
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
  cylinder->SetCenter( objRad * 3, 0.0, 0.0 );
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
    for ( int i = 0; i < 6; i ++ )
      {
      renWin->SetSize( 400 + 51 * i, 400 );
      renWin->Render();
      if ( i == 5 ) 
        {
        retVal = vtkRegressionTestImage( renWin );
        }
      usleep( 1000000 );
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
