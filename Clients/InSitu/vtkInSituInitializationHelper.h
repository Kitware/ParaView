/*=========================================================================

  Program:   ParaView
  Module:    vtkInSituInitializationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkInSituInitializationHelper
 * @brief initialization helper for in situ environments.
 * @ingroup Insitu
 *
 * vtkInSituInitializationHelper used by ParaView-Catalyst to initialize the
 * ParaView engine. It simply setups up defaults for suitable for in situ
 * use-cases. Internally, it relies on vtkInitializationHelper to do the actual
 * initialization.
 *
 * vtkInSituInitializationHelper is also intended to be used by other in situ
 * codes includes custom Catalyst API implementations or other in situ
 * frameworks.
 *
 * @sa vtkInitializationHelper
 *
 * @defgroup Insitu ParaView In Situ
 *
 * The ParaView In Situ components include classes that can be used when
 * implementing custom in situ libraries that use ParaView for
 * data analysis and visualization. [ParaView-Catalyst](@ref ParaViewCatalyst),
 * ParaView implementation for the Catalyst API, uses these classes internally.
 * Simulation developers using the Catalyst API generally don't need to worry
 * about these. These will be of interest to those who want to develop a custom
 * Catalyst API implementation that uses ParaView or when incorporating ParaView
 * in a different in situ framework.
 */

#ifndef vtkInSituInitializationHelper_h
#define vtkInSituInitializationHelper_h

#include "vtkObject.h"
#include "vtkPVInSituModule.h" // For windows import/export of shared libraries

class vtkCPCxxHelper;
class vtkInSituPipeline;
class vtkSMSourceProxy;

#include <string> // for std::string

class VTKPVINSITU_EXPORT vtkInSituInitializationHelper : public vtkObject
{
public:
  vtkTypeMacro(vtkInSituInitializationHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initializes the engine. This can only be called once in the lifetime of the
   * process. Calling this after the first call has no effect.
   *
   * 'comm' is used to determine the 'MPI_Comm' handle to be used for ParaView.
   * In non-MPI builds of ParaView, this argument is simply ignored. In
   * MPI-enabled builds of ParaView, this argument must be 'MPI_Fint' that
   * represents the Fortran handle for MPI_Comm to use. The Fortran handle is
   * an integer and hence we use 'vtkTypeUInt64' as a wide-enough container for
   * the Fortran handle without having to include 'mpi.h' in this public header.
   * The Fortran MPI communicator handle can be obtained from a `MPI_Comm` using
   * `MPI_Comm_c2f()`. Refer to MPI specification for details.
   */
  static void Initialize(vtkTypeUInt64 comm);

  /**
   * Finalizes the engine. This can be called only once in the lifetime of the
   * progress, and that too only after `Initialize` has been called.
   */
  static void Finalize();

  /**
   * Add Python analysis scripts or packages. This should be called after
   * `Initialize` but before `Finalize` to add analysis pipelines. The `path`
   * can point to a Python script, a directory containing a Python package or a
   * zip-file which containing a Python package.
   */
  static void AddPipeline(const std::string& path);

  /**
   * Add a vtkInSituPipeline instance.
   */
  static void AddPipeline(vtkInSituPipeline* pipeline);

  /**
   * Specify the algorithm or data-producer proxy that produces the mesh for named
   * catalyst channel. This cannot be changed once specified.
   *
   * On success, this registers the `producer` with the session proxy manager using
   * the same name as the channelName.
   */
  static void SetProducer(const std::string& channelName, vtkSMSourceProxy* producer);

  /**
   * Provides access to the producer proxy for a given channel, if any.
   */
  static vtkSMSourceProxy* GetProducer(const std::string& channelName);

  /**
   * Convenience method to call Update/UpdatePipeline on all known producers.
   */
  static void UpdateAllProducers(double time);

  //@{
  /**
   * This is provided as a convenience to indicate a particular producer has
   * been modified or has new data for current timestep.
   */
  static void MarkProducerModified(const std::string& channelName);
  static void MarkProducerModified(vtkSMSourceProxy* producer);
  //@}

  /**
   * Executes pipelines.
   */
  static bool ExecutePipelines(int timestep, double time);

  //@{
  /**
   * Provides access to current time and timestep during `ExecutePipelines`
   * call. The value is not valid outside `ExecutePipelines`.
   */
  static int GetTimeStep();
  static double GetTime();
  //@}

  /**
   * Returns true if vtkInSituInitializationHelper has been initialized; which
   * means that ParaView is operating in in situ mode.
   */
  static bool IsInitialized() { return vtkInSituInitializationHelper::Internals != nullptr; }

  /**
   * Returns true if ParaView is built with Python support enabled.
   */
  static bool IsPythonSupported();

protected:
  vtkInSituInitializationHelper();
  ~vtkInSituInitializationHelper();

private:
  vtkInSituInitializationHelper(const vtkInSituInitializationHelper&) = delete;
  void operator=(const vtkInSituInitializationHelper&) = delete;

  static int WasInitializedOnce;
  static int WasFinalizedOnce;

  class vtkInternals;
  static vtkInternals* Internals;
};

#endif
