/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSpyPlotReader.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSpyPlotReader - Read SPCTH Spy Plot file format
// .SECTION Description
// vtkSpyPlotReader is a reader that reads SPCTH Spy Plot file format
// through an ascii meta file called the "case" file (extension .spcth). This
// case file lists all the actual binary files that describe a dataset. Each
// binary file describes a part of the dataset. However, if only a single
// binary file describes the whole dataset, it is possible to load it directly
// without using a case file.
//
// The reader supports both Spy dataset types: flat mesh and AMR
// (Adaptive Mesh Refinement).
//
// It has parallel capabilities. Each processor is supposed to have access
// to the case file and to all the binary files. All the binary files
// have to be coherent: they describe the same fields of data.
// Each binary file may content multiple time stamp. The time stamp to read is
// specified with SetTimestamp().
//
// In parallel mode, there are two ways to distribute data over processors
// (controlled by SetDistributeFiles() ):
// - either by distributing blocks: all processors read all the files, but
// only some number of blocks per files. Hence, load balancing is good even if
// there is only one file.
// - or by distributing files: a file is read entirely by one processor. If
// there is only one file, all the other processors are not used at all.
//
// .SECTION Implementation Details
// - All processors read the first binary file listed in the case file to get
// informations about the fields.
// - Each block of data is already surrounded by ghost cells in the file,
// even on part of the block that don't have actual neighbor cells. The
// reader removes those wrong ghost cells.
// - Each time step contains all the cell array name variables

#ifndef __vtkSpyPlotReader_h
#define __vtkSpyPlotReader_h

#include "vtkCompositeDataSetAlgorithm.h"

class vtkBoundingBox;
class vtkCallbackCommand;
class vtkCellData;
class vtkDataArray;
class vtkDataArraySelection;
class vtkDataSetAttributes;
class vtkHierarchicalBoxDataSet;
class vtkMultiBlockDataSet;
class vtkMultiProcessController;
class vtkRectilinearGrid;
class vtkSpyPlotBlock;
class vtkSpyPlotBlockIterator;
class vtkSpyPlotReaderMap;
class vtkSpyPlotUniReader;

class VTK_EXPORT vtkSpyPlotReader : public vtkCompositeDataSetAlgorithm
{
public:
  static vtkSpyPlotReader* New();
  vtkTypeMacro(vtkSpyPlotReader,vtkCompositeDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  void PrintBlockList(vtkHierarchicalBoxDataSet *hbds, int myProcId);

  // Description:
  // Get and set the file name. It is either the name of the case file or the
  // name of the single binary file.  
  virtual void SetFileName(const char* filename);
  vtkGetStringMacro(FileName);

  // Description:
  // Set and get the time step. The time step is an index, NOT a time
  // given in seconds.
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  // Description:
  // If true, the reader distributes files over processors. If false,
  // the reader distributes blocks over processors. Default is false.
  // Distributing blocks should provide a better load balancing:
  // if there is only one file and several processors, only the first
  // processor is used in the case of the file-distributed method.
  vtkSetMacro(DistributeFiles,int);
  vtkGetMacro(DistributeFiles,int);
  vtkBooleanMacro(DistributeFiles,int);

  // Description:
  // If true, the reader generate a cell array in each block that
  // stores the level in the hierarchy, starting from 0.
  // False by default.
  vtkSetMacro(GenerateLevelArray,int);
  vtkGetMacro(GenerateLevelArray,int);
  vtkBooleanMacro(GenerateLevelArray,int);

  // Description:
  // If true, the reader generate a cell array in each block that
  // stores a unique but not necessarily contiguous id.
  // False by default.
  vtkSetMacro(GenerateBlockIdArray,int);
  vtkGetMacro(GenerateBlockIdArray,int);
  vtkBooleanMacro(GenerateBlockIdArray,int);

  // Description:
  // If true, the reader generate a cell array in each block that
  // corresponds to the Active field in the file.
  // False by default.
  vtkSetMacro(GenerateActiveBlockArray,int);
  vtkGetMacro(GenerateActiveBlockArray,int);
  vtkBooleanMacro(GenerateActiveBlockArray,int);

  // Description:
  // If true, the reader will extract tracer data at each time 
  // step and include a field data array for the tracers at that 
  // time.
  vtkSetMacro(GenerateTracerArray,int);
  vtkGetMacro(GenerateTracerArray,int);
  vtkBooleanMacro(GenerateTracerArray,int);

  // Description:
  // If true, the reader will convert volume fraction arrays to unsigned char.
  // True by default.
  void SetDownConvertVolumeFraction(int vf);
  vtkGetMacro(DownConvertVolumeFraction,int);
  vtkBooleanMacro(DownConvertVolumeFraction,int);

  // Description:
  // If true, the reader will calculate all derived variables it can given
  // which properties the user has selected
  // True by default.
  vtkSetMacro(ComputeDerivedVariables, int);
  vtkGetMacro(ComputeDerivedVariables,int);
  vtkBooleanMacro(ComputeDerivedVariables,int);

  // Description:
  // If true, the reader will merge scalar arrays named, for example, "X velocity"
  // "Y velocity" and "Z velocity" into a vector array named "velocity" with
  // scalar components X, Y and Z. It will also merge X and Y scalar arrays
  // (with no Z component) into a vector with scalar components X, Y and 0.
  // True by default.
  void SetMergeXYZComponents(int merge);
  vtkGetMacro(MergeXYZComponents,int);
  vtkBooleanMacro(MergeXYZComponents,int);

  // Description:
  // Get the time step range.
  vtkGetVector2Macro(TimeStepRange, int);

  // Description:
  // Get the data array selection tables used to configure which data
  // arrays are loaded by the reader.
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);

