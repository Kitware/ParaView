#include "vtkBMPReader.h"
#include "vtkTexture.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkToolkits.h"

#include <string>

int main()
{
    // Load in the texture map. A texture is any unsigned char image. If it
    // is not of this type, you will have to map it through a lookup table
    // or by using vtkImageShiftScale.
    string filename = VTK_DATA_ROOT;
    filename += "/Data/masonry.bmp";
    vtkBMPReader *bmpReader = vtkBMPReader::New();
    bmpReader->SetFileName(filename.c_str());
    vtkTexture *atext = vtkTexture::New();
    atext->SetInputConnection(bmpReader->GetOutputPort());
    atext->InterpolateOn();

    // Create a plane source and actor. The vtkPlanesSource generates
    // texture coordinates.
    vtkPlaneSource *plane = vtkPlaneSource::New();
    vtkPolyDataMapper *planeMapper = vtkPolyDataMapper::New();
    planeMapper->SetInputConnection(plane->GetOutputPort());
    vtkActor *planeActor = vtkActor::New();
    planeActor->SetMapper(planeMapper);
    planeActor->SetTexture(atext);

    // Create the RenderWindow, Renderer and both Actors
    vtkRenderer *ren = vtkRenderer::New();
    vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren);
    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

    // Add the actors to the renderer, set the background and size
    ren->AddActor(planeActor);
    ren->SetBackground(0.1, 0.2, 0.4);
    renWin->SetSize(500, 500);
    
    ren->ResetCamera();
    vtkCamera *camera = ren->GetActiveCamera();
    camera->Elevation(-30);
    camera->Roll(-20);
    ren->ResetCameraClippingRange();
    
    iren->Initialize();
    renWin->Render();
    iren->Start();
}
