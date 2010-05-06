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
#include "vtkCompositeRenderManager.h"
#include "vtkDataSetReader.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDepthPeelingPass.h"
#include "vtkDistributedDataFilter.h"
#include "vtkIceTCompositePass.h"
#include "vtkImageRenderManager.h"
#include "vtkLightsPass.h"
#include "vtkMPIController.h"
#include "vtkOpaquePass.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOverlayPass.h"
#include "vtkParallelFactory.h"
#include "vtkPieceScalars.h"
#include "vtkPKdTree.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSequencePass.h"
#include "vtkSmartPointer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkSynchronizedRenderers.h"
#include "vtkSynchronizedRenderWindows.h"
#include "vtkTestUtilities.h"
#include "vtkTranslucentPass.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVolumetricPass.h"
/*
** This test only builds if MPI is in use
*/
#include "vtkMPICommunicator.h"

#include "vtkProcess.h"

#include <vtksys/CommandLineArguments.hxx>

class MyProcess : public vtkProcess
{
  vtkSmartPointer<vtkPKdTree> KdTree;
  int UseOrderedCompositing;
  int UseDepthPeeling;
public:
  static MyProcess *New();
  vtkTypeRevisionMacro(MyProcess, vtkProcess);

  vtkSetVector2Macro(TileDimensions, int);
  vtkGetVector2Macro(TileDimensions, int);

  vtkSetMacro(ImageReductionFactor, int);
  vtkSetMacro(UseOrderedCompositing, int);
  vtkSetMacro(UseDepthPeeling, int);

  void SetArgs(int anArgc, char *anArgv[]);

  virtual void Execute();
private:
  // Creates the visualization pipeline and adds it to the renderer.
  void CreatePipeline(vtkRenderer* renderer);

  // Setups render passes.
  void SetupRenderPasses(vtkRenderer* renderer);

protected:
  MyProcess();

  int Argc;
  char **Argv;

  int ImageReductionFactor;
  int TileDimensions[2];
};

vtkCxxRevisionMacro(MyProcess, "$Revision$");
vtkStandardNewMacro(MyProcess);

//-----------------------------------------------------------------------------
MyProcess::MyProcess()
{
  this->Argc=0;
  this->Argv=0;
  this->ImageReductionFactor = 1;
  this->TileDimensions[0] = this->TileDimensions[1];
  this->UseOrderedCompositing = 0;
  this->UseDepthPeeling = 0;
}

//-----------------------------------------------------------------------------
void MyProcess::SetArgs(int anArgc, char *anArgv[])
{
  this->Argc=anArgc;
  this->Argv=anArgv;
}

//-----------------------------------------------------------------------------
void MyProcess::CreatePipeline(vtkRenderer* renderer)
{
  int num_procs = this->Controller->GetNumberOfProcesses();
  int my_id = this->Controller->GetLocalProcessId();

  vtkSphereSource* sphere = vtkSphereSource::New();
  sphere->SetPhiResolution(100);
  sphere->SetThetaResolution(100);

  vtkDistributedDataFilter* d3 = vtkDistributedDataFilter::New();
  d3->SetInputConnection(sphere->GetOutputPort());
  d3->SetController(this->Controller);
  d3->SetBoundaryModeToSplitBoundaryCells();
  d3->UseMinimalMemoryOff();

  vtkDataSetSurfaceFilter* surface = vtkDataSetSurfaceFilter::New();
  surface->SetInputConnection(d3->GetOutputPort());

  vtkPieceScalars *piecescalars = vtkPieceScalars::New();
  piecescalars->SetInputConnection(surface->GetOutputPort());
  piecescalars->SetScalarModeToCellData();

  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(piecescalars->GetOutputPort());
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("Piece");
  mapper->SetScalarRange(0, num_procs-1);
  mapper->SetPiece(my_id);
  mapper->SetNumberOfPieces(num_procs);
  mapper->Update();

  this->KdTree = d3->GetKdtree();

  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetOpacity(this->UseOrderedCompositing? 0.5 : 1.0);
  renderer->AddActor(actor);

  actor->Delete();
  mapper->Delete();
  piecescalars->Delete();
  surface->Delete();
  d3->Delete();
  sphere->Delete();
}

