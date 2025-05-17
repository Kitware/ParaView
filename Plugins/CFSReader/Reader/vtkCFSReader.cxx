// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-FileCopyrightText: Copyright (c) 2022 Verein zur Foerderung der Software openCFS
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkBiQuadraticQuad.h>
#include <vtkCFSReader.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkErrorCode.h>
#include <vtkHexahedron.h>
#include <vtkInformation.h>
#include <vtkInformationStringKey.h>
#include <vtkInformationVector.h>
#include <vtkIntArray.h>
#include <vtkLine.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPyramid.h>
#include <vtkQuad.h>
#include <vtkQuadraticEdge.h>
#include <vtkQuadraticHexahedron.h>
#include <vtkQuadraticPyramid.h>
#include <vtkQuadraticQuad.h>
#include <vtkQuadraticTetra.h>
#include <vtkQuadraticTriangle.h>
#include <vtkQuadraticWedge.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTetra.h>
#include <vtkTriQuadraticHexahedron.h>
#include <vtkTriangle.h>
#include <vtkUnsignedIntArray.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVertex.h>
#include <vtkWedge.h>
#include <vtksys/SystemTools.hxx>

#include <list>
#include <sstream>

vtkStandardNewMacro(vtkCFSReader);

// Initialize Information Keys
vtkInformationKeyMacro(vtkCFSReader, CFS_RESULT_NAME, String);
vtkInformationKeyMacro(vtkCFSReader, CFS_DOF_NAMES, StringVector);
vtkInformationKeyMacro(vtkCFSReader, CFS_DEFINED_ON, Integer);
vtkInformationKeyMacro(vtkCFSReader, CFS_ENTITY_NAME, String);
vtkInformationKeyMacro(vtkCFSReader, CFS_ENTRY_TYPE, Integer);
vtkInformationKeyMacro(vtkCFSReader, CFS_STEP_NUMS, IntegerVector);
vtkInformationKeyMacro(vtkCFSReader, CFS_STEP_VALUES, DoubleVector);
vtkInformationKeyMacro(vtkCFSReader, CFS_UNIT, String);
vtkInformationKeyMacro(vtkCFSReader, CFS_ENTITY_IDS, StringVector);
vtkInformationKeyMacro(vtkCFSReader, CFS_MULTI_SEQ_INDEX, Integer);
vtkInformationKeyMacro(vtkCFSReader, CFS_ANALYSIS_TYPE, Integer);

//-----------------------------------------------------------------------------
vtkCFSReader::vtkCFSReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkCFSReader::~vtkCFSReader()
{
  if (this->MBDataSet)
  {
    this->MBDataSet->Delete();
  }
  if (this->MBActiveDataSet)
  {
    this->MBActiveDataSet->Delete();
  }
}

//-----------------------------------------------------------------------------
// taken from vtkSetStringMacro and enhanced to reset history result info
void vtkCFSReader::SetFileName(const char* arg)
{
  // do nothing if nothing changes
  if (this->FileName.empty() && arg == nullptr)
  {
    return;
  }
  if (!this->FileName.empty() && arg != nullptr && strcmp(this->FileName.c_str(), arg) == 0)
  {
    return;
  }

  // something changed
  this->Reader.CloseFile();

  if (arg != nullptr)
  {
    this->FileName.assign(arg);
  }
  else
  {
    this->FileName.clear();
  }
  this->Modified();
  this->Hdf5InfoRead = false; // re-initialize info on history results
}

//-----------------------------------------------------------------------------
const char* vtkCFSReader::GetFileName()
{
  if (this->FileName.empty())
  {
    return nullptr;
  }
  else
  {
    return this->FileName.c_str();
  }
}

//-----------------------------------------------------------------------------
void vtkCFSReader::CloseFile()
{
  this->Reader.CloseFile();

  this->FileName.clear();
}

