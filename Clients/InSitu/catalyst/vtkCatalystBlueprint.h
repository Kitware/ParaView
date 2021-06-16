/*=========================================================================

  Program:   ParaView
  Module:    vtkCatalystBlueprint.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkCatalystBlueprint
 * @brief blueprint for ParaView Catalyst
 *
 * vtkCatalystBlueprint is used to verify that the `conduit::Node` passed to
 * various functions in Catalyst implementation are in accordance to the
 * supported protocols defined in
 * [ParaView Catalyst Blueprint](@ref ParaViewCatalystBlueprint).
 */

#ifndef vtkCatalystBlueprint_h
#define vtkCatalystBlueprint_h

#include "vtkObject.h"

#include <conduit.hpp> // for conduit::Node

class vtkCatalystBlueprint : public vtkObject
{
public:
  vtkTypeMacro(vtkCatalystBlueprint, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true is the conduit::Node `n` has the required structure.
   *
   * Supported `protocol` values are "initialize", "execute", and "finalize"
   * each corresponding to the appropriate call in the Catalyst API.
   */
  static bool Verify(const std::string& protocol, const conduit::Node& n);

protected:
  vtkCatalystBlueprint();
  ~vtkCatalystBlueprint();

private:
  vtkCatalystBlueprint(const vtkCatalystBlueprint&) = delete;
  void operator=(const vtkCatalystBlueprint&) = delete;
};

#endif
