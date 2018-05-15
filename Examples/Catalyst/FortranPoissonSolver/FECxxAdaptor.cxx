// Adaptor for getting Fortran simulation code into ParaView Catalyst.

// CoProcessor specific headers
#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

// Fortran specific header
#include "vtkCPPythonAdaptorAPI.h"

// These will be called from the Fortran "glue" code"
// Completely dependent on data layout, structured vs. unstructured, etc.
// since VTK/ParaView uses different internal layouts for each.

// Creates the data container for the CoProcessor.
extern "C" void createcpimagedata_(int* dimensions, int* extent)
{
  if (!vtkCPPythonAdaptorAPI::GetCoProcessorData())
  {
    vtkGenericWarningMacro("Unable to access CoProcessorData.");
    return;
  }

  // The simulation grid is a 3-dimensional topologically and geometrically
  // regular grid. In VTK/ParaView, this is considered an image data set.
  vtkSmartPointer<vtkImageData> grid = vtkSmartPointer<vtkImageData>::New();

  grid->SetExtent(
    extent[0] - 1, extent[1] - 1, extent[2] - 1, extent[3] - 1, extent[4] - 1, extent[5] - 1);
  grid->SetSpacing(1. / (dimensions[0] - 1), 1. / (dimensions[1] - 1), 1. / (dimensions[2] - 1));

  // Name should be consistent between here, Fortran and Python client script.
  vtkCPPythonAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input")->SetGrid(grid);
  vtkCPPythonAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input")->SetWholeExtent(
    0, dimensions[0] - 1, 0, dimensions[1] - 1, 0, dimensions[2] - 1);
}

// Add field(s) to the data container.
// Separate from above because this will be dynamic, grid is static.
// By hand name mangling for fortran.
extern "C" void addfield_(double* scalars, char* name)
{
  vtkCPInputDataDescription* idd =
    vtkCPPythonAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input");

  vtkImageData* image = vtkImageData::SafeDownCast(idd->GetGrid());

  if (!image)
  {
    vtkGenericWarningMacro("No adaptor grid to attach field data to.");
    return;
  }

  // field name must match that in the fortran code.
  if (idd->IsFieldNeeded(name, vtkDataObject::POINT))
  {
    vtkSmartPointer<vtkDoubleArray> field = vtkSmartPointer<vtkDoubleArray>::New();
    field->SetName(name);
    field->SetArray(scalars, image->GetNumberOfPoints(), 1);
    image->GetPointData()->AddArray(field);
  }
}