//-----------------------------------------------------------------------------
void vtkCFSReader::SetMultiSequenceStep(int step)
{
  if (step == static_cast<int>(this->MultiSequenceStep))
  {
    return;
  }
  if (step > this->MultiSequenceRange[1] || step < this->MultiSequenceRange[0])
  {
    vtkErrorMacro(<< "Please enter a valid multisequence step between "
                  << this->MultiSequenceRange[0] << " and " << this->MultiSequenceRange[1] << "!"
                  << "selected value was " << step << "\n");
  }

  this->MultiSequenceStep = step;
  this->MSStepChanged = true;

  // In addition trigger resetting the data value arrays, as the results in
  // the new msStep can be different than from the previous one.
  this->ResetDataArrays = true;

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCFSReader::SetTimeStep(unsigned int step)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting TimeStep to " << step);

  // security check: immediately leave, if no time / frequency steps are available
  if (this->StepVals.empty())
  {
    return;
  }

  step++;

  if (this->TimeStep != step && this->StepVals.size() >= step)
  {
    this->TimeStep = step;
    this->TimeOrFrequencyValue = this->StepVals[step - 1];
    this->TimeOrFrequencyValueStr = std::to_string(this->TimeOrFrequencyValue);

    // update pipeline
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
const char* vtkCFSReader::GetTimeOrFrequencyValueStr()
{
  return (!this->HarmonicData || this->HarmonicDataAsModeShape == 0)
    ? "-"
    : this->TimeOrFrequencyValueStr.c_str();
}

//-----------------------------------------------------------------------------
void vtkCFSReader::SetAddDimensionsToArrayNames(int flag)
{
  this->AddDimensionsToArrayNames = flag;

  // In addition trigger resetting the data value arrays
  this->ResetDataArrays = true;

  // update pipeline
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCFSReader::SetHarmonicDataAsModeShape(int flag)
{
  this->HarmonicDataAsModeShape = flag;

  if (flag == 1 && this->HarmonicData)
  {
    this->TimeStepNumberRange[1] = static_cast<int>(this->NumberOfTimeSteps - 1);
  }
  else
  {
    this->TimeStepNumberRange[1] = 0;
  }

  // In addition trigger resetting the data value arrays
  this->ResetDataArrays = true;

  // update pipeline
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCFSReader::SetFillMissingResults(int flag)
{
  this->FillMissingResults = flag;

  // In addition trigger resetting the data value arrays
  this->ResetDataArrays = true;

  // update pipeline
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkCFSReader::GetRegionArrayStatus(const char* name)
{
  assert(name != nullptr);
  std::map<std::string, int>::const_iterator it = this->RegionSwitch.find(name);
  if (it != this->RegionSwitch.end())
  {
    return it->second;
  }
  vtkErrorMacro(<< "Region '" << name << "' not found.");
  return 0;
}

//-----------------------------------------------------------------------------
void vtkCFSReader::SetRegionArrayStatus(const char* name, int status)
{
  assert(name != nullptr);
  this->RegionSwitch[name] = status;

  // IMPORTANT: Let VTK know, that the pipeline needs an update
  // (otherwise nothing would happen)
  this->Modified();

  // In addition, set flag to false, that regions have to be updated
  this->ActiveRegionsChanged = true;
}

//-----------------------------------------------------------------------------
const char* vtkCFSReader::GetRegionArrayName(int index)
{
  // we assume the vector is properly traversed
  assert(index < static_cast<int>(this->RegionNames.size()));
  return this->RegionNames[index].c_str();
}

//-----------------------------------------------------------------------------
int vtkCFSReader::GetNamedNodeArrayStatus(const char* name)
{
  assert(name != nullptr);
  std::map<std::string, int>::const_iterator it = this->NamedNodeSwitch.find(name);
  if (it != this->NamedNodeSwitch.end())
  {
    return it->second;
  }
  vtkErrorMacro(<< "Named nodes '" << name << "' not found.");
  return 0;
}

//-----------------------------------------------------------------------------
void vtkCFSReader::SetNamedNodeArrayStatus(const char* name, int status)
{
  assert(name != nullptr);
  this->NamedNodeSwitch[name] = status;
  this->Modified();
  // In addition, set flag to false, that regions have to be updated
  this->ActiveRegionsChanged = true;
}

//-----------------------------------------------------------------------------
const char* vtkCFSReader::GetNamedNodeArrayName(int index)
{
  assert(index < static_cast<int>(this->NamedNodeNames.size()));
  return this->NamedNodeNames[index].c_str();
}

//-----------------------------------------------------------------------------
int vtkCFSReader::GetNamedElementArrayStatus(const char* name)
{
  assert(name != nullptr);
  std::map<std::string, int>::const_iterator it = this->NamedElementSwitch.find(name);
  if (it != this->NamedElementSwitch.end())
  {
    return it->second;
  }
  vtkErrorMacro(<< "Named elems '" << name << "' not found.");
  return 0;
}

//-----------------------------------------------------------------------------
void vtkCFSReader::SetNamedElementArrayStatus(const char* name, int status)
{
  assert(name != nullptr);
  this->NamedElementSwitch[name] = status;
  this->Modified();
  // In addition, set flag to false, that regions have to be updated
  this->ActiveRegionsChanged = true;
}

//-----------------------------------------------------------------------------
const char* vtkCFSReader::GetNamedElementArrayName(int index)
{
  assert(index < static_cast<int>(this->NamedElementNames.size()));
  return this->NamedElementNames[index].c_str();
}

//-----------------------------------------------------------------------------
int vtkCFSReader::CanReadFile(const char* fname)
{
  if (fname == nullptr)
  {
    return 0;
  }

  // Try to "load" the file (which internally just opens the mesh group)
  // to see, if this is a true CFS HDF5 file.
  std::string fileName = std::string(fname);
  H5CFS::Hdf5Reader temp;
  try
  {
    temp.LoadFile(fileName);
    // If this succeeds, we assume that this is a true CFS HDF5 file.
    return 1;
  }
  catch (...)
  {
    return 0;
  }
}

//-----------------------------------------------------------------------------
void vtkCFSReader::ReadHdf5Informations()
{
  const int enableGroups = 0;

  // Load hdf file (only done first time)
  if (!this->Hdf5InfoRead)
  {
    try
    {
      this->Reader.LoadFile(this->FileName);
      this->Dimension = static_cast<int>(this->Reader.GetDimension());
      this->GridOrder = static_cast<int>(this->Reader.GetGridOrder());
      this->RegionNames = this->Reader.GetAllRegionNames();
      this->NamedNodeNames = this->Reader.GetNodeNames();
      this->NamedElementNames = this->Reader.GetElementNames();

      for (auto& name : this->RegionNames)
      {
        auto it = this->RegionSwitch.find(name);
        if (it == this->RegionSwitch.end())
        {
          this->RegionSwitch[name] = 1;
        }
      }

      for (auto& name : this->NamedNodeNames)
      {
        auto it = this->NamedNodeSwitch.find(name);
        if (it == this->NamedNodeSwitch.end())
        {
          this->NamedNodeSwitch[name] = enableGroups;
        }
      }

      for (auto& name : this->NamedElementNames)
      {
        auto it = this->NamedElementSwitch.find(name);
        if (it == this->NamedElementSwitch.end())
        {
          this->NamedElementSwitch[name] = enableGroups;
        }
      }

      // store info on multisequence steps with mesh results
      std::map<unsigned int, unsigned int> numSteps;
      this->Reader.GetNumberOfMultiSequenceSteps(this->ResAnalysisType, numSteps, false);

      // Get available mesh results
      for (auto msIt : this->ResAnalysisType)
      {
        this->Reader.GetResultTypes(msIt.first, this->MeshResultInfos[msIt.first], false);
      }

      // store info on multisequence steps for history results
      this->Reader.GetNumberOfMultiSequenceSteps(this->HistAnalysisType, numSteps, true);

      // Get available history results
      for (auto msIt : this->HistAnalysisType)
      {
        this->Reader.GetResultTypes(msIt.first, this->HistResultInfos[msIt.first], true);
      }
    }
    catch (const std::runtime_error& er)
    {
      vtkErrorMacro(<< "Caught exception when trying to read hdf5 info from file '"
                    << this->FileName << "' : '" << er.what() << "'");
      return;
    }

    this->Hdf5InfoRead = true;
    this->MSStepChanged = true;
    this->IsInitialized = false;
  } // info red
}

//-----------------------------------------------------------------------------
int vtkCFSReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // Read basic information from HDF5
  this->ReadHdf5Informations();
  if (!this->Hdf5InfoRead)
  {
    return 0;
  }

  try
  {
    // Check for new multisequence step
    if (this->MSStepChanged)
    {
      // Request number of multiSequenceSteps

      // adjust range for
      if (!this->ResAnalysisType.empty())
      {
        this->MultiSequenceRange[0] = static_cast<int>(this->ResAnalysisType.begin()->first);
        this->MultiSequenceRange[1] = static_cast<int>(this->ResAnalysisType.rbegin()->first);

        // check, if any result at all are present
        // Run over all result types
        std::map<unsigned int, double> globSteps;

        if (this->ResAnalysisType.find(this->MultiSequenceStep) != this->ResAnalysisType.end())
        {
          // store current analysis type
          this->AnalysisType = this->ResAnalysisType[this->MultiSequenceStep];
          this->HarmonicData = (this->AnalysisType == H5CFS::HARMONIC ||
            this->AnalysisType == H5CFS::EIGENVALUE || this->AnalysisType == H5CFS::EIGENFREQUENCY);

          for (auto& mri : MeshResultInfos[this->MultiSequenceStep])
          {
            std::map<unsigned int, double> actStepVals;
            this->Reader.GetStepValues(this->MultiSequenceStep, mri->name, actStepVals, false);
            std::map<unsigned int, double>::const_iterator it;
            for (it = actStepVals.begin(); it != actStepVals.end(); it++)
            {
              globSteps[it->first] = it->second;
            }
          }
        }

        this->StepNumbers.resize(globSteps.size());
        this->StepVals.resize(globSteps.size());
        auto itStep = globSteps.begin();
        for (unsigned int i = 0; itStep != globSteps.end(); ++itStep, ++i)
        {
          this->StepNumbers[i] = itStep->first;
          this->StepVals[i] = itStep->second;
        }
      }
      this->NumberOfTimeSteps = static_cast<unsigned int>(this->StepNumbers.size());
      if (this->HarmonicData && this->HarmonicDataAsModeShape != 0)
      {
        this->TimeStepNumberRange[1] = static_cast<int>(this->NumberOfTimeSteps - 1);
      }
      else
      {
        this->TimeStepNumberRange[1] = 0;
      }
      if (this->NumberOfTimeSteps != 0)
      {
        this->TimeStepValuesRange[0] = this->StepVals[0];
        this->TimeStepValuesRange[1] = this->StepVals[NumberOfTimeSteps - 1];
      }

      this->MSStepChanged = false;
    } // if multisequence step has changed
  }
  catch (const std::runtime_error& er)
  {
    vtkErrorMacro(<< "Caught exception when trying to request information from file '"
                  << this->FileName << "' : '" << er.what() << "'");
    return 0;
  }

  if (this->NumberOfTimeSteps > 0)
  {
    if (this->HarmonicData && this->HarmonicDataAsModeShape != 0)
    {
      // From harmonic data we can produce a continuous time data set which ranges from 0 to one
      // All other kinds of data are discrete in time/freq.
      this->TimeStepValuesRange[0] = 0;
      this->TimeStepValuesRange[1] = 1.0;

      outputVector->GetInformationObject(0)->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    }
    else
    {
      outputVector->GetInformationObject(0)->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
        &(this->StepVals[0]), static_cast<int>(this->NumberOfTimeSteps));
    }

    outputVector->GetInformationObject(0)->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
      static_cast<double*>(this->TimeStepValuesRange), 2);
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkCFSReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // obtain detail for what data is actually requested
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outInfo);

  // ===================
  // Time Stepping Part
  // ===================
  // Attention: Only change value of pipeline / step values, if
  // there are any results present at all.
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) != 0 &&
    !this->StepVals.empty())
  {
    RequestedTimeValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    int nSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double* rsteps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int timeI = 0;
    while (timeI < nSteps - 1 && rsteps[timeI] < this->RequestedTimeValue)
    {
      timeI++;
    }

    if (this->HarmonicData && this->HarmonicDataAsModeShape > 0)
    {
      outInfo->Set(vtkDataObject::DATA_TIME_STEP(), this->RequestedTimeValue);
    }
    else
    {
      outInfo->Set(vtkDataObject::DATA_TIME_STEP(), this->StepVals[timeI]);
      this->TimeStep = timeI + 1;
      this->TimeOrFrequencyValue = this->StepVals[timeI];
      this->TimeOrFrequencyValueStr = std::to_string(this->TimeOrFrequencyValue);
    }
  }

  // Read in data from file
  this->ReadFile(output);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkCFSReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  // vtkGetStringMacro
  os << indent << "File Name: " << this->FileName << "\n";

  // vtkGetMacro of interest
  os << indent << "Multi Sequence Step: " << this->GetMultiSequenceStep() << "\n";
  os << indent << "Time Step: " << this->GetTimeStep() << "\n";
  os << indent << "Complex Model Real" << this->GetComplexModeReal() << "\n";
  os << indent << "Complex Model Imaginary" << this->GetComplexModeImaginary() << "\n";
  os << indent << "Complex Model Amplitude" << this->GetComplexModeAmplitude() << "\n";
  os << indent << "Complex Model Phase" << this->GetComplexModePhase() << "\n";

  // further properties of interest
  os << indent << "Num Steps: " << this->GetNumberOfSteps() << "\n";
  os << indent << "Time Freq: " << this->GetTimeOrFrequencyValueStr() << "\n";
  os << indent << "Grid Dimensions: " << this->GetGridDimension() << "\n";
  os << indent << "Grid Order: " << this->GetGridOrder() << "\n";
  os << indent << "Number Region Arrays: " << this->GetNumberOfRegionArrays() << "\n";
  os << indent << "Number Named Node Arrays: " << this->GetNumberOfNamedNodeArrays() << "\n";
  os << indent << "Number Named Element Arrays: " << this->GetNumberOfNamedElementArrays() << "\n";
}

