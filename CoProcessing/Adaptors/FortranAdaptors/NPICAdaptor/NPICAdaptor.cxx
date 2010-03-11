#include "FortranAdaptorAPI.h"
#include "vtkCellType.h"
#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkStdString.h"
#include "vtkUnstructuredGrid.h"

extern "C" void createstructuredgrid_(
  int *myid, int *xdim, int *ystart, int *ystop, double *xspc, double *yspc)
{
  if(!GetCoProcessorData())
    {
    vtkGenericWarningMacro("Unable to access CoProcessorData.");
    return;
    }

  /*
  vtkImageData* img = vtkImageData::New ();
  img->Initialize ();
  img->SetSpacing (*xspc, *yspc, 0.0);
  
  // Offsets account for a ghost point on either end in both directions
  // They also account for the 1 base in Fortran vs the 0 base here.
  img->SetExtent (-1, *xdim, ystart[*myid] - 2, ystop[*myid], 0, 0);

  img->SetOrigin (0.0, 0.0, 0.0);

  cerr << "Number of Points " << img->GetNumberOfPoints () << endl;
  cerr << "Number of Cells " << img->GetNumberOfCells () << endl;
  */

  vtkPoints *points = vtkPoints::New ();
  int ydim = ystop[*myid] - ystart[*myid] + 1;
  points->SetNumberOfPoints ((*xdim + 2) * (ydim + 2));
  vtkIdType index = 0;
  for (int y = ystart[*myid] - 2; y <= ystop[*myid]; y ++)
    {
    for (int x = -1; x <= *xdim; x ++) 
      {
      points->SetPoint (index ++, x * *xspc, y * *yspc, 0.0);
      }
    }
  vtkUnstructuredGrid *grid = vtkUnstructuredGrid::New ();
  // grid->DeepCopy (img);
  // img->Delete ();
  grid->SetPoints (points);
  points->Delete();

  for (int y = 0; y <= ydim; y ++)
    {
    for (int x = 0; x <= *xdim; x ++)
      {
      vtkIdType pts[4];
      pts[0] = y * (*xdim + 2) + x + 1;
      pts[1] = pts[0] - 1;
      pts[2] = pts[1] + (*xdim + 2);
      pts[3] = pts[2] + 1;
      grid->InsertNextCell (VTK_QUAD, 4, pts);
      }
    }

  GetCoProcessorData()->GetInputDescriptionByName("input")->SetGrid(grid);
  grid->Delete ();
}

extern "C" void add_scalar_(char *fname, int *len, double *data, int *size)
{
  vtkDoubleArray *arr = vtkDoubleArray::New ();
  vtkStdString name (fname, *len);
  arr->SetName (name);
  arr->SetNumberOfComponents (1);
  // arr->SetNumberOfTuples (*size);
  arr->SetArray (data, *size, 1);
  vtkUnstructuredGrid *grid = 
          vtkUnstructuredGrid::SafeDownCast (
            GetCoProcessorData()->GetInputDescriptionByName ("input")->GetGrid ());
  grid->GetPointData ()->AddArray (arr);
  arr->Delete ();
}

extern "C" void add_vector_(char *fname, int *len, double *data0, double *data1, double *data2, int *size)
{
  vtkDoubleArray *arr = vtkDoubleArray::New ();
  vtkStdString name (fname, *len);
  arr->SetName (name);
  arr->SetNumberOfComponents (3);
  arr->SetNumberOfTuples (*size);
  for (int i = 0; i < *size; i ++) 
    {
    arr->SetComponent (i, 0, data0[i]);
    arr->SetComponent (i, 1, data1[i]);
    arr->SetComponent (i, 2, data2[i]);
    }
  vtkUnstructuredGrid *grid = 
          vtkUnstructuredGrid::SafeDownCast (
            GetCoProcessorData()->GetInputDescriptionByName ("input")->GetGrid ());
  grid->GetPointData ()->AddArray (arr);
  arr->Delete ();
}

extern "C" void add_pressure_ (int *index, double *data, int *size)
{
  static char Pe[3] = "Pe";
  static char Pi[3] = "Pi";
  static int componentMap[6] = { 0, 3, 5, 1, 4, 2 };
  vtkUnstructuredGrid *grid = 
          vtkUnstructuredGrid::SafeDownCast (
            GetCoProcessorData()->GetInputDescriptionByName ("input")->GetGrid ());

  int real_index = *index - 24;
  char *name;
  if (real_index < 6)
    {
    name = Pe;
    }
  else 
    {
    name = Pi;
    real_index -= 6;
    }

  // reorder the tensor components to paraview's assumption
  real_index = componentMap[real_index];

  int ignore;
  vtkDoubleArray *arr = 
          vtkDoubleArray::SafeDownCast (
                          grid->GetPointData ()->GetArray (name, ignore));
  if (!arr)
    {
    arr = vtkDoubleArray::New ();
    arr->SetName (name);
    arr->SetNumberOfComponents (6);
    arr->SetNumberOfTuples (*size);
    grid->GetPointData ()->AddArray (arr);
    arr->Delete ();
    }

  for (int i = 0; i < *size; i ++) 
    {
    arr->SetComponent (i, real_index, data[i]);
    }
}
