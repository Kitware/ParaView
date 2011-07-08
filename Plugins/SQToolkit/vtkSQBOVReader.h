/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
// .NAME vtkSQBOVReader -- Connects the VTK pipeline to BOVReader class.
// .SECTION Description
//
// Implements the VTK style pipeline and manipulates and instance of
// BOVReader so that "brick of values" datasets, including time series,
// can be read in parallel.
//
// .SECTION See Also
// BOVReader

#ifndef __vtkSQBOVReader_h
#define __vtkSQBOVReader_h

#include "vtkDataSetAlgorithm.h"

// define this for cerr status.
// #define vtkSQBOVReaderDEBUG

//BTX
class BOVReader;
class vtkInformationStringKey;
class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationIntegerVectorKey;
//ETX

class VTK_EXPORT vtkSQBOVReader : public vtkDataSetAlgorithm
{
public:
  static vtkSQBOVReader *New();
  vtkTypeRevisionMacro(vtkSQBOVReader,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the file to read. Setting the file name opens
  // the file. Perhaps it's bad style but this is where open
  // fits best in VTK/PV pipeline execution.
  void SetFileName(const char *file);
  vtkGetStringMacro(FileName);
  // Description
  // Determine if the file can be read by opening it. If the open
  // succeeds then we assume th file is readable. Open is restricted
  // to the calling rank. Only one rank should call CanReadFile.
  int CanReadFile(const char *file);
  // Descritpion:
  // Get status indicating if a file is opened successfully.
  bool IsOpen();

  // Description:
  // Array selection.
  void SetPointArrayStatus(const char *name, int status);
  int GetPointArrayStatus(const char *name);
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int idx);

  // Description:
  // Subseting interface.
  void SetSubset(int ilo,int ihi, int jlo, int jhi, int klo, int khi);
  void SetSubset(const int *s);
  vtkGetVector6Macro(Subset,int);

  // Description:
  // For PV UI. Range domains only work with arrays of size 2.
  void SetISubset(int ilo, int ihi);
  void SetJSubset(int jlo, int jhi);
  void SetKSubset(int klo, int khi);
  vtkGetVector2Macro(ISubsetRange,int);
  vtkGetVector2Macro(JSubsetRange,int);
  vtkGetVector2Macro(KSubsetRange,int);

  // Description:
  // Time domain discovery interface.
  int GetNumberOfTimeSteps();
  void GetTimeSteps(double *times);

  // Description:
  // Mark a coordinate direction as periodic. When periodic boundaries
  // are specified out of core reads will load ghost cells.
  void SetPeriodicBC(int *flags);
  void SetXHasPeriodicBC(int flag);
  void SetYHasPeriodicBC(int flag);
  void SetZHasPeriodicBC(int flag);

  // Description:
  // Set/Get the number of ghost cells to load during ooc
  // reads. Does not affect in core operation.
  void SetNumberOfGhostCells(int n) { this->NGhosts=n; }
  int GetNumberOfGhostCells() { return this->NGhosts; }

  // Description:
  // Set the size of the domain decomposition of the requested
  // subset in each direction.
  vtkSetVector3Macro(DecompDims,int);
  vtkGetVector3Macro(DecompDims,int);
  // TODO PV range domain 1-nCells[0], 0-nCells[1], 0-nCells[2]

  // Description:
  // Set the size of the block cache used during out-of-core
  // operation.
  vtkSetMacro(BlockCacheSize,int);
  vtkGetMacro(BlockCacheSize,int);
  // TODO PV range domain 0-nCells[0]*nCells[1]*nCells[2]


  // Description:
  // If set cahce is cleared after the filter is done
  // with each pass. If you can afford the memory then
  // unset it.
  vtkSetMacro(ClearCachedBlocks,int);
  vtkGetMacro(ClearCachedBlocks,int);

  // // Description:
  // // Sets modified if array selection changes.
  // static void SelectionModifiedCallback( 
  //     vtkObject*,
  //     unsigned long,
  //     void* clientdata,
  //     void* );

  //BTX
  enum
    {
    HINT_DEFAULT=0,
    HINT_AUTOMATIC=0,
    HINT_DISABLED=1,
    HINT_ENABLED=2
    };
  //ETX
  // Description:
  // Set/Get MPI file hints.
  vtkSetMacro(UseCollectiveIO,int);
  vtkGetMacro(UseCollectiveIO,int);

  vtkSetMacro(NumberOfIONodes,int);
  vtkGetMacro(NumberOfIONodes,int);

  vtkSetMacro(CollectBufferSize,int);
  vtkGetMacro(CollectBufferSize,int);

  vtkSetMacro(UseDeferredOpen,int);
  vtkGetMacro(UseDeferredOpen,int);

  vtkSetMacro(UseDataSieving,int);
  vtkGetMacro(UseDataSieving,int);

  vtkSetMacro(SieveBufferSize,int);
  vtkGetMacro(SieveBufferSize,int);

  // Description:
  // Activate a meta read where no arrays are read.
  // The meta data incuding file name is passed
  // downstream via pipeline information objects. The
  // reader may be used in either mode, however take care
  // that RequestInformation is called after.
  vtkSetMacro(MetaRead,int);
  vtkGetMacro(MetaRead,int);

protected:
  /// Pipeline internals.
  int RequestDataObject(vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  int RequestData(vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  int RequestInformation(vtkInformation*,vtkInformationVector**,vtkInformationVector*);

  vtkSQBOVReader();
  ~vtkSQBOVReader();

private:
  vtkSQBOVReader(const vtkSQBOVReader &); // Not implemented
  void operator=(const vtkSQBOVReader &); // Not implemented
  //
  void Clear();
  //
  void SetMPIFileHints();

private:
  BOVReader *Reader;       // Implementation
  char *FileName;          // Name of data file to load.
  bool FileNameChanged;    // Flag indicating that the dataset needs to be opened
  int Subset[6];           // Subset to read
  int ISubsetRange[2];     // bounding extents of the subset
  int JSubsetRange[2];
  int KSubsetRange[2];
  int MetaRead;            // flag indicating type of read meta or actual
  int PeriodicBC[3];       // flag indicating which directions have periodic BC
  int NGhosts;             // number of ghosts cells to load (ooc only)
  int DecompDims[3];       // subset split into an LxMxN cartesian decomposition
  int BlockCacheSize;      // number of blocks to cache during ooc oepration
  int ClearCachedBlocks;   // control persistence of cahce
  int WorldRank;           // rank of this process
  int WorldSize;           // number of processes
  char HostName[5];        // short host name where this process runs
  int UseCollectiveIO;     // Turn on/off collective IO
  int NumberOfIONodes;     // Number of aggregator for CIO
  int CollectBufferSize;   // Gather buffer size (if small IO is staged).
  int UseDeferredOpen;     // Turn on/off deffered open (only agg.'s open)
  int UseDataSieving;      // Turn on/off data sieving
  int SieveBufferSize;     // Sieve size.
};

#endif
