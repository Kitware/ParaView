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

// Test of vtkDistributedDataFilter and supporting classes, covering as much
// code as possible.  This test requires 4 MPI processes.
//
// To cover ghost cell creation, use vtkDataSetSurfaceFilter.
//
// To cover clipping code:  SetBoundaryModeToSplitBoundaryCells()
//
// To run fast redistribution: SetUseMinimalMemoryOff() (Default)
// To run memory conserving code instead: SetUseMinimalMemoryOn()

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkClearZPass.h"
#include "vtkClientServerCompositePass.h"
#include "vtkCompositeRenderManager.h"
#include "vtkDataSetReader.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDepthPeelingPass.h"
#include "vtkDistributedDataFilter.h"
#include "vtkFrameBufferObject.h"
#include "vtkGaussianBlurPass.h"
#include "vtkIceTCompositePass.h"
#include "vtkImageRenderManager.h"
#include "vtkLightsPass.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"
#include "vtkOpaquePass.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOverlayPass.h"
#include "vtkPKdTree.h"
#include "vtkPieceScalars.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkSmartPointer.h"
#include "vtkSobelGradientMagnitudePass.h"
#include "vtkSocketController.h"
#include "vtkSphereSource.h"
#include "vtkSynchronizedRenderWindows.h"
#include "vtkSynchronizedRenderers.h"
#include "vtkTestUtilities.h"
#include "vtkTranslucentPass.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVolumetricPass.h"

/*
** This test only builds if MPI is in use
*/
#include "vtkMPICommunicator.h"
#include <vtk_mpi.h>

#include "vtkProcess.h"

#include <vtksys/CommandLineArguments.hxx>

void ResetCameraClippingRange(
  vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void* vtkNotUsed(calldata))
{
  double* bounds = reinterpret_cast<double*>(clientdata);
  vtkRenderer* ren = vtkRenderer::SafeDownCast(caller);
  ren->ResetCameraClippingRange(bounds);
}

class MyProcess : public vtkProcess
{
  vtkSmartPointer<vtkPKdTree> KdTree;
  bool UseOrderedCompositing;
  bool UseDepthPeeling;
  bool UseBlurPass;
  bool UseSobelPass;
  bool DepthOnly;

public:
  static MyProcess* New();
  vtkTypeMacro(MyProcess, vtkProcess);

  vtkSetVector2Macro(TileDimensions, int);
  vtkGetVector2Macro(TileDimensions, int);

  vtkSetMacro(ImageReductionFactor, int);
  vtkSetMacro(UseOrderedCompositing, bool);
  vtkSetMacro(UseDepthPeeling, bool);
  vtkSetMacro(ServerMode, bool);
  vtkSetMacro(UseBlurPass, bool);
  vtkSetMacro(UseSobelPass, bool);
  vtkSetMacro(DepthOnly, bool);
  vtkSetObjectMacro(SocketController, vtkMultiProcessController);

  void SetArgs(int anArgc, char* anArgv[]);

  virtual void Execute();

private:
  // Creates the visualization pipeline and adds it to the renderer.
  void CreatePipeline(vtkRenderer* renderer);
  // Creates the visualization pipeline and adds it to the renderer on layer
  // 1.
  void CreatePipeline2(vtkRenderer* renderer);

  // Setups render passes.
  void SetupRenderPasses(vtkRenderer* renderer);

protected:
  MyProcess();
  ~MyProcess();

  vtkMultiProcessController* SocketController;
  int Argc;
  char** Argv;

  int ImageReductionFactor;
  int TileDimensions[2];
  bool ServerMode;
};

vtkStandardNewMacro(MyProcess);

//-----------------------------------------------------------------------------
MyProcess::MyProcess()
{
  this->Argc = 0;
  this->Argv = 0;
  this->ImageReductionFactor = 1;
  this->TileDimensions[0] = this->TileDimensions[1];
  this->UseOrderedCompositing = false;
  this->UseDepthPeeling = false;
  this->ServerMode = false;
  this->SocketController = 0;
  this->UseBlurPass = false;
  this->UseSobelPass = false;
  this->DepthOnly = false;
}

