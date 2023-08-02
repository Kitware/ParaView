// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkVectorJSON_h
#define vtkVectorJSON_h

#include "vtkVector.h"

#include "nlohmann/json.hpp" // for json

#include <stdexcept>

/// Convert a vtkVector (or any vtkTuple) into a json::array.
template <typename T, int S>
void to_json(nlohmann::json& j, const vtkTuple<T, S>& vec)
{
  j = nlohmann::json::array(vec.GetData(), vec.GetData() + S);
}

/// Convert a json::array (whose entries may be cast to the proper type) into a vtkTuple/vtkVector.
template <typename T, int S>
void from_json(const nlohmann::json& j, vtkTuple<T, S>& vec)
{
  if (!j.is_array())
  {
    throw std::invalid_argument("type must be array, but is " + std::string(j.type_name()));
  }
  if (static_cast<int>(j.size()) != vec.GetSize())
  {
    throw std::invalid_argument("array sizes do not match");
  }
  int ii = 0;
  for (auto it = j.begin(); it != j.end(); ++it, ++ii)
  {
    vec[ii] = static_cast<T>(*it);
  }
}

#endif // vtkVectorJSON_h

// VTK-HeaderTest-Exclude: vtkVectorJSON.h
