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
#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkClearZPass.h"
#include "vtkClientServerCompositePass.h"
#include "vtkCompositeRenderManager.h"
#include "vtkCompositeZPass.h"
#include "vtkConeSource.h"
#include "vtkCubeSource.h"
#include "vtkDataSetReader.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDepthPeelingPass.h"
#include "vtkDistributedDataFilter.h"
#include "vtkFrameBufferObject.h"
#include "vtkIceTCompositePass.h"
#include "vtkImageRenderManager.h"
#include "vtkInformation.h"
#include "vtkLight.h"
#include "vtkLightActor.h"
#include "vtkLightCollection.h"
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
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkShadowMapBakerPass.h"
#include "vtkShadowMapPass.h"
#include "vtkSmartPointer.h"
#include "vtkSocketController.h"
#include "vtkSphereSource.h"
#include "vtkSynchronizedRenderWindows.h"
#include "vtkSynchronizedRenderers.h"
#include "vtkTestUtilities.h"
#include "vtkTranslucentPass.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVolumetricPass.h"

#include <cassert>

/*
** This test only builds if MPI is in use
*/
#include "vtkMPICommunicator.h"

#include "vtkProcess.h"
#include "vtk_mpi.h"

#include <vtksys/CommandLineArguments.hxx>

// Defined in TestLightActor.cxx
// For each spotlight, add a light frustum wireframe representation and a cone
// wireframe representation, colored with the light color.
void AddLightActors(vtkRenderer* r);

class MyProcess : public vtkProcess
{
  vtkSmartPointer<vtkPKdTree> KdTree;
  bool UseOrderedCompositing;
  bool UseDepthPeeling;

public:
  static MyProcess* New();
  vtkTypeMacro(MyProcess, vtkProcess);

  vtkSetVector2Macro(TileDimensions, int);
  vtkGetVector2Macro(TileDimensions, int);

  vtkSetMacro(ImageReductionFactor, int);
  vtkSetMacro(UseOrderedCompositing, bool);
  vtkSetMacro(UseDepthPeeling, bool);
  vtkSetMacro(ServerMode, bool);
  vtkSetObjectMacro(SocketController, vtkMultiProcessController);

  void SetArgs(int anArgc, char* anArgv[]);

  virtual void Execute();

private:
  // Creates the visualization pipeline and adds it to the renderer.
  void CreatePipeline(vtkRenderer* renderer);

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
  int numProcs = this->Controller->GetNumberOfProcesses();
  int me = this->Controller->GetLocalProcessId();

  vtkPlaneSource* rectangleSource = vtkPlaneSource::New();
  rectangleSource->SetOrigin(-5.0, 0.0, 5.0);
  rectangleSource->SetPoint1(5.0, 0.0, 5.0);
  rectangleSource->SetPoint2(-5.0, 0.0, -5.0);
  rectangleSource->SetResolution(100, 100);

  vtkPolyDataMapper* rectangleMapper = vtkPolyDataMapper::New();
  rectangleMapper->SetInputConnection(rectangleSource->GetOutputPort());
  rectangleSource->Delete();
  rectangleMapper->SetScalarVisibility(0);

  vtkActor* rectangleActor = vtkActor::New();
  vtkInformation* rectangleKeyProperties = vtkInformation::New();
  rectangleKeyProperties->Set(vtkShadowMapBakerPass::OCCLUDER(), 0); // dummy val.
  rectangleKeyProperties->Set(vtkShadowMapBakerPass::RECEIVER(), 0); // dummy val.
  rectangleActor->SetPropertyKeys(rectangleKeyProperties);
  rectangleKeyProperties->Delete();
  rectangleActor->SetMapper(rectangleMapper);
  rectangleMapper->Delete();
  rectangleActor->SetVisibility(1);
  rectangleActor->GetProperty()->SetColor(1.0, 1.0, 1.0);

  vtkCubeSource* boxSource = vtkCubeSource::New();
  boxSource->SetXLength(2.0);
  vtkPolyDataNormals* boxNormals = vtkPolyDataNormals::New();
  boxNormals->SetInputConnection(boxSource->GetOutputPort());
  boxNormals->SetComputePointNormals(0);
  boxNormals->SetComputeCellNormals(1);
  boxNormals->Update();
  boxNormals->GetOutput()->GetPointData()->SetNormals(0);

