// See license.md for copyright information.
#ifndef vtkVectorJSON_h
#define vtkVectorJSON_h

#include "vtkVector.h"

#include "nlohmann/json.hpp"

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
    throw nlohmann::detail::type_error::create(
      302, "type must be array, but is " + std::string(j.type_name()));
  }
  if (static_cast<int>(j.size()) != vec.GetSize())
  {
    throw nlohmann::detail::type_error::create(302, "array sizes do not match");
  }
  int ii = 0;
  for (auto it = j.begin(); it != j.end(); ++it, ++ii)
  {
    vec[ii] = static_cast<T>(*it);
  }
}

#endif // vtkVectorJSON_h
