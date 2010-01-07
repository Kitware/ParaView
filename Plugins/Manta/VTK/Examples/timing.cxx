// recording timings of a parallel mesa run using mpi
// this uses the TransmitPolyDataPiece filter

#include <stdio.h>
#include <ostream>

#include "vtkCompositeRenderManager.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDistributedDataFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPieceScalars.h"
#include "vtkMPIController.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataReader.h"
#include "vtkMPICommunicator.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkTriangle.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkCamera.h"
#include "vtkTimerLog.h"
#include "vtkProperty.h"
#include "vtkLight.h"
#include "vtkTriangleFilter.h"
#include "vtkTransmitPolyDataPiece.h"
#include "vtkPolyData.h"
#include "vtkSelection.h"
#include "vtkExtractSelectedPolyDataIds.h"
#include "vtkInformation.h"
#include "vtkLoopSubdivisionFilter.h"
#include "vtkRTAnalyticSource.h"
#include "vtkImageData.h"
#include "vtkContourFilter.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkCommand.h"

// struct that holds all the different possible arguments
typedef struct
{
  int xres;
  int yres;
  char* input;
  char* output;
  int numThreads;
  int numTriangles;
  bool showHelp;
} arguments;

// default values of args
#define D_XRES 500
#define D_YRES 500
#define D_INPUT NULL
#define D_OUTPUT NULL
#define D_NUMTHREADS 1
#define D_NUMTRIANGLES -1
#define D_SHOWHELP false

#define SHOW_ROOT_IMAGE true
#define SHOW_SLAVE_IMAGE true
#define INTERACTIVE true

// total number of frames to render
#define NUM_FRAMES 20

// how many of the last number of frames to take an average of
#define AVG_FRAMES 10

static int NumProcs, myId;
FILE* outputptr;
double slate_grey[] =
  {
  0.4392, 0.5020, 0.5647
  };

void process(vtkMultiProcessController* controller, void* arg);
vtkPolyDataMapper* addFile(vtkRenderer* renderer, char* filename,
    vtkMultiProcessController* contr, double* boundingbox, arguments args);
void addLights(vtkRenderer* renderer);
double render(vtkRenderWindow* renWin);
static void Run(vtkMultiProcessController* contr, void* arg);
vtkPolyData* setNumberOfTriangles(vtkPolyData* polydata, int numTriangles);
vtkPolyData* reduceTriangles(vtkPolyData* polydata, int numTriangles);
vtkPolyData* increaseTriangles(vtkPolyData* polydata, int numTriangles);
arguments parseArguments(int argc, char** argv);
vtkPolyData* addDefaultMesh(vtkRenderer* renderer, arguments args);
void displayHelp();

struct DDArgs_tmp
{
  int* retVal;
  int argc;
  char** argv;
};

int main(int argc, char** argv)
{
  int retVal = 1;

  vtkMPIController* controller = vtkMPIController::New();
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);
  NumProcs = controller->GetNumberOfProcesses();
  myId = controller->GetLocalProcessId();

  // make sure mpi is running
  if (!controller->IsA("vtkMPIController"))
    {
    if (myId == 0)
      {
      printf("Error! This program requires MPI.\n");
      printf("Aborting program...\n");
      }
    controller->Delete();
    return -1;
    }

  // store the arguments for later
  DDArgs_tmp args;
  args.retVal = &retVal;
  args.argc = argc;
  args.argv = argv;

  // execute a common function for all processes
  controller->SetSingleMethod(Run, &args);
  controller->SingleMethodExecute();

  // cleanup and exit
  controller->Finalize();
  controller->Delete();

  return 0;
}

// Callback for the interaction
class vtkMyCallback: public vtkCommand
  {
public:
  char title[50];
  vtkTimerLog* timer;
  char *old_title;

  vtkMyCallback()
  {
    this->old_title = 0;
    this->timer = vtkTimerLog::New();
  }
  static vtkMyCallback *New()
  {
    return new vtkMyCallback;
  }
  virtual void Execute(vtkObject *caller, unsigned long id, void*)
  {
    vtkRenderWindow *renWin = reinterpret_cast<vtkRenderWindow*> (caller);
    if (!this->old_title)
      {
      this->old_title = strdup(renWin->GetWindowName());
      }
    switch (id)
      {
    case vtkCommand::StartEvent:
      this->timer->StartTimer();
      break;
    case vtkCommand::EndEvent:
      timer->StopTimer();
      sprintf(title, "%s fps %4.2f", this->old_title, 1.0
          / (timer->GetElapsedTime()));
      renWin->SetWindowName(title);
      break;
      }
  }
  };

