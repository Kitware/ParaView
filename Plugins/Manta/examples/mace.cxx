#include "vtkSphereSource.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkAssembly.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCamera.h"

#include <signal.h>

#ifndef alarm
#define alarm(num)
#endif

#ifndef signal
#define signal(alarm, dfl)
#endif

vtkAssembly *assembly;
vtkRenderWindow *win;
static void
sig_alrm(int signo)
{   
    // SIG_DFL (default) / SIG_IGN (ignore) / alarm(0): cancell all alarms
    static  double  rotAngle = 0.0;

    if( win == NULL || assembly == NULL )
    {
       alarm(0);
       signal(SIGALRM, SIG_DFL);
       cerr << endl << "... either vtkRenderWindow or vtkAssembly NULL ...";
       cerr << endl << endl;
       return;
    }

    if( win->CheckInRenderStatus() == 1 || win->GetEventPending() == 1 )
    {
       alarm(0);
       alarm(5);
       cerr << endl << "... rendering session in process ..." << endl << endl;
       return;
    }
    alarm(0);
    cerr << endl << "... rendering session completed (interaction done)" << endl;
    cerr << endl << "... new rendering session (signal handler) ..."     << endl;
    rotAngle += 5.0;
    assembly->RotateZ(rotAngle);
    win->Render();
    cerr << endl << "... rendering session completed (signal responded)" << endl;
    alarm(1);
}

int main()
{   win      = NULL;
    assembly = NULL;
    
    // create a sphere source, mapper, and actor
    vtkSphereSource *sphere = vtkSphereSource::New();
    vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
    sphereMapper->SetInputConnection(sphere->GetOutputPort());
    sphereMapper->GlobalImmediateModeRenderingOn();
    vtkActor *sphereActor = vtkActor::New();
    sphereActor->SetMapper(sphereMapper);
    sphereActor->GetProperty()->SetDiffuseColor(0.8900, 0.8100, 0.3400);
    sphereActor->GetProperty()->SetSpecular(0.4);
    sphereActor->GetProperty()->SetSpecularPower(20);

    // create the spikes by glyphing the sphere with a cone.  Create the
    // mapper and actor for the glyphs.
    vtkConeSource *cone = vtkConeSource::New();
    cone->SetResolution(20);
    vtkGlyph3D *glyph = vtkGlyph3D::New();
    glyph->SetInputConnection(sphere->GetOutputPort());
    glyph->SetSource(cone->GetOutput());
    glyph->SetVectorModeToUseNormal();
    glyph->SetScaleModeToScaleByVector();
    glyph->SetScaleFactor(0.25);

    vtkPolyDataMapper *spikeMapper = vtkPolyDataMapper::New();
    spikeMapper->SetInputConnection(glyph->GetOutputPort());
    vtkActor *spikeActor = vtkActor::New();
    spikeActor->SetMapper(spikeMapper);
    spikeActor->GetProperty()->SetDiffuseColor(1.0000, 0.3882, 0.2784);
    spikeActor->GetProperty()->SetSpecular(0.4);
    spikeActor->GetProperty()->SetSpecularPower(20);

    vtkAssembly *mace = vtkAssembly::New();
    mace->AddPart(sphereActor);
    mace->AddPart(spikeActor);

    // Create the Renderer, RenderWindow, etc. and set the Picker.
    vtkRenderer *ren = vtkRenderer::New();
    vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren);
    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

    ren->AddActor(mace);
    ren->SetBackground(0.1, 0.2, 0.4);
    renWin->SetSize(300,300);

    vtkCamera *camera = vtkCamera::New();
    ren->SetActiveCamera(camera);

    ren->ResetCamera();
    ren->GetActiveCamera()->Zoom(1.4);

    win      = renWin;
    assembly = mace;
    
    cerr << endl << "... RenderWindowInteractor not yet started ";
    cerr << "... but to install an alarm handler first ..." << endl << endl;
    signal(SIGALRM, sig_alrm);
    alarm(1);
    cerr << endl << "... alarm handler installed" << endl << endl;

    // TODO: there is some thread safety issue, both WindowInteractor and
    // sig_alrm calls to renWin->Render which is a bad thing
    // Also, which thread is actually handling the signal?
    iren->Start();

    cerr << endl << "... vtkRender, vtkRenderWindow, and ";
    cerr << "vtkRenderWindowInteractor offline" << endl << endl;
    win      = NULL;
    assembly = NULL;
    alarm(0);
    signal(SIGALRM, SIG_DFL);
    cerr << endl << "... alarm handler uninstalled" << endl << endl;

    sphere->Delete();
    sphereMapper->Delete();
    sphereActor->Delete();
    cone->Delete();
    glyph->Delete();
    spikeMapper->Delete();
    spikeActor->Delete();
    mace->Delete();
    camera->Delete();
    ren->Delete();
    renWin->Delete();
    iren->Delete();
    cerr << endl << "... normal exit ... great!" << endl << endl;
    return  0;
}
