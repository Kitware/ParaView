/*=========================================================================

  Program:   ParaView
  Module:    vtkPythonRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPythonRepresentation
 *
 * Representation for showing data in a vtkPythonView. This representation
 * does not create any intermediate data for display. Instead, it simply
 * fetches data from the server.
*/

#ifndef vtkPythonRepresentation_h
#define vtkPythonRepresentation_h

#include "vtkPVDataRepresentation.h"

#include "vtkPVClientServerCorePythonRenderingModule.h" //needed for exports

class vtkReductionFilter;

class VTKPVCLIENTSERVERCOREPYTHONRENDERING_EXPORT vtkPythonRepresentation
  : public vtkPVDataRepresentation
{
public:
  static vtkPythonRepresentation* New();
  vtkTypeMacro(vtkPythonRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   * Overridden to skip processing when visibility if off.
   */
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  //@{
  /**
   * Gets local copy of the input. This will be NULL on the client when running in client-only
   * mode until after Update() is called.
   */
  vtkGetMacro(LocalInput, vtkDataObject*);
  //@}

  //@{
  /**
   * Gets the client's copy of the input
   */
  vtkGetMacro(ClientDataObject, vtkDataObject*);
  //@}

  /**
   * Get number of arrays in an attribute (e.g., vtkDataObject::POINT,
   * vtkDataObject::CELL, vtkDataObject::ROW, vtkDataObject::FIELD_DATA).
   */
  int GetNumberOfAttributeArrays(int attributeType);

  /**
   * From the input data, get the name of attribute array at index for
   * the given attribute type.
   */
  const char* GetAttributeArrayName(int attributeType, int arrayIndex);

  /**
   * Set the array status for the input data object. A
   * status of 1 means that the array with the given name for the
   * given attribute will be copied to the client. A status of 0 means
   * the array will not be copied to the client. The status is 0 by
   * default.
   */
  void SetAttributeArrayStatus(int attributeType, const char* name, int status);

  /**
   * Get the status indicating whether the array with the given name
   * and attribute type in the input will be copied to the
   * client. Status is 0 by default.
   */
  int GetAttributeArrayStatus(int attributeType, const char* name);

  /**
   * Enable all arrays. When called, all arrays will be marked as enabled.
   */
  void EnableAllAttributeArrays();

  /**
   * Disable all arrays. When called, all arrays will be marked as disabled.
   */
  void DisableAllAttributeArrays();

protected:
  vtkPythonRepresentation();
  ~vtkPythonRepresentation() override;

  /**
   * Overridden to make input optional.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Overridden to copy data from the server to the client
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPythonRepresentation(const vtkPythonRepresentation&) = delete;
  void operator=(const vtkPythonRepresentation&) = delete;

  /**
   * Local input for each processor.
   */
  vtkDataObject* LocalInput;

  /**
   * Data object located on the client.
   */
  vtkDataObject* ClientDataObject;

  //@{
  /**
   * Internal data for the representation.
   */
  class vtkPythonRepresentationInternal;
  vtkPythonRepresentationInternal* Internal;
  //@}

  /**
   * Sets the pre-gather helper on the reduction filter based on the
   * data object
   */
  void InitializePreGatherHelper(vtkReductionFilter* reductionFilter, vtkDataObject* input);

  /**
   * Sets the post-gather helper on the reduction filter based on the
   * data object
   */
  void InitializePostGatherHelper(vtkReductionFilter* reductionFilter, vtkDataObject* input);

  /**
   * Query whether this process has a particular role.
   */
  bool HasProcessRole(vtkTypeUInt32 role);

  /**
   * Query whether this process is a client
   */
  bool IsClientProcess();

  /**
   * Query whether this process is a data server
   */
  bool IsDataServerProcess();

  /**
   * Sends the data type from the root node of the server to the client
   */
  int SendDataTypeToClient(int& dataType);

  /**
   * Transfers local data from the server nodes to the client.
   */
  void TransferLocalDataToClient();
};

#endif // vtkPythonRepresentation_h
