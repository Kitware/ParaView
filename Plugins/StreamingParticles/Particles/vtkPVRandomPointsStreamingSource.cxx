#include "vtkPVRandomPointsStreamingSource.h"

#include "vtkCellArray.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <cassert>

class vtkPVRandomPointsStreamingSource::vtkInternals
{
public:
  std::vector<int> GeneratedSeeds;
  vtkMinimalStandardRandomSequence* RNG;
};

vtkStandardNewMacro(vtkPVRandomPointsStreamingSource);

vtkPVRandomPointsStreamingSource::vtkPVRandomPointsStreamingSource()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->Internal = new vtkInternals;
  this->Internal->RNG = vtkMinimalStandardRandomSequence::New();
  this->NumLevels = 5;
  this->PointsPerBlock = 100;
  this->Seed = 1;
}

vtkPVRandomPointsStreamingSource::~vtkPVRandomPointsStreamingSource()
{
  this->Internal->RNG->Delete();
  delete this->Internal;
}

void vtkPVRandomPointsStreamingSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkPVRandomPointsStreamingSource::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  // tell the pipeline that this dataset is distributed
  outputVector->GetInformationObject(0)->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  // Initialize the datastructures for the metadata
  vtkSmartPointer<vtkMultiBlockDataSet> outline = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  outline->SetNumberOfBlocks(this->NumLevels);
  // Initialize the random seed used to generate the block seeds
  this->Internal->RNG->SetSeed(this->Seed);
  this->Internal->GeneratedSeeds.clear();

  // Initialize the block bounds and seeds
  for (int i = 0; i < this->NumLevels; ++i)
  {
    int blocksInLevel = 1 << (3 * i); // 8 ^ i
    vtkNew<vtkMultiBlockDataSet> ds;
    vtkNew<vtkMultiBlockDataSet> dsTmp;
    ds->SetNumberOfBlocks(blocksInLevel);
    outline->SetBlock(i, ds.GetPointer());
    for (int j = 0; j < blocksInLevel; ++j)
    {
      this->Internal->RNG->Next();
      this->Internal->GeneratedSeeds.push_back(this->Internal->RNG->GetSeed() * 49);
      int blocksPerSide = 1 << i;
      int xJump = blocksPerSide * blocksPerSide;
      int yJump = blocksPerSide;
      int x = j / xJump;
      int y = (j % xJump) / yJump;
      int z = j % yJump;
      double sideLen = 128.0 / blocksPerSide;
      double bounds[6];
      bounds[0] = sideLen * x;
      bounds[1] = bounds[0] + sideLen;
      bounds[2] = sideLen * y;
      bounds[3] = bounds[2] + sideLen;
      bounds[4] = sideLen * z;
      bounds[5] = bounds[4] + sideLen;
      vtkInformation* info = ds->GetMetaData(j);
      info->Set(vtkStreamingDemandDrivenPipeline::BOUNDS(), bounds, 6);
    }
  }
  outputVector->GetInformationObject(0)->Set(
    vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA(), outline);
  return 1;
}

int vtkPVRandomPointsStreamingSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  // Get the input and output objects
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector, 0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // make sure the output has the right structure
  output->SetNumberOfBlocks(this->NumLevels);
  for (int i = 0; i < this->NumLevels; ++i)
  {
    int blocksInLevel = 1 << (3 * i); // 8 ^ i
    vtkNew<vtkMultiBlockDataSet> ds;
    ds->SetNumberOfBlocks(blocksInLevel);
    output->SetBlock(i, ds.GetPointer());
  }

  // Find out which blocks have been requested
  int defaultIds[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
  int size = 9;
  int* ids = defaultIds;
  if (outInfo->Has(vtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS()))
  {
    size = outInfo->Length(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
    ids = outInfo->Get(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
  }
  // sort the requested ids
  std::sort(ids, ids + size);
  int offset = 0;
  int level = 0;

  // for each requested id, generate that block
  for (int i = 0; i < size; ++i)
  {
    // find out which level the block is on
    while (ids[i] >= offset + (1 << (3 * level)))
    {
      offset += 1 << (3 * level);
      ++level;
      assert(level <= this->NumLevels);
    }
    int blocksPerSide = 1 << level;
    int xJump = blocksPerSide * blocksPerSide;
    int yJump = blocksPerSide;
    int xBlock = (ids[i] - offset) / xJump;
    int yBlock = ((ids[i] - offset) % xJump) / yJump;
    int zBlock = (ids[i] - offset) % yJump;
    double sideLen = 128.0 / blocksPerSide;
    vtkNew<vtkPolyData> grid;
    grid->Initialize();
    vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(level))
      ->SetBlock(ids[i] - offset, grid.GetPointer());
    vtkNew<vtkPoints> points;
    grid->SetPoints(points.GetPointer());
    vtkNew<vtkCellArray> cells;
    // Generate the points
    this->Internal->RNG->SetSeed(this->Internal->GeneratedSeeds[ids[i]]);
    for (vtkIdType j = 0; j < this->PointsPerBlock; ++j)
    {
      double x = sideLen * (xBlock + this->Internal->RNG->GetValue());
      this->Internal->RNG->Next();
      double y = sideLen * (yBlock + this->Internal->RNG->GetValue());
      this->Internal->RNG->Next();
      double z = sideLen * (zBlock + this->Internal->RNG->GetValue());
      this->Internal->RNG->Next();
      points->InsertNextPoint(x, y, z);
      cells->InsertNextCell(1, &j);
    }
    grid->SetVerts(cells.GetPointer());
  }
  return 1;
}