//----------------------------------------------------------------------------
void vtkCFSReader::ReadFile(vtkMultiBlockDataSet* output)
{
  try
  {
    // The first time, the reader is initialized, we have to
    // fill the  multi block data set
    if (!this->IsInitialized)
    {
      this->MBDataSet = vtkMultiBlockDataSet::New();
      this->MBDataSet->ShallowCopy(output);

      // Assemble number of blocks, which is
      // #regions, #named elemens, #named nodes
      unsigned int numBlocks = static_cast<unsigned int>(
        this->RegionNames.size() + this->NamedElementNames.size() + this->NamedNodeNames.size());

      for (unsigned int i = 0, n = numBlocks; i < n; i++)
      {
        vtkNew<vtkUnstructuredGrid> newGrid;
        this->MBDataSet->SetBlock(i, newGrid.GetPointer());
      }

      // initialize mapping from
      // (region,globalNodeNumber)->regionLocalNodeNumber
      this->NodeMap.resize(numBlocks);

      // read nodal connectivity
      this->ReadNodes(this->MBDataSet);

      // read element definition
      this->ReadCells(this->MBDataSet);

      // Now the grid is finalized and we store it initially also
      // to the activeSet
      this->MBActiveDataSet = vtkMultiBlockDataSet::New();
      this->MBActiveDataSet->ShallowCopy(this->MBDataSet);

      // now it should be definitely initialized
      this->IsInitialized = true;
    }

    // If region status has changed, we have to perform an update
    // of the active regions
    if (this->ActiveRegionsChanged || this->ResetDataArrays)
    {
      this->UpdateActiveRegions();
    }

    // only read data, if time/frequency steps are defined
    if (this->NumberOfTimeSteps > 0)
    {
      // read nodal data
      this->ReadNodeCellData(this->MBActiveDataSet, true);

      // read cell data
      this->ReadNodeCellData(this->MBActiveDataSet, false);
    }
    // Use the active set as base for the current set
    output->ShallowCopy(this->MBActiveDataSet);
  }
  catch (const std::runtime_error& er)
  {
    vtkErrorMacro(<< "Caught exception when trying read file '" << this->FileName << "' : '"
                  << er.what() << "'");
  }
}

