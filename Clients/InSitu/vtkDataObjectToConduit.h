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

//#include <catalyst_conduit.hpp> // for conduit_cpp::Node
namespace conduit_cpp
{
class Node;
}

class vtkDataObject;
class vtkDataSet;
class vtkFieldData;
class vtkDataArray;

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
  ~vtkDataObjectToConduit();

private:
  vtkDataObjectToConduit(const vtkDataObjectToConduit&) = delete;
  void operator=(const vtkDataObjectToConduit&) = delete;

  static bool FillConduitNode(vtkDataSet* data_set, conduit_cpp::Node& conduit_node);
  static bool FillTopology(vtkDataSet* data_set, conduit_cpp::Node& conduit_node);
  static bool FillFields(vtkDataSet* data_set, conduit_cpp::Node& conduit_node);
  static bool FillFields(
    vtkFieldData* field_data, const std::string& association, conduit_cpp::Node& conduit_node);
  static bool ConvertDataArrayToMCArray(vtkDataArray* data_array, conduit_cpp::Node& conduit_node);
};

#endif