  // Description:
  // Cell array selection
  int GetNumberOfCellArrays();
  const char* GetCellArrayName(int idx);
  int GetCellArrayStatus(const char *name);
  void SetCellArrayStatus(const char *name, int status);  

  // Description:
  // Set the controller used to coordinate parallel reading.
  // The "global controller" has all processes while the 
  // "controller" has only those who have blocks.
  void SetGlobalController(vtkMultiProcessController* controller);

  // Description:
  // Determine if the file can be readed with this reader.
  virtual int CanReadFile(const char* fname);

protected:
  vtkSpyPlotReader();
  ~vtkSpyPlotReader();

  // Determine the bounds of just this reader
  void GetLocalBounds(vtkSpyPlotBlockIterator *biter,
                      int nBlocks, int progressInterval);

  // Set the global bounds of all readers
  void SetGlobalBounds(vtkSpyPlotBlockIterator *biter,
                      int total_num_of_block, 
                      int progressInterval,
                      int *rightHasBounds,
                      int *leftHasBounds);

  // Determine the box size on just this reader
  // returns true if box size is a constant on this reader
  // false if not.
  bool GetLocalBoxSize(vtkSpyPlotBlockIterator *biter,
                       int *localBoxSize) const;

  // Determine box size if it is a constant across the data set
  // If not then this is set to -1,-1,-1.
  void SetGlobalBoxSize(vtkSpyPlotBlockIterator *biter);

  // Determine the minimum level that is used on just this level
  // and get the spacing there
  void GetLocalMinLevelAndSpacing(vtkSpyPlotBlockIterator *biter,
                                  int *localMinLevel,
                                  double spacing[3]) const;

  // Set the minimum level that is used
  // and get the spacing there
  void SetGlobalMinLevelAndSpacing(vtkSpyPlotBlockIterator *biter);

  // Set things up to process an AMR Block
  int PrepareAMRData(vtkHierarchicalBoxDataSet *hb,
                     vtkSpyPlotBlock *block, 
                     int *level,
                     int extents[6],
                     int realExtents[6],
                     int realDims[3],
                     vtkCellData **cd);

  // Set things up to process a non-AMR Block
  int PrepareData(vtkMultiBlockDataSet* hb,
                  vtkSpyPlotBlock *block,
                  vtkRectilinearGrid **rg,
                  int extents[6],
                  int realExtents[6],
                  int realDims[3],
                  vtkCellData **cd);

  // Update the field data (interms of ghost cells) that
  // contain whose block did not contain any bad ghost cells
  void UpdateFieldData(int numFields, int dims[3],
                       int level, int blockID,
                       vtkSpyPlotUniReader *uniReader,
                       vtkCellData *cd);


  // Update the field data (interms of ghost cells) that
  // contain whose block did contain bad ghost cells
  void UpdateBadGhostFieldData(int numFields, int dims[3],
                               int realDims[3],
                               int realExtents[6],
                               int level, int blockID,
                               vtkSpyPlotUniReader *uniReader,
                               vtkCellData *cd);
  // The array selections.
  vtkDataArraySelection *CellDataArraySelection;