//-----------------------------------------------------------------------------
void vtkCFSReader::UpdateActiveRegions()
{
  // Idea:
  // Create new multiblock data set, based on the internal stored one and copy only those
  // grid-blocks, which are active.
  vtkNew<vtkMultiBlockDataSet> newSet;

  //  Combine region and group info
  unsigned int numBlocks = static_cast<unsigned int>(
    this->RegionNames.size() + this->NamedElementNames.size() + this->NamedNodeNames.size());
  std::vector<std::string> names(numBlocks);
  std::map<std::string, int> switches;
  for (unsigned int iBlock = 0; iBlock < numBlocks; ++iBlock)
  {
    if (iBlock < this->RegionNames.size())
    {
      std::string name = this->RegionNames[iBlock];
      names[iBlock] = name;
      switches[name] = this->RegionSwitch[name];
    }
    else if (iBlock < this->RegionNames.size() + this->NamedElementNames.size())
    {
      unsigned int index = static_cast<unsigned int>(iBlock - this->RegionNames.size());
      std::string name = this->NamedElementNames[index];
      names[iBlock] = name;
      switches[name] = this->NamedElementSwitch[name];
    }
    else
    {
      unsigned int index = static_cast<unsigned int>(
        iBlock - this->RegionNames.size() - this->NamedElementNames.size());
      std::string name = this->NamedNodeNames[index];
      names[iBlock] = name;
      switches[name] = this->NamedNodeSwitch[name];
    }
  }

  //  Run over all active regions / element groups and copy grid
  unsigned int index = 0;
  for (unsigned int i = 0; i < numBlocks; i++)
  {
    // check, if region is active
    if (switches[names[i]] != 0)
    {
      // create new grid with output and make shallow copy
      vtkNew<vtkUnstructuredGrid> newGrid;
      newGrid->ShallowCopy(this->MBDataSet->GetBlock(i));

      // try to find the corresponding block of this region/group in the current
      // active set in order to copy its data to this block.
      unsigned int numActBlocks = this->MBActiveDataSet->GetNumberOfBlocks();
      unsigned int activeIndex = 0;
      bool found = false;
      for (unsigned int j = 0; j < numActBlocks; ++j)
      {
        const char* blockName =
          this->MBActiveDataSet->GetMetaData(j)->Get(vtkCompositeDataSet::NAME());
        if (blockName != nullptr)
        {
          if (std::string(blockName) == names[i])
          {
            found = true;
            activeIndex = j;
          }
        }
      }

      if (found && !this->ResetDataArrays)
      {
        vtkUnstructuredGrid* actGrid =
          vtkUnstructuredGrid::SafeDownCast(this->MBActiveDataSet->GetBlock(activeIndex));
        newGrid->GetPointData()->ShallowCopy(actGrid->GetPointData());
        newGrid->GetCellData()->ShallowCopy(actGrid->GetCellData());
      }

      // Get the array containing the actual node numbers for the current region
      newGrid->GetPointData()->SetActiveScalars("origNodeNums");
      vtkUnsignedIntArray::SafeDownCast(newGrid->GetPointData()->GetScalars());
      newGrid->GetPointData()->SetActiveScalars("");

      newSet->SetBlock(index, newGrid);

      // Tag new block with region name
      newSet->GetMetaData(index)->Set(vtkCompositeDataSet::NAME(), names[i].c_str());
      index++;
    } // if block is active
  }   // loop: all blocks (= regions, elem/node groups)

  // in the end, replace the active set by the current one
  this->MBActiveDataSet->ShallowCopy(newSet.GetPointer());

  // in the end, re-set the status of the update flag
  this->ActiveRegionsChanged = false;
  this->ResetDataArrays = false;
}

