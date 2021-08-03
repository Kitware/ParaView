/*=========================================================================

  Program:   ParaView
  Module:    vtkDataObjectToConduit.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkDataObjectToConduit
 * @brief Convert VTK Data Object to Conduit Node
 */

#ifndef vtkDataObjectToConduit_h
#define vtkDataObjectToConduit_h

#include "vtkObject.h"
#include "vtkPVInSituModule.h" // For windows import/export of shared libraries

namespace conduit_cpp
{
class Node;
}

class vtkDataObject;
class vtkDataSet;
class vtkFieldData;
class vtkDataArray;
class vtkPoints;
class vtkUnstructuredGrid;

class VTKPVINSITU_EXPORT vtkDataObjectToConduit : public vtkObject
{
public:
  vtkTypeMacro(vtkDataObjectToConduit, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Fill the given conduit node with the data from the data object.
   * The final structure is a valid blueprint mesh.
   *
   * At the moment, only vtkDataSet are supported.
   */
  static bool FillConduitNode(vtkDataObject* data_object, conduit_cpp::Node& conduit_node);

protected:
  vtkDataObjectToConduit();
  virtual ~vtkDataObjectToConduit();

private:
  vtkDataObjectToConduit(const vtkDataObjectToConduit&) = delete;
  void operator=(const vtkDataObjectToConduit&) = delete;

  static bool FillConduitNode(vtkDataSet* data_set, conduit_cpp::Node& conduit_node);
  static bool FillTopology(vtkDataSet* data_set, conduit_cpp::Node& conduit_node);
  static bool FillFields(vtkDataSet* data_set, conduit_cpp::Node& conduit_node);
  static bool FillFields(
    vtkFieldData* field_data, const std::string& association, conduit_cpp::Node& conduit_node);
  static bool ConvertDataArrayToMCArray(vtkDataArray* data_array, conduit_cpp::Node& conduit_node);
  static bool ConvertDataArrayToMCArray(
    vtkDataArray* data_array, int offset, int stride, conduit_cpp::Node& conduit_node);
  static bool ConvertPoints(vtkPoints* points, conduit_cpp::Node& x_values_node,
    conduit_cpp::Node& y_values_node, conduit_cpp::Node& z_values_node);
  static bool IsMixedShape(vtkUnstructuredGrid* unstructured_grid);
  static bool IsSignedIntegralType(int data_type);
  static bool IsUnsignedIntegralType(int data_type);
  static bool IsFloatType(int data_type);
};

#endif
