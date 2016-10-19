/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MantaBenchmark.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the manta vs openGL render speed

// TODO: Measure pipeline change and render setup time
// TODO: Make it run and compile without MPI

#include <mpi.h>

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkClipPolyData.h"
#include "vtkCompositeRenderManager.h"
#include "vtkElevationFilter.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMantaTestSource.h"
#include "vtkObjectFactory.h"
#include "vtkPLYWriter.h"
#include "vtkPlane.h"
#include "vtkPolyDataMapper.h"
#include "vtkProcess.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTimerLog.h"

#include "vtkMantaActor.h"
#include "vtkMantaPolyDataMapper.h"
#include "vtkMantaRenderer.h"

#include <assert.h>

#include "vtkSmartPointer.h"

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkPlane* plane = NULL;
vtkElevationFilter* elev = NULL;

// call back to set the clip plane location value.
void PlaceClipRMI(void* localArg, void* remoteArg, int vtkNotUsed(remoteArgLen), int vtkNotUsed(id))
{
  double* origin = (double*)remoteArg;
  if (plane)
  {
    plane->SetOrigin(origin[0], origin[1], origin[2]);
  }
}

// call back to set the clip plane orientation
void OrientClipRMI(
  void* localArg, void* remoteArg, int vtkNotUsed(remoteArgLen), int vtkNotUsed(id))
{

  double* normal = (double*)remoteArg;
  if (plane)
  {
    plane->SetNormal(normal[0], normal[1], normal[2]);
  }
}

// call back to set the elevation filter orientation
void OrientElevRMI(
  void* localArg, void* remoteArg, int vtkNotUsed(remoteArgLen), int vtkNotUsed(id))
{
  double* segment = (double*)remoteArg;
  if (segment)
  {
    elev->SetLowPoint(segment[0], segment[1], segment[2]);
    elev->SetHighPoint(segment[3], segment[4], segment[5]);
  }
}

class MyProcess : public vtkProcess
{
public:
  static MyProcess* New();
  ~MyProcess() { this->Timer->Delete(); }

  vtkTypeMacro(MyProcess, vtkProcess);

  virtual void Execute();

  void SetArgs(int anArgc, char* anArgv[])
  {
    this->Argc = anArgc;
    this->Argv = anArgv;

    this->Frame = 0;
    this->PrepFrames = 0;
    this->PrepTime = 0.0;
    this->RenderTime = 0.0;
    this->CompositeTime = 0.0;
    this->AllTime = 0.0;
  }
  void NoteTime(vtkParallelRenderManager* prm, bool SetupFrame)
  {
    this->Timer->StopTimer();
    double allTime = this->Timer->GetElapsedTime();

    double renTime = prm->GetRenderTime();
    double prepTime = 0.0;
    if (SetupFrame)
    {
      prepTime = renTime;
      renTime = 0.0;
    }
    double compTime = prm->GetImageProcessingTime();
    cerr << this->Frame << " " << renTime << " " << compTime << " " << allTime << endl;

    if (SetupFrame)
    {
      this->PrepFrames++;
    }
    else
    {
      this->Frame++;
    }
    this->RenderTime += renTime;
    this->CompositeTime += compTime;
    this->AllTime += allTime;
    this->PrepTime += prepTime;
    this->Timer->StartTimer();
  }
  void StartTime() { this->Timer->StartTimer(); }
  void PrintStats()
  {
    if (this->PrepFrames != 0)
    {
      cerr << "avg_prep_time " << this->PrepTime / this->PrepFrames << endl;
    }
    if (this->Frame != 0)
    {
      cerr << "avg_render_time " << this->RenderTime / this->Frame << endl;
    }
    if (this->AllTime != 0.0)
    {
      cerr << "avg_composite_time " << this->CompositeTime / (this->Frame + this->PrepFrames)
           << endl;
      cerr << "avg_framerate " << (this->Frame + this->PrepFrames) / this->AllTime << endl;
    }
  }

protected:
  MyProcess();

  int Argc;
  char** Argv;
  int Frame;
  int PrepFrames;
  double PrepTime; // an approximation, assumes whole first frame is prep and not rendering
  double RenderTime;
  double CompositeTime;
  double AllTime;
  vtkTimerLog* Timer;
};

vtkStandardNewMacro(MyProcess);

MyProcess::MyProcess()
{
  this->Argc = 0;
  this->Argv = 0;
  this->Timer = vtkTimerLog::New();
  this->Frame = 0;
  this->RenderTime = 0.0;
  this->CompositeTime = 0.0;
  this->AllTime = 0;
}

