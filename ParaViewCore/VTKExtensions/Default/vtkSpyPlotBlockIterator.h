#ifndef vtkSpyPlotBlockIterator_h
#define vtkSpyPlotBlockIterator_h

#include "assert.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkSpyPlotReaderMap.h"
#include "vtkSpyPlotUniReader.h"

class vtkSpyBlock;
class vtkSpyPlotReaderMap;
class vtkSpyPlotReader;

//-----------------------------------------------------------------------------
class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkSpyPlotBlockIterator
{
public:
  // Description:
  // Initialize the iterator with informations about processors,
  // files and timestep.
  virtual void Init(int numberOfProcessors, int processorId, vtkSpyPlotReader* parent,
    vtkSpyPlotReaderMap* fileMap, int currentTimeStep);

  // Description:
  // Go to first block if any.
  virtual void Start() = 0;

  // Description:
  // Returns the total number of blocks to be processed by this Processor.
  // Can be called only after Init().
  virtual int GetNumberOfBlocksToProcess() = 0;

  // Description:
  // Are there still blocks to iterate over?
  int IsActive() const;

  // Description:
  // Go to the next block if any
  // \pre is_active: IsActive()
  void Next();

  // Description:
  // Return the block at current position.
  // \pre is_active: IsActive()
  vtkSpyPlotBlock* GetBlock() const;

  // Description:
  // Return the blockID at current position.
  // \pre is_active: IsActive()
  int GetBlockID() const;

  // Description:
  // Return the number of fields at current position.
  // \pre is_active: IsActive()
  int GetNumberOfFields() const;

  // Description:
  // Return the SPCTH API handle at current position.
  // \pre is_active: IsActive()
  vtkSpyPlotUniReader* GetUniReader() const;

  // Description:
  // Return the number of processors being used
  int GetNumberOfProcessors() const;

  // Description:
  // Return the id of this processor
  int GetProcessorId() const;

  virtual ~vtkSpyPlotBlockIterator() {}

protected:
  vtkSpyPlotBlockIterator();

  virtual void FindFirstBlockOfCurrentOrNextFile() = 0;

  int NumberOfProcessors;
  int ProcessorId;
  vtkSpyPlotReaderMap* FileMap;
  int CurrentTimeStep;

  int NumberOfFiles;

  int Active;
  int Block;
  int NumberOfFields;
  vtkSpyPlotUniReader* UniReader;

  vtkSpyPlotReaderMap::MapOfStringToSPCTH::iterator FileIterator;
  int FileIndex;

  int BlockEnd;
  vtkSpyPlotReader* Parent;
};

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkSpyPlotBlockDistributionBlockIterator
  : public vtkSpyPlotBlockIterator
{
public:
  vtkSpyPlotBlockDistributionBlockIterator() {}
  virtual ~vtkSpyPlotBlockDistributionBlockIterator() {}
  virtual void Start();
  virtual int GetNumberOfBlocksToProcess();

protected:
  virtual void FindFirstBlockOfCurrentOrNextFile();
};

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkSpyPlotFileDistributionBlockIterator
  : public vtkSpyPlotBlockIterator
{
public:
  vtkSpyPlotFileDistributionBlockIterator();
  virtual ~vtkSpyPlotFileDistributionBlockIterator() {}
  virtual void Init(int numberOfProcessors, int processorId, vtkSpyPlotReader* parent,
    vtkSpyPlotReaderMap* fileMap, int currentTimeStep);
  virtual void Start();
  virtual int GetNumberOfBlocksToProcess();

protected:
  virtual void FindFirstBlockOfCurrentOrNextFile();
  int FileStart;
  int FileEnd;
};

inline void vtkSpyPlotBlockIterator::Next()
{
  assert("pre: is_active" && IsActive());

  ++this->Block;
  if (this->Block > this->BlockEnd)
  {
    ++this->FileIterator;
    ++this->FileIndex;
    this->FindFirstBlockOfCurrentOrNextFile();
  }
}

inline int vtkSpyPlotBlockIterator::GetProcessorId() const
{
  return this->ProcessorId;
}

inline int vtkSpyPlotBlockIterator::GetNumberOfProcessors() const
{
  return this->NumberOfProcessors;
}

inline vtkSpyPlotUniReader* vtkSpyPlotBlockIterator::GetUniReader() const
{
  assert("pre: is_active" && IsActive());
  return this->UniReader;
}

inline int vtkSpyPlotBlockIterator::IsActive() const
{
  return this->Active;
}

inline vtkSpyPlotBlock* vtkSpyPlotBlockIterator::GetBlock() const
{
  assert("pre: is_active" && IsActive());
  return this->UniReader->GetBlock(Block);
}

inline int vtkSpyPlotBlockIterator::GetBlockID() const
{
  assert("pre: is_active" && IsActive());
  return Block;
}

inline int vtkSpyPlotBlockIterator::GetNumberOfFields() const
{
  assert("pre: is_active" && IsActive());
  return this->NumberOfFields;
}

#endif

// VTK-HeaderTest-Exclude: vtkSpyPlotBlockIterator.h