//-----------------------------------------------------------------------------
void MyProcess::SetupRenderPasses(vtkRenderer* renderer)
{
  // the rendering passes
  vtkCameraPass *cameraP=vtkCameraPass::New();
  vtkSequencePass *seq=vtkSequencePass::New();
  vtkOpaquePass *opaque=vtkOpaquePass::New();
  vtkDepthPeelingPass *peeling=vtkDepthPeelingPass::New();
  peeling->SetMaximumNumberOfPeels(200);
  peeling->SetOcclusionRatio(0.1);

  vtkTranslucentPass *translucent=vtkTranslucentPass::New();
  peeling->SetTranslucentPass(translucent);

  vtkVolumetricPass *volume=vtkVolumetricPass::New();
  vtkOverlayPass *overlay=vtkOverlayPass::New();
  vtkLightsPass *lights=vtkLightsPass::New();

  vtkClearZPass *clearZ=vtkClearZPass::New();
  clearZ->SetDepth(0.9);

  vtkRenderPassCollection *passes=vtkRenderPassCollection::New();
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

  cameraP->SetDelegatePass(iceTPass);
  cameraP->SetAspectRatioOverride(
    (double)this->TileDimensions[0] / this->TileDimensions[1]);

  renderer->SetPass(cameraP);

  // setting viewport doesn't work in tile-display mode correctly yet.
  //renderer->SetViewport(0, 0, 0.75, 1);

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
  int my_id = this->Controller->GetLocalProcessId();

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  // enable alpha bit-planes.
  renWin->AlphaBitPlanesOn();

  // use double bufferring.
  renWin->DoubleBufferOn();

  // don't waste time swapping buffers unless needed.
  renWin->SwapBuffersOff();

  vtkRenderer* renderer = vtkRenderer::New();
  renWin->AddRenderer(renderer);

  vtkSynchronizedRenderWindows* syncWindows =
    vtkSynchronizedRenderWindows::New();
  syncWindows->SetRenderWindow(renWin);
  syncWindows->SetParallelController(this->Controller);
  syncWindows->SetIdentifier(1);

  vtkSynchronizedRenderers* syncRenderers = vtkSynchronizedRenderers::New();
  syncRenderers->SetRenderer(renderer);
  syncRenderers->SetParallelController(this->Controller);

  this->CreatePipeline(renderer);
  this->SetupRenderPasses(renderer);

  if (my_id == 0)
    {
    vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);
    renWin->SwapBuffersOn();
    renWin->Render();

    iren->Start();
    iren->Delete();

    this->Controller->TriggerBreakRMIs();
    this->Controller->Barrier();
    }
  else
    {
    if (this->TileDimensions[0] > 1 || this->TileDimensions[1] > 1)
      {
      renWin->SwapBuffersOn();
      }
    this->Controller->ProcessRMIs();
    this->Controller->Barrier();
    }

  renderer->Delete();
  renWin->Delete();
  syncWindows->Delete();
  syncRenderers->Delete();
}

//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  int retVal = 1;

  vtkSmartPointer<vtkMPIController> contr = vtkSmartPointer<vtkMPIController>::New();
  contr->Initialize(&argc, &argv);

  int tile_dimensions[2] = {1, 1};
  int image_reduction_factor = 1;
  int use_ordered_compositing = 0;
  int use_depth_peeling = 0;

  vtksys::CommandLineArguments args;
  args.Initialize(argc, argv);
  args.StoreUnusedArguments(false);
  args.AddArgument("-tdx", vtksys::CommandLineArguments::SPACE_ARGUMENT,
    &tile_dimensions[0], "");
  args.AddArgument("-tdy", vtksys::CommandLineArguments::SPACE_ARGUMENT,
    &tile_dimensions[1], "");
  args.AddArgument("--image-reduction-factor",
    vtksys::CommandLineArguments::SPACE_ARGUMENT,
    &image_reduction_factor, "Image reduction factor");
  args.AddArgument("-irf",
    vtksys::CommandLineArguments::SPACE_ARGUMENT,
    &image_reduction_factor, "Image reduction factor");
  args.AddArgument("--use-ordered-compositing",
    vtksys::CommandLineArguments::NO_ARGUMENT,
    &use_ordered_compositing, "Use ordered compositing");
  args.AddArgument("--use-depth-peeling",
    vtksys::CommandLineArguments::NO_ARGUMENT,
    &use_depth_peeling, "Use depth peeling."
    "This works only when --use-ordered-compositing is true.");
  if (!args.Parse())
    {
    if (contr->GetLocalProcessId() == 0)
      {
      cout << args.GetHelp() << endl;
      }
    contr->Finalize();
    return 1;
    }

  tile_dimensions[0] = tile_dimensions[0] < 1? 1 : tile_dimensions[0];
  tile_dimensions[1] = tile_dimensions[1] < 1? 1 : tile_dimensions[1];

  int numProcs = contr->GetNumberOfProcesses();
  if (tile_dimensions[0] > 1 || tile_dimensions[1] > 1)
    {
    if (numProcs != tile_dimensions[0] * tile_dimensions[1])
      {
      cerr << "When running in tile-display mode, number of processes must "
        "match number of tiles" << endl;
      return 1;
      }
    cout << "Rendering as a " << tile_dimensions[0] << "x"
      << tile_dimensions[1] << " tile display" << endl;
    }

  vtkMultiProcessController::SetGlobalController(contr);

  MyProcess *p=MyProcess::New();
  p->SetArgs(argc,argv);
  p->SetTileDimensions(tile_dimensions);
  p->SetImageReductionFactor(image_reduction_factor);
  p->SetUseOrderedCompositing(use_ordered_compositing);
  p->SetUseDepthPeeling(use_depth_peeling);
  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal=p->GetReturnValue();

  vtkMultiProcessController::SetGlobalController(0);
  p->Delete();
  contr->Finalize();
  contr = 0;
  return !retVal;
}