void MyProcess::Execute()
{
  // parse environment and arguments
  int numProcs = this->Controller->GetNumberOfProcesses();
  int me = this->Controller->GetLocalProcessId();
  bool changeData = true;
  bool changeCamera = true;
  int screensize = 400;
  int triangles = 100000;
  double fuzziness = 0.0;
  int AStype = 0;
  int threads = 1;
  int processes = numProcs;
  bool useGL = false;
  bool useDisplayList = false;

  long unsigned int kB = 0;

  for (int i = 0; i < this->Argc; i++)
  {
    if (!strcmp(this->Argv[i], "-triangles"))
    {
      triangles = atoi(this->Argv[i + 1]);
    }
    if (!strcmp(this->Argv[i], "-fuzziness"))
    {
      fuzziness = atof(this->Argv[i + 1]);
    }
    if (!strcmp(this->Argv[i], "-threads"))
    {
      threads = atoi(this->Argv[i + 1]);
    }
    if (!strcmp(this->Argv[i], "-asType"))
    {
      AStype = atoi(this->Argv[i + 1]);
    }
    if (!strcmp(this->Argv[i], "-useGL"))
    {
      useGL = true;
    }
    if (!strcmp(this->Argv[i], "-useDisplayList"))
    {
      useDisplayList = true;
    }
    if (!strcmp(this->Argv[i], "-screensize"))
    {
      screensize = atoi(this->Argv[i + 1]);
    }
    if (!strcmp(this->Argv[i], "-noCamera"))
    {
      changeCamera = false;
    }
    if (!strcmp(this->Argv[i], "-noChanges"))
    {
      changeData = false;
    }
  }

  // Make a scene that has a configurable number of triangles
  // and that we can change
  double bds[6];
  vtkMantaTestSource* source = vtkMantaTestSource::New();
  source->SetResolution(triangles);
  source->SetSlidingWindow(fuzziness);
#if 0
  //for comparison with Manta standalone
  vtkPLYWriter *writer = vtkPLYWriter::New();
  writer->SetInputConnection(source->GetOutputPort(0));
  char buff[120];
  sprintf(buff, "mesh_%d.ply", triangles);
  cerr << buff << endl;
  writer->SetFileName(buff);
  writer->Write();
  writer->Delete();
  exit(0);
#endif

  // Compute local geometric bounds
  source->UpdatePiece(me, numProcs, 0);
  source->GetOutput()->GetBounds(bds);
  /*
  cerr << "LBDS(" << me << ") "
       << bds[0] << ","<< bds[1] << ","
       << bds[2] << ","<< bds[3] << ","
       << bds[4] << ","<< bds[5] << endl;
  */
  vtkClipPolyData* clipper = vtkClipPolyData::New();
  clipper->SetInputConnection(source->GetOutputPort());
  plane = vtkPlane::New();
  clipper->SetClipFunction(plane);
  source->Delete();

  elev = vtkElevationFilter::New();
  elev->SetInputConnection(clipper->GetOutputPort());
  clipper->Delete();

  vtkPolyDataMapper* mapper = NULL;
  if (useGL)
  {
    mapper = vtkPolyDataMapper::New();
    mapper->SetImmediateModeRendering(!useDisplayList);
  }
  else
  {
    mapper = vtkMantaPolyDataMapper::New();
  }
  mapper->SetInputConnection(elev->GetOutputPort());
  mapper->SetPiece(me); // each processor makes a different part of the data
  mapper->SetNumberOfPieces(numProcs);

  // Now set up parallel display pipeline to show it
  vtkCompositeRenderManager* prm = vtkCompositeRenderManager::New();

  vtkRenderWindowInteractor* iren = NULL;
  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  if (me == 0)
  {
    iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);
  }
  renWin->SetSize(screensize, screensize);
  renWin->SetPosition(0, (screensize + 60) * me); // translate the window

  vtkRenderer* renderer = NULL;
  vtkMantaRenderer* mRenderer = NULL;
  if (useGL)
  {
    renderer = vtkRenderer::New();
  }
  else
  {
    mRenderer = vtkMantaRenderer::New();
    mRenderer->SetNumberOfWorkers(threads);
    renderer = mRenderer;
  }

  renWin->AddRenderer(renderer);
  prm->SetRenderWindow(renWin);

  vtkActor* actor = NULL;
  if (useGL)
  {
    actor = vtkActor::New();

    // quick way to be sure which one is actually rendering
    // vtkProperty *prop = actor->GetProperty();
    // prop->SetRepresentationToWireframe();
  }
  else
  {
    vtkMantaActor* act = vtkMantaActor::New();
    act->SetSortType(AStype);
    actor = act;
  }

  actor->SetMapper(mapper);

  renderer->AddViewProp(actor);
  actor->Delete();
  renderer->Delete();

  prm->SetRenderWindow(renWin);
  prm->SetController(this->Controller);

  int retVal;
  const int MY_RETURN_VALUE_MESSAGE = 0x518113;
  const int ORIGIN_RMI = 0x518114;
  const int PLANE_RMI = 0x518115;
  const int ELEV_RMI = 0x518116;
  const int SETUP_BOUNDS = 0x518117;
  if (me > 0)
  {
    // Figure out global bounds so we can use it for filters/viewpoints
    this->Controller->Send(bds, 6, 0, SETUP_BOUNDS);

    // Last, set up RMI call backs to change pipeline parameters
    // camera changes are handled by the prm
    this->Controller->AddRMI(PlaceClipRMI, NULL, ORIGIN_RMI);
    this->Controller->AddRMI(OrientClipRMI, NULL, PLANE_RMI);
    this->Controller->AddRMI(OrientElevRMI, NULL, ELEV_RMI);

    // satellite nodes, start loop that receives and responds to window events
    // from root
    prm->StartServices();

    // receive return value from root process.
    this->Controller->Receive(&retVal, 1, 0, MY_RETURN_VALUE_MESSAGE);
  }
  else
  {
    // root node drives all the action, everyone else just does what it is told
    double origin[3];
    double normal[3];
    double segment[6];

    // Figure out global bounds so we can use it for filters/viewpoints
    for (int p = 1; p < numProcs; ++p)
    {
      double rbds[6];
      this->Controller->Receive(rbds, 6, p, SETUP_BOUNDS);
      /*
      cerr << "RBDS "
           << rbds[0] << ","<< rbds[1] << ","
           << rbds[2] << ","<< rbds[3] << ","
           << rbds[4] << ","<< rbds[5] << endl;
      */
      for (int i = 0; i < 3; i++)
      {
        if (rbds[i * 2 + 0] < bds[i * 2 + 0])
        {
          bds[i * 2 + 0] = rbds[i * 2 + 0];
        }
        if (rbds[i * 2 + 1] > bds[i * 2 + 1])
        {
          bds[i * 2 + 1] = rbds[i * 2 + 1];
        }
      }
    }
    /*
    cerr << "GDS "
         << bds[0] << ","<< bds[1] << ","
         << bds[2] << ","<< bds[3] << ","
         << bds[4] << ","<< bds[5] << endl;
    */

    // set colors to show Z initially
    segment[0] = 0;
    segment[1] = 0;
    segment[2] = bds[4];
    segment[3] = 0;
    segment[4] = 0;
    segment[5] = bds[5];
    elev->SetLowPoint(segment[0], segment[1], segment[2]);
    elev->SetHighPoint(segment[3], segment[4], segment[5]);
    for (int p = 1; p < numProcs; ++p)
    {
      this->Controller->TriggerRMI(p, (void*)segment, 6 * sizeof(double), ELEV_RMI);
    }

    // Playback some canned camera and data manipulations
    vtkCamera* camera = renderer->GetActiveCamera();
    renderer->ResetCamera(bds);

    // move plane off data so entire thing is visible
    origin[0] = bds[0];
    origin[1] = bds[2];
    origin[2] = bds[4];
    plane->SetOrigin(origin[0], origin[1], origin[2]);
    normal[0] = 1;
    normal[1] = 0;
    normal[2] = 0;
    plane->SetNormal(normal[0], normal[1], normal[2]);
    for (int p = 1; p < numProcs; ++p)
    {
      this->Controller->TriggerRMI(p, (void*)origin, 3 * sizeof(double), ORIGIN_RMI);
      this->Controller->TriggerRMI(p, (void*)normal, 3 * sizeof(double), PLANE_RMI);
    }

    this->StartTime(); // start benchmarking Rendering now

    // initial render
    renWin->Render();
    this->NoteTime(prm, true);
    kB = mapper->GetInputAsDataSet()->GetActualMemorySize();

    // Change elevation filter's parameter to test color transfer function
    for (int i = 2; i > -1; i--)
    {
      if (changeData)
      {
        cerr << "Color shows " << ((i == 0) ? "X" : "") << ((i == 1) ? "Y" : "")
             << ((i == 2) ? "Z" : "") << endl;

        segment[0] = 0;
        segment[1] = 0;
        segment[2] = 0;
        segment[3] = 0;
        segment[4] = 0;
        segment[5] = 0;
        segment[i] = bds[i * 2 + 0];
        segment[i + 3] = bds[i * 2 + 1];

        elev->SetLowPoint(segment[0], segment[1], segment[2]);
        elev->SetHighPoint(segment[3], segment[4], segment[5]);
        for (int p = 1; p < numProcs; ++p)
        {
          this->Controller->TriggerRMI(p, (void*)segment, 6 * sizeof(double), ELEV_RMI);
        }
      }
      // render
      renWin->Render();
      this->NoteTime(prm, changeData);
      // Move the camera and render a bunch of frames to measure rerender time
      // TODO: Make step size configurable
      double direction = 20.0;
      if (i == 1)
      {
        direction = direction * -1.0;
      }
      for (int f = 0; f < 18 && changeCamera; f++)
      {
        camera->Azimuth(direction);
        renderer->ResetCameraClippingRange(bds);
        renWin->Render();
        this->NoteTime(prm, false);
      }
    }

    // put initial clip plane in the middle of the data...
    if (changeData)
    {
      origin[0] = (bds[0] + bds[1]) * 0.5;
      origin[1] = (bds[2] + bds[3]) * 0.5;
      origin[2] = (bds[4] + bds[5]) * 0.5;
      plane->SetOrigin(origin[0], origin[1], origin[2]); //...locally
      for (int p = 1; p < numProcs; ++p)
      {
        this->Controller->TriggerRMI(
          p, (void*)origin, 3 * sizeof(double), ORIGIN_RMI); //...and on remotes
      }
    }

    // Change a clip filter's parameter to measure render setup time
    for (int i = 2; i > -1; i--)
    {
      if (changeData)
      {
        cerr << "Clip off " << ((i == 0) ? "X" : "") << ((i == 1) ? "Y" : "")
             << ((i == 2) ? "Z" : "") << endl;

        normal[0] = 0;
        normal[1] = 0;
        normal[2] = 0;
        normal[i] = 1;
        plane->SetNormal(normal[0], normal[1], normal[2]);
        for (int p = 1; p < numProcs; ++p)
        {
          this->Controller->TriggerRMI(p, (void*)normal, 3 * sizeof(double), PLANE_RMI);
        }
      }
      // render
      renWin->Render();
      this->NoteTime(prm, changeData);

      // Move the camera and render a bunch of frames to measure rerender time
      // TODO: Make step size configurable
      double direction = 20.0;
      if (i == 1)
      {
        direction = direction * -1.0;
      }
      for (int f = 0; f < 18 && changeCamera; f++)
      {
        camera->Azimuth(direction);
        renderer->ResetCameraClippingRange(bds);
        renWin->Render();
        this->NoteTime(prm, false);
      }
    }

    // TODO: should also test with camera zoomed in

    // double thresh=10;
    int i;
    VTK_CREATE(vtkTesting, testing);
    for (i = 0; i < this->Argc; ++i)
    {
      testing->AddArgument(this->Argv[i]);
    }

    if (testing->IsInteractiveModeSpecified())
    {
      // Let user manipulate view manually
      retVal = vtkTesting::DO_INTERACTOR;
      iren->Start();
    }
    else
    {
      // the purpose of this test is to measure timing
      // image comparisons are not needed for that
      // flaws will show up when speed is drastically faster/slower than before
      retVal = 1;
      renWin->Render();
    }

    prm->StopServices(); // tells satellites to stop listening.

    // send the return value to the satellites
    i = 1;
    while (i < numProcs)
    {
      this->Controller->Send(&retVal, 1, i, MY_RETURN_VALUE_MESSAGE);
      ++i;
    }
    iren->Delete();

    cerr << "processes " << processes << endl;
    cerr << "threads " << (useGL ? 1 : threads) << endl;
    cerr << "screensize " << screensize << endl;
    cerr << "triangles " << triangles << endl;
    cerr << "fuzziness " << fuzziness << endl;
    cerr << "kB " << kB << endl;
    cerr << "render_with " << (useGL ? "GL" : "Manta") << endl;
    if (useGL)
    {
      cerr << "display_lists " << (useDisplayList ? "ON" : "OFF") << endl;
    }
    if (!useGL)
    {
      cerr << "accel_structure " << AStype << endl;
    }
    this->PrintStats();
  }

  mapper->Delete();
  renWin->Delete();
  prm->Delete();
  elev->Delete();
  plane->Delete();
  this->ReturnValue = retVal;
}

int MantaBenchmark(int argc, char** argv)
{
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkMPIController* contr = vtkMPIController::New();
  contr->Initialize(&argc, &argv, 1);

  int retVal = 1; // 1==failed

  vtkMultiProcessController::SetGlobalController(contr);

  int me = contr->GetLocalProcessId();
  if (!contr->IsA("vtkMPIController"))
  {
    if (me == 0)
    {
      cout << "DistributedData test requires MPI" << endl;
    }
    contr->Delete();
    return retVal;
  }

  MyProcess* p = MyProcess::New();
  p->SetArgs(argc, argv);

  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal = p->GetReturnValue();
  p->Delete();
  contr->Finalize();
  contr->Delete();

  return !retVal;
}