//----------------------------------------------------------------------------
void vtkCFSReader::ReadNodeCellData(vtkMultiBlockDataSet* output, bool isNode)
{
  H5CFS::EntityType entType = isNode ? H5CFS::NODE : H5CFS::ELEMENT;
  const double pi = vtkMath::Pi();
  double h180degOverPi = 180.0 / pi;

  //  Combine region and group info
  unsigned int numBlocks = static_cast<unsigned int>(
    this->RegionNames.size() + this->NamedElementNames.size() + this->NamedNodeNames.size());
  std::vector<std::string> names(numBlocks);
  std::map<std::string, int> switches;
  for (unsigned int iBlock = 0; iBlock < numBlocks; ++iBlock)
  {
    if (iBlock < this->RegionNames.size())
    {
      std::string name = this->RegionNames[iBlock];
      names[iBlock] = name;
      switches[name] = this->RegionSwitch[name];
    }
    else if (iBlock < this->RegionNames.size() + this->NamedElementNames.size())
    {
      unsigned int index = static_cast<unsigned int>(iBlock - this->RegionNames.size());
      std::string name = this->NamedElementNames[index];
      names[iBlock] = name;
      switches[name] = this->NamedElementSwitch[name];
    }
    else
    {
      unsigned int index = static_cast<unsigned int>(
        iBlock - this->RegionNames.size() - this->NamedElementNames.size());
      std::string name = this->NamedNodeNames[index];
      names[iBlock] = name;
      switches[name] = this->NamedNodeSwitch[name];
    }
  }

  try
  {
    if (MeshResultInfos.find(this->MultiSequenceStep) == this->MeshResultInfos.end())
    {
      vtkWarningMacro(
        "Multisequence step index " + std::to_string(this->MultiSequenceStep) + " is invalid.");
      return;
    }

    // get result infos for 1ms step and step 1

    // Read all result types occuring in the current multisequence step
    std::set<std::string> resultNames;
    std::map<std::string, std::shared_ptr<H5CFS::ResultInfo>> nameInfo;
    for (const auto& mri : this->MeshResultInfos[this->MultiSequenceStep])
    {
      std::string& resultName = mri->name;
      resultNames.insert(resultName);
      nameInfo[resultName] = mri;
    }

    std::shared_ptr<H5CFS::Result> result;
    //  Loop over all result names
    for (const auto& resultName : resultNames)
    {
      // Ensure, that in the nodal case we only pick nodal results.
      // In the case of element results, we accept elements and surface elements
      if ((isNode && (nameInfo[resultName]->listType != H5CFS::NODE)) ||
        (!isNode &&
          (nameInfo[resultName]->listType != H5CFS::ELEMENT &&
            nameInfo[resultName]->listType != H5CFS::SURF_ELEM)))
      {
        continue;
      }

      //  Loop over all active blocks
      unsigned int blockIndex = 0;
      for (unsigned int iBlock = 0; iBlock < numBlocks; iBlock++)
      {
        // variable used to write the result
        std::string writeName = resultName;

        // check, if block is active. If not, just leave
        if (switches[names[iBlock]] == 0)
        {
          continue;
        }

        // get grip of grid of current block
        vtkUnstructuredGrid* actGrid =
          vtkUnstructuredGrid::SafeDownCast(output->GetBlock(blockIndex++));

        //  Find resultinfo for this result and this block
        bool found = false;
        std::shared_ptr<H5CFS::ResultInfo> actInfo;
        for (auto& mri : this->MeshResultInfos[this->MultiSequenceStep])
        {
          if (mri->name == resultName && mri->listName == names[iBlock] && mri->listType == entType)
          {
            actInfo = mri;
            found = true;
            break;
          }
        }

        // if result for this block was not found and we should not generate
        // "empty" datasets for missing result, we just leave
        if (!found)
        {
          if (this->FillMissingResults != 0)
          {
            actInfo = nameInfo[resultName];
          }
          else
          {
            continue;
          }
        }

        std::vector<unsigned int> entities = this->Reader.GetEntities(entType, names[iBlock]);
        unsigned int numEntities = static_cast<unsigned int>(entities.size());
        unsigned int numDofs = static_cast<unsigned int>(actInfo->dofNames.size());
        std::vector<double>* ptRealVals = nullptr;
        std::vector<double>* ptImaginaryVals = nullptr;
        bool isComplex = false;
        std::vector<double> dummyVals;
        if (found)
        {
          result = std::make_shared<H5CFS::Result>(H5CFS::Result());
          result->resultInfo = actInfo;
          try
          {
            this->Reader.GetMeshResult(
              this->MultiSequenceStep, this->StepNumbers[this->TimeStep - 1], result.get());
          }
          catch (...)
          {
            // If we are here, the current result is not defined on the current
            // step and we should NOT fill with empty results. In this case
            // we perform the following:
            // a) if the current time/freq step is smaller than the first
            //    one the result is defined, we update to the first defined step
            // b) otherwise we simply return and keep the result from the
            //    previous steps
            std::map<unsigned int, double> steps;
            this->Reader.GetStepValues(this->MultiSequenceStep, actInfo->name, steps, false);
            if (this->StepNumbers[this->TimeStep - 1] < steps.begin()->first)
            {
              this->Reader.GetMeshResult(
                this->MultiSequenceStep, steps.begin()->first, result.get());
            }
            else
            {
              // in this case we are in the middle of the steps and just keep
              // the old value
              break;
            }
          }
          isComplex = result->isComplex;
          ptRealVals = &(result->realVals);
          ptImaginaryVals = &(result->imaginaryVals);
        }
        else
        {
          dummyVals.resize(numEntities * numDofs);
          ptRealVals = &(dummyVals);
          ptImaginaryVals = &(dummyVals);
          // NOTE: we have to find a different strategy how to
          // find out about complex valued results
          isComplex = (this->AnalysisType == H5CFS::HARMONIC);
        }

        std::vector<double>& realVals = *(ptRealVals);

        std::vector<vtkDoubleArray*> vals;

        // REAL-VALUED
        if (!isComplex)
        {
          if (!(this->HarmonicData && this->HarmonicDataAsModeShape != 0))
          {
            if (this->AddDimensionsToArrayNames != 0)
            {
              writeName += " [";
              writeName += actInfo->unit;
              writeName += "]";
            }
            vals.push_back(SaveToArray(realVals, actInfo->dofNames, numEntities, writeName));
          }
          else
          { // We obviously have a real-valued eigenvalue solution
            unsigned int numEntries = static_cast<unsigned int>(realVals.size());

            writeName = actInfo->name;
            writeName += "-mode";
            if (this->AddDimensionsToArrayNames != 0)
            {
              writeName += " [";
              writeName += actInfo->unit;
              writeName += "]";
            }

            double reRot = cos(-2 * pi * this->RequestedTimeValue);

            for (unsigned int i = 0; i < numEntries; i++)
            {
              realVals[i] *= reRot;
            }

            vals.push_back(SaveToArray(realVals, actInfo->dofNames, numEntities, writeName));
          }
        }
        else
        {
          std::vector<double>& imaginaryVals = *ptImaginaryVals;
          if (this->ComplexModeReal > 0)
          {
            std::string resName = actInfo->name + "Real";
            if (this->AddDimensionsToArrayNames != 0)
            {
              resName += " [";
              resName += actInfo->unit;
              resName += "]";
            }
            vals.push_back(SaveToArray(realVals, actInfo->dofNames, numEntities, resName));
          }
          if (this->ComplexModeImaginary > 0)
          {
            std::string resName = actInfo->name + "Imag";
            if (this->AddDimensionsToArrayNames != 0)
            {
              resName += " [";
              resName += actInfo->unit;
              resName += "]";
            }
            vals.push_back(SaveToArray(imaginaryVals, actInfo->dofNames, numEntities, resName));
          }
          if (this->ComplexModeAmplitude > 0)
          {
            unsigned int numEntries = static_cast<unsigned int>(realVals.size());
            std::string amplitudeName = actInfo->name + "Ampl";
            if (this->AddDimensionsToArrayNames != 0)
            {
              amplitudeName += " [";
              amplitudeName += actInfo->unit;
              amplitudeName += "]";
            }
            std::vector<double> amplitude(numEntries);
            for (unsigned int i = 0; i < numEntries; i++)
            {
              amplitude[i] = hypot(realVals[i], imaginaryVals[i]);
            }
            vals.push_back(SaveToArray(amplitude, actInfo->dofNames, numEntities, amplitudeName));
          }
          if (this->ComplexModePhase > 0)
          {
            unsigned int numEntries = static_cast<unsigned int>(realVals.size());
            std::string phaseName = actInfo->name + "Phase";
            if (this->AddDimensionsToArrayNames != 0)
            {
              phaseName += " [deg]";
            }
            std::vector<double> phase(numEntries);
            for (unsigned int i = 0; i < numEntries; i++)
            {
              phase[i] = (std::abs(imaginaryVals[i]) > 1e-16)
                ? std::atan2(imaginaryVals[i], realVals[i]) * h180degOverPi
                : (realVals[i] < 0.0) ? 180
                                      : 0;
            }
            vals.push_back(SaveToArray(phase, actInfo->dofNames, numEntities, phaseName));
          }
          // Compute time continuous field from harmonic data by multiplying complex
          // result field with rotating phasor
          if (this->HarmonicDataAsModeShape != 0)
          {
            unsigned int numEntries = static_cast<unsigned int>(realVals.size());
            writeName = actInfo->name;
            if (AddDimensionsToArrayNames != 0)
            {
              writeName += " [";
              writeName += actInfo->unit;
              writeName += "]";
            }
            std::vector<double> modeShape(numEntries);
            // Compute the rotation pointer from the RequestedTimeValue which is in the
            // interval [0,1]
            double reRot = cos(2 * pi * this->RequestedTimeValue);
            double imRot = sin(2 * pi * this->RequestedTimeValue);
            for (unsigned int i = 0; i < numEntries; i++)
            {
              // Just compute the real part of the field
              modeShape[i] = realVals[i] * reRot - imaginaryVals[i] * imRot;
            }
            // vals5 = SaveToArray( modeShape, actInfo->dofNames, numEntities, writeName );
            vals.push_back(SaveToArray(modeShape, actInfo->dofNames, numEntities, writeName));
          } // harmonic data as mode shape
        }   // if isComplex

        // save values to grid
        if (entType == H5CFS::NODE)
        {
          for (auto& val : vals)
          {
            actGrid->GetPointData()->AddArray(val);
          }
        }
        else
        {
          for (auto& val : vals)
          {
            actGrid->GetCellData()->AddArray(val);
          }
        }
        // delete values (de-crement reference counter)
        for (auto& val : vals)
        {
          val->Delete();
        }
      } // loop over active blocks
    }   // loop over result types
  }
  catch (const std::runtime_error& er)
  {
    vtkErrorMacro(<< "Caught exception when reading results: '" << er.what() << "'");
  }
}

