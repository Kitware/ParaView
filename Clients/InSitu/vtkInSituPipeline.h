// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkInSituPipeline
 * @brief abstract base class for all in situ pipelines.
 * @ingroup Insitu
 *
 * vtkInSituPipeline is an abstract base-class for all in situ analysis
 * pipelines.
 *
 * A pipeline has three stages: Initialize, Execute and Finalize.
 *
 * `Initialize` is called exactly once before the first call to `Execute`. If
 * `Initialize` returns `false`, the initialization is deemed failed and
 * pipeline is skipped for rest of the execution i.e. either Execute nor
 * Finalize will be called.
 *
 * `Execute` is called on each cycle. If the method returns false, then the
 * execution is deemed failed and the `Execute` method will not be called in
 * subsequent cycles.
 *
 * If `Initialize` succeeded, then `Finalize` is called as the end of the
 * simulation execution. `Finalize` is called even if `Execute` returned
 * failure. However, it will not be called if `Initialize` returned failure too.
 *
 * @sa vtkInitializationHelper
 */

#ifndef vtkInSituPipeline_h
#define vtkInSituPipeline_h

#include "vtkObject.h"
#include "vtkPVInSituModule.h" // For windows import/export of shared libraries

class VTKPVINSITU_EXPORT vtkInSituPipeline : public vtkObject
{
public:
  vtkTypeMacro(vtkInSituPipeline, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize is called once before the first call to 'Execute'.
   */
  virtual bool Initialize() { return true; }

  /*
   * Called every time a new timestep is produced by the simulation.
   */
  virtual bool Execute(int timestep, double time) = 0;

  /*
   * Called optionally after Execute.
   */
  virtual bool Results() { return true; }

  /**
   * Called once before the in situ analysis is finalized.
   */
  virtual bool Finalize() { return true; }

  /**
   * Name is used to identify individual pipelines.
   */
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

protected:
  vtkInSituPipeline();
  ~vtkInSituPipeline() override;

  char* Name;

private:
  vtkInSituPipeline(const vtkInSituPipeline&) = delete;
  void operator=(const vtkInSituPipeline&) = delete;
};

#endif