static void Run(vtkMultiProcessController* contr, void* arg)
{
  int go;

  vtkTimerLog* mainTimer = vtkTimerLog::New();
  mainTimer->StartTimer();

  // parse arguments
  DDArgs_tmp* args_struct = reinterpret_cast<DDArgs_tmp*> (arg);
  arguments args = parseArguments(args_struct->argc, args_struct->argv);

  if (args.showHelp)
    {
    if (myId == 0)
      {
      displayHelp();
      }
    exit(0);
    }

  char* input = args.input;
  char* output = args.output;
  int xres = args.xres;
  int yres = args.yres;

  // get output stream ready
  if (output == NULL)
    {
    // output everything to standard output
    outputptr = stdout;
    }
  else
    {
    outputptr = fopen(output, "a");
    if (outputptr == NULL)
      {
      printf("Error opening output file %s\n", output);
      printf("program is now exiting...\n");
      exit(-1);
      }
    }

  // only write output from node 0. this means that all timings are recorded
  // on node 0 only. this should be ok since node 0 does the compositing, so
  // its time should be the longest out of all the processes.
  if (myId == 0)
    {
    fprintf(outputptr, "-----------------------------------------------\n");
    fprintf(outputptr, "processing %s at resolution %i %i\n", input, xres, yres);
    fprintf(outputptr, "using parallel mesa with transmit filter\n");
    fprintf(outputptr, "using %i threads\n", NumProcs);
    }

  vtkCompositeRenderManager* renderManager = vtkCompositeRenderManager::New();
  vtkRenderer* renderer = renderManager->MakeRenderer();
  vtkRenderWindow* renWin = renderManager->MakeRenderWindow();

  // add objects to the scene
  double boundingbox[6];
  vtkPolyDataMapper* meshMapper = addFile(renderer, input, contr, boundingbox,
      args);
  addLights(renderer);

  renWin->AddRenderer(renderer);
  //char title[50];
  //sprintf(title, "I am node %i", myId);
  //renWin->SetWindowName(title);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // set interaction it to trackball style
  //vtkInteractorStyleTrackballCamera* style =
  //    vtkInteractorStyleTrackballCamera::New();
  //iren->SetInteractorStyle(style);

  renderer->SetBackground(slate_grey);

  // Here is where we setup the observer, we do a new and ren1 will
  // eventually free the observer
  vtkMyCallback *mo1 = vtkMyCallback::New();
  //mo1->renderer = renderer;
  renWin->AddObserver(vtkCommand::StartEvent, mo1);
  renWin->AddObserver(vtkCommand::EndEvent, mo1);
  mo1->Delete();

  renWin->SetSize(xres, yres);
  renWin->SetPosition(0, yres * myId);

  renderManager->SetRenderWindow(renWin);
  renderManager->SetController(contr);

  // We must update the whole pipeline here, otherwise node 0
  // goes into GetActiveCamera which updates the pipeline, putting
  // it into vtkDistributedDataFilter::Execute() which then hangs.
  // If it executes here, dd will be up-to-date won't have to
  // execute in GetActiveCamera.

  meshMapper->SetPiece(myId);
  meshMapper->SetNumberOfPieces(NumProcs);
  meshMapper->Update();

  vtkPolyData* temp = meshMapper->GetInput();

  // setup camera
  if (myId == 0)
    {
    renderer->ResetCamera(boundingbox);
    }

  if (!SHOW_ROOT_IMAGE && myId == 0)
    {
    // not showing an image
    renWin->OffScreenRenderingOn();
    }

  if (!SHOW_SLAVE_IMAGE && myId > 0)
    {
    // only do rendering on the root node
    renWin->OffScreenRenderingOn();
    }
#if 1
  // don't count the couple of frame in the average
  for (int i = 0; i < NUM_FRAMES - AVG_FRAMES; i++)
    {
    render(renWin);
    }

  // start taking the average
  double sum = 0.0;
  for (int i = NUM_FRAMES - AVG_FRAMES; i < NUM_FRAMES; i++)
    {
    sum += render(renWin);
    }
  double avg = sum / (AVG_FRAMES);
  if (myId == 0)
    {
    fprintf(outputptr, "avg render time: %f\n", avg);
    fprintf(outputptr, "avg fps: %f\n", 1.0 / avg);

    mainTimer->StopTimer();
    fprintf(outputptr, "total program time: %f\n", mainTimer->GetElapsedTime());
    }
#endif
  if (myId == 0)
    {
    fclose(outputptr);
    }

  if (INTERACTIVE)
    {
//    renderManager->StartInteractor();
    }

  // clean up
  meshMapper->Delete();
  renderer->Delete();
  renWin->Delete();
  //style->Delete();

  renderManager->Delete();
}

