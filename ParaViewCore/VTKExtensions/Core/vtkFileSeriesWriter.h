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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

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
  virtual vtkMTimeType GetMTime() VTK_OVERRIDE;

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
   * Must be set to true to write all timesteps, otherwise only the current
   * timestep will be written out. Off by default.
   */
  vtkGetMacro(WriteAllTimeSteps, int);
  vtkSetMacro(WriteAllTimeSteps, int);
  vtkBooleanMacro(WriteAllTimeSteps, int);
  //@}

  /**
   * see vtkAlgorithm for details
   */
  virtual int ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  /**
   * Get/Set the interpreter to use to call methods on the writer.
   */
  void SetInterpreter(vtkClientServerInterpreter* interp) { this->Interpreter = interp; }

protected:
  vtkFileSeriesWriter();
  ~vtkFileSeriesWriter();

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

private:
  vtkFileSeriesWriter(const vtkFileSeriesWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFileSeriesWriter&) VTK_DELETE_FUNCTION;

  void SetWriterFileName(const char* fname);
  void WriteATimestep(vtkDataObject*, vtkInformation* inInfo);
  void WriteInternal();

  vtkAlgorithm* Writer;
  char* FileNameMethod;

  int WriteAllTimeSteps;
  int NumberOfTimeSteps;
  int CurrentTimeIndex;

  // The name of the output file.
  char* FileName;

  vtkClientServerInterpreter* Interpreter;
};

#endif
