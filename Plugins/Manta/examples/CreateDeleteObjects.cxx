#include "vtkConeSource.h"
#include "vtkSphereSource.h"
#include "vtkCylinderSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"

// this program tests creating and deleting objects

//----------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
  int     second = 1;
  int     objRes = 12;
  double  objRad = 0.075;
  
  vtkRenderer * renderer = vtkRenderer::New();
  renderer->SetBackground( 0.0, 0.0, 1.0 );

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
    renWin->Render();
    usleep( second * 1000000 );

    // create cone
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

    renderer->AddActor( coneActor );
    renWin->Render();
    usleep( second * 1000000 );
  
    // create sphere
    vtkSphereSource * sphere = vtkSphereSource::New();
    sphere->SetCenter( 0.0, 0.0, 0.0 );
    sphere->SetRadius( objRad );
    sphere->SetThetaResolution( objRes );
    sphere->SetPhiResolution  ( objRes );

    vtkPolyDataMapper * sphereMapper = vtkPolyDataMapper::New();
    sphereMapper->SetInputConnection( sphere->GetOutputPort() );
  
    vtkActor * sphereActor = vtkActor::New();
    sphereActor->SetMapper( sphereMapper );

    renderer->AddActor( sphereActor );
    renWin->Render();
    usleep( second * 1000000 );

    // create cylinder
    vtkCylinderSource * cylinder = vtkCylinderSource::New();
    cylinder->SetCenter( 0.0, -objRad * 2, 0.0 );
    cylinder->SetRadius( objRad );
    cylinder->SetHeight( objRad * 2 );
    cylinder->SetResolution( objRes );

    vtkPolyDataMapper * cylinderMapper = vtkPolyDataMapper::New();
    cylinderMapper->SetInputConnection( cylinder->GetOutputPort() );
  
    vtkActor * cylinderActor = vtkActor::New();
    cylinderActor->SetMapper( cylinderMapper );

    renderer->AddActor( cylinderActor );
    renWin->Render();
    usleep( second * 1000000 );

    retVal = vtkRegressionTestImage( renWin );

    // delete cone
    renderer->RemoveActor( coneActor );
    cone->Delete();
    coneMapper->Delete();
    coneActor->Delete();
  
    renWin->Render();
    usleep( second * 1000000 );

    // delete sphere
    renderer->RemoveActor( sphereActor );
    sphere->Delete();
    sphereMapper->Delete();
    sphereActor->Delete();

    renWin->Render();
    usleep( second * 1000000 );

    // delete cylinder
    renderer->RemoveActor( cylinderActor );
    cylinder->Delete();
    cylinderMapper->Delete();
    cylinderActor->Delete();

    renWin->Render();
    usleep( second * 1000000 );
    }

  renderer->Delete();
  renWin->Delete();
  interactor->Delete();

  return !retVal;
}
