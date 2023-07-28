// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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

#include <catalyst_conduit.hpp> // for conduit_cpp::Node

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
  static bool Verify(const std::string& protocol, const conduit_cpp::Node& n);

protected:
  vtkCatalystBlueprint();
  ~vtkCatalystBlueprint() override;

private:
  vtkCatalystBlueprint(const vtkCatalystBlueprint&) = delete;
  void operator=(const vtkCatalystBlueprint&) = delete;
};

#endif
