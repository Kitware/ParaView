// this example show how to do parallel rendering using the
// distributed data filter to automatically partition the data
// and perform load balancing over the different threads.
// mpi is needed to run this example.
// and example usage:
// mpirun -np 4 ./ParallelRendering mesh.vtk


#include "vtkCompositeRenderManager.h"
#include "vtkTreeCompositer.h"
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
#include "vtkPoints.h"
#include "vtkTriangle.h"
#include "vtkInteractorStyleTrackballCamera.h"

static int NumProcs, myId;
double slate_grey[] =
  {
  0.4392, 0.5020, 0.5647
  };

static void Run(vtkMultiProcessController* contr, void* arg);
vtkUnstructuredGrid* polydataToUnstructuredGrid(vtkPolyData* polydata);

struct DDArgs_tmp
  {
  int* retVal;
  int argc;
  char** argv;
  }; 

int main(int argc, char** argv)
{
  if (argc < 2)
    {
    printf("usage: <mesh file>\n");
    return 0;
    }

  int retVal = 1;

  vtkMPIController* contr = vtkMPIController::New();
  contr->Initialize(&argc, &argv);

  vtkMultiProcessController::SetGlobalController(contr);

  NumProcs = contr->GetNumberOfProcesses();
  myId = contr->GetLocalProcessId();

  if (!contr->IsA("vtkMPIController"))
    {
    if (myId == 0)
      {
      printf("Error! This program requires MPI.\n");
      printf("Aborting program...\n");
      }
    contr->Delete();
    return -1;
    }

  DDArgs_tmp args;
  args.retVal = &retVal;
  args.argc = argc;
  args.argv = argv;

  contr->SetSingleMethod(Run, &args);
  contr->SingleMethodExecute();

  contr->Finalize();
  contr->Delete();

  return 1;
}

static void Run(vtkMultiProcessController* contr, void* arg)
{
  int go;
  DDArgs_tmp* args = reinterpret_cast<DDArgs_tmp*> (arg);
  vtkCompositeRenderManager* prm = vtkCompositeRenderManager::New();
  prm->SetCompositer(vtkTreeCompositer::New());

  // READER
  vtkUnstructuredGrid* ug = vtkUnstructuredGrid::New();
  vtkDataSet* ds = NULL;
  vtkPolyData* data;
  vtkPolyDataReader* reader = NULL;
  double boundingbox[6];

  if (myId == 0)
    {
    // read in polydata file
    reader = vtkPolyDataReader::New();
    reader->SetFileName(args->argv[1]);
    reader->Update();
    data = reader->GetOutput();
    data->GetWholeBoundingBox(boundingbox);

    // since the data distribution filter only works on unstructured
    // grid data, convert the polydata to unstructured grid
    ds = polydataToUnstructuredGrid(data);

    go = 1;
    }
  else
    {
    ds = (vtkDataSet*) ug;
    }

  vtkMPICommunicator* comm = vtkMPICommunicator::SafeDownCast(
      contr->GetCommunicator());

  comm->Broadcast(&go, 1, 0);

  if (!go)
    {
    ug->Delete();
    prm->Delete();
    return;
    }

  // data distribution filter
  vtkDistributedDataFilter* dd = vtkDistributedDataFilter::New();
  dd->SetInput(ds);
  dd->SetController(contr);
  dd->SetBoundaryModeToSplitBoundaryCells();
  dd->UseMinimalMemoryOff();

  // color by process number
  vtkPieceScalars* ps = vtkPieceScalars::New();
  ps->SetInputConnection(dd->GetOutputPort());
  ps->SetScalarModeToCellData();

  // more filtering - this will request ghost cells
  vtkDataSetSurfaceFilter* dss = vtkDataSetSurfaceFilter::New();
  dss->SetInputConnection(ps->GetOutputPort());

  // rendering stuff
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(dss->GetOutputPort());

  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("Piece");
  mapper->SetScalarRange(0, NumProcs - 1);

  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);

  vtkRenderer* renderer = prm->MakeRenderer();
  renderer->AddActor(actor);

  vtkRenderWindow* renWin = prm->MakeRenderWindow();
  renWin->AddRenderer(renderer);
  char title[50];
  sprintf(title, "I am node %i", myId);
  renWin->SetWindowName(title);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // set interaction it to trackball style
  vtkInteractorStyleTrackballCamera* style =
      vtkInteractorStyleTrackballCamera::New();
  iren->SetInteractorStyle(style);

  renderer->SetBackground(slate_grey);
  renWin->SetSize(400, 400);
  renWin->SetPosition(0, 460 * myId);

  prm->SetRenderWindow(renWin);
  prm->SetController(contr);

  // We must update the whole pipeline here, otherwise node 0
  // goes into GetActiveCamera which updates the pipeline, putting
  // it into vtkDistributedDataFilter::Execute() which then hangs.
  // If it executes here, dd will be up-to-date won't have to
  // execute in GetActiveCamera.

  mapper->SetPiece(myId);
  mapper->SetNumberOfPieces(NumProcs);
  mapper->Update();

  if (myId == 0)
    {
    // need to make another mapper that will get the true bounds of
    // the entire data, not just the first distributed piece
    vtkPolyDataMapper* mapper2 = vtkPolyDataMapper::New();
    mapper2->SetInputConnection(reader->GetOutputPort());
    mapper2->GetBounds(boundingbox);
    mapper2->Delete();

    renderer->ResetCamera(boundingbox);
    renWin->Render();
    }

  prm->StartInteractor();

  // clean up
  mapper->Delete();
  actor->Delete();
  renderer->Delete();
  renWin->Delete();
  style->Delete();

  dd->Delete();
  ug->Delete();

  ps->Delete();
  dss->Delete();

  prm->Delete();
}

vtkUnstructuredGrid* polydataToUnstructuredGrid(vtkPolyData* polydata)
{
  // given a polydata dataset, convert it to an unstructured grid
  vtkUnstructuredGrid* ug = vtkUnstructuredGrid::New();
  vtkCellArray* array = polydata->GetPolys();
  vtkIdTypeArray* idarray = array->GetData();

  // set the points
  ug->SetPoints(polydata->GetPoints());

  // create all the triangles
  ug->Allocate(1, 1);
  int j = 0;
  for (int i = 0; i < array->GetNumberOfCells(); i++)
    {
    // copy over the point id's for the next triangle
    j++;
    int id1 = idarray->GetValue(j);
    j++;
    int id2 = idarray->GetValue(j);
    j++;
    int id3 = idarray->GetValue(j);
    j++;

    vtkTriangle* tri = vtkTriangle::New();
    tri->GetPointIds()->SetId(0, id1);
    tri->GetPointIds()->SetId(1, id2);
    tri->GetPointIds()->SetId(2, id3);

    ug->InsertNextCell(tri->GetCellType(), tri->GetPointIds());
    }

  return ug;
}

