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

#include "vtkSQBOVReaderBase.h"

#include <vector> // for vector
using std::vector;
#include <string> // for string
using std::string;

//BTX
class BOVReader;
class vtkPVXMLElement;
//ETX

class VTK_EXPORT vtkSQBOVMetaReader : public vtkSQBOVReaderBase
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
        vector<string> &arrays);
  //ETX

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
  // If set cahce is cleared after the filter is done
  // with each pass. If you can afford the memory then
  // unset it.
  vtkSetMacro(ClearCachedBlocks,int);
  vtkGetMacro(ClearCachedBlocks,int);

  // Description:
  // Does nothing, here to support GUI.
  // TODO -- fix the GUI panel.
  vtkSetMacro(BlockCacheSizeGUI,int);
  vtkGetMacro(BlockCacheSizeGUI,int);

  // Description:
  // Does nothing, here to support GUI.
  // TODO -- fix the GUI panel.
  vtkSetMacro(BlockSizeGUI,int);
  vtkGetMacro(BlockSizeGUI,int);

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

  virtual void Clear();

private:
  vtkSQBOVMetaReader(const vtkSQBOVMetaReader &); // Not implemented
  void operator=(const vtkSQBOVMetaReader &); // Not implemented

private:
  int PeriodicBC[3];       // flag indicating which directions have periodic BC
  int NGhosts;             // number of ghosts cells to load (ooc only)
  int DecompDims[3];       // subset split into an LxMxN cartesian decomposition
  int BlockCacheSize;      // number of blocks to cache during ooc oepration
  int ClearCachedBlocks;   // control persistence of cahce
  int BlockSizeGUI;        // does nothing here to support GUI. TODO -- fix the GUI panel.
  int BlockCacheSizeGUI;   // does nothing here to support GUI. TODO -- fix the GUI panel.
};

#endif