  vtkPolyDataMapper* boxMapper = vtkPolyDataMapper::New();
  boxMapper->SetInputConnection(boxNormals->GetOutputPort());
  boxNormals->Delete();
  boxSource->Delete();
  boxMapper->SetScalarVisibility(0);

  vtkActor* boxActor = vtkActor::New();
  vtkInformation* boxKeyProperties = vtkInformation::New();
  boxKeyProperties->Set(vtkShadowMapBakerPass::OCCLUDER(), 0); // dummy val.
  boxKeyProperties->Set(vtkShadowMapBakerPass::RECEIVER(), 0); // dummy val.
  boxActor->SetPropertyKeys(boxKeyProperties);
  boxKeyProperties->Delete();

  boxActor->SetMapper(boxMapper);
  boxMapper->Delete();
  boxActor->SetVisibility(1);
  boxActor->SetPosition(-2.0, 2.0, 0.0);
  boxActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  vtkConeSource* coneSource = vtkConeSource::New();
  coneSource->SetResolution(24);
  coneSource->SetDirection(1.0, 1.0, 1.0);
  vtkPolyDataMapper* coneMapper = vtkPolyDataMapper::New();
  coneMapper->SetInputConnection(coneSource->GetOutputPort());
  coneSource->Delete();
  coneMapper->SetScalarVisibility(0);

  vtkActor* coneActor = vtkActor::New();
  vtkInformation* coneKeyProperties = vtkInformation::New();
  coneKeyProperties->Set(vtkShadowMapBakerPass::OCCLUDER(), 0); // dummy val.
  coneKeyProperties->Set(vtkShadowMapBakerPass::RECEIVER(), 0); // dummy val.
  coneActor->SetPropertyKeys(coneKeyProperties);
  coneKeyProperties->Delete();
  coneActor->SetMapper(coneMapper);
  coneMapper->Delete();
  coneActor->SetVisibility(1);
  coneActor->SetPosition(0.0, 1.0, 1.0);
  coneActor->GetProperty()->SetColor(0.0, 0.0, 1.0);
  //  coneActor->GetProperty()->SetLighting(false);

  vtkSphereSource* sphereSource = vtkSphereSource::New();
  sphereSource->SetThetaResolution(32);
  sphereSource->SetPhiResolution(32);
  vtkPolyDataMapper* sphereMapper = vtkPolyDataMapper::New();
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereSource->Delete();
  sphereMapper->SetScalarVisibility(0);

  vtkActor* sphereActor = vtkActor::New();
  vtkInformation* sphereKeyProperties = vtkInformation::New();
  sphereKeyProperties->Set(vtkShadowMapBakerPass::OCCLUDER(), 0); // dummy val.
  sphereKeyProperties->Set(vtkShadowMapBakerPass::RECEIVER(), 0); // dummy val.
  sphereActor->SetPropertyKeys(sphereKeyProperties);
  sphereKeyProperties->Delete();
  sphereActor->SetMapper(sphereMapper);
  sphereMapper->Delete();
  sphereActor->SetVisibility(1);
  sphereActor->SetPosition(2.0, 2.0, -1.0);
  sphereActor->GetProperty()->SetColor(1.0, 1.0, 0.0);

  renderer->AddViewProp(rectangleActor);
  rectangleActor->Delete();
  renderer->AddViewProp(boxActor);
  boxActor->Delete();
  renderer->AddViewProp(coneActor);
  coneActor->Delete();
  renderer->AddViewProp(sphereActor);
  sphereActor->Delete();

  // Spotlights.

  // lighting the box.
  vtkLight* l1 = vtkLight::New();
  l1->SetPosition(-4.0, 4.0, -1.0);
  l1->SetFocalPoint(boxActor->GetPosition());
  l1->SetColor(1.0, 1.0, 1.0);
  l1->SetPositional(1);
  renderer->AddLight(l1);
  l1->SetSwitch(1);
  l1->Delete();

  // lighting the sphere
  vtkLight* l2 = vtkLight::New();
  l2->SetPosition(4.0, 5.0, 1.0);
  l2->SetFocalPoint(sphereActor->GetPosition());
  l2->SetColor(1.0, 0.0, 1.0);
  //  l2->SetColor(1.0,1.0,1.0);
  l2->SetPositional(1);
  renderer->AddLight(l2);
  l2->SetSwitch(1);
  l2->Delete();

  AddLightActors(renderer);

