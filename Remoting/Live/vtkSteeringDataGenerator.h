// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSteeringDataGenerator
 * @brief source to generate dataset given field arrays
 *
 * vtkSteeringDataGenerator is simply a dataset generator that generates a
 * single partition vtkMultiBlockDataSet with the 1st partition being non-empty
 * dataset of the type specified using
 * `vtkSteeringDataGenerator::SetPartitionType` with arrays added to the field
 * type chosen using vtkSteeringDataGenerator::SetFieldAssociation`.
 *
 * If the PartitionType is a vtkPointSet subclass, then one can use **coords**
 * as the field array name to add point coordinates when FieldAssociation is
 * set to vtkDataObject::FIELD_ASSOCIATION_POINTS.
 *
 * A sample proxy definition that uses this source is as follows:
 *
 * @code{xml}
 *
 * <ServerManagerConfiguration>
 *  <ProxyGroup name="sources">
 *     <!-- ==================================================================== -->
 *     <SourceProxy class="vtkSteeringDataGenerator" name="TestSteeringDataGeneratorSource">
 *
 *      <InputProperty command="SetSelectionConnection"
 *                    name="Selection"
 *                    panel_visibility="default"
 *                    port_index="0">
 *        <DataTypeDomain name="input_type">
 *          <DataType value="vtkSelection"/>
 *        </DataTypeDomain>
 *        <Documentation>
 *          The input that provides the selection object.
 *        </Documentation>
 *        <Hints>
 *          <Optional/>
 *          <SelectionInput/>
 *        </Hints>
 *      </InputProperty>
 *
 *       <IntVectorProperty name="PartitionType"
 *                          command="SetPartitionType"
 *                          number_of_elements="1"
 *                          default_values="0"
 *                          panel_visibility="never">
 *         <!-- 0 == VTK_POLY_DATA -->
 *       </IntVectorProperty>
 *
 *       <IntVectorProperty name="FieldAssociation"
 *                          command="SetFieldAssociation"
 *                          number_of_elements="1"
 *                          default_values="0"
 *                          panel_visibility="never" />
 *
 *       <DoubleVectorProperty name="Center"
 *                             command="SetTuple3Double"
 *                             use_index="1"
 *                             clean_command="Clear"
 *                             initial_string="coords"
 *                             number_of_elements_per_command="3"
 *                             repeat_command="1" />
 *
 *       <IdTypeVectorProperty name="Id"
 *                             command="SetTuple2IdType"
 *                             clean_command="Clear"
 *                             use_index="1"
 *                             initial_string="Id"
 *                             number_of_elements_per_command="2"
 *                             repeat_command="1" />
 *
 *       <IntVectorProperty name="Type"
 *                          command="SetTuple1Int"
 *                          clean_command="Clear"
 *                          use_index="1"
 *                          initial_string="Type"
 *                          number_of_elements_per_command="1"
 *                          repeat_command="1" />
 *     </SourceProxy>
 *   </ProxyGroup>
 * </ServerManagerConfiguration>
 *
 * @endcode
 *
 * @section vtkSteeringDataGenerator_caveats Caveats
 *
 * This filter should ideally generated `vtkPartitionedDataSet`. However,
 * until `vtkPartitionedDataSet` is well supported, we are making it generate
 * vtkMultiBlockDataSet.
 */

#ifndef vtkSteeringDataGenerator_h
#define vtkSteeringDataGenerator_h

#include "vtkAlgorithmOutput.h"
#include "vtkDataObjectAlgorithm.h"
#include "vtkRemotingLiveModule.h" //needed for exports

class vtkSelection;

class VTKREMOTINGLIVE_EXPORT vtkSteeringDataGenerator : public vtkDataObjectAlgorithm
{
public:
  static vtkSteeringDataGenerator* New();
  vtkTypeMacro(vtkSteeringDataGenerator, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Choose the type for a partition in the output vtkMultiBlockDataSet.
   * Accepted values are any non-composite dataset type know to
   * vtkDataObjectTypes.
   */
  vtkSetMacro(PartitionType, int);
  vtkGetMacro(PartitionType, int);
  ///@}

  ///@{
  /**
   * Indicate the field association to which the specified data arrays are
   * added. The FieldAssociation must make sense for the chosen PartitionType
   * i.e. setting FieldAssociation to vtkDataObjectTypes::FIELD_ASSOCIATION_ROWS
   * for a PartitionType of VTK_POLY_DATA is invalid.
   */
  vtkSetMacro(FieldAssociation, int);
  vtkGetMacro(FieldAssociation, int);
  ///@}

  ///@{
  /**
   * Convenience method to specify the selection connection (2nd input
   * port).
   *
   * Note that for now only the first node of the selection will be considered as we didn't support
   * expresion.
   */
  void SetSelectionConnection(int index, vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(index, algOutput);
  }

  void SetSelectionConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(0u, algOutput);
  }
  ///@}

  ///@{
  /**
   * Methods to add individual tuples to the data arrays. The number of
   * components and type of the array depends on the API overload used. The
   * array is idenfied using arrayname. Array names are assumed unique. Changing
   * the type or components of any array without calling `Clear` first is not
   * supported. The array will be resized to contain the tuple indicated.
   */
  void SetTuple1Double(const char* arrayname, vtkIdType index, double val);
  void SetTuple1Int(const char* arrayname, vtkIdType index, int val);
  void SetTuple1IdType(const char* arrayname, vtkIdType index, vtkIdType val);
  void SetTuple2Double(const char* arrayname, vtkIdType index, double val0, double val1);
  void SetTuple2Int(const char* arrayname, vtkIdType index, int val0, int val1);
  void SetTuple2IdType(const char* arrayname, vtkIdType index, vtkIdType val0, vtkIdType val1);
  void SetTuple3Double(
    const char* arrayname, vtkIdType index, double val0, double val1, double val2);
  void SetTuple3Int(const char* arrayname, vtkIdType index, int val0, int val1, int val2);
  void SetTuple3IdType(
    const char* arrayname, vtkIdType index, vtkIdType val0, vtkIdType val1, vtkIdType val3);
  ///@}

  /**
   * Remove the array identified by the arrayname, if any.
   */
  void Clear(const char* arrayname);

  /**
   * Append as array the list of selected id and the field type of the current selection.
   */
  void TransferSelectionToInternals(vtkSelection* selection);

protected:
  vtkSteeringDataGenerator();
  ~vtkSteeringDataGenerator() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector) override;

  int PartitionType;
  int FieldAssociation;

private:
  vtkSteeringDataGenerator(const vtkSteeringDataGenerator&) = delete;
  void operator=(const vtkSteeringDataGenerator&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
