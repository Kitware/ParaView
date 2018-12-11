/*=========================================================================

  Program:   ParaView
  Module:    vtkFileSeriesWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFileSeriesWriter
 * @brief   meta-writer for writing a file series using writers
 * that are not time-aware.
 *
 * vtkFileSeriesWriter is a meta-writer that enables writing a file series using
 * writers that are not time-aware.
*/

#ifndef vtkFileSeriesWriter_h
#define vtkFileSeriesWriter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsCoreModule.h" //needed for exports
class vtkClientServerInterpreter;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkFileSeriesWriter : public vtkDataObjectAlgorithm
{
public:
  static vtkFileSeriesWriter* New();
  vtkTypeMacro(vtkFileSeriesWriter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/get the internal writer.
   */
  virtual void SetWriter(vtkAlgorithm*);
  vtkGetObjectMacro(Writer, vtkAlgorithm);
  //@}

  /**
   * Return the MTime also considering the internal writer.
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Name of the method used to set the file name of the internal
   * writer. By default, this is SetFileName.
   */
  vtkSetStringMacro(FileNameMethod);
  vtkGetStringMacro(FileNameMethod);
  //@}

  //@{
  /**
   * Get/Set the name of the output file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  /**
   * Invoke the writer.  Returns 1 for success, 0 for failure.
   */
  int Write();

  //@{
  /**
   * If Off, which is the default, only the current timestep is written.
   * If true the writer will write every timestep, or at least those
   * within the range of min to max.
   */
  vtkGetMacro(WriteAllTimeSteps, int);
  vtkSetMacro(WriteAllTimeSteps, int);
  vtkBooleanMacro(WriteAllTimeSteps, int);
  //@}

  //@{
  /**
   * Provides an option to pad the time step when writing out time series data.
   * Only allow this format: ABC%.Xd where ABC is an arbitrary string which may
   * or may not exist and d must exist and d must be the last character
   * '.' and X may or may not exist, X must be an integer if it exists.
   * Default is nullptr.
   */
  vtkGetStringMacro(FileNameSuffix);
  vtkSetStringMacro(FileNameSuffix);
  //@}

  //@{
  /**
   * Sets a minimum timestep constraint on WriteAllTimeSteps.
   */
  vtkGetMacro(MinTimeStep, int);
  vtkSetClampMacro(MinTimeStep, int, 0, VTK_INT_MAX);
  //@}

  //@{
  /**
   * Sets a maximum timestep constraint on WriteAllTimeSteps. If less than
   * MinTimeStep, then the MaxTimeStep constraint is ignored (i.e. all time steps
   * from MinTimeStep to the actual last time step are written out).
   */
  vtkGetMacro(MaxTimeStep, int);
  vtkSetMacro(MaxTimeStep, int);
  //@}

  //@{
  /**
   * Sets a stride to write out time series.
   */
  vtkGetMacro(TimeStepStride, int);
  vtkSetClampMacro(TimeStepStride, int, 1, VTK_INT_MAX);
  //@}

  /**
   * see vtkAlgorithm for details
   */
  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Get/Set the interpreter to use to call methods on the writer.
   */
  void SetInterpreter(vtkClientServerInterpreter* interp) { this->Interpreter = interp; }

  /**
   * Utility function for validating the file name suffix.
   */
  static bool SuffixValidation(char* fileNameSuffix);

protected:
  vtkFileSeriesWriter();
  ~vtkFileSeriesWriter() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkFileSeriesWriter(const vtkFileSeriesWriter&) = delete;
  void operator=(const vtkFileSeriesWriter&) = delete;

  void SetWriterFileName(const char* fname);
  bool WriteATimestep(vtkDataObject*, vtkInformation* inInfo);
  void WriteInternal();

  vtkAlgorithm* Writer;
  char* FileNameMethod;

  int WriteAllTimeSteps;
  char* FileNameSuffix;
  int NumberOfTimeSteps;
  int CurrentTimeIndex;
  int MinTimeStep;
  int MaxTimeStep;
  int TimeStepStride;

  // The name of the output file.
  char* FileName;

  vtkClientServerInterpreter* Interpreter;
};

#endif