//-----------------------------------------------------------------------------
MyProcess::~MyProcess()
{
  this->SetSocketController(0);
}

//-----------------------------------------------------------------------------
void MyProcess::SetArgs(int anArgc, char* anArgv[])
{
  this->Argc = anArgc;
  this->Argv = anArgv;
}

//-----------------------------------------------------------------------------
void MyProcess::CreatePipeline(vtkRenderer* renderer)
{
  int num_procs = this->Controller->GetNumberOfProcesses();
  int my_id = this->Controller->GetLocalProcessId();

  vtkSphereSource* sphere = vtkSphereSource::New();
  sphere->SetPhiResolution(20);
  sphere->SetThetaResolution(20);

  vtkDistributedDataFilter* d3 = vtkDistributedDataFilter::New();
  d3->SetInputConnection(sphere->GetOutputPort());
  d3->SetController(this->Controller);
  d3->SetBoundaryModeToSplitBoundaryCells();
  d3->UseMinimalMemoryOff();

  vtkDataSetSurfaceFilter* surface = vtkDataSetSurfaceFilter::New();
  surface->SetInputConnection(d3->GetOutputPort());

  vtkPieceScalars* piecescalars = vtkPieceScalars::New();
  piecescalars->SetInputConnection(surface->GetOutputPort());
  piecescalars->SetScalarModeToCellData();

  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(piecescalars->GetOutputPort());
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("Piece");
  mapper->SetScalarRange(0, num_procs - 1);
  mapper->SetPiece(my_id);
  mapper->SetNumberOfPieces(num_procs);
  mapper->Update();

  this->KdTree = d3->GetKdtree();

  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetOpacity(this->UseOrderedCompositing ? 0.5 : 1.0);
  actor->GetProperty()->SetInterpolationToFlat();
  actor->GetProperty()->EdgeVisibilityOn();
  renderer->AddActor(actor);

  actor->Delete();
  mapper->Delete();
  piecescalars->Delete();
  surface->Delete();
  d3->Delete();
  sphere->Delete();
}

//-----------------------------------------------------------------------------
void MyProcess::CreatePipeline2(vtkRenderer* renderer)
{
  renderer->SetInteractive(0);

  vtkPlaneSource* plane = vtkPlaneSource::New();
  plane->SetOrigin(-0.5, -0.5, 2);
  plane->SetPoint1(0.5, -0.5, 2);
  plane->SetPoint2(-0.5, 0.5, 2);
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(plane->GetOutputPort());
  mapper->SetScalarModeToUseCellFieldData();

  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  actor->Delete();
  mapper->Delete();
  plane->Delete();

  double bds[] = { -0.25, 0.25, -0.25, 0.25, 2, 2 };
  renderer->ResetCamera(bds);
}

