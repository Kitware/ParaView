/*=========================================================================

  Program:   ParaView
  Module:    vtkCPProcessor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#ifndef vtkCPProcessor_h
#define vtkCPProcessor_h

#include "vtkObject.h"
#include "vtkPVCatalystModule.h" // For windows import/export of shared libraries

struct vtkCPProcessorInternals;
class vtkCPDataDescription;
class vtkCPPipeline;
class vtkMPICommunicatorOpaqueComm;
class vtkMultiProcessController;
class vtkSMSourceProxy;

/// @defgroup CoProcessing ParaView CoProcessing
/// The CoProcessing library is designed to be called from parallel
/// simulation codes to reduce the size of the information that is saved
/// while also keeping the important information available as results.

/// @ingroup CoProcessing
/// There are 3 distinct phases for the operation of a co-processor.\n
/// 1) Initialization -- set up for the run.\n
/// 2) Processing -- the run.\n
/// 3) Finalization -- clean up before exit.\n
/// The processing phase occurs repeatedly and is composed of two distinct steps,
/// namely 1) Configuration (see vtkCPProcessor::ProcessDataDescription) and
/// 2) Processing (see vtkCPProcessor::ProcessData).
///
/// Configuration step:\n
/// In the first step the Co-Processor implementation is called with a
/// vtkDataDescription describing the data that the simulation can provide
/// This gives the Co-Processor implementation a chance to identify what
/// (if any) of the available data it will process during this pass. By
/// default all of the available data is selected, so that if the Co-Processor
/// implementation does nothing it will receive all data during the Processing
/// step. The Co-Processor implementation should extract what ever meta-data
/// it will need (or alternatively can save a reference to the DataDescription),
/// during the Processing step.
///
/// Processing step:\n
/// In the second step the Co-Processor implementation is called with the
/// actual data that it has been asked to provide, if any. If no data was
/// selected during the Configuration Step than the priovided vtkDataObject
/// may be NULL.
class VTKPVCATALYST_EXPORT vtkCPProcessor : public vtkObject
{
public:
  static vtkCPProcessor* New();
  vtkTypeMacro(vtkCPProcessor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Add in a pipeline that is externally configured. Returns 1 if
  /// successful and 0 otherwise.
  virtual int AddPipeline(vtkCPPipeline* pipeline);

  /// Get the number of pipelines.
  virtual int GetNumberOfPipelines();

  /// Return a specific pipeline.
  virtual vtkCPPipeline* GetPipeline(int which);

  /// Remove pipelines.
  virtual void RemovePipeline(vtkCPPipeline* pipeline);
  virtual void RemoveAllPipelines();

  // Controls cache size, in terms of timesteps, available for temporal
  // data caching. Default is zero, which disables any caching.
  virtual void SetTemporalCacheSize(int);
  vtkGetMacro(TemporalCacheSize, int);

  // Accessor to specific temporal cache. Names match CPInputData names
  virtual void MakeTemporalCache(const char* name);
  virtual vtkSMSourceProxy* GetTemporalCache(const char* name);

  /// Initialize the co-processor. Returns 1 if successful and 0
  /// otherwise. If Catalyst is built with MPI then Initialize()
  /// can also be called with a specific MPI communicator if
  /// MPI_COMM_WORLD isn't the proper one. Catalyst is initialized
  /// to use MPI_COMM_WORLD by default. Both methods have an optional
  /// workingDirectory argument which will set *WorkingDirectory* so
  /// that files will be put relative to this directory.
  virtual int Initialize(const char* workingDirectory = nullptr);
#ifndef __WRAP__
  virtual int Initialize(
    vtkMPICommunicatorOpaqueComm& comm, const char* workingDirectory = nullptr);
#endif

  /// The Catalyst input field data string array name. This array will
  /// be generated automatically by Catalyst and have a single entry
  /// that has the input name for identifying the associated
  /// vtkCPInputDataDescription. This will allow automatically mapping
  // adaptor inputs to pipeline sources.
  static const char* GetInputArrayName() { return "__CatalystChannel__"; }

  /// Configuration Step:
  /// The coprocessor first determines if any coprocessing needs to be done
  /// at this TimeStep/Time combination returning 1 if it does and 0
  /// otherwise.  If coprocessing does need to be performed this time step
  /// it fills in the FieldNames array that the coprocessor requires
  /// in order to fulfill all the coprocessing requests for this
  /// TimeStep/Time combination.
  virtual int RequestDataDescription(vtkCPDataDescription* dataDescription);

  /// Processing Step:
  /// Provides the grid and the field data for the co-procesor to process.
  /// Return value is 1 for success and 0 for failure.
  virtual int CoProcess(vtkCPDataDescription* dataDescription);

  /// Called after all co-processing is complete giving the Co-Processor
  /// implementation an opportunity to clean up, before it is destroyed.
  virtual int Finalize();

  /// Get the current working directory for outputting Catalyst files.
  /// If not set then Catalyst output files will be relative to the
  /// current working directory. This will not affect where Catalyst
  /// looks for Python scripts. *WorkingDirectory* gets set through
  /// the *Initialize()* methods.
  vtkGetStringMacro(WorkingDirectory);

protected:
  vtkCPProcessor();
  ~vtkCPProcessor() override;

  /// Create a new instance of the InitializationHelper.
  virtual vtkObject* NewInitializationHelper();

  /// Set the current working directory for outputting Catalyst files.
  /// This is a protected method since simulation code adaptors should
  /// set this through the *Initialize()* methods.
  vtkSetStringMacro(WorkingDirectory);

  /**
   * Finalizes and remove all pipelines.
   */
  void FinalizeAndRemovePipelines();

private:
  vtkCPProcessor(const vtkCPProcessor&) = delete;
  void operator=(const vtkCPProcessor&) = delete;

  vtkCPProcessorInternals* Internal;
  vtkObject* InitializationHelper;
  static vtkMultiProcessController* Controller;
  char* WorkingDirectory;
  int TemporalCacheSize = 0;
};

#endif
