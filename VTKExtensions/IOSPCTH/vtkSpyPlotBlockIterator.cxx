#include "vtkSpyPlotBlockIterator.h"
#include "vtkSpyPlotReader.h"
#include <assert.h>

vtkSpyPlotBlockIterator::vtkSpyPlotBlockIterator()
{
  this->NumberOfProcessors = 0;
  this->ProcessorId = 0;
  this->CurrentTimeStep = 0;

  this->NumberOfFiles = 0;

  this->Active = 1;
  this->Block = 0;
  this->NumberOfFields = 0;
  this->FileIndex = 0;

  this->BlockEnd = 0;
  this->FileMap = nullptr;
  this->UniReader = nullptr;
  this->Parent = nullptr;
}

void vtkSpyPlotBlockIterator::Init(int numberOfProcessors, int processorId,
  vtkSpyPlotReader* parent, vtkSpyPlotReaderMap* fileMap, int currentTimeStep)
{
  assert("pre: fileMap_exists" && fileMap != 0);

  this->NumberOfProcessors = numberOfProcessors;
  this->ProcessorId = processorId;
  this->FileMap = fileMap;
  this->Parent = parent;
  this->CurrentTimeStep = currentTimeStep;
  this->NumberOfFiles = static_cast<int>(this->FileMap->Files.size());
}

void vtkSpyPlotBlockDistributionBlockIterator::Start()
{
  this->FileIterator = this->FileMap->Files.begin();
  this->FileIndex = 0;
  this->FindFirstBlockOfCurrentOrNextFile();
}

int vtkSpyPlotBlockDistributionBlockIterator::GetNumberOfBlocksToProcess()
{
  // When distributing blocks, each process is as such
  // going to read each file, so it's okay if this method
  // creates reader for all files and reads information from them.
  int total_num_blocks = 0;
  vtkSpyPlotReaderMap::MapOfStringToSPCTH::iterator fileIterator;
  fileIterator = this->FileMap->Files.begin();
  size_t numFiles = this->FileMap->Files.size();
  int cur_file = 1;
  int progressInterval = (int)(numFiles / 20 + 1);
  for (; fileIterator != this->FileMap->Files.end(); fileIterator++, cur_file++)
  {
    if (!(cur_file % progressInterval))
    {
      this->Parent->UpdateProgress(0.2 * static_cast<double>(cur_file) / numFiles);
    }
    vtkSpyPlotUniReader* reader = this->FileMap->GetReader(fileIterator, this->Parent);
    reader->ReadInformation();
    if (!reader->SetCurrentTimeStep(this->CurrentTimeStep))
    {
      // This reader does not have that time step so skip it
      continue;
    }

    int numBlocks = reader->GetNumberOfDataBlocks();
    int numBlocksPerProcess = (numBlocks / this->NumberOfProcessors);
    int leftOverBlocks = numBlocks - (numBlocksPerProcess * this->NumberOfProcessors);
    if (this->ProcessorId < leftOverBlocks)
    {
      total_num_blocks += numBlocksPerProcess + 1;
    }
    else
    {
      total_num_blocks += numBlocksPerProcess;
    }
  }
  return total_num_blocks;
}

void vtkSpyPlotBlockDistributionBlockIterator::FindFirstBlockOfCurrentOrNextFile()
{
  this->Active = this->FileIndex < this->NumberOfFiles;
  while (this->Active)
  {
    const char* fname = this->FileIterator->first.c_str();
    this->UniReader = this->FileMap->GetReader(this->FileIterator, this->Parent);
    this->UniReader->SetFileName(fname);
    this->UniReader->ReadInformation();

    // Does this reader contain the requested time step?
    if (this->UniReader->SetCurrentTimeStep(this->CurrentTimeStep))
    {
      this->NumberOfFields = this->UniReader->GetNumberOfCellFields();

      int numBlocks = this->UniReader->GetNumberOfDataBlocks();

      if (this->ProcessorId < numBlocks) // otherwise skip to the next file
      {
        int numBlocksPerProcess = numBlocks / this->NumberOfProcessors;
        int leftOverBlocks = numBlocks - (numBlocksPerProcess * this->NumberOfProcessors);

        int blockStart;

        if (this->ProcessorId < leftOverBlocks)
        {
          blockStart = (numBlocksPerProcess + 1) * this->ProcessorId;
          this->BlockEnd = blockStart + (numBlocksPerProcess + 1) - 1;
        }
        else
        {
          blockStart = numBlocksPerProcess * this->ProcessorId + leftOverBlocks;
          this->BlockEnd = blockStart + numBlocksPerProcess - 1;
        }
        this->Block = blockStart;
        if (this->Block <= this->BlockEnd)
        {
          break; // Done
        }
      }
    }
    ++this->FileIterator;
    ++this->FileIndex;
    this->Active = this->FileIndex < this->NumberOfFiles;

#if 0
    else
      {
      cout<<"proc="<<this->ProcessorId<<" file="<<this->FileIndex
          <<" blockStart="<<this->Block<<" blockEnd="<<this->BlockEnd
          <<" numBlock="<<numBlocks<<endl;
      }
#endif
  }
}