//-----------------------------------------------------------------------------
void MyProcess::SetupRenderPasses(vtkRenderer* renderer)
{
  // the rendering passes
  vtkCameraPass* cameraP = vtkCameraPass::New();
  vtkSequencePass* seq = vtkSequencePass::New();
  vtkOpaquePass* opaque = vtkOpaquePass::New();
  vtkDepthPeelingPass* peeling = vtkDepthPeelingPass::New();
  peeling->SetMaximumNumberOfPeels(200);
  peeling->SetOcclusionRatio(0.1);

  vtkTranslucentPass* translucent = vtkTranslucentPass::New();
  peeling->SetTranslucentPass(translucent);

  vtkVolumetricPass* volume = vtkVolumetricPass::New();
  vtkOverlayPass* overlay = vtkOverlayPass::New();
  vtkLightsPass* lights = vtkLightsPass::New();

  vtkClearZPass* clearZ = vtkClearZPass::New();
  clearZ->SetDepth(0.9);

  vtkRenderPassCollection* passes = vtkRenderPassCollection::New();
  passes->AddItem(lights);
  passes->AddItem(opaque);
  //  passes->AddItem(clearZ);
  if (this->UseDepthPeeling && this->UseOrderedCompositing)
  {
    passes->AddItem(peeling);
  }
  else
  {
    passes->AddItem(translucent);
  }
  passes->AddItem(volume);
  passes->AddItem(overlay);
  seq->SetPasses(passes);

  vtkIceTCompositePass* iceTPass = vtkIceTCompositePass::New();
  iceTPass->SetController(this->Controller);
  iceTPass->SetUseOrderedCompositing(this->UseOrderedCompositing);
  iceTPass->SetKdTree(this->KdTree);
  iceTPass->SetRenderPass(seq);
  iceTPass->SetTileDimensions(this->TileDimensions);
  iceTPass->SetImageReductionFactor(this->ImageReductionFactor);
  iceTPass->SetDepthOnly(this->DepthOnly);
  iceTPass->SetFixBackground(true);

  if (this->ServerMode && this->Controller->GetLocalProcessId() == 0)
  {
    vtkClientServerCompositePass* csPass = vtkClientServerCompositePass::New();
    csPass->SetRenderPass(iceTPass);
    csPass->SetProcessIsServer(true);
    csPass->ServerSideRenderingOn();
    csPass->SetController(this->SocketController);
    cameraP->SetDelegatePass(csPass);
    csPass->Delete();
  }
  else
  {
    cameraP->SetDelegatePass(iceTPass);
  }

  cameraP->SetAspectRatioOverride((double)this->TileDimensions[0] / this->TileDimensions[1]);

  vtkOpenGLRenderer* glrenderer = vtkOpenGLRenderer::SafeDownCast(renderer);
  glrenderer->SetPass(cameraP);

  if (this->UseBlurPass)
  {
    vtkGaussianBlurPass* blurPass = vtkGaussianBlurPass::New();
    blurPass->SetDelegatePass(glrenderer->GetPass());
    glrenderer->SetPass(blurPass);
    blurPass->Delete();
  }

  if (this->UseSobelPass)
  {
    vtkSobelGradientMagnitudePass* sobelPass = vtkSobelGradientMagnitudePass::New();
    sobelPass->SetDelegatePass(glrenderer->GetPass());
    glrenderer->SetPass(sobelPass);
    sobelPass->Delete();
  }

  // setting viewport doesn't work in tile-display mode correctly yet.
  // renderer->SetViewport(0, 0, 0.75, 1);

  iceTPass->Delete();
  opaque->Delete();
  peeling->Delete();
  translucent->Delete();
  volume->Delete();
  overlay->Delete();
  seq->Delete();
  passes->Delete();
  cameraP->Delete();
  lights->Delete();
  clearZ->Delete();
}

