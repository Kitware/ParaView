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

namespace vtkDataObjectToConduit
{

/**
 * Fill the given conduit node with the data from the data object.
 * The final structure is a valid blueprint mesh.
 *
 * At the moment, only vtkDataSet are supported.
 */
VTKPVINSITU_EXPORT bool FillConduitNode(
  vtkDataObject* data_object, conduit_cpp::Node& conduit_node);
}

#endif
