/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// .NAME vtkSQBOVWriter -- Connects the VTK pipeline to BOVWriter class.
// .SECTION Description
//
// Implements the VTK style pipeline and manipulates and instance of
// BOVWriter so that "brick of values" datasets, including time series,
// can be writen in parallel.
//
// It's assumed that the executive is responsible for the domain
// decomposition so make sure to initialize it.
//
// .SECTION See Also
// BOVWriter

#ifndef vtkSQBOVWriter_h
#define vtkSQBOVWriter_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"

// define this for cerr status.
// #define vtkSQBOVWriterDEBUG

class BOVWriter;
class vtkPVXMLElement;
class vtkInformationStringKey;
class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationIntegerVectorKey;

class VTKSCIBERQUEST_EXPORT vtkSQBOVWriter : public vtkDataSetAlgorithm
{
public:
  static vtkSQBOVWriter *New();
  vtkTypeMacro(vtkSQBOVWriter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the writer from an xml document.
  int Initialize(vtkPVXMLElement *elem);

  // Description:
  // Get/Set the file to write. Setting the file name opens
  // the file. Perhaps it's bad style but this is where open
  // fits best in VTK/PV pipeline execution.
  void SetFileName(const char *file);
  vtkGetStringMacro(FileName);

  // Descritpion:
  // Get status indicating if a file is opened successfully.
  bool IsOpen();

  // Description:
  // Array selection.
  void SetPointArrayStatus(const char *name, int status);
  int GetPointArrayStatus(const char *name);
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int idx);
  void ClearPointArrayStatus();

  // Description:
  // Time domain discovery interface.
  int GetNumberOfTimeSteps();
  double GetTimeStep(int i);
  void GetTimeSteps(double *times);

  enum
    {
    HINT_DEFAULT=0,
    HINT_AUTOMATIC=0,
    HINT_DISABLED=1,
    HINT_ENABLED=2
    };

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

  vtkSetMacro(StripeSize,int);
  vtkGetMacro(StripeSize,int);

  vtkSetMacro(StripeCount,int);
  vtkGetMacro(StripeCount,int);

  // Description:
  // Writes a bov metadata file.
  int WriteMetaData();

  // Description:
  // Force a write.
  void Write();

  // Description:
  // When set Write will drive the pipeline once for each
  // available timestep.
  vtkSetMacro(WriteAllTimeSteps,int);
  vtkGetMacro(WriteAllTimeSteps,int);

  // Description:
  // Set the log level.
  // 0 -- no logging
  // 1 -- basic logging
  // .
  // n -- advanced logging
  vtkSetMacro(LogLevel,int);
  vtkGetMacro(LogLevel,int);

protected:
  /// Pipeline internals.
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestDataObject(vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  int RequestData(vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  int RequestInformation(vtkInformation*,vtkInformationVector**,vtkInformationVector*);

  vtkSQBOVWriter();
  ~vtkSQBOVWriter();

private:
  vtkSQBOVWriter(const vtkSQBOVWriter &) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQBOVWriter &) VTK_DELETE_FUNCTION;
  //
  void Clear();
  //
  void SetMPIFileHints();

private:
  BOVWriter *Writer;       // Implementation
  int DimMode;             // 2d or 3d cases
  char *FileName;          // Name of data file to load.
  bool FileNameChanged;    // Flag indicating that the dataset needs to be opened
  int IncrementalMetaData; // append to metadata with each update
  int WriteAllTimeSteps;   // drive the pipeline for all timesteps.
  int TimeStepId;          // step id only used for writing all steps.
  int WorldRank;           // rank of this process
  int WorldSize;           // number of processes
  int UseCollectiveIO;     // Turn on/off collective IO
  int UseDirectIO;         //
  int NumberOfIONodes;     // Number of aggregator for CIO
  int CollectBufferSize;   // Gather buffer size (if small IO is staged).
  int UseDeferredOpen;     // Turn on/off deffered open (only agg.'s open)
  int UseDataSieving;      // Turn on/off data sieving
  int SieveBufferSize;     // Sieve size.
  int StripeSize;          // Stripe size in bytes
  int StripeCount;         // Stripe count in OST's
  int LogLevel;            // control timing/logging
};

#endif
