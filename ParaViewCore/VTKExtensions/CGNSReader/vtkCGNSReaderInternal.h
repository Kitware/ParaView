/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCGNSReaderInternal.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//  Copyright (c) 2013-2014 Mickael Philit

/**
 * @class   vtkCGNSReaderInternal
 *
 *     parse a file in "CGNS" format
 *
 * @warning
 *     Only Cell/Vertex data are supported.
 *
 * @par Thanks:
 * Thanks to .
*/

#ifndef vtkCGNSReaderInternal_h
#define vtkCGNSReaderInternal_h

#include <iostream>
#include <map>
#include <string.h> // for inline strcmp
#include <string>
#include <vector>

#include "vtkIdTypeArray.h"
#include "vtkMultiProcessController.h"
#include "vtkPoints.h"
#include "vtk_cgns.h"

namespace CGNSRead
{

namespace detail
{
template <typename T>
struct is_double
{
  static const bool value = false;
};

template <>
struct is_double<double>
{
  static const bool value = true;
};

template <typename T>
struct is_float
{
  static const bool value = false;
};

template <>
struct is_float<float>
{
  static const bool value = true;
};
}

typedef char char_33[33];

//------------------------------------------------------------------------------
class vtkCGNSArraySelection : public std::map<std::string, bool>
{
public:
  void Merge(const vtkCGNSArraySelection& other)
  {
    vtkCGNSArraySelection::const_iterator iter = other.begin();
    for (; iter != other.end(); ++iter)
    {
      (*this)[iter->first] = iter->second;
    }
  }

  void AddArray(const char* name, bool status = true) { (*this)[name] = status; }

  bool ArrayIsEnabled(const char* name)
  {
    vtkCGNSArraySelection::iterator iter = this->find(name);
    if (iter != this->end())
    {
      return iter->second;
    }

    // don't know anything about this array, enable it by default.
    return true;
  }

  bool HasArray(const char* name)
  {
    vtkCGNSArraySelection::iterator iter = this->find(name);
    return (iter != this->end());
  }

  int GetArraySetting(const char* name) { return this->ArrayIsEnabled(name) ? 1 : 0; }

  void SetArrayStatus(const char* name, bool status) { this->AddArray(name, status); }

  const char* GetArrayName(int index)
  {
    int cc = 0;
    for (vtkCGNSArraySelection::iterator iter = this->begin(); iter != this->end(); ++iter)
    {

      if (cc == index)
      {
        return iter->first.c_str();
      }
      cc++;
    }
    return NULL;
  }

  int GetNumberOfArrays() { return static_cast<int>(this->size()); }
};

//------------------------------------------------------------------------------
typedef struct
{
  int cnt; // 0  1 or 3
  int pos; // variable position in zone
  int xyzIndex;
  int owner_pos;
  CGNS_ENUMT(DataType_t) dt;
  char_33 name;
} Variable;

//------------------------------------------------------------------------------
typedef struct
{
  int xyzIndex;
  bool isComponent;
  CGNS_ENUMT(DataType_t) dt;
  char_33 name;
} CGNSVariable;

//------------------------------------------------------------------------------
typedef struct
{
  int numComp;
  char_33 name;
  int xyzIndex[3];
} CGNSVector;

//------------------------------------------------------------------------------
typedef struct
{
  bool isVector;
  int xyzIndex;
  char_33 name;
} VTKVariable;

//------------------------------------------------------------------------------
class ZoneInformation
{
public:
  char_33 name;
};

//------------------------------------------------------------------------------
class FamilyInformation
{
public:
  char_33 name;
  bool isBC;
};

//------------------------------------------------------------------------------
class BaseInformation
{
public:
  char_33 name;

  int cellDim;
  int physicalDim;
  //
  int baseNumber;

  std::vector<int> steps;
  std::vector<double> times;

  // For unsteady meshes :
  // if useGridPointers == True:
  //    loadGridPointers for first zone
  //    and assume every zone use the same
  //    notation
  // else :
  //    assume only one grid is stored
  //    only first grid is read
  //
  // For unsteady flow
  // if useFlowPointers == True :
  //    same behavior as GridPointers
  // else if ( nstates > 1 ) :
  //    assume flow_solution are sorted
  //    to keep VisIt like behavior
  // else :
  //    only first solution is read
  //