//-----------------------------------------------------------------------------
void MyProcess::Execute()
{
  // test required extensions by creating a dummy rendering context.
  vtkSmartPointer<vtkRenderWindow> temp = vtkSmartPointer<vtkRenderWindow>::New();
  if (!vtkFrameBufferObject::IsSupported(temp))
  {
    vtkWarningMacro("Rendering context doesn't support required extensions.\n"
                    "Skipping test.");
    this->ReturnValue = vtkTesting::PASSED;
    return;
  }
  temp = NULL;

  int myId = this->Controller->GetLocalProcessId();

  vtkRenderWindow* renWin = vtkRenderWindow::New();

  int y = myId / this->TileDimensions[0];
  int x = myId % this->TileDimensions[0];

  const int width = 400;
  const int height = 400;

  // 400x400 windows, +24 in y to avoid gnome menu bar.
  renWin->SetPosition(x * (width + 6), y * (height + 20) + 24);
  renWin->SetSize(width, height);

  // enable alpha bit-planes.
  renWin->AlphaBitPlanesOn();

  // use double bufferring.
  renWin->DoubleBufferOn();

  // don't waste time swapping buffers unless needed.
  renWin->SwapBuffersOff();

  // Depth-peeling requires that multi-samples ==0.
  renWin->SetMultiSamples(0);

  vtkRenderer* renderer = vtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.66, 0.66, 0.66);
  renderer->SetBackground2(157.0 / 255.0 * 0.66, 186 / 255.0 * 0.66, 192.0 / 255.0 * 0.66);
  renderer->SetGradientBackground(true);
  vtkCallbackCommand* observer = vtkCallbackCommand::New();
  observer->SetCallback(&ResetCameraClippingRange);
  double bounds[6] = { -0.5, 0.5, -0.5, 0.5, -0.5, 0.5 };
  observer->SetClientData(&bounds);
  renderer->AddObserver(vtkCommand::ResetCameraClippingRangeEvent, observer);
  observer->Delete();

  vtkSynchronizedRenderers* syncRenderers2 = 0;
  if (this->DepthOnly)
  {
    vtkRenderer* renderer2 = vtkRenderer::New();
    renderer2->SetLayer(1);
    renderer2->SetPreserveDepthBuffer(true);

    renWin->AddRenderer(renderer2);
    renWin->SetNumberOfLayers(2);
    renderer2->Delete();
    this->CreatePipeline2(renderer2);
  }

  vtkSynchronizedRenderWindows* syncWindows = vtkSynchronizedRenderWindows::New();
  syncWindows->SetRenderWindow(renWin);
  syncWindows->SetParallelController(this->Controller);
  syncWindows->SetIdentifier(1);

  vtkSynchronizedRenderers* syncRenderers = vtkSynchronizedRenderers::New();
  syncRenderers->SetRenderer(renderer);
  syncRenderers->SetParallelController(this->Controller);

  this->CreatePipeline(renderer);
  this->SetupRenderPasses(renderer);

  int retVal = vtkTesting::FAILED;
  if (myId == 0)
  {
    // root node
    vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);
    renWin->SwapBuffersOn();

    if (this->ServerMode)
    {
      vtkSynchronizedRenderWindows* syncWindows2 = vtkSynchronizedRenderWindows::New();
      syncWindows2->SetRenderWindow(renWin);
      syncWindows2->SetParallelController(this->SocketController);
      syncWindows2->SetIdentifier(2);
      syncWindows2->SetRootProcessId(1);

      syncRenderers2->SetRenderer(renderer);
      syncRenderers2->SetParallelController(this->SocketController);
      syncRenderers2->SetRootProcessId(1);

      this->SocketController->ProcessRMIs();
      retVal = vtkTesting::PASSED;
    }
    else
    {
      if (!this->DepthOnly)
      {
        renderer->GetActiveCamera()->SetPosition(-0.78, 0.68, -0.20);
        renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
        renderer->GetActiveCamera()->SetViewUp(-0.3, -0.57, -0.75);
      }
      else
      {
        renderer->ResetCamera(bounds);
      }
      renWin->Render();
      retVal = vtkTesting::Test(this->Argc, this->Argv, renWin, 10);
      if (retVal == vtkRegressionTester::DO_INTERACTOR)
      {
        iren->Start();
      }
    }
    iren->Delete();

    this->Controller->TriggerBreakRMIs();
    this->Controller->Barrier();
  }
  else
  {
    // satellite nodes
    if (this->TileDimensions[0] > 1 || this->TileDimensions[1] > 1)
    {
      renWin->SwapBuffersOn();
    }
    this->Controller->ProcessRMIs();
    this->Controller->Barrier();
    retVal = vtkTesting::PASSED;
  }

  renderer->Delete();
  renWin->Delete();
  syncWindows->Delete();
  syncRenderers->Delete();
  if (syncRenderers2 != 0)
  {
    syncRenderers2->Delete();
  }

  this->ReturnValue = retVal;
}

