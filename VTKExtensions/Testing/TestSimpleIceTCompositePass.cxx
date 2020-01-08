// A simple example that demonstrate how to use the vtkIceTCompositePass and
// supporting classes to render a sphere in parallel.
// This only uses the minimal set of functionality and hence does not support
// opacity < 1.0. Refer to TestIceTCompositePass.cxx for a more exhaustive
// example.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkIceTCompositePass.h"
#include "vtkLightsPass.h"
#include "vtkMPIController.h"
#include "vtkOpaquePass.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPSphereSource.h"
#include "vtkPieceScalars.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkSmartPointer.h"
#include "vtkSynchronizedRenderWindows.h"
#include "vtkSynchronizedRenderers.h"
#include "vtkTesting.h"

#include "vtk_mpi.h"

int main(int argc, char** argv)
{
  //---------------------------------------------------------------------------
  // Initialize MPI.

  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  vtkSmartPointer<vtkMPIController> controller = vtkSmartPointer<vtkMPIController>::New();
  controller->Initialize(&argc, &argv, 1);

  // Get information about the group of processes involved.
  int my_id = controller->GetLocalProcessId();
  int num_procs = controller->GetNumberOfProcesses();
  int retVal = vtkTesting::PASSED;

  // This block ensures that controller is released by all filters before we
  // reach the end to avoid leaks
  if (true)
  {
    //---------------------------------------------------------------------------
    // Create Visualization Pipeline.
    // This code is common to all processes.
    vtkSmartPointer<vtkPSphereSource> sphere = vtkSmartPointer<vtkPSphereSource>::New();
    sphere->SetThetaResolution(50);
    sphere->SetPhiResolution(50);

    // Gives separate colors for each process. Just makes it easier to see how the
    // data is distributed among processes.
    vtkSmartPointer<vtkPieceScalars> piecescalars = vtkSmartPointer<vtkPieceScalars>::New();
    piecescalars->SetInputConnection(sphere->GetOutputPort());
    piecescalars->SetScalarModeToCellData();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(piecescalars->GetOutputPort());
    mapper->SetScalarModeToUseCellFieldData();
    mapper->SelectColorArray("Piece");
    mapper->SetScalarRange(0, num_procs - 1);
    // This sets up the piece-request. This tells vtkPSphereSource to only
    // generate part of the data on this processes.
    mapper->SetPiece(my_id);
    mapper->SetNumberOfPieces(num_procs);

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor(actor);

    vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
    renWin->AddRenderer(renderer);

    // 400x400 windows, +24 in y to avoid gnome menu bar.
    renWin->SetPosition(my_id * 410, 0);
    renWin->SetSize(400, 400);

    //---------------------------------------------------------------------------
    // Setup the render passes. This is just a very small subset of necessary
    // render passes needed to render a opaque sphere.
    vtkSmartPointer<vtkCameraPass> cameraP = vtkSmartPointer<vtkCameraPass>::New();
    vtkSmartPointer<vtkSequencePass> seq = vtkSmartPointer<vtkSequencePass>::New();
    vtkSmartPointer<vtkOpaquePass> opaque = vtkSmartPointer<vtkOpaquePass>::New();
    vtkSmartPointer<vtkLightsPass> lights = vtkSmartPointer<vtkLightsPass>::New();
    vtkSmartPointer<vtkRenderPassCollection> passes =
      vtkSmartPointer<vtkRenderPassCollection>::New();
    passes->AddItem(lights);
    passes->AddItem(opaque);
    seq->SetPasses(passes);

    // Each processes only has part of the data, so each process will render only
    // part of the data. To ensure that root node gets a composited result (or in
    // case of tile-display mode all nodes show part of tile), we use
    // vtkIceTCompositePass.
    vtkSmartPointer<vtkIceTCompositePass> iceTPass = vtkSmartPointer<vtkIceTCompositePass>::New();
    iceTPass->SetController(controller);

    // this is the pass IceT is going to use to render the geometry.
    iceTPass->SetRenderPass(seq);

    // insert the iceT pass into the pipeline.
    cameraP->SetDelegatePass(iceTPass);

    vtkOpenGLRenderer* glrenderer = vtkOpenGLRenderer::SafeDownCast(renderer);
    glrenderer->SetPass(cameraP);

    //---------------------------------------------------------------------------
    // In parallel configurations, typically one node acts as the driver i.e. the
    // node where the user interacts with the window e.g. mouse interactions,
    // resizing windows etc. Typically that's the root-node.
    // To ensure that the window parameters get propagated to all processes from
    // the root node, we use the vtkSynchronizedRenderWindows.
    vtkSmartPointer<vtkSynchronizedRenderWindows> syncWindows =
      vtkSmartPointer<vtkSynchronizedRenderWindows>::New();
    syncWindows->SetRenderWindow(renWin);
    syncWindows->SetParallelController(controller);

    // Since there could be multiple render windows that could be synced
    // separately, to identify the windows uniquely among all processes, we need
    // to give each vtkSynchronizedRenderWindows a unique id that's consistent
    // across all the processes.
    syncWindows->SetIdentifier(231);

    // Now we need to ensure that the render is synchronized as well. This is
    // essential to ensure all processes have the same camera orientation etc.
    // This is done using the vtkSynchronizedRenderers class.
    vtkSmartPointer<vtkSynchronizedRenderers> syncRenderers =
      vtkSmartPointer<vtkSynchronizedRenderers>::New();
    syncRenderers->SetRenderer(renderer);
    syncRenderers->SetParallelController(controller);

    //---------------------------------------------------------------------------
    // Now start the event loop on the root node, on the satellites, we start the
    // vtkMultiProcessController::ProcessRMIs() so those processes start listening
    // to commands from the root-node.

    if (my_id == 0)
    {
      vtkSmartPointer<vtkRenderWindowInteractor> iren =
        vtkSmartPointer<vtkRenderWindowInteractor>::New();
      iren->SetRenderWindow(renWin);
      renderer->ResetCamera(-0.5, 0.5, -0.5, 0.5, -0.5, 0.5);

      renWin->Render();
      retVal = vtkTesting::Test(argc, argv, renWin, 10);
      if (retVal == vtkRegressionTester::DO_INTERACTOR)
      {
        iren->Start();
      }

      controller->TriggerBreakRMIs();
      controller->Barrier();
    }
    else
    {
      controller->ProcessRMIs();
      controller->Barrier();
    }
  }
  controller->Finalize();
  return !retVal;
}