  bool useGridPointers; // for unsteady mesh
  bool useFlowPointers; // for unsteady flow

  std::vector<CGNSRead::FamilyInformation> family;
  std::map<std::string, double> referenceState;

  int nzones;

  // std::vector<CGNSRead::zone> zone;
  vtkCGNSArraySelection PointDataArraySelection;
  vtkCGNSArraySelection CellDataArraySelection;
};

//------------------------------------------------------------------------------
class vtkCGNSMetaData
{
public:
  /**
   * quick parsing of cgns file to get interesting information
   * from a VTK point of view
   */
  bool Parse(const char* cgnsFileName);

  /**
   * return number of base nodes
   */
  int GetNumberOfBaseNodes() { return static_cast<int>(this->baseList.size()); }

  /**
   * return const reference to a base information
   */
  const CGNSRead::BaseInformation& GetBase(int numBase) { return this->baseList[numBase]; }

  /**
   * return reference to GlobalTime
   */
  std::vector<double>& GetTimes() { return this->GlobalTime; }

  /**
   * print object debugging purpose
   */
  void PrintSelf(std::ostream& os);

  void Broadcast(vtkMultiProcessController* controller, int rank);

  //@{
  /**
   * Constructor/Destructor
   */
  vtkCGNSMetaData();
  ~vtkCGNSMetaData();
  //@}

private:
  vtkCGNSMetaData(const vtkCGNSMetaData&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCGNSMetaData&) VTK_DELETE_FUNCTION;