  // Create either vtkHierarchicalBoxDataSet or vtkMultiBlockDataSet based on
  // whether the dataset is AMR.
  virtual int RequestDataObject(vtkInformation *req,
    vtkInformationVector **inV,
    vtkInformationVector *outV);

  
  // Read the case file and the first binary file do get meta
  // informations (number of files, number of fields, number of timestep).
  virtual int RequestInformation(vtkInformation *request, 
                                 vtkInformationVector **inputVector, 
                                 vtkInformationVector *outputVector);

  // Read the data: get the number of pieces (=processors) and get
  // my piece id (=my processor id).
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  // Callback registered with the SelectionObserver.
  static void SelectionModifiedCallback(vtkObject *caller, unsigned long eid,
                                        void *clientdata, void *calldata);

  // Description:
  // This does the updating of meta data of the dataset from the
  // first binary file registered in the map:
  // - number of time steps
  // - number of fields
  // - name of fields
  int UpdateMetaData(vtkInformation* request,
                     vtkInformationVector* outputVector);

  // Description:
  // This does the updating of the meta data of the case file
  int UpdateCaseFile(const char *fname,
                     vtkInformation* request, 
                     vtkInformationVector* outputVector);

  // Description:
  // This does the updating of the meta data for a series, when no case file provided
  int UpdateSpyDataFile(vtkInformation* request, 
                        vtkInformationVector* outputVector);

  int UpdateFile(vtkInformation *request, 
                 vtkInformationVector *outputVector);

  void AddGhostLevelArray(int numLevels);
  int AddBlockIdArray(vtkCompositeDataSet *cds);
  int AddAttributes(vtkHierarchicalBoxDataSet *hbds);
  int AddActiveBlockArray(vtkCellData *cd,vtkIdType nCells,unsigned char status);

  // Have all the readers have the same global level structure
  void SetGlobalLevels(vtkCompositeDataSet *cds);
  // Description:
  // Get and set the current file name. Protected because
  // this method should only be used internally
  vtkSetStringMacro(CurrentFileName);
  vtkGetStringMacro(CurrentFileName);

  // The observer to modify this object when the array selections are
  // modified.
  vtkCallbackCommand *SelectionObserver;
  char *FileName;
  char *CurrentFileName;
  int TimeStep; // set by the user
  int TimeStepRange[2];
  int CurrentTimeStep; // computed

  int IsAMR; // AMR (hierarchy of uniform grids)
  // or flat mesh (set of rectilinear grids)?

  // access to all processes
  vtkMultiProcessController *GlobalController;

  // Description:
  // Set the current time step.
  int UpdateTimeStep(vtkInformation *requestInfo,
                     vtkInformationVector *outputInfo,
                     vtkCompositeDataSet *hb);

  // The file format stores a vector field as separated scalar component
  // fields. This method rebuilds the vector field from those scalar
  // component fields.
  void MergeVectors(vtkDataSetAttributes *da);
  int MergeVectors(vtkDataSetAttributes *da, 
                   vtkDataArray *a1,
                   vtkDataArray *a2);
  int MergeVectors(vtkDataSetAttributes *da, 
                   vtkDataArray *a1,
                   vtkDataArray *a2,
                   vtkDataArray *a3);

  int ComputeDerivedVariables;
  int ComputeDerivedVars(vtkCellData* data, 
    vtkSpyPlotBlock *block, vtkSpyPlotUniReader *reader, const int& blockID, int dims[3]);
  

  vtkSpyPlotReaderMap *Map;
  
  int DistributeFiles;

  vtkBoundingBox *Bounds; //bounds of the hierarchy without the bad ghostcells.
  int BoxSize[3];         // size of boxes if they are all the same, else -1,-1,-1
  int MinLevel;       // first used level
  double MinLevelSpacing[3]; // grid spacing on first used level

  int GenerateLevelArray; // user flag
  int GenerateBlockIdArray; // user flag
  int GenerateActiveBlockArray; // user flag
  int GenerateTracerArray; // user flag

  int DownConvertVolumeFraction;
  
  bool TimeRequestedFromPipeline;

  int MergeXYZComponents;

  int UpdateFileCallCount;

private:
  vtkSpyPlotReader(const vtkSpyPlotReader&);  // Not implemented.
  void operator=(const vtkSpyPlotReader&);  // Not implemented.
};

#endif