//-----------------------------------------------------------------------------
vtkDoubleArray* vtkCFSReader::SaveToArray(const std::vector<double>& vals,
  const std::vector<std::string>& dofNames, unsigned int numEntities, const std::string& name)
{
  vtkDoubleArray* ret = vtkDoubleArray::New();
  unsigned int numDofs = static_cast<unsigned int>(dofNames.size());
  // Case 1: numDofs is 1 (scalar) or >= 3 (real vector/tensor)
  if (numDofs == 1 || numDofs >= 3)
  {
    ret->SetNumberOfComponents(static_cast<int>(numDofs));
    for (unsigned int i = 0; i < numDofs; ++i)
    {
      ret->SetComponentName(i, dofNames[i].c_str());
    }
    ret->SetNumberOfTuples(numEntities);
    ret->SetName(name.c_str());
    double* retPtr = ret->GetPointer(0);
    unsigned int numEntries = numDofs * numEntities;
    for (unsigned int j = 0; j < numEntries; j++)
    {
      retPtr[j] = vals[j];
    }
  }
  else
  {
    // Case 2: numDofs (vector field in 2D)
    // -> add artificial 3rd component, which is 0
    ret->SetNumberOfComponents(3);
    for (unsigned int i = 0; i < numDofs; ++i)
    {
      ret->SetComponentName(i, dofNames[i].c_str());
    }
    ret->SetComponentName(2, "");
    ret->SetNumberOfTuples(numEntities);
    ret->SetName(name.c_str());
    double* retPtr = ret->GetPointer(0);
    unsigned int index = 0;
    for (unsigned int iEnt = 0; iEnt < numEntities; iEnt++)
    {
      retPtr[index++] = vals[iEnt * 2 + 0];
      retPtr[index++] = vals[iEnt * 2 + 1];
      retPtr[index++] = 0.0;
    }
  }
  return ret;
}