vtkPolyDataMapper* addFile(vtkRenderer* renderer, char* filename,
    vtkMultiProcessController* contr, double* boundingbox, arguments args)
{
  double diffuseColor[3] =
    {
    0.73, 0.9383, 0.2739
    };
  vtkPolyData* data;
  vtkPolyDataReader* reader;

  if (myId == 0)
    {
    // read in polydata file
    reader = vtkPolyDataReader::New();
    reader->SetFileName(filename);
    reader->Update();
    data = reader->GetOutput();

    int totalTriangles = data->GetNumberOfPolys();
    fprintf(outputptr, "total number of triangles is %i\n", totalTriangles);
    }
  else
    {
    data = vtkPolyData::New();
    }

  vtkMPICommunicator* comm = vtkMPICommunicator::SafeDownCast(
      contr->GetCommunicator());

  vtkTransmitPolyDataPiece* pass = vtkTransmitPolyDataPiece::New();
  pass->SetController(contr);
  if (myId == 0)
    {
    pass->SetInput(data);
    }
  else
    {
    pass->SetInput(data);
    }

  // rendering stuff
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(pass->GetOutputPort());
  mapper->ScalarVisibilityOff();

  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);

  vtkProperty* property = (vtkProperty*) actor->GetProperty();
  property->SetColor(diffuseColor);
  property->SetSpecularColor(0.5, 0.5, 0.5);
  property->SetSpecular(0.2);
  property->SetSpecularPower(100);

  renderer->AddActor(actor);

  // in order to correctly place the camera, need to find the bounding
  // box of the original, unsplit data. this can be done by passing it
  // to a mapper
  if (myId == 0)
    {
    vtkPolyDataMapper* mapper2 = vtkPolyDataMapper::New();
    mapper2->SetInput(data);
    mapper2->Update();
    mapper2->GetBounds(boundingbox);
    }

  return mapper;
}

void addLights(vtkRenderer* renderer)
{
  // light in front of camera
  vtkLight *light1 = vtkLight::New();
  light1->PositionalOn();
  light1->SetPosition(renderer->GetActiveCamera()->GetPosition());
  light1->SetFocalPoint(renderer->GetActiveCamera()->GetFocalPoint());
  light1->SetColor(0.7, 0.7, 0.7);
  light1->SetLightTypeToCameraLight();
  //renderer->SetLightFollowCamera(1);
  renderer->AddLight(light1);
}

double render(vtkRenderWindow* renWin)
{
  vtkTimerLog* timer = vtkTimerLog::New();
  timer->StartTimer();
  renWin->Render();
  timer->StopTimer();
  if (myId == 0)
    {
    fprintf(outputptr, "render time: %f\n", timer->GetElapsedTime());
    }
  timer->Delete();
  return timer->GetElapsedTime();
}

arguments parseArguments(int argc, char** argv)
{
  // parse the arguments

  // first set default values to arguments
  arguments args;
  args.xres = D_XRES;
  args.yres = D_YRES;
  args.input = D_INPUT;
  args.output = D_OUTPUT;
  args.numThreads = D_NUMTHREADS;
  args.numTriangles = D_NUMTRIANGLES;
  args.showHelp = D_SHOWHELP;

  // step through args and look for switches, set values when one is found
  for (int i = 0; i < argc; i++)
    {
    if (i < argc - 1)
      {
      // these guys need some kind of input
      if (strcmp("-xres", argv[i]) == 0)
        {
        args.xres = atoi(argv[i + 1]);
        }
      if (strcmp("-yres", argv[i]) == 0)
        {
        args.yres = atoi(argv[i + 1]);
        }
      if (strcmp("-input", argv[i]) == 0)
        {
        args.input = argv[i + 1];
        }
      if (strcmp("-output", argv[i]) == 0)
        {
        args.output = argv[i + 1];
        }
      if (strcmp("-np", argv[i]) == 0)
        {
        args.numThreads = atoi(argv[i + 1]);
        }
      if (strcmp("-numtriangles", argv[i]) == 0)
        {
        args.numTriangles = atoi(argv[i + 1]);
        }
      }
    if (strcmp("-help", argv[i]) == 0)
      {
      args.showHelp = true;
      }
    if (strcmp("-h", argv[i]) == 0)
      {
      args.showHelp = true;
      }
    }

  return args;
}

void displayHelp()
{
  printf("options:\n");
  printf("  -input <mesh filename>\n");
  printf("    filename of input mesh\n");
  printf("    must be a .vtk file\n");
  printf("  -output <output filename>\n");
  printf("    file to write out timings to\n");
  printf("    if not output is given, will goto standard out\n");
  printf("  -xres <number>\n");
  printf("    x-resolution, default is %i\n", D_XRES);
  printf("  -yres <number>\n");
  printf("    y-resolution, default is %i\n", D_YRES);
  printf("  -numtriangles <number>\n");
  printf("    number of triangles to process\n");
  printf("    default is to leave the mesh file unaffected\n");
  printf("  -h or -help\n");
  printf("    show this help message\n");
}
