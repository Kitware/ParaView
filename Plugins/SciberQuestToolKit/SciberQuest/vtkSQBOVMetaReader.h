/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQBOVMetaReader --A demand loading reader for BOV datasets.
// .SECTION Description
// .SECTION See Also
// vtkSQFieldTracer

#ifndef __vtkSQBOVMetaReader_h
#define __vtkSQBOVMetaReader_h
// #define vtkSQBOVReaderDEBUG

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkSQBOVReaderBase.h"

#include <vector> // for vector
#include <string> // for string

//BTX
class BOVReader;
class vtkPVXMLElement;
//ETX

class VTKSCIBERQUEST_EXPORT vtkSQBOVMetaReader : public vtkSQBOVReaderBase
{
public:
  static vtkSQBOVMetaReader *New();
  vtkTypeMacro(vtkSQBOVMetaReader,vtkSQBOVReaderBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Iitialize the reader from an XML document. You also need to
  // pass in the bov file name so that subsetting and array selection
  // can be applied which has to occur after the file has been opened.
  //BTX
  virtual int Initialize(
        vtkPVXMLElement *root,
        const char *fileName,
        std::vector<std::string> &arrays);
  //ETX

  // Description:
  // Get/Set the file to read. Setting the file name opens
  // the file. Perhaps it's bad style but this is where open
  // fits best in VTK/PV pipeline execution.
  virtual void SetFileName(const char* _arg);

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

  // Description:
  // Set the size of the block cache used during out-of-core
  // operation.
  vtkSetMacro(BlockCacheSize,int);
  vtkGetMacro(BlockCacheSize,int);

  // Description:
  // Sets the size of the blocks to use during out-of-core
  // operation.
  void SetBlockSize(int *size){ this->SetBlockSize(size[0],size[1],size[2]); }
  void SetBlockSize(int nx, int ny, int nz);
  vtkGetVector3Macro(BlockSize,int);

  // Description:
  // Sets the amount (in percent of per core) ram  to use per process
  // for the block cache.
  void SetBlockCacheRamFactor(double factor);
  vtkGetMacro(BlockCacheRamFactor,double);

  // Description:
  // If set cahce is cleared after the filter is done
  // with each pass. If you can afford the memory then
  // unset it.
  vtkSetMacro(ClearCachedBlocks,int);
  vtkGetMacro(ClearCachedBlocks,int);

protected:
  virtual int RequestInformation(
        vtkInformation *req,
        vtkInformationVector **inInfos,
        vtkInformationVector *outInfos);

  virtual int RequestData(
        vtkInformation *req,
        vtkInformationVector **inInfos,
        vtkInformationVector *outInfos);

  vtkSQBOVMetaReader();
  virtual ~vtkSQBOVMetaReader();

  // Description:
  // Free resources and initialize the object.
  virtual void Clear();

  // Description:
  // Sets BlockCacheSize and DecompDims based on avalialable ram per core
  // BlockCacheRamFactor and BlockSize.
  void EstimateBlockCacheSize();

  // Description:
  // Get the amount of ram per mpi process available on this
  // host.
  long long GetProcRam();

private:
  vtkSQBOVMetaReader(const vtkSQBOVMetaReader &); // Not implemented
  void operator=(const vtkSQBOVMetaReader &); // Not implemented

private:
  int PeriodicBC[3];       // flag indicating which directions have periodic BC
  int NGhosts;             // number of ghosts cells to load (ooc only)
  int DecompDims[3];       // subset split into an LxMxN cartesian decomposition
  int BlockCacheSize;      // number of blocks to cache during ooc oepration
  int ClearCachedBlocks;   // control persistence of cahce
  int BlockSize[3];        // size of block in the decomp
  double BlockCacheRamFactor; // % of per-core ram to use for the block cache
  long long ProcRam;       // ram available on this host for all ranks.
};

#endif