//-----------------------------------------------------------------------------
void vtkCFSReader::ReadCells(vtkMultiBlockDataSet* output)
{
  std::vector<H5CFS::ElemType> types;
  std::vector<std::vector<unsigned int>> connect;

  try
  {
    this->Reader.GetElements(types, connect);
    vtkDebugMacro(<< "Read in the element definitions");

    // loop over all regions
    for (unsigned int iRegion = 0; iRegion < this->RegionNames.size(); iRegion++)
    {
      const std::vector<unsigned int> regionElements =
        this->Reader.GetElementsOfRegion(this->RegionNames[iRegion]);

      vtkUnstructuredGrid* actGrid = vtkUnstructuredGrid::SafeDownCast(output->GetBlock(iRegion));

      // Add a result vector containing the original element numbers
      // to the current unstructured grid.
      vtkNew<vtkUnsignedIntArray> origElementNumbers;
      origElementNumbers->SetNumberOfValues(regionElements.size());
      origElementNumbers->SetName("origElementNums");
      actGrid->GetCellData()->AddArray(origElementNumbers.GetPointer());

      // loop over all elements
      for (unsigned int i = 0; i < regionElements.size(); i++)
      {
        origElementNumbers->SetValue(i, regionElements[i]);
      }

      this->AddElements(actGrid, iRegion, regionElements, types, connect);
    }

    unsigned int offset = static_cast<unsigned int>(this->RegionNames.size());

    // loop over all element groups
    for (unsigned int iGroup = 0; iGroup < this->NamedElementNames.size(); iGroup++)
    {
      std::string groupName = this->NamedElementNames[iGroup];
      std::vector<unsigned int> groupElements = this->Reader.GetNamedElements(groupName);

      vtkUnstructuredGrid* actGrid =
        vtkUnstructuredGrid::SafeDownCast(output->GetBlock(iGroup + offset));
      this->AddElements(actGrid, iGroup + offset, groupElements, types, connect);
    }

    offset += static_cast<unsigned int>(this->NamedElementNames.size());

    //  loop over all nodal groups
    for (unsigned int iGroup = 0; iGroup < static_cast<unsigned int>(this->NamedNodeNames.size());
         iGroup++)
    {
      std::string groupName = this->NamedNodeNames[iGroup];
      std::vector<unsigned int> groupNodes = this->Reader.GetNamedNodes(groupName);
      unsigned int numGroupNodes = static_cast<unsigned int>(groupNodes.size());

      // setup array with virtual element numbers and virtual connqectivity
      std::vector<unsigned int> vElementNumbers(numGroupNodes);
      std::vector<std::vector<unsigned int>> vElemConnect(numGroupNodes);
      std::vector<H5CFS::ElemType> vElemType(numGroupNodes);
      for (unsigned int iNode = 0; iNode < numGroupNodes; ++iNode)
      {
        vElementNumbers[iNode] = iNode + 1;
        vElemConnect[iNode].resize(1);
        vElemConnect[iNode][0] = groupNodes[iNode];
        vElemType[iNode] = H5CFS::ET_POINT;
      }

      vtkUnstructuredGrid* actGrid =
        vtkUnstructuredGrid::SafeDownCast(output->GetBlock(iGroup + offset));
      AddElements(actGrid, iGroup + offset, vElementNumbers, vElemType, vElemConnect);
    }
  }
  catch (const std::runtime_error& er)
  {
    vtkErrorMacro(<< "Caught exception when reading cells: '" << er.what() << "'");
  }
}

//-----------------------------------------------------------------------------
void vtkCFSReader::AddElements(vtkUnstructuredGrid* actGrid, unsigned int blockIndex,
  const std::vector<unsigned int>& elems, std::vector<H5CFS::ElemType>& types,
  std::vector<std::vector<unsigned int>>& connect)
{
  VTKCellType elemType = VTK_EMPTY_CELL;
  try
  {
    unsigned int numElements = static_cast<unsigned int>(elems.size());
    actGrid->Allocate(numElements);

    // loop over all elements
    for (unsigned int i = 0; i < numElements; i++)
    {
      unsigned int actElem = elems[i];

      // create correct vtk element
      elemType = GetCellIdType(types[actElem - 1]);
      if (elemType == VTK_EMPTY_CELL)
      {
        vtkErrorMacro(<< "FE type " << types[actElem - 1] << " not implemented yet");
      }

      // set connectivity
      std::array<vtkIdType, 27> nodeIds;
      for (unsigned int j = 0, n = static_cast<unsigned int>(connect[actElem - 1].size()); j < n;
           j++)
      {
        vtkDebugMacro(<< "addding nodeNum" << connect[actElem - 1][j]);
        nodeIds[j] = this->NodeMap[blockIndex][connect[actElem - 1][j] - 1] - 1;
      }
      if (elemType == VTK_TRIQUADRATIC_HEXAHEDRON)
      {
        // top
        //   7--14--6
        //   |      |
        //  15  25  13
        //   |      |
        //   4--12--5

        //   middle
        //  19--23--18
        //   |      |
        //  20  26  21
        //   |      |
        //  16--22--17

        //  bottom
        //   3--10--2
        //   |      |
        //  11  24  9
        //   |      |
        //   0-- 8--1

        nodeIds[20] = this->NodeMap[blockIndex][connect[actElem - 1][23] - 1] - 1;
        nodeIds[21] = this->NodeMap[blockIndex][connect[actElem - 1][21] - 1] - 1;
        nodeIds[22] = this->NodeMap[blockIndex][connect[actElem - 1][20] - 1] - 1;
        nodeIds[23] = this->NodeMap[blockIndex][connect[actElem - 1][22] - 1] - 1;
      }

      actGrid->InsertNextCell(elemType, connect[actElem - 1].size(), nodeIds.data());
    } // loop: elements
  }
  catch (const std::runtime_error& er)
  {
    vtkErrorMacro(<< "Caught exception when adding elements: '" << er.what() << "'");
  }
}

