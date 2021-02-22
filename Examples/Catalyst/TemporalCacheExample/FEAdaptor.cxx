#include "FEAdaptor.h"
#include "FEDataStructures.h"

#include <iostream>

#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkCPPythonPipeline.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkIntArray.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkSMSourceProxy.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTemporalDataSetCache.h>
#include <vtkTemporalStatistics.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLDataSetWriter.h>

namespace CPPPipeline
{
class vtkCPTestPipeline : public vtkCPPipeline
{
  // A sample C++ pipeline that incorporates a temporal filter
public:
  static vtkCPTestPipeline* New();
  vtkTypeMacro(vtkCPTestPipeline, vtkCPPipeline);

  int RequestDataDescription(vtkCPDataDescription* dataDescription) override
  {
    dataDescription->GetInputDescriptionByName("volume")->AllFieldsOn();
    dataDescription->GetInputDescriptionByName("volume")->GenerateMeshOn();
    return 1;
  }

  // Execute the pipeline. Returns 1 for success and 0 for failure.
  int CoProcess(vtkCPDataDescription* dataDescription) override
  {
    this->OutputCounter++;

    vtkCPInputDataDescription* idd = dataDescription->GetInputDescriptionByName("volume");
    if (!idd)
    {
      return 1;
    }

    vtkDataObject* gridNow = idd->GetGrid();
    auto dsw = vtkSmartPointer<vtkXMLDataSetWriter>::New();
    dsw->SetInputData(gridNow);
    // Output the volume at each timestep like you might do normally.
    std::string fname = "tcache_ex_time_" + std::to_string(this->OutputCounter) + ".vti";
    dsw->SetFileName(fname.c_str());
    dsw->Write();

    vtkCPInputDataDescription* idd2 = dataDescription->GetInputDescriptionByName("points");
    if (!idd2)
    {
      return 1;
    }
    vtkDataObject* pointsNow = idd2->GetGrid();
    dsw->SetInputData(pointsNow);
    // Ditto for the points at each timestep.
    fname = "tcache_ex_pts_time_" + std::to_string(this->OutputCounter) + ".vtu";
    dsw->SetFileName(fname.c_str());
    dsw->Write();

    vtkSMSourceProxy* pcache = idd->GetTemporalCache();
    // KEY POINT:
    // Get access to the cache
    vtkTemporalDataSetCache* cache =
      vtkTemporalDataSetCache::SafeDownCast(pcache->GetClientSideObject());
    if (!cache)
    {
      cerr << "Something is wrong, pipeline should have a temporal cache." << endl;
      return 1;
    }

    // The fun part do something across timesteps
    vtkInformation* info = cache->GetOutputInformation(0);
    if (info->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
    {
      double* tr = info->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
      // We will output a temporal statistics volume every 10 frames
      int tdumpcounter = this->OutputCounter / 10;
      if (!(this->OutputCounter % 10))
      {
        auto tstats = vtkSmartPointer<vtkTemporalStatistics>::New();
        // KEY POINT:
        // Use the cache as input to a processing pipeline
        tstats->SetInputConnection(cache->GetOutputPort());
        auto dsw = vtkSmartPointer<vtkXMLDataSetWriter>::New();
        dsw->SetInputConnection(tstats->GetOutputPort());
        std::string fname = "tcache_ex_tstats_" + std::to_string(tdumpcounter) + ".vti";
        dsw->SetFileName(fname.c_str());
        dsw->Write();
      }
    }

    return 1;
  }

protected:
  vtkCPTestPipeline() { this->OutputCounter = 0; }
  virtual ~vtkCPTestPipeline() {}
  int OutputCounter;

private:
  vtkCPTestPipeline(const vtkCPTestPipeline&) = delete;
  void operator=(const vtkCPTestPipeline&) = delete;
};

vtkStandardNewMacro(vtkCPTestPipeline);
}

//******************************************************************************
namespace
{
// Internal helpers used by the Adaptor itself
vtkCPProcessor* Processor = nullptr;
vtkImageData* VTKVolume = nullptr;
vtkUnstructuredGrid* VTKPoints = nullptr;

void BuildVTKVolume(Grid& grid)
{
  // The grid topological structure doesn't change so we just build
  // the first time it's needed.
  if (VTKVolume == nullptr)
  {
    VTKVolume = vtkImageData::New();
    int extent[6];
    for (int i = 0; i < 6; i++)
    {
      extent[i] = grid.GetExtent()[i];
    }
    VTKVolume->SetExtent(extent);
    VTKVolume->SetSpacing(grid.GetSpacing());
  }
  // The points however do vary, so let's remake them every time.
  if (VTKPoints)
    VTKPoints->Delete();
  VTKPoints = vtkUnstructuredGrid::New();
}

void UpdateVTKAttributes(Grid& grid, Attributes& attributes, vtkCPInputDataDescription* idd)
{
  if (idd->IsFieldNeeded("occupancy", vtkDataObject::CELL) == true)
  {
    if (VTKVolume->GetCellData()->GetNumberOfArrays() == 0)
    {
      // occupancy array, in other words how many spheres are present at the voxel
      vtkNew<vtkDoubleArray> occupancy;
      occupancy->SetName("occupancy");
      occupancy->SetNumberOfComponents(1);
      VTKVolume->GetCellData()->AddArray(occupancy);
    }
    vtkDoubleArray* occupancy =
      vtkDoubleArray::SafeDownCast(VTKVolume->GetCellData()->GetArray("occupancy"));
    // The occupancy array is a scalar array so we can reuse
    // memory as long as we ordered the points properly.
    double* occupancyData = attributes.GetOccupancyArray();
    occupancy->SetArray(occupancyData, static_cast<vtkIdType>(grid.GetNumberOfLocalCells()), 1);
    occupancy->Modified();
  }
  const std::vector<double>& pts = attributes.GetParticles();
  int numpts = pts.size() / 5;
  auto points = vtkSmartPointer<vtkPoints>::New();
  auto rads = vtkSmartPointer<vtkDoubleArray>::New();
  rads->SetName("radius");
  rads->SetNumberOfComponents(1);
  auto ids = vtkSmartPointer<vtkIntArray>::New();
  ids->SetName("pointid");
  ids->SetNumberOfComponents(1);
  for (int i = 0; i < numpts; i++)
  {
    points->InsertNextPoint(pts[i * 5 + 2], pts[i * 5 + 1], pts[i * 5 + 0]);
    rads->InsertNextValue(pts[i * 5 + 3]);
    ids->InsertNextValue(pts[i * 5 + 4]);
  }

  VTKPoints->SetPoints(points);
  VTKPoints->GetPointData()->AddArray(rads);
  VTKPoints->GetPointData()->AddArray(ids);
}

void BuildVTKDataStructures(Grid& grid, Attributes& attributes, vtkCPInputDataDescription* idd)
{
  BuildVTKVolume(grid);
  UpdateVTKAttributes(grid, attributes, idd);
}
}

namespace FEAdaptor
{
// The main adaptor proper : Initialize(), CoProcess() and Finalize()
void Initialize(int argc, char* argv[])
{
  std::string home = ".";
  int tcachesize = 100;
  bool enableCaching = true;
  bool enableCxxPipeline = false;

  int numScripts = 0;
  char** scripts = new char*[argc];
  for (int a = 0; a < argc; a++)
  {
    if (!strcmp(argv[a], "-HOME") && a < argc - 1)
    {
      home = std::string(argv[a + 1]);
      a += 1;
    }
    else if (!strcmp(argv[a], "-CACHESIZE") && a < argc - 1)
    {
      tcachesize = atoi(argv[a + 1]);
      a += 1;
    }
    else if (!strcmp(argv[a], "-ENABLECXXPIPELINE"))
    {
      enableCxxPipeline = true;
    }
    else if (!strcmp(argv[a], "-NOCACHING"))
    {
      enableCaching = false;
    }
    else
    {
      // pass unmatched arguments through as pythonscripts
      scripts[numScripts] = argv[a];
      numScripts++;
    }
  }
  if (!enableCaching)
  {
    tcachesize = 1;
  }

  if (enableCaching)
  {
    // KEY POINT:
    // If you want to use memkind features, you have to tell VTK where you want to map from.
    cout << "Extended memory is backed by " << home << endl;
    vtkObjectBase::SetMemkindDirectory(home.c_str());
  }

  if (Processor == nullptr)
  {
    Processor = vtkCPProcessor::New();
    // KEY POINT:
    // You need to tell the processor how big its temporal caches need to be
    Processor->SetTemporalCacheSize(tcachesize);
    Processor->Initialize();
    // KEY POINT:
    // You have to make a temporal cache for every output you want to temporally process
    if (enableCaching)
    {
      Processor->MakeTemporalCache("volume");
      Processor->MakeTemporalCache("points");
    }
  }
  else
  {
    Processor->RemoveAllPipelines();
  }
  // Python Pipelines
  for (int i = 0; i < numScripts; i++)
  {
    if (auto pipeline = vtkCPPythonPipeline::CreateAndInitializePipeline(scripts[i]))
    {
      Processor->AddPipeline(pipeline);
    }
    else
    {
      vtkLogF(ERROR, "failed to setup pipeline for '%s'", scripts[i]);
    }
  }
  // Optionally, the example C++ Pipeline too.
  if (enableCxxPipeline)
  {
    vtkNew<CPPPipeline::vtkCPTestPipeline> cpipeline;
    Processor->AddPipeline(cpipeline);
  }
  delete[] scripts;
}

void CoProcess(
  Grid& grid, Attributes& attributes, double time, unsigned int timeStep, bool lastTimeStep)
{
  vtkNew<vtkCPDataDescription> dataDescription;
  dataDescription->AddInput("volume");
  dataDescription->AddInput("points");
  dataDescription->SetTimeData(time, timeStep);
  dataDescription->ForceOutputOn();
  if (lastTimeStep == true)
  {
    // assume that we want to all the pipelines to execute if it
    // is the last time step.
    dataDescription->ForceOutputOn();
  }
  if (Processor->RequestDataDescription(dataDescription) != 0)
  {
    vtkCPInputDataDescription* idd = dataDescription->GetInputDescriptionByName("volume");
    BuildVTKDataStructures(grid, attributes, idd);
    idd->SetGrid(VTKVolume);
    int wholeExtent[6];
    for (int i = 0; i < 3; i++)
    {
      wholeExtent[2 * i] = 0;
      wholeExtent[2 * i + 1] = grid.GetNumPoints()[i];
    }

    idd->SetWholeExtent(wholeExtent);

    vtkSMSourceProxy* cache = Processor->GetTemporalCache("volume");
    if (cache)
    {
      // KEY POINT:
      // The adaptor has to associate the cache with the pipeline every timestep
      idd->SetTemporalCache(cache);
    }

    vtkCPInputDataDescription* idd2 = dataDescription->GetInputDescriptionByName("points");
    idd2->SetGrid(VTKPoints);
    vtkSMSourceProxy* cache2 = Processor->GetTemporalCache("points");
    if (cache2)
    {
      // KEY POINT:
      // The adaptor has to associate the cache with the pipeline every timestep
      idd2->SetTemporalCache(cache2);
    }
    Processor->CoProcess(dataDescription);
  }
}

void Finalize()
{
  if (Processor)
  {
    Processor->Delete();
    Processor = nullptr;
  }
  if (VTKVolume)
  {
    VTKVolume->Delete();
    VTKVolume = nullptr;
  }
  if (VTKPoints)
  {
    VTKPoints->Delete();
    VTKPoints = nullptr;
  }
}

} // end of Catalyst namespace