  // Tell the pipeline which piece we want to update.
  sphereMapper->SetNumberOfPieces(numProcs);
  sphereMapper->SetPiece(me);
  coneMapper->SetNumberOfPieces(numProcs);
  coneMapper->SetPiece(me);
  rectangleMapper->SetNumberOfPieces(numProcs);
  rectangleMapper->SetPiece(me);
  boxMapper->SetNumberOfPieces(numProcs);
  boxMapper->SetPiece(me);
}

//-----------------------------------------------------------------------------
void MyProcess::SetupRenderPasses(vtkRenderer* renderer)
{
  // the rendering passes
  vtkCameraPass* cameraP = vtkCameraPass::New();

  vtkOpaquePass* opaque = vtkOpaquePass::New();

  vtkDepthPeelingPass* peeling = vtkDepthPeelingPass::New();
  peeling->SetMaximumNumberOfPeels(200);
  peeling->SetOcclusionRatio(0.1);

  vtkTranslucentPass* translucent = vtkTranslucentPass::New();
  peeling->SetTranslucentPass(translucent);

  vtkVolumetricPass* volume = vtkVolumetricPass::New();
  vtkOverlayPass* overlay = vtkOverlayPass::New();

  vtkLightsPass* lights = vtkLightsPass::New();

  vtkSequencePass* opaqueSequence = vtkSequencePass::New();

  vtkRenderPassCollection* passes2 = vtkRenderPassCollection::New();
  passes2->AddItem(lights);
  passes2->AddItem(opaque);
  opaqueSequence->SetPasses(passes2);
  passes2->Delete();

  vtkCameraPass* opaqueCameraPass = vtkCameraPass::New();
  opaqueCameraPass->SetDelegatePass(opaqueSequence);

  vtkShadowMapBakerPass* shadowsBaker = vtkShadowMapBakerPass::New();
  shadowsBaker->SetOpaquePass(opaqueCameraPass);
  opaqueCameraPass->Delete();
  shadowsBaker->SetResolution(1024);
  // To cancel self-shadowing.
  shadowsBaker->SetPolygonOffsetFactor(3.1f);
  shadowsBaker->SetPolygonOffsetUnits(10.0f);

  vtkShadowMapPass* shadows = vtkShadowMapPass::New();
  shadows->SetShadowMapBakerPass(shadowsBaker);
  shadows->SetOpaquePass(opaqueSequence);

  vtkCompositeZPass* compositeZPass = vtkCompositeZPass::New();
  compositeZPass->SetController(this->Controller);
  shadowsBaker->SetCompositeZPass(compositeZPass);
  compositeZPass->Delete();

  vtkSequencePass* seq = vtkSequencePass::New();
  vtkRenderPassCollection* passes = vtkRenderPassCollection::New();
  passes->AddItem(shadowsBaker);
  passes->AddItem(shadows);
  passes->AddItem(lights);

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
  shadows->Delete();
  shadowsBaker->Delete();
  opaqueSequence->Delete();
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
  renWin->SetSize(width, height);

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

      vtkSynchronizedRenderers* syncRenderers2 = vtkSynchronizedRenderers::New();
      syncRenderers2->SetRenderer(renderer);
      syncRenderers2->SetParallelController(this->SocketController);
      syncRenderers2->SetRootProcessId(1);

      this->SocketController->ProcessRMIs();
    }
    else
    {
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
  args.AddArgument("--server", vtksys::CommandLineArguments::NO_ARGUMENT, &act_as_server,
    "When present, the root process acts as a server process for a client.");

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

// DUPLICATE for VTK/Rendering/Testing/Cxx/TestLightActor.cxx

// For each spotlight, add a light frustum wireframe representation and a cone
// wireframe representation, colored with the light color.
void AddLightActors(vtkRenderer* r)
{
  assert("pre: r_exists" && r != 0);

  vtkLightCollection* lights = r->GetLights();

  lights->InitTraversal();
  vtkLight* l = lights->GetNextItem();
  while (l != 0)
  {
    double angle = l->GetConeAngle();
    if (l->LightTypeIsSceneLight() && l->GetPositional() && angle < 90.0) // spotlight
    {
      vtkLightActor* la = vtkLightActor::New();
      la->SetLight(l);
      r->AddViewProp(la);
      la->Delete();
    }
    l = lights->GetNextItem();
  }
}