//-----------------------------------------------------------------------------
void vtkCFSReader::ReadNodes(vtkMultiBlockDataSet* output)
{
  try
  {
    // get all nodal coordinates
    std::vector<std::vector<double>> coords;
    this->Reader.GetNodeCoordinates(coords);

    // loop over all regions
    for (unsigned int iRegion = 0; iRegion < this->RegionNames.size(); iRegion++)
    {
      NodeMap[iRegion].resize(coords.size());
      const std::vector<unsigned int> regionNodes =
        this->Reader.GetNodesOfRegion(this->RegionNames[iRegion]);
      unsigned int numRegionNodes = static_cast<unsigned int>(regionNodes.size());

      vtkUnstructuredGrid* actGrid = vtkUnstructuredGrid::SafeDownCast(output->GetBlock(iRegion));

      vtkNew<vtkPoints> points;
      points->SetNumberOfPoints(numRegionNodes);

      // Add a result vector containing the original node numbers
      // to the current unstructured grid.
      vtkNew<vtkUnsignedIntArray> origNodeNumbers;
      origNodeNumbers->SetNumberOfValues(numRegionNodes);
      origNodeNumbers->SetName("origNodeNums");
      actGrid->GetPointData()->AddArray(origNodeNumbers.GetPointer());

      for (unsigned int i = 0; i < numRegionNodes; i++)
      {
        const std::vector<double>& p = coords[regionNodes[i] - 1];
        origNodeNumbers->SetValue(i, regionNodes[i]);
        NodeMap[iRegion][regionNodes[i] - 1] = i + 1;
        points->SetPoint(i, p[0], p[1], p[2]);
      }
      actGrid->SetPoints(points.GetPointer());
    }

    // loop over all element groups and set up nodal mapping
    unsigned int pos = static_cast<unsigned int>(RegionNames.size());
    for (unsigned int iGroup = 0; iGroup < this->NamedElementNames.size(); iGroup++, pos++)
    {
      std::string groupName = this->NamedElementNames[iGroup];
      std::vector<unsigned int> groupNodes = this->Reader.GetNamedNodes(groupName);

      this->NodeMap[pos].resize(coords.size());
      unsigned int numGroupNodes = static_cast<unsigned int>(groupNodes.size());

      vtkUnstructuredGrid* actGrid = vtkUnstructuredGrid::SafeDownCast(output->GetBlock(pos));

      vtkNew<vtkPoints> points;
      points->SetNumberOfPoints(numGroupNodes);

      for (unsigned int i = 0; i < numGroupNodes; i++)
      {
        const std::vector<double>& p = coords[groupNodes[i] - 1];
        this->NodeMap[pos][groupNodes[i] - 1] = i + 1;
        points->SetPoint(i, p[0], p[1], p[2]);
      }
      actGrid->SetPoints(points.GetPointer());
    }

    // loop over all nodal groups and set up nodal mapping
    for (unsigned int iGroup = 0; iGroup < this->NamedNodeNames.size(); iGroup++, pos++)
    {
      std::string groupName = this->NamedNodeNames[iGroup];
      std::vector<unsigned int> groupNodes = this->Reader.GetNamedNodes(groupName);
      NodeMap[pos].resize(coords.size());
      unsigned int numGroupNodes = static_cast<unsigned int>(groupNodes.size());

      vtkUnstructuredGrid* actGrid = vtkUnstructuredGrid::SafeDownCast(output->GetBlock(pos));

      vtkNew<vtkPoints> points;
      points->SetNumberOfPoints(numGroupNodes);

      for (unsigned int i = 0; i < numGroupNodes; i++)
      {
        const std::vector<double>& p = coords[groupNodes[i] - 1];
        this->NodeMap[pos][groupNodes[i] - 1] = i + 1;
        points->SetPoint(i, p[0], p[1], p[2]);
      }
      actGrid->SetPoints(points.GetPointer());
    }
  }
  catch (const std::runtime_error& er)
  {
    vtkErrorMacro(<< "Caught exception when reading nodes: '" << er.what() << "'");
  }

  vtkDebugMacro(<< "Finished reading nodes");
}

//-----------------------------------------------------------------------------
VTKCellType vtkCFSReader::GetCellIdType(H5CFS::ElemType cfsType)
{
  VTKCellType type = VTK_EMPTY_CELL;
  switch (cfsType)
  {
    case H5CFS::ET_UNDEF:
      break;
    case H5CFS::ET_POINT:
      type = VTK_VERTEX;
      break;
    case H5CFS::ET_LINE2:
      type = VTK_LINE;
      break;
    case H5CFS::ET_LINE3:
      type = VTK_QUADRATIC_EDGE;
      break;
    case H5CFS::ET_TRIA3:
      type = VTK_TRIANGLE;
      break;
    case H5CFS::ET_TRIA6:
      type = VTK_QUADRATIC_TRIANGLE;
      break;
    case H5CFS::ET_QUAD4:
      type = VTK_QUAD;
      break;
    case H5CFS::ET_QUAD8:
      type = VTK_QUADRATIC_QUAD;
      break;
    case H5CFS::ET_QUAD9:
      type = VTK_BIQUADRATIC_QUAD;
      break;
    case H5CFS::ET_TET4:
      type = VTK_TETRA;
      break;
    case H5CFS::ET_TET10:
      type = VTK_QUADRATIC_TETRA;
      break;
    case H5CFS::ET_HEXA8:
      type = VTK_HEXAHEDRON;
      break;
    case H5CFS::ET_HEXA20:
      type = VTK_QUADRATIC_HEXAHEDRON;
      break;
    case H5CFS::ET_HEXA27:
      type = VTK_TRIQUADRATIC_HEXAHEDRON;
      break;
    case H5CFS::ET_PYRA5:
      type = VTK_PYRAMID;
      break;
    case H5CFS::ET_PYRA13:
    case H5CFS::ET_PYRA14:
      type = VTK_QUADRATIC_PYRAMID;
      break;
    case H5CFS::ET_WEDGE6:
      type = VTK_WEDGE;
      break;
    case H5CFS::ET_WEDGE15:
      type = VTK_QUADRATIC_WEDGE;
      break;
    case H5CFS::ET_WEDGE18:
      type = VTK_BIQUADRATIC_QUADRATIC_WEDGE;
      break;
  }
  return type;
}

//-----------------------------------------------------------------------------
void vtkCFSReader::GetNodeCoordinates(vtkDoubleArray* vtkCoords)
{

  // Read basic information from HDF5
  ReadHdf5Informations();
  if (!this->Hdf5InfoRead)
  {
    return;
  }

  std::vector<std::vector<double>> coords;
  try
  {
    this->Reader.GetNodeCoordinates(coords);
  }
  catch (const std::runtime_error& er)
  {
    vtkErrorMacro(<< "Caught exception when obtaining node coords: '" << er.what() << "'");
    return;
  }

  unsigned int numNodes = static_cast<unsigned int>(coords.size());

  vtkCoords->SetNumberOfComponents(3);
  vtkCoords->SetNumberOfTuples(numNodes);

  if (vtkCoords->HasStandardMemoryLayout())
  {
    double* vtkPtr = vtkCoords->GetPointer(0);
    unsigned int idx = 0;

    for (unsigned int i = 0; i < numNodes; ++i)
    {
      vtkPtr[idx] = coords[i][0];
      vtkPtr[idx + 1] = coords[i][1];
      vtkPtr[idx + 2] = coords[i][2];
      idx += 3;
    }
  }
  else
  {
    vtkErrorMacro(<< "Contiguous memory layout is required.");
  }
}