//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
  int retVal = 1;

  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);
  vtkSmartPointer<vtkMPIController> contr = vtkSmartPointer<vtkMPIController>::New();
  contr->Initialize(&argc, &argv, 1);

  int tile_dimensions[2] = { 1, 1 };
  int image_reduction_factor = 1;
  int use_ordered_compositing = 0;
  int use_depth_peeling = 0;
  int act_as_server = 0;
  int add_blur_pass = 0;
  int add_sobel_pass = 0;
  int depthOnly = 0;
  int interactive = 0;
  std::string data;
  std::string temp;
  std::string baseline;

  vtksys::CommandLineArguments args;
  args.Initialize(argc, argv);
  args.StoreUnusedArguments(false);
  args.AddArgument("-D", vtksys::CommandLineArguments::SPACE_ARGUMENT, &data, "");
  args.AddArgument("-T", vtksys::CommandLineArguments::SPACE_ARGUMENT, &temp, "");
  args.AddArgument("-V", vtksys::CommandLineArguments::SPACE_ARGUMENT, &baseline, "");
  args.AddArgument("-I", vtksys::CommandLineArguments::NO_ARGUMENT, &interactive, "");
  args.AddArgument("--tdx", vtksys::CommandLineArguments::SPACE_ARGUMENT, &tile_dimensions[0], "");
  args.AddArgument("--tdy", vtksys::CommandLineArguments::SPACE_ARGUMENT, &tile_dimensions[1], "");
  args.AddArgument("--image-reduction-factor", vtksys::CommandLineArguments::SPACE_ARGUMENT,
    &image_reduction_factor, "Image reduction factor");
  args.AddArgument("--irf", vtksys::CommandLineArguments::SPACE_ARGUMENT, &image_reduction_factor,
    "Image reduction factor");
  args.AddArgument("--use-ordered-compositing", vtksys::CommandLineArguments::NO_ARGUMENT,
    &use_ordered_compositing, "Use ordered compositing");
  args.AddArgument("--use-depth-peeling", vtksys::CommandLineArguments::NO_ARGUMENT,
    &use_depth_peeling, "Use depth peeling."
                        "This works only when --use-ordered-compositing is true.");
  args.AddArgument("--depth-only", vtksys::CommandLineArguments::NO_ARGUMENT, &depthOnly,
    "pass depth buffer only.");
  args.AddArgument("--server", vtksys::CommandLineArguments::NO_ARGUMENT, &act_as_server,
    "When present, the root process acts as a server process for a client.");
  args.AddArgument("--blur", vtksys::CommandLineArguments::NO_ARGUMENT, &add_blur_pass,
    "When present, a vtkGaussianBlurPass will be added.");
  args.AddArgument("--sobel", vtksys::CommandLineArguments::NO_ARGUMENT, &add_sobel_pass,
    "When present, a vtkGaussianBlurPass will be added.");

  if (!args.Parse())
  {
    if (contr->GetLocalProcessId() == 0)
    {
      cout << args.GetHelp() << endl;
    }
    contr->Finalize();
    return 1;
  }

  tile_dimensions[0] = tile_dimensions[0] < 1 ? 1 : tile_dimensions[0];
  tile_dimensions[1] = tile_dimensions[1] < 1 ? 1 : tile_dimensions[1];

  int numProcs = contr->GetNumberOfProcesses();
  if (tile_dimensions[0] > 1 || tile_dimensions[1] > 1)
  {
    if (numProcs != tile_dimensions[0] * tile_dimensions[1])
    {
      cerr << "When running in tile-display mode, number of processes must "
              "match number of tiles"
           << endl;
      return 1;
    }
    cout << "Rendering as a " << tile_dimensions[0] << "x" << tile_dimensions[1] << " tile display"
         << endl;
  }

  vtkMultiProcessController::SetGlobalController(contr);

  MyProcess* p = MyProcess::New();
  p->SetArgs(argc, argv);
  p->SetTileDimensions(tile_dimensions);
  p->SetImageReductionFactor(image_reduction_factor);
  p->SetUseOrderedCompositing(use_ordered_compositing == 1);
  p->SetUseDepthPeeling(use_depth_peeling == 1);
  p->SetServerMode(act_as_server != 0);
  p->SetUseBlurPass(add_blur_pass != 0);
  p->SetUseSobelPass(add_sobel_pass != 0);
  p->SetDepthOnly(depthOnly == 1);

  if (contr->GetLocalProcessId() == 0 && act_as_server)
  {
    vtkSmartPointer<vtkSocketController> socket_contr = vtkSmartPointer<vtkSocketController>::New();
    socket_contr->Initialize(&argc, &argv);
    cout << "Waiting for client on 11111" << endl;
    socket_contr->WaitForConnection(11111);
    p->SetSocketController(socket_contr);
  }

  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal = p->GetReturnValue();

  vtkMultiProcessController::SetGlobalController(0);
  p->Delete();
  contr->Finalize();
  contr = 0;
  return !retVal;
}
