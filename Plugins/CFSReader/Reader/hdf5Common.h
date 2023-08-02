// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-FileCopyrightText: Copyright (c) 2022 Verein zur Foerderung der Software openCFS
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   hdf5Common
 * @brief   provides a template based interface of hdf5 c-api for hdf5Reader
 *
 * Only the hdf5 c-api and c-based high level api are used. This class provides a
 * templated C++ interface based on the c-api for the hdf5Reader. Also some basic element type
 * naming of openCFS is made available here.
 */

#ifndef HDF5COMMON_H
#define HDF5COMMON_H

#include <array>
#include <exception>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <vtk_hdf5.h>

namespace H5CFS
{

/**
 * openCFS element types
 */
typedef enum
{
  ET_UNDEF = 0,
  ET_POINT = 1,
  ET_LINE2 = 2,
  ET_LINE3 = 3,
  ET_TRIA3 = 4,
  ET_TRIA6 = 5,
  ET_QUAD4 = 6,
  ET_QUAD8 = 7,
  ET_QUAD9 = 8,
  ET_TET4 = 9,
  ET_TET10 = 10,
  ET_HEXA8 = 11,
  ET_HEXA20 = 12,
  ET_HEXA27 = 13,
  ET_PYRA5 = 14,
  ET_PYRA13 = 15,
  ET_PYRA14 = 19,
  ET_WEDGE6 = 16,
  ET_WEDGE15 = 17,
  ET_WEDGE18 = 18
} ElemType;

/**
 * Number of nodes sorted by H5CFS::ElemType
 */
const std::array<int, 20> NUM_ELEM_NODES = {
  0,  // ET_UNDEF
  1,  // ET_POINT
  2,  // ET_LINE2
  3,  // ET_LINE3
  3,  // ET_TRIA3
  6,  // ET_TRIA6
  4,  // ET_QUAD4
  8,  // ET_QUAD8
  9,  // ET_QUAD9
  4,  // ET_TET4
  10, // ET_TET10
  8,  // ET_HEXA8
  20, // ET_HEXA20
  27, // ET_HEXA27
  5,  // ET_PYRA5
  13, // ET_PYRA13
  6,  // ET_WEDGE6
  15, // ET_WEDGE15
  18, // ET_WEDGE18
  14  // ET_PYRA14
};

/**
 * openCFS entry types
 */
typedef enum
{
  UNKNOWN = 0,
  SCALAR = 1,
  VECTOR = 3,
  TENSOR = 6,
  STRING = 32
} EntryType;

/**
 * openCFS entity types
 */
typedef enum
{
  NODE = 1,
  EDGE = 2,
  FACE = 3,
  ELEMENT = 4,
  SURF_ELEM = 5,
  PFEM = 6,
  REGION = 7,
  SURF_REGION = 8,
  NODE_GROUP = 9,
  COIL = 10,
  FREE = 11
} EntityType;

/**
 * openCFS analysis type
 */
typedef enum
{
  NO_ANALYSIS_TYPE = -1,
  STATIC = 1,
  TRANSIENT = 2,
  HARMONIC = 3,
  EIGENFREQUENCY = 4,
  MULTIHARMONIC = 5,
  BUCKLING = 6,
  EIGENVALUE = 7
} AnalysisType;

/**
 * Meta data of a result within the openCFS h5 file
 */
struct ResultInfo
{
  std::string name;
  std::string unit;
  std::vector<std::string> dofNames;
  EntryType entryType = H5CFS::UNKNOWN;
  EntityType listType = H5CFS::FREE;
  std::string listName;
  bool isHistory = false;
};

/**
 * The values of a single openCFS result
 */
struct Result
{
  std::shared_ptr<ResultInfo> resultInfo;
  bool isComplex = false;
  std::vector<double> realVals;
  std::vector<double> imaginaryVals;
};

/**
 * Wrapper for H5 c-api H5Lget_name_by_idx
 *
 * Including memory and error management
 * @throws std::runtime_error
 */
std::string GetObjNameByIdx(hid_t loc, hsize_t idx);

/**
 * Wrapper for H5 c-api H5Gopen
 *
 * @param throwException throw an exception on error
 * @return negative if not found but not throwException
 * @throws std::runtime_error
 */
hid_t OpenGroup(hid_t loc, const std::string& name, bool throwException = true);

/**
 * Wrapper for H5 c-api H5Dopen
 *
 * @throws std::runtime_error
 */
hid_t OpenDataSet(hid_t loc, const std::string& name);

/**
 * Wrapper for H5 c-api H5Gget_info
 *
 * @throws std::runtime_error
 */
H5G_info_t GetInfo(hid_t group_id);

/**
 * Parses the labels of a group content using H5Giterate()
 *
 * @return a vector of all labels within the specified group
 */
std::vector<std::string> ParseGroup(hid_t loc_id, const std::string& name);

/**
 * This is a helper function of ParseGroup()
 *
 * This is the callback function for H5Giterate from ParseGroup()
 */
herr_t FileInfo(hid_t loc_id, const char* name, void* opdata);

/**
 * This is a service function based on ParseGroup()
 *
 * @param child test if this label is in the group
 */
bool TestGroupChild(hid_t loc_id, const std::string& group, const std::string& child);

/**
 * Wrapper for H5 c-api H5LTget_dataset_ndims
 *
 * @throws std::runtime_error
 */
int GetDataSetSize(hid_t loc_id, const char* dsetName);

/**
 * Read h5 attribute via H5LTget_attribute_uint, and other types.
 *
 * Implemented via template specific implementations.
 * @param loc_id group or file
 * @param obj_name element of loc_id object
 * @param data output
 * @throws std::runtime_error
 */
template <typename TYPE>
void ReadAttribute(
  hid_t loc_id, const std::string& obj_name, const std::string& attrName, TYPE& data);

/**
 * Convenience ReadAttribute which provides own variable space
 */
template <typename TYPE>
TYPE ReadAttribute(hid_t loc_id, const std::string& obj_name, const std::string& attrName)
{
  TYPE val;
  ReadAttribute(loc_id, obj_name, attrName, val);
  return val;
}

/**
 * Read an attribute of the given group id.
 *
 * Assumes the current object ('.')
 * @see ReadAttribute
 */
template <typename TYPE>
TYPE ReadAttribute(hid_t loc_id, const std::string& attrName)
{
  TYPE val;
  ReadAttribute(loc_id, ".", attrName, val); // object name is this name
  return val;
}

/**
 * Wrapper to the HL H5LTread_dataset_* functions
 *
 * Template instants for the types are implemented.
 * @throws std::runtime_error
 */
template <typename TYPE>
void ReadDataSet(hid_t loc, const std::string& name, TYPE* out);

/**
 * Convenience variant of ReadDataSet with return value
 *
 * @throws std::runtime_error
 */
template <typename TYPE>
TYPE ReadDataSet(hid_t loc, const std::string& name);

/**
 * Retrieve rank and dimensionality of a dataset
 *
 * @return total number of entries in the dataset
 * @throws std::runtime_error
 */
std::vector<unsigned int> GetArrayDims(hid_t loc, const std::string& name);

/**
 * Return number of entries of a dataset / array
 *
 * @throws std::runtime_error
 */
unsigned int GetNumberOfEntries(hid_t id, const std::string& name);

/**
 * Retrieve array data from a dataset
 *
 * Read data from a an dataset of arbitrary dimension into a linear buffer
 * Read data from a dataset into a stl vector
 * @throws std::runtime_error
 */
template <typename TYPE>
void ReadArray(hid_t loc, const std::string& name, std::vector<TYPE>& data);

/**
 * Obtain grid result group for specified multisequence step.
 *
 * @param msStep 1-based multisequence step. There is always at least the first one.
 * @param isHistory indicated region result instead of element/node result
 * @throws std::runtime_error
 */
hid_t GetMultiStepGroup(hid_t root, unsigned int msStep, bool isHistory);

/**
 * Obtain grid result group for specified step in a given multistep element/node result
 *
 * @param stepNum the 0-based step for nodal and element results
 * @see GetMultiStepGroup() */
hid_t GetStepGroup(hid_t root, unsigned int msStep, unsigned int stepNum);

/**
 * Map EntityUnknownType enum to string representation
 */
std::string MapUnknownTypeAsString(EntityType t);

} // end of namespace H5CFS

#endif // HDF5COMMON_H
