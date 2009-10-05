#include "vtkIdList.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"

#include "vtkPolyData.h"
#include "vtkAppendPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkMantaRenderer.h"

#include "vtkRegressionTestImage.h"

// this program tests
//
// * rendering of hybrid vertices+lines vtkpolyData objects
//
// * image compositing between multiple renderers on one or two layers
//   and the following #define _MULTI_RENDERS_ needs to be turned ON

//#define _RECORD_EVENTS_ // ON only for initial recording and OFF for formal testing
//#define _VTKMANTA_MODE_ // OFF for pure OpenGL rendering
#define _MULTI_RENDERS_

double blue[]   = { 0.0000, 0.0000, 0.8000 };
double black[]  = { 0.0000, 0.0000, 0.0000 };
double banana[] = { 0.8900, 0.8100, 0.3400 };
double tomato[] = { 1.0000, 0.3882, 0.2784 };

//----------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
  int  invalidCmd = 0;

  if ( argc < 2 )
    {
    invalidCmd = 1;
    }

# ifndef _RECORD_EVENTS_
  if ( strstr( argv[1], "multiRensEvents.log" ) )
    {
    FILE * file = NULL;
    file = fopen( argv[1], "r" );
    if ( file == NULL )
      {
      invalidCmd = 1;
      }
    fclose( file );
    }
# endif

  if ( invalidCmd )
    {
    cerr << "multiRens ${vtkManta_source_directory}"
         << "/examples/multiRensEvents.log [-I]" << endl;
    return 1;
    }

  int  i;

  // the smaller cube (surface)
  static double    cord1[8][3] = 
    { 
      { -0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f, -0.5f }, 
      {  0.5f,  0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f },
      { -0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f,  0.5f }, 
      {  0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f }
    };

  static vtkIdType quads[6][4] = 
    { 
      { 0, 1, 2, 3 }, { 4, 5, 6, 7 }, 
      { 0, 1, 5, 4 }, { 1, 2, 6, 5 }, 
      { 2, 3, 7, 6 }, { 3, 0, 4, 7 }
    };

  vtkPoints*     pnts = vtkPoints::New();
  vtkPolyData*   cube = vtkPolyData::New();
  vtkCellArray*  surf = vtkCellArray::New();
  vtkFloatArray* vals = vtkFloatArray::New();

  for ( i = 0; i < 8; i ++ ) pnts->InsertPoint( i, cord1[i] );
  for ( i = 0; i < 6; i ++ ) surf->InsertNextCell( 4, quads[i] );
  for ( i = 0; i < 8; i ++ ) vals->InsertTuple1( i, i );
    
  cube->SetPoints( pnts );
  pnts->Delete();
  cube->SetPolys( surf );
  surf->Delete();
  cube->GetPointData()->SetActiveAttribute( 
    cube->GetPointData()->SetScalars( vals ), vtkDataSetAttributes::SCALARS );
  vals->Delete();

  // the bigger cube (edges / lines)
  static double cord2[8][3] = 
    { 
      { -1.0f, -1.0f, -1.0f }, {  1.0f, -1.0f, -1.0f }, 
      {  1.0f,  1.0f, -1.0f }, { -1.0f,  1.0f, -1.0f },
      { -1.0f, -1.0f,  1.0f }, {  1.0f, -1.0f,  1.0f }, 
      {  1.0f,  1.0f,  1.0f }, { -1.0f,  1.0f,  1.0f }
    };

  vtkPoints*    p_pnt = vtkPoints::New();
  vtkIdList*    edges = vtkIdList::New();
  vtkPolyData*  lines = vtkPolyData::New();
  vtkCellArray* line2 = vtkCellArray::New();
    
  for ( i = 0; i < 4; i ++ )
    {
    p_pnt->InsertPoint( i, cord2[i] );
    edges->InsertNextId( i );
    }
  edges->InsertNextId ( 0 );
  line2->InsertNextCell( edges );
  edges->Reset();

  for ( i = 4; i < 8; i ++ )
    {
    p_pnt->InsertPoint( i, cord2[i] );
    edges->InsertNextId( i );
    }
  edges->InsertNextId ( 4 );
  line2->InsertNextCell( edges );
  edges->Reset();

  edges->InsertNextId( 0 );
  edges->InsertNextId( 4 );
  line2->InsertNextCell( edges );
  edges->Reset();
  edges->InsertNextId( 5 );
  edges->InsertNextId( 1 );
  line2->InsertNextCell( edges );
  edges->Reset();

  edges->InsertNextId( 2 );
  edges->InsertNextId( 6 );
  line2->InsertNextCell( edges );
  edges->Reset();
  edges->InsertNextId( 3 );
  edges->InsertNextId( 7 );
  line2->InsertNextCell( edges );

  lines->SetPoints( p_pnt );
  p_pnt->Delete();
  lines->SetLines( line2 );
  line2->Delete();
  edges->Delete();
    
  // eight vertices outside the smaller cube
  double    ptCrd[8][3] = 
    { 
      { -0.75f, -0.75f, -0.75f }, 
      {  0.75f, -0.75f, -0.75f }, 
      {  0.75f,  0.75f, -0.75f }, 
      { -0.75f,  0.75f, -0.75f }, 
      { -0.75f, -0.75f,  0.75f }, 
      {  0.75f, -0.75f,  0.75f }, 
      {  0.75f,  0.75f,  0.75f }, 
      { -0.75f,  0.75f,  0.75f }
    };

  vtkIdType pt_id[8][1] = 
    { 
      { 0 }, { 1 }, { 2 }, { 3 }, 
      { 4 }, { 5 }, { 6 }, { 7 } 
    };

  vtkPoints*    ponts = vtkPoints::New();
  vtkCellArray* verts = vtkCellArray::New();
  for ( i = 0;  i < 8;  i ++ )  ponts->InsertPoint   ( i, ptCrd[i] );
  for ( i = 0;  i < 8;  i ++ )  verts->InsertNextCell( 1, pt_id[i] );
  vtkPolyData*  vPoly = vtkPolyData::New();
  vPoly->SetPoints( ponts );  ponts->Delete();
  vPoly->SetVerts ( verts );  verts->Delete();

  // hybrid vertices+lines polydata
  vtkAppendPolyData* apend = vtkAppendPolyData::New();
  apend->AddInput( vPoly );
  apend->AddInput( lines );

  // hybrid mapper and actor
  vtkPolyDataMapper* hybridMapper = vtkPolyDataMapper::New();
  hybridMapper->SetInputConnection( apend->GetOutputPort() );
  hybridMapper->ScalarVisibilityOff();
  vtkActor* hybridActor = vtkActor::New();
  hybridActor->SetMapper( hybridMapper );
  hybridActor->GetProperty()->SetDiffuseColor( tomato );
  hybridActor->GetProperty()->SetSpecular( 0.4 );
  hybridActor->GetProperty()->SetSpecularPower( 10 );
  hybridActor->GetProperty()->SetLineWidth( 16.0 );
  hybridActor->GetProperty()->SetPointSize( 32.0 );

  // cube mapper and actor
  vtkPolyDataMapper* cubeMapper = vtkPolyDataMapper::New();
  cubeMapper->SetInput( cube );
  cubeMapper->ScalarVisibilityOff();
  vtkActor* cubeActor = vtkActor::New();
  cubeActor->SetMapper( cubeMapper );
  cubeActor->GetProperty()->SetDiffuseColor( banana );
  cubeActor->GetProperty()->SetSpecular( 0.4 );
  cubeActor->GetProperty()->SetSpecularPower( 10 );
  cubeActor->GetProperty()->SetOpacity( 1.0 );

  // add a light for shadows
  vtkLight*    light   = vtkLight::New();
  light->SetLightTypeToCameraLight();
  vtkCamera*   camera  = vtkCamera::New();
  vtkRenderer* cubeRen = vtkRenderer::New();

