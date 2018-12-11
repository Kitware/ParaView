/*=========================================================================

  Program:   ParaView
  Module:    vtkParallelSerialWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkParallelSerialWriter
 * @brief   parallel meta-writer for serial formats
 *
 * vtkParallelSerialWriter is a meta-writer that enables serial writers
 * to work in parallel. It gathers data to the 1st node and invokes the
 * internal writer. The reduction is controlled defined by the PreGatherHelper
 * and PostGatherHelper.
 * This also makes it possible to write time-series for temporal datasets using
 * simple non-time-aware writers.
*/

#ifndef vtkParallelSerialWriter_h
#define vtkParallelSerialWriter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsCoreModule.h" //needed for exports

class vtkClientServerInterpreter;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkParallelSerialWriter : public vtkDataObjectAlgorithm
{
public:
  static vtkParallelSerialWriter* New();
  vtkTypeMacro(vtkParallelSerialWriter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/get the internal writer.
   */
  void SetWriter(vtkAlgorithm*);
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
   * Get/Set the piece number to write.  The same piece number is used
   * for all inputs.
   */
  vtkGetMacro(Piece, int);
  vtkSetMacro(Piece, int);
  //@}

  //@{
  /**
   * Get/Set the number of pieces into which the inputs are split.
   */
  vtkGetMacro(NumberOfPieces, int);
  vtkSetMacro(NumberOfPieces, int);
  //@}

  //@{
  /**
   * Get/Set the number of ghost levels to be written.
   */
  vtkGetMacro(GhostLevel, int);
  vtkSetMacro(GhostLevel, int);
  //@}

  //@{
  /**
   * Get/Set the pre-reduction helper. Pre-Reduction helper is an algorithm
   * that runs on each node's data before it is sent to the root.
   */
  void SetPreGatherHelper(vtkAlgorithm*);
  vtkGetObjectMacro(PreGatherHelper, vtkAlgorithm);
  //@}

  //@{
  /**
   * Get/Set the reduction helper. Reduction helper is an algorithm with
   * multiple input connections, that produces a single output as
   * the reduced output. This is run on the root node to produce a result
   * from the gathered results of each node.
   */
  void SetPostGatherHelper(vtkAlgorithm*);
  vtkGetObjectMacro(PostGatherHelper, vtkAlgorithm);
  //@}

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
   * Get/Set the interpreter to use to call methods on the writer.
   */
  void SetInterpreter(vtkClientServerInterpreter* interp) { this->Interpreter = interp; }

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

protected:
  vtkParallelSerialWriter();
  ~vtkParallelSerialWriter() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkParallelSerialWriter(const vtkParallelSerialWriter&) = delete;
  void operator=(const vtkParallelSerialWriter&) = delete;

  void WriteATimestep(vtkDataObject* input);
  void WriteAFile(const char* fname, vtkDataObject* input);

  void SetWriterFileName(const char* fname);
  void WriteInternal();

  vtkAlgorithm* PreGatherHelper;
  vtkAlgorithm* PostGatherHelper;

  vtkAlgorithm* Writer;
  char* FileNameMethod;
  int Piece;
  int NumberOfPieces;
  int GhostLevel;

  int WriteAllTimeSteps;
  int NumberOfTimeSteps;
  int CurrentTimeIndex;

  // The name of the output file.
  char* FileName;
  char* FileNameSuffix;

  vtkClientServerInterpreter* Interpreter;
};

#endif
