#include "vtkMultiOutputPortReader.h"

#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cmath>
#include <fstream>
#include <sstream>

vtkStandardNewMacro(vtkMultiOutputPortReader);

namespace
{
const int SQUARE_RESOLUTION = 20; // 20x20 grid
const int CIRCLE_RESOLUTION = 30; // 30 radial x 60 angular points
const double PI = 3.14159265358979323846;
}

//------------------------------------------------------------------------------
vtkMultiOutputPortReader::vtkMultiOutputPortReader()
{
  this->FileName = nullptr;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
}

//------------------------------------------------------------------------------
vtkMultiOutputPortReader::~vtkMultiOutputPortReader()
{
  this->SetFileName(nullptr);
}

//------------------------------------------------------------------------------
void vtkMultiOutputPortReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
}

//------------------------------------------------------------------------------
int vtkMultiOutputPortReader::FillOutputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//------------------------------------------------------------------------------
int vtkMultiOutputPortReader::CanReadFile(const char* filename)
{
  if (!filename || !filename[0])
  {
    return 0;
  }

  // Check extension
  std::string fname(filename);
  const std::string ext = ".mopr";
  if (fname.size() < ext.size() || fname.substr(fname.size() - ext.size()) != ext)
  {
    return 0;
  }

  // Try to read the time value
  double time;
  return this->ReadTimeValue(filename, time) ? 1 : 0;
}

//------------------------------------------------------------------------------
bool vtkMultiOutputPortReader::ReadTimeValue(const char* filename, double& time)
{
  std::ifstream file(filename);
  if (!file.is_open())
  {
    return false;
  }

  file >> time;
  return !file.fail();
}

//------------------------------------------------------------------------------
int vtkMultiOutputPortReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (!this->FileName)
  {
    vtkErrorMacro("No filename specified");
    return 0;
  }

  double time;
  if (!this->ReadTimeValue(this->FileName, time))
  {
    vtkErrorMacro("Failed to read time from file: " << this->FileName);
    return 0;
  }

  // Report the time value for this file
  for (int port = 0; port < 2; ++port)
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(port);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &time, 1);
    double timeRange[2] = { time, time };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkMultiOutputPortReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (!this->FileName)
  {
    vtkErrorMacro("No filename specified");
    return 0;
  }

  double time;
  if (!this->ReadTimeValue(this->FileName, time))
  {
    vtkErrorMacro("Failed to read time from file: " << this->FileName);
    return 0;
  }

  // Generate square mesh for port 0
  vtkInformation* outInfo0 = outputVector->GetInformationObject(0);
  vtkPolyData* output0 = vtkPolyData::SafeDownCast(outInfo0->Get(vtkDataObject::DATA_OBJECT()));
  auto squareMesh = this->GenerateSquareMesh(time);
  output0->ShallowCopy(squareMesh);

  // Generate circle mesh for port 1
  vtkInformation* outInfo1 = outputVector->GetInformationObject(1);
  vtkPolyData* output1 = vtkPolyData::SafeDownCast(outInfo1->Get(vtkDataObject::DATA_OBJECT()));
  auto circleMesh = this->GenerateCircleMesh(time);
  output1->ShallowCopy(circleMesh);

  return 1;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkMultiOutputPortReader::GenerateSquareMesh(double time)
{
  auto polyData = vtkSmartPointer<vtkPolyData>::New();
  auto points = vtkSmartPointer<vtkPoints>::New();
  auto cells = vtkSmartPointer<vtkCellArray>::New();
  auto scalars = vtkSmartPointer<vtkDoubleArray>::New();

  scalars->SetName("SquareWave");
  scalars->SetNumberOfComponents(1);

  const int n = SQUARE_RESOLUTION;
  const double dx = 2.0 / (n - 1); // Square from -1 to 1

  // Create points and scalar values
  for (int j = 0; j < n; ++j)
  {
    for (int i = 0; i < n; ++i)
    {
      double x = -1.0 + i * dx;
      double y = -1.0 + j * dx;
      points->InsertNextPoint(x, y, 0.0);

      // f(t,x,y) = sin(2*pi*(x + y + t))
      double value = std::sin(2.0 * PI * (x + y + time));
      scalars->InsertNextValue(value);
    }
  }

  // Create quad cells
  for (int j = 0; j < n - 1; ++j)
  {
    for (int i = 0; i < n - 1; ++i)
    {
      vtkIdType quad[4];
      quad[0] = j * n + i;
      quad[1] = j * n + i + 1;
      quad[2] = (j + 1) * n + i + 1;
      quad[3] = (j + 1) * n + i;
      cells->InsertNextCell(4, quad);
    }
  }

  polyData->SetPoints(points);
  polyData->SetPolys(cells);
  polyData->GetPointData()->SetScalars(scalars);

  return polyData;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkMultiOutputPortReader::GenerateCircleMesh(double time)
{
  auto polyData = vtkSmartPointer<vtkPolyData>::New();
  auto points = vtkSmartPointer<vtkPoints>::New();
  auto cells = vtkSmartPointer<vtkCellArray>::New();
  auto scalars = vtkSmartPointer<vtkDoubleArray>::New();

  scalars->SetName("CircleWave");
  scalars->SetNumberOfComponents(1);

  const int nRadial = CIRCLE_RESOLUTION;
  const int nAngular = 2 * CIRCLE_RESOLUTION;
  const double dr = 1.0 / nRadial;
  const double dTheta = 2.0 * PI / nAngular;

  // Create center point
  points->InsertNextPoint(0.0, 0.0, 0.0);
  scalars->InsertNextValue(std::cos(2.0 * PI * time)); // r=0

  // Create ring points
  for (int ri = 1; ri <= nRadial; ++ri)
  {
    double r = ri * dr;
    for (int ai = 0; ai < nAngular; ++ai)
    {
      double theta = ai * dTheta;
      double x = r * std::cos(theta);
      double y = r * std::sin(theta);
      points->InsertNextPoint(x, y, 0.0);

      // f(t,x,y) = cos(2*pi*(r + t))
      double value = std::cos(2.0 * PI * (r + time));
      scalars->InsertNextValue(value);
    }
  }

  // Create triangles for center fan
  for (int ai = 0; ai < nAngular; ++ai)
  {
    vtkIdType tri[3];
    tri[0] = 0; // center
    tri[1] = 1 + ai;
    tri[2] = 1 + ((ai + 1) % nAngular);
    cells->InsertNextCell(3, tri);
  }

  // Create quads for rings
  for (int ri = 1; ri < nRadial; ++ri)
  {
    int innerStart = 1 + (ri - 1) * nAngular;
    int outerStart = 1 + ri * nAngular;
    for (int ai = 0; ai < nAngular; ++ai)
    {
      vtkIdType quad[4];
      quad[0] = innerStart + ai;
      quad[1] = outerStart + ai;
      quad[2] = outerStart + ((ai + 1) % nAngular);
      quad[3] = innerStart + ((ai + 1) % nAngular);
      cells->InsertNextCell(4, quad);
    }
  }

  polyData->SetPoints(points);
  polyData->SetPolys(cells);
  polyData->GetPointData()->SetScalars(scalars);

  return polyData;
}