# ifdef _MULTI_RENDERS_

  // renderer #0
  vtkRenderer* hybridRen = vtkRenderer::New();
  hybridRen->SetViewport( 0.0, 0.0, 1.0, 1.0 );
  hybridRen->SetLayer( 0);
  hybridRen->SetBackground( blue );
  hybridRen->AddActor( hybridActor );
    
  // renderer #1 --- to emulate an annotation layer
  cubeRen->SetViewport( 0.0, 0.0, 0.6, 1.0 );
  cubeRen->SetLayer( 1 );
  cubeRen->InteractiveOff();
  cubeRen->SetBackground( black );
  cubeRen->AddActor( cubeActor );

# else // _MULTI_RENDERS_

  cubeRen->SetBackground( black );
  cubeRen->AddActor( cubeActor );
  cubeRen->AddActor( hybridActor );

# endif // _MULTI_RENDERS_

  cubeRen->SetLightFollowCamera( 1 );
  cubeRen->AddLight( light );
  cubeRen->SetActiveCamera( camera );
    
  cubeRen->ResetCameraClippingRange();
  cubeRen->GetActiveCamera()->Dolly( 1.2 );
  cubeRen->GetActiveCamera()->Azimuth( 30 );
  cubeRen->GetActiveCamera()->Elevation( 20 );
  cubeRen->ResetCamera();

  vtkRenderWindow* renWin = vtkRenderWindow::New();

# ifdef _MULTI_RENDERS_
  renWin->SetNumberOfLayers( 2 );
  renWin->AddRenderer( hybridRen );
# endif

  renWin->AddRenderer( cubeRen );
  renWin->SetSize( 800, 400 );

  vtkRenderWindowInteractor* interactor = vtkRenderWindowInteractor::New();
  interactor->SetRenderWindow( renWin );

  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR )
    {
    interactor->Start();
    }
  else
    {
    vtkInteractorEventRecorder * recorder 
      = vtkInteractorEventRecorder::New();
    recorder->SetInteractor( interactor );
    recorder->SetFileName( argv[1] );

    #ifdef _RECORD_EVENTS_
    recorder->SetEnabled( true );
    recorder->Record();
    renWin->Render();
    interactor->Start();
    recorder->SetEnabled( false );
    #else
    renWin->Render();
    recorder->Play();
    retVal = vtkRegressionTestImage( renWin );
    recorder->Off();
    #endif
  
    recorder->Delete();
    }

  // memory deallocation
  cube->Delete();
  lines->Delete();
  vPoly->Delete();
  apend->Delete();

  hybridMapper->Delete();
  hybridActor->Delete();
  cubeMapper->Delete();
  cubeActor->Delete();

  light->Delete();
  camera->Delete();

  // These following two lines are MUST for MANTA-VTK. otherwise 
  // there would be segfaults even no any user interaction is performed. 
  //
  // However, they MUST be turned OFF if the program runs in pure
  // VTK mode, otherwise there would be a severe crash.
  //
  // In other words, if _VTKMANTA_MODE_ and _MULTI_RENDERS_ are both ON in
  // pure OpenGL mode (actually the former should NOT be on in this mode),
  // a crash will occur.
# if defined( _VTKMANTA_MODE_ ) && defined( _MULTI_RENDERS_ )
  renWin->RemoveRenderer( cubeRen );
  renWin->RemoveRenderer( hybridRen );
# endif

  cubeRen->Delete();

# ifdef _MULTI_RENDERS_
  hybridRen->Delete();
# endif

  renWin->Delete();
  interactor->Delete();

  return !retVal;
}