vtkSpyPlotFileDistributionBlockIterator::vtkSpyPlotFileDistributionBlockIterator()
{
  this->FileStart = 0;
  this->FileEnd = 0;
}

void vtkSpyPlotFileDistributionBlockIterator::Init(int numberOfProcessors, int processorId,
  vtkSpyPlotReader* parent, vtkSpyPlotReaderMap* fileMap, int currentTimeStep)
{
  vtkSpyPlotBlockIterator::Init(numberOfProcessors, processorId, parent, fileMap, currentTimeStep);

  if (this->ProcessorId >= this->NumberOfFiles)
  {
    this->FileEnd = this->NumberOfFiles;
    this->FileStart = this->FileEnd + 1;
  }
  else
  {
    int numFilesPerProcess = this->NumberOfFiles / this->NumberOfProcessors;
    int leftOverFiles = this->NumberOfFiles - (numFilesPerProcess * this->NumberOfProcessors);

    if (this->ProcessorId < leftOverFiles)
    {
      this->FileStart = (numFilesPerProcess + 1) * this->ProcessorId;
      this->FileEnd = this->FileStart + (numFilesPerProcess + 1) - 1;
    }
    else
    {
      this->FileStart = numFilesPerProcess * this->ProcessorId + leftOverFiles;
      this->FileEnd = this->FileStart + numFilesPerProcess - 1;
    }
  }
}

void vtkSpyPlotFileDistributionBlockIterator::Start()
{
  this->Active = this->ProcessorId < this->NumberOfFiles; // processor not used
  if (this->Active)
  {
    // skip the first files
    this->FileIndex = 0;
    this->FileIterator = this->FileMap->Files.begin();
    while (this->FileIndex < this->FileStart)
    {
      ++this->FileIterator;
      ++this->FileIndex;
    }

    this->FindFirstBlockOfCurrentOrNextFile();
  }
}

int vtkSpyPlotFileDistributionBlockIterator::GetNumberOfBlocksToProcess()
{
  int total_num_blocks = 0;
  vtkSpyPlotReaderMap::MapOfStringToSPCTH::iterator fileIterator;

  int file_index = 0;
  int numFiles = this->FileEnd - this->FileStart + 1;
  int progressInterval = numFiles / 20 + 1;
  for (fileIterator = this->FileMap->Files.begin();
       fileIterator != this->FileMap->Files.end() && file_index <= this->FileEnd;
       fileIterator++, file_index++)
  {
    if (file_index < this->FileStart)
    {
      continue;
    }
    if (!(file_index % progressInterval))
    {
      this->Parent->UpdateProgress(0.2 * (file_index + 1.0) / numFiles);
    }
    vtkSpyPlotUniReader* reader = this->FileMap->GetReader(fileIterator, this->Parent);
    reader->ReadInformation();
    // Does this reader contain the requested time step?
    if (reader->SetCurrentTimeStep(this->CurrentTimeStep))
    {
      total_num_blocks += reader->GetNumberOfDataBlocks();
    }
  }
  return total_num_blocks;
}

void vtkSpyPlotFileDistributionBlockIterator::FindFirstBlockOfCurrentOrNextFile()
{
  this->Active = this->FileIndex <= this->FileEnd;
  while (this->Active)
  {
    const char* fname = this->FileIterator->first.c_str();
    this->UniReader = this->FileMap->GetReader(this->FileIterator, this->Parent);
    //        vtkDebugMacro("Reading data from file: " << fname);

    this->UniReader->SetFileName(fname);
    this->UniReader->ReadInformation();
    // Does this reader contain the requested time step?

    if (this->UniReader->SetCurrentTimeStep(this->CurrentTimeStep))
    {
      this->NumberOfFields = this->UniReader->GetNumberOfCellFields();

      int numberOfBlocks;
      numberOfBlocks = this->UniReader->GetNumberOfDataBlocks();

      this->BlockEnd = numberOfBlocks - 1;
      this->Block = 0;
      if (this->Block <= this->BlockEnd)
      {
        break;
      }
    }
    ++this->FileIterator;
    ++this->FileIndex;
    this->Active = this->FileIndex <= this->FileEnd;
  }
}
