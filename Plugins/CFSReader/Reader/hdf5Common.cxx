// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-FileCopyrightText: Copyright (c) 2022 Verein zur Foerderung der Software openCFS
// SPDX-License-Identifier: BSD-3-Clause
#include "hdf5Common.h"
#include <algorithm>
#include <cassert>

namespace H5CFS
{

//-----------------------------------------------------------------------------
std::string GetObjNameByIdx(hid_t loc, hsize_t idx)
{
  // We first obtain the length of the name, then by the same function write the name to a buffer
  // and third copy it to a std::string we return. There is no shortcut possible

  // nullptr for name gives the length
  ssize_t nameLen = H5Lget_name_by_idx(loc, ".", H5_INDEX_NAME, H5_ITER_NATIVE, idx, nullptr, 0, 0);
  if (nameLen < 0)
  {
    throw std::runtime_error("Was not able to determine name");
  }

  // now, allocate buffer to get the name, we may not write to std::string data directly.
  // std::make_unique is not C++11
  // https://stackoverflow.com/questions/1042940/writing-directly-to-stdstring-internal-buffers
  std::vector<char> buf(nameLen + 1); // the trailing NULL is written but not counted

  if (H5Lget_name_by_idx(loc, ".", H5_INDEX_NAME, H5_ITER_NATIVE, idx, buf.data(), nameLen + 1, 0) <
    0)
  {
    throw std::runtime_error(std::string("error obtaining obj name with index ") +
      std::to_string(idx) + " of size " + std::to_string(nameLen));
  }

  std::string name(buf.data());
  return name;
}

//-----------------------------------------------------------------------------
hid_t OpenGroup(hid_t loc, const std::string& name, bool throwException)
{
  hid_t ret = H5Gopen(loc, name.c_str(), H5P_DEFAULT);
  if (ret < 0 && throwException)
  {
    throw std::runtime_error("cannot open group '" + name + "'");
  }

  return ret;
}

//-----------------------------------------------------------------------------
hid_t OpenDataSet(hid_t loc, const std::string& name)
{
  hid_t ret = H5Dopen(loc, name.c_str(), H5P_DEFAULT);
  if (ret < 0)
  {
    throw std::runtime_error("cannot open group '" + name + "'");
  }

  return ret;
}

//-----------------------------------------------------------------------------
H5G_info_t GetInfo(hid_t group_id)
{
  H5G_info_t info;
  if (H5Gget_info(group_id, &info) < 0)
  {
    throw std::runtime_error("cannot get group info");
  }

  return info;
}

//-----------------------------------------------------------------------------
// FileInfo() is a callback function for H5Giterate().
// By comment we prevent warnings for unused loc_id
herr_t FileInfo(hid_t /* loc_id */, const char* name, void* opdata)
{
  std::vector<std::string>* names = reinterpret_cast<std::vector<std::string>*>(opdata);
  assert(names != nullptr);
  names->push_back(std::string(name));

  return 0;
}

//-----------------------------------------------------------------------------
std::vector<std::string> ParseGroup(hid_t loc_id, const std::string& name)
{
  std::vector<std::string> result;

  H5Giterate(loc_id, name.c_str(), nullptr, H5CFS::FileInfo, &result);

  return result;
}

//-----------------------------------------------------------------------------
bool TestGroupChild(hid_t loc_id, const std::string& group, const std::string& child)
{
  std::vector<std::string> list = ParseGroup(loc_id, group);

  return std::find(list.begin(), list.end(), child) != list.end();
}

//-----------------------------------------------------------------------------
int GetDataSetSize(hid_t loc_id, const char* dsetName)
{
  int val = 0;
  if (H5LTget_dataset_ndims(loc_id, dsetName, &val) < 0)
  {
    throw std::runtime_error("cannot get size of data set '" + std::string(dsetName) + "'");
  }

  return val;
}

//-----------------------------------------------------------------------------
template <>
void ReadAttribute<int>(
  hid_t loc_id, const std::string& objName, const std::string& attrName, int& data)
{
  if (H5LTget_attribute_int(loc_id, objName.c_str(), attrName.c_str(), &data) < 0)
  {
    throw std::runtime_error(std::string("cannot read int attribute ") + objName + "/" + attrName);
  }
}

//-----------------------------------------------------------------------------
template <>
void ReadAttribute<unsigned int>(
  hid_t loc_id, const std::string& objName, const std::string& attrName, unsigned int& data)
{
  if (H5LTget_attribute_uint(loc_id, objName.c_str(), attrName.c_str(), &data) < 0)
  {
    throw std::runtime_error(std::string("cannot read uint attribute ") + objName + "/" + attrName);
  }
}

//-----------------------------------------------------------------------------
template <>
void ReadAttribute<double>(
  hid_t loc_id, const std::string& objName, const std::string& attrName, double& data)
{
  if (H5LTget_attribute_double(loc_id, objName.c_str(), attrName.c_str(), &data) < 0)
  {
    throw std::runtime_error(
      std::string("cannot read double attribute ") + objName + "/" + attrName);
  }
}

//-----------------------------------------------------------------------------
template <>
void ReadAttribute<std::string>(
  hid_t loc_id, const std::string& objName, const std::string& attrName, std::string& data)
{
  // H5LTget_attribute_string is a very poorly described function
  // it has a char* data with OUT: Buffer with data.
  // but it needs the address of the pointer and it is not said who manages the buffer?!
  // we follow stackoverflow but need a type cast
  // https://stackoverflow.com/questions/64467420/how-to-use-the-h5ltget-attribute-string-function

  char* out = nullptr;
  if (H5LTget_attribute_string(loc_id, objName.c_str(), attrName.c_str(),
        reinterpret_cast<char*>(&out)) < 0) // see comment for cast
  {
    throw std::runtime_error(
      std::string("cannot obtain string attribute value for ") + objName + "/" + attrName);
  }

  data = std::string(out);
  free(out);
}

//-----------------------------------------------------------------------------
std::vector<unsigned int> GetArrayDims(hid_t loc, const std::string& name)
{
  hid_t set = H5CFS::OpenDataSet(loc, name);
  hid_t space_id = H5Dget_space(set);

  if (H5Sis_simple(space_id) <= 0)
  {
    throw std::runtime_error(std::string("no simple data space ") + name);
  }

  const int ndims = H5Sget_simple_extent_ndims(space_id);

  std::vector<hsize_t> dims(ndims);
  // nullptr means no max dims
  if (H5Sget_simple_extent_dims(space_id, dims.data(), nullptr) != ndims)
  {
    throw std::runtime_error(std::string("read dimensions not as expected for ") + name);
  }

  H5Sclose(space_id);
  H5Dclose(set);

  std::vector<unsigned int> ret(ndims);
  for (int i = 0; i < ndims; i++)
  {
    // dims is long long, so we cannot directly write to reg.data()
    ret[i] = static_cast<unsigned int>(dims[i]);
  }

  return ret;
}

//-----------------------------------------------------------------------------
unsigned int GetNumberOfEntries(hid_t id, const std::string& name)
{
  hid_t set = H5CFS::OpenDataSet(id, name);
  hid_t dspace = H5Dget_space(set);

  if (H5Sis_simple(dspace) <= 0)
  {
    throw std::runtime_error(std::string("no simple data space ") + name);
  }

  hssize_t npoints = H5Sget_simple_extent_npoints(dspace);

  H5Sclose(dspace);
  H5Dclose(set);

  if (npoints < 0)
  {
    throw std::runtime_error(std::string("cannot get number of elements for ") + name);
  }

  return static_cast<unsigned int>(npoints);
}

//-----------------------------------------------------------------------------
template <>
void ReadDataSet<double>(hid_t loc, const std::string& name, double* out)
{
  assert(out != nullptr);
  if (H5LTread_dataset_double(loc, name.c_str(), out) < 0)
  {
    throw std::runtime_error(std::string("cannot read double dataset ") + name);
  }
}

//-----------------------------------------------------------------------------
template <>
void ReadDataSet<int>(hid_t loc, const std::string& name, int* out)
{
  assert(out != nullptr);
  if (H5LTread_dataset_int(loc, name.c_str(), out) < 0)
  {
    throw std::runtime_error(std::string("cannot read int dataset ") + name);
  }
}

//-----------------------------------------------------------------------------
template <>
void ReadDataSet<unsigned int>(hid_t loc, const std::string& name, unsigned int* out)
{
  H5CFS::ReadDataSet(loc, name, reinterpret_cast<int*>(out)); // no uint version in HL available :(
}

//-----------------------------------------------------------------------------
template <>
void ReadDataSet<std::string>(hid_t loc, const std::string& name, std::string* out)
{
  assert(out != nullptr);
  char* buffer = nullptr;
  if (H5LTread_dataset_string(loc, name.c_str(), reinterpret_cast<char*>(&buffer)) < 0)
  {
    throw std::runtime_error(std::string("cannot read string dataset ") + name);
  }

  *out = std::string(buffer); // convert to string
  free(buffer);
}

//-----------------------------------------------------------------------------
template <typename TYPE>
TYPE ReadDataSet(hid_t loc, const std::string& name)
{
  assert(H5CFS::GetNumberOfEntries(loc, name) == 1);
  TYPE val;
  H5CFS::ReadDataSet(loc, name, &val);
  return val;
}

//-----------------------------------------------------------------------------
template <typename TYPE>
void ReadArray(hid_t loc, const std::string& name, std::vector<TYPE>& data)
{
  data.resize(H5CFS::GetNumberOfEntries(loc, name));
  H5CFS::ReadDataSet(loc, name, data.data()); // directly write to the vector data space
}

//-----------------------------------------------------------------------------
template <>
void ReadArray<std::string>(hid_t loc, const std::string& name, std::vector<std::string>& data)
{
  const unsigned int n = H5CFS::GetNumberOfEntries(loc, name);
  // We use RAII instead of char** buffer = new char*[n] as suggested in h5 documentation
  std::vector<char*> buffer(n);
  if (H5LTread_dataset_string(loc, name.c_str(), reinterpret_cast<char*>(buffer.data())) < 0)
  {
    throw std::runtime_error(std::string("cannot read string array dataset ") + name);
  }

  data.resize(n);
  for (unsigned int i = 0; i < n; i++)
  {
    data[i] = std::string(buffer[i]);
    free(buffer[i]);
  }
}

//-----------------------------------------------------------------------------
/** little helper to for root based key for simulation results. */
std::string GetMultiStepKey(unsigned int msStep, bool isHistory)
{
  std::stringstream ss;
  ss << "/Results/";
  ss << (isHistory ? "History" : "Mesh");
  ss << "/MultiStep_";
  ss << std::to_string(msStep);
  return ss.str();
}

//-----------------------------------------------------------------------------
hid_t GetMultiStepGroup(hid_t root, unsigned int msStep, bool isHistory)
{
  return H5CFS::OpenGroup(root, H5CFS::GetMultiStepKey(msStep, isHistory));
}

//-----------------------------------------------------------------------------
hid_t GetStepGroup(hid_t root, unsigned int msStep, unsigned int stepNum)
{
  // no history but element/node result
  std::string key = H5CFS::GetMultiStepKey(msStep, false) + std::string("/Step_") +
    std::string(std::to_string(stepNum));
  return H5CFS::OpenGroup(root, key);
}

//-----------------------------------------------------------------------------
std::string MapUnknownTypeAsString(EntityType t)
{
  std::string definedOn;

  switch (t)
  {
    case H5CFS::NODE:
      definedOn = "Nodes";
      break;
    case H5CFS::EDGE:
      definedOn = "Edges";
      break;
    case H5CFS::FACE:
      definedOn = "Faces";
      break;
    case H5CFS::ELEMENT:
    case H5CFS::SURF_ELEM:
      definedOn = "Elements";
      break;
    case H5CFS::PFEM:
      definedOn = "Nodes";
      break;
    case H5CFS::REGION:
      definedOn = "Regions";
      break;
    case H5CFS::SURF_REGION:
      definedOn = "ElementGroup";
      break;
    case H5CFS::NODE_GROUP:
      definedOn = "NodeGroup";
      break;
    case H5CFS::COIL:
      definedOn = "Coils";
      break;
    case H5CFS::FREE:
      definedOn = "Unknowns";
      break;
  }

  return definedOn;
}

template void ReadArray<unsigned int>(
  hid_t loc, const std::string& name, std::vector<unsigned int>& data);
template void ReadArray<int>(hid_t loc, const std::string& name, std::vector<int>& data);
template void ReadArray<double>(hid_t loc, const std::string& name, std::vector<double>& data);
template unsigned int ReadDataSet<unsigned int>(hid_t loc, const std::string& name);
template std::string ReadDataSet<std::string>(hid_t loc, const std::string& name);

} // end of namespace H5CFS
