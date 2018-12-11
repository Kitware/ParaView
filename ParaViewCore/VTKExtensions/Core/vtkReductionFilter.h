/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReductionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkReductionFilter
 * @brief   A generic filter that can reduce any type of
 * dataset using any reduction algorithm.
 *
 * A generic filter that can reduce any type of dataset using any reduction
 * algorithm. Actual reduction is performed by running the PreGatherHelper
 * and PostGatherHelper algorithms. The PreGatherHelper runs on each node
 * in parallel. Next the intermediate results are gathered to the root node.
 * Then the root node then runs the PostGatherHelper algorithm to produce a
 * single result. The PostGatherHelper must be an algorithm that takes
 * multiple input connections and produces a single reduced output.
 *
 * In addition to doing reduction the PassThrough variable lets you choose
 * to pass through the results of any one node instead of aggregating all of
 * them together.
*/

#ifndef vtkReductionFilter_h
#define vtkReductionFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro
#include "vtkSmartPointer.h"              // needed for vtkSmartPointer.
#include <vector>                         //  needed for std::vector

class vtkMultiProcessController;
class vtkSelection;
class VTKPVVTKEXTENSIONSCORE_EXPORT vtkReductionFilter : public vtkDataObjectAlgorithm
{
public:
  static vtkReductionFilter* New();
  vtkTypeMacro(vtkReductionFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  typedef enum ReductionModeType {
    REDUCE_ALL_TO_ONE = 0,
    MOVE_ALL_TO_ONE = 1,
    REDUCE_ALL_TO_ALL = 2
  } ReductionModeType;

  //@{
  /**
   * Get/Set the Reduction Mode.
   * REDUCE_ALL_TO_ONE is the default behavior.
   * It reduces all data on a single node, while other nodes keeps their data.
   * MOVE_ALL_TO_ONE Reduce all data on a single node while other nodes delete their data.
   * ALL NODES Reduce all data on all nodes.
   */
  vtkSetClampMacro(ReductionMode, int, vtkReductionFilter::REDUCE_ALL_TO_ONE,
    vtkReductionFilter::REDUCE_ALL_TO_ALL);
  vtkGetMacro(ReductionMode, int);
  //@}

  //@{
  /**
   * Get/Set the node to reduce to, default is 0.
   * Not used with REDUCE_ALL_TO_ALL Reduction mode
   */
  vtkSetMacro(ReductionProcessId, int);
  vtkGetMacro(ReductionProcessId, int);
  //@}

  //@{
  /**
   * Get/Set the pre-reduction helper. Pre-Reduction helper is an algorithm
   * that runs on each node's data before it is sent to the root.
   */
  void SetPreGatherHelper(vtkAlgorithm*);
  void SetPreGatherHelperName(const char*);
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
  void SetPostGatherHelperName(const char*);
  vtkGetObjectMacro(PostGatherHelper, vtkAlgorithm);
  //@}

  /**
   * Get/Set the MPI controller used for gathering.
   */
  void SetController(vtkMultiProcessController*);

  //@{
  /**
   * Get/Set the PassThrough flag which (when set to a nonnegative number N)
   * tells the filter to produce results that come from node N only. The
   * data from that node still runs through the PreReduction and
   * PostGatherHelper algorithms.
   */
  vtkSetMacro(PassThrough, int);
  vtkGetMacro(PassThrough, int);
  //@}

  //@{
  /**
   * When set, a new array vtkOriginalProcessIds will be added
   * to the output of the the pre-gather helper (or input, if no pre-gather
   * helper is set). The values in the array indicate the process id.
   * Note that the array is added only if the number of processes is > 1.
   */
  vtkSetMacro(GenerateProcessIds, int);
  vtkGetMacro(GenerateProcessIds, int);
  //@}

  enum Tags
  {
    TRANSMIT_DATA_OBJECT = 23484
  };

protected:
  vtkReductionFilter();
  ~vtkReductionFilter() override;

  // Overridden to mark input as optional, since input data may
  // not be available on all processes that this filter is instantiated.
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  void Reduce(vtkDataObject* input, vtkDataObject* output);
  vtkDataObject* PreProcess(vtkDataObject* input);
  void PostProcess(
    vtkDataObject* output, vtkSmartPointer<vtkDataObject> inputs[], unsigned int num_inputs);

  /**
   * Gather for vtkSelection
   * sendData is a vtkSelection while receiveData is a vector of NumberOfProcesses
   * vtkSelections. Selections are gathered on destProcessId
   */
  int GatherSelection(vtkSelection* sendData,
    std::vector<vtkSmartPointer<vtkDataObject> >& receiveData, int destProcessId);

  vtkAlgorithm* PreGatherHelper;
  vtkAlgorithm* PostGatherHelper;
  vtkMultiProcessController* Controller;
  int PassThrough;
  int GenerateProcessIds;
  int ReductionMode;
  int ReductionProcessId;

private:
  vtkReductionFilter(const vtkReductionFilter&) = delete;
  void operator=(const vtkReductionFilter&) = delete;
};

#endif
