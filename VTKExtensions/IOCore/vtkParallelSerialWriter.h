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
 * to work in parallel. By default, it gathers data to the 1st node (root node)
 * and invokes the internal writer. The reduction is controlled by the
 * PreGatherHelper and PostGatherHelper. Instead of collecting all the data to
 * the root node the filter supports reducing down to a target number of ranks
 * which ranks chosen in either round-robin or contiguous fashion.
 *
 * This also makes it possible to write time-series for temporal datasets using
 * simple non-time-aware writers.
 */

#ifndef vtkParallelSerialWriter_h
#define vtkParallelSerialWriter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsIOCoreModule.h" //needed for exports
#include "vtkSmartPointer.h"                // needed for vtkSmartPointer
#include <string>                           // for std::string

class vtkClientServerInterpreter;
class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSIOCORE_EXPORT vtkParallelSerialWriter : public vtkDataObjectAlgorithm
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

  //@{
  /**
   * In parallel runs, this writer can consolidate output from multiple ranks to
   * a subset of ranks. This specifies the number of ranks that will do the final writing
   * to disk. If NumberOfIORanks is 0, then all ranks will save the local data.
   * If set to 1 (default), the root node alone will write to disk. All data from all ranks will
   * be gathered to the root node before being written out.
   */
  vtkSetClampMacro(NumberOfIORanks, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfIORanks, int);
  //@}

  enum
  {
    ASSIGNMENT_MODE_CONTIGUOUS,
    ASSIGNMENT_MODE_ROUND_ROBIN
  };

  //@{
  /**
   * When `NumberOfIORanks` is greater than 1 and less than the number of MPI ranks,
   * this controls how the ranks that write to disk are determined. This also affects which
   * ranks send data to which rank for IO.
   *
   * In ASSIGNMENT_MODE_CONTIGUOUS (default), all MPI ranks are numerically grouped into
   * `NumberOfIORanks` groups with the first rank in each group acting as the `root` node for that
   * group and is the one that does IO. For example, if there are 16 MPI ranks and NumberOfIORanks
   * is set to 3 then the groups are
   * `[0 - 5], [6 - 10], [11 - 15]` with the first rank in each group 0, 6, and 15 doing the IO.
   *
   * In ASSIGNMENT_MODE_ROUND_ROBIN, the grouping is done in round robin fashion, thus for 16 MPI
   * ranks with NumberOfIORanks set to 3, the groups are
   * `[0, 3, ..., 15], [1, 4, ..., 13], [2, 5, ..., 14]` with 0, 1 and 2 doing the IO.
   */
  vtkSetClampMacro(
    RankAssignmentMode, int, ASSIGNMENT_MODE_CONTIGUOUS, ASSIGNMENT_MODE_ROUND_ROBIN);
  vtkGetMacro(RankAssignmentMode, int);
  //@}

  //@{
  /**
   * Get/Set the controller to use. By default initialized to
   * `vtkMultiProcessController::GetGlobalController` in the constructor.
   */
  void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
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
  void WriteAFile(const std::string& fname, vtkDataObject* input);

  void SetWriterFileName(const char* fname);
  void WriteInternal();

  std::string GetPartitionFileName(const std::string& fname);

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

  int NumberOfIORanks;
  int RankAssignmentMode;

  vtkMultiProcessController* Controller;
  vtkSmartPointer<vtkMultiProcessController> SubController;
  int SubControllerColor;
};

#endif