  std::vector<CGNSRead::BaseInformation> baseList;
  std::string LastReadFilename;
  // Not very elegant :
  std::vector<double> GlobalTime;
};

//------------------------------------------------------------------------------
// compare name return true if name1 == name2
inline bool compareName(const char_33 nameOne, const char_33 nameTwo)
{
  return (strncmp(nameOne, nameTwo, 32) == 0);
}

//------------------------------------------------------------------------------
// remove trailing whitespaces
inline void removeTrailingWhiteSpaces(char_33 name)
{
  char* end = name + strlen(name) - 1;
  while (end >= name && isspace(*end))
  {
    --end;
  }
  ++end;
  assert(end >= name && end < name + 33);
  *end = '\0';
}

//------------------------------------------------------------------------------
// get vector from name
inline std::vector<CGNSVector>::iterator getVectorFromName(
  std::vector<CGNSVector>& vectorList, const char_33 name)
{
  for (std::vector<CGNSVector>::iterator iter = vectorList.begin(); iter != vectorList.end();
       ++iter)
  {
    if (strncmp(iter->name, name, 31) == 0)
    {
      return iter;
    }
  }
  return vectorList.end();
}

//------------------------------------------------------------------------------
inline bool isACGNSVariable(const std::vector<CGNSVariable>& varList, const char_33 name)
{
  for (std::vector<CGNSVariable>::const_iterator iter = varList.begin(); iter != varList.end();
       ++iter)
  {
    if (strncmp(iter->name, name, 32) == 0)
    {
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void fillVectorsFromVars(std::vector<CGNSRead::CGNSVariable>& vars,
  std::vector<CGNSRead::CGNSVector>& vectors, const int physicalDim);
//------------------------------------------------------------------------------
int setUpRind(const int cgioNum, const double rindId, int* rind);
//------------------------------------------------------------------------------
/**
 * Find the first node with the given `label`. If `name` is non-NULL, then the
 * first node with given `label` that has the given `name` as well.
 */
int getFirstNodeId(
  const int cgioNum, const double parentId, const char* label, double* id, const char* name = NULL);
//------------------------------------------------------------------------------
int get_section_connectivity(const int cgioNum, const double cgioSectionId, const int dim,
  const cgsize_t* srcStart, const cgsize_t* srcEnd, const cgsize_t* srcStride,
  const cgsize_t* memStart, const cgsize_t* memEnd, const cgsize_t* memStride,
  const cgsize_t* memDim, vtkIdType* localElements);
//------------------------------------------------------------------------------
int GetVTKElemType(
  CGNS_ENUMT(ElementType_t) elemType, bool& higherOrderWarning, bool& cgnsOrderFlag);
//------------------------------------------------------------------------------
void CGNS2VTKorder(const vtkIdType size, const int* cells_types, vtkIdType* elements);
//------------------------------------------------------------------------------
void CGNS2VTKorderMonoElem(const vtkIdType size, const int cell_type, vtkIdType* elements);
//------------------------------------------------------------------------------
template <typename T, typename Y>
int get_XYZ_mesh(const int cgioNum, const std::vector<double>& gridChildId,
  const std::size_t& nCoordsArray, const int cellDim, const vtkIdType nPts,
  const cgsize_t* srcStart, const cgsize_t* srcEnd, const cgsize_t* srcStride,
  const cgsize_t* memStart, const cgsize_t* memEnd, const cgsize_t* memStride,
  const cgsize_t* memDims, vtkPoints* points)
{
  T* coords = static_cast<T*>(points->GetVoidPointer(0));
  T* currentCoord = static_cast<T*>(&(coords[0]));

  CGNSRead::char_33 coordName;
  std::size_t len;
  bool sameType = true;
  double coordId;

  memset(coords, 0, 3 * nPts * sizeof(T));

  for (std::size_t c = 1; c <= nCoordsArray; ++c)
  {
    // Read CoordName
    if (cgio_get_name(cgioNum, gridChildId[c - 1], coordName) != CG_OK)
    {
      char message[81];
      cgio_error_message(message);
      std::cerr << "get_XYZ_mesh : cgio_get_name :" << message;
    }

    // Read node data type
    CGNSRead::char_33 dataType;
    if (cgio_get_data_type(cgioNum, gridChildId[c - 1], dataType))
    {
      continue;
    }

    if (strcmp(dataType, "R8") == 0)
    {
      const bool doubleType = detail::is_double<T>::value;
      sameType = doubleType;
    }
    else if (strcmp(dataType, "R4") == 0)
    {
      const bool floatType = detail::is_float<T>::value;
      sameType = floatType;
    }
    else
    {
      std::cerr << "Invalid datatype for GridCoordinates\n";
      continue;
    }

    // Determine direction X,Y,Z
    len = strlen(coordName) - 1;
    switch (coordName[len])
    {
      case 'X':
        currentCoord = static_cast<T*>(&(coords[0]));
        break;
      case 'Y':
        currentCoord = static_cast<T*>(&(coords[1]));
        break;
      case 'Z':
        currentCoord = static_cast<T*>(&(coords[2]));
        break;
    }

    coordId = gridChildId[c - 1];

    // quick transfer of data if same data types
    if (sameType == true)
    {
      if (cgio_read_data(cgioNum, coordId, srcStart, srcEnd, srcStride, cellDim, memEnd, memStart,
            memEnd, memStride, (void*)currentCoord))
      {
        char message[81];
        cgio_error_message(message);
        std::cerr << "cgio_read_data :" << message;
      }
    }
    else
    {
      Y* dataArray = 0;
      const cgsize_t memNoStride[3] = { 1, 1, 1 };

      // need to read into temp array to convert data
      dataArray = new Y[nPts];
      if (dataArray == 0)
      {
        std::cerr << "Error allocating buffer array\n";
        break;
      }
      if (cgio_read_data(cgioNum, coordId, srcStart, srcEnd, srcStride, cellDim, memDims, memStart,
            memDims, memNoStride, (void*)dataArray))
      {
        delete[] dataArray;
        char message[81];
        cgio_error_message(message);
        std::cerr << "Buffer array cgio_read_data :" << message;
        break;
      }
      for (vtkIdType ii = 0; ii < nPts; ++ii)
      {
        currentCoord[memStride[0] * ii] = static_cast<T>(dataArray[ii]);
      }
      delete[] dataArray;
    }
  }
  return 0;
}
}

#endif // vtkCGNSReaderInternal_h
// VTK-HeaderTest-Exclude: vtkCGNSReaderInternal.h
