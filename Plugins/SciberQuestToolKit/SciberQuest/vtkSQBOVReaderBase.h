/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQBOVReaderBase -- Connects the VTK pipeline to BOVReader class.
// .SECTION Description
//
// Implements the VTK style pipeline and manipulates and instance of
// BOVReader so that "brick of values" datasets, including time series,
// can be read in parallel.
//
// .SECTION See Also
// BOVReader

#ifndef __vtkSQBOVReaderBase_h
#define __vtkSQBOVReaderBase_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"

#include <vector> // for vector
#include <string> // for string

// #define vtkSQBOVReaderDEBUG

//BTX
class BOVReader;
class vtkPVXMLElement;
class vtkInformationStringKey;
class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationIntegerVectorKey;
//ETX

class VTKSCIBERQUEST_EXPORT vtkSQBOVReaderBase : public vtkDataSetAlgorithm
{
public:
  static vtkSQBOVReaderBase *New();
  vtkTypeMacro(vtkSQBOVReaderBase,vtkDataSetAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

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
  virtual void SetFileName(const char *file);
  vtkGetStringMacro(FileName);
  // Description
  // Determine if the file can be read by opening it. If the open
  // succeeds then we assume th file is readable. Open is restricted
  // to the calling rank. Only one rank should call CanReadFile.
  virtual int CanReadFile(const char *file);
  // Descritpion:
  // Get status indicating if a file is opened successfully.
  virtual bool IsOpen();

  // Description:
  // Array selection.
  virtual void SetPointArrayStatus(const char *name, int status);
  virtual int GetPointArrayStatus(const char *name);
  virtual int GetNumberOfPointArrays();
  virtual const char* GetPointArrayName(int idx);
  virtual void ClearPointArrayStatus();

  // Description:
  // Subseting interface.
  virtual void SetSubset(
        int ilo,
        int ihi,
        int jlo,
        int jhi,
        int klo,
        int khi);
  virtual void SetSubset(const int *s);
  vtkGetVector6Macro(Subset,int);

  // Description:
  // For PV UI. Range domains only work with arrays of size 2.
  virtual void SetISubset(int ilo, int ihi);
  virtual void SetJSubset(int jlo, int jhi);
  virtual void SetKSubset(int klo, int khi);
  vtkGetVector2Macro(ISubsetRange,int);
  vtkGetVector2Macro(JSubsetRange,int);
  vtkGetVector2Macro(KSubsetRange,int);

  // Description:
  // Set the reader's projection mode.
  // If set then the reader will project the field into one
  // of the axis aligned spaces, XY,XZ, or YZ.
  virtual void SetVectorProjection(int mode);
  virtual int GetVectorProjection();

  // Description:
  // Time domain discovery interface.
  virtual int GetNumberOfTimeSteps();
  virtual void GetTimeSteps(double *times);

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

  vtkSetMacro(UseDirectIO,int);
  vtkGetMacro(UseDirectIO,int);

  vtkSetMacro(UseDeferredOpen,int);
  vtkGetMacro(UseDeferredOpen,int);

  vtkSetMacro(UseDataSieving,int);
  vtkGetMacro(UseDataSieving,int);

  vtkSetMacro(SieveBufferSize,int);
  vtkGetMacro(SieveBufferSize,int);

  // Description:
  // Set the log level.
  // 0 -- no logging
  // 1 -- basic logging
  // .
  // n -- advanced logging
  vtkSetMacro(LogLevel,int);
  vtkGetMacro(LogLevel,int);

protected:
  virtual int RequestInformation(
        vtkInformation *req,
        vtkInformationVector **inInfos,
        vtkInformationVector *outInfos);

  virtual int RequestDataObject(
        vtkInformation *req,
        vtkInformationVector** inInfos,
        vtkInformationVector* outInfos);

  vtkSQBOVReaderBase();
  virtual ~vtkSQBOVReaderBase();

  // iniitialize the object
  virtual void Clear();

  // pass the hints onto MPI
  virtual void SetMPIFileHints();

  //
  virtual int GetTimeStepId(vtkInformation *inInfo, vtkInformation *outInfo);

private:
  vtkSQBOVReaderBase(const vtkSQBOVReaderBase &); // Not implemented
  void operator=(const vtkSQBOVReaderBase &); // Not implemented

protected:
  BOVReader *Reader;       // Implementation
  char *FileName;          // Name of data file to load.
  bool FileNameChanged;    // Flag indicating that the dataset needs to be opened
  int Subset[6];           // Subset to read
  int ISubsetRange[2];     // bounding extents of the subset
  int JSubsetRange[2];
  int KSubsetRange[2];
  int WorldRank;           // rank of this process
  int WorldSize;           // number of processes
  int UseCollectiveIO;     // Turn on/off collective IO
  int NumberOfIONodes;     // Number of aggregator for CIO
  int CollectBufferSize;   // Gather buffer size (if small IO is staged).
  int UseDirectIO;         // turn on/off direct I/O
  int UseDeferredOpen;     // Turn on/off deffered open (only agg.'s open)
  int UseDataSieving;      // Turn on/off data sieving
  int SieveBufferSize;     // Sieve size.
  int VectorProjection;    // to zero out one vector component
  int LogLevel;
};

#endif
