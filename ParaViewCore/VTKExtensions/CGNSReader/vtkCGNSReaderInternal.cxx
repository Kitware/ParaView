/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCGNSReaderInternal.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Copyright 2013-2014 Mickael Philit.

#include "vtkCGNSReaderInternal.h"

#include "cgio_helpers.h"
#include "vtkCellType.h"
#include <algorithm>

namespace CGNSRead
{
//------------------------------------------------------------------------------
int setUpRind(const int cgioNum, const double rindId, int* rind)
{
  CGNSRead::char_33 dataType;
  if (cgio_get_data_type(cgioNum, rindId, dataType) != CG_OK)
  {
    std::cerr << "Problem while reading Rind data type\n";
    return 1;
  }

  if (strcmp(dataType, "I4") == 0)
  {
    std::vector<int> mdata;
    CGNSRead::readNodeData<int>(cgioNum, rindId, mdata);
    for (std::size_t index = 0; index < mdata.size(); index++)
    {
      rind[index] = static_cast<int>(mdata[index]);
    }
  }
  else if (strcmp(dataType, "I8") == 0)
  {
    std::vector<cglong_t> mdata;
    CGNSRead::readNodeData<cglong_t>(cgioNum, rindId, mdata);
    for (std::size_t index = 0; index < mdata.size(); index++)
    {
      rind[index] = static_cast<int>(mdata[index]);
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
int getFirstNodeId(
  const int cgioNum, const double parentId, const char* label, double* id, const char* name)
{
  int nId, n, nChildren, len;
  int ier = 0;
  char nodeLabel[CGIO_MAX_NAME_LENGTH + 1];
  char nodeName[CGIO_MAX_NAME_LENGTH + 1];

  if (cgio_number_children(cgioNum, parentId, &nChildren) != CG_OK)
  {
    return 1;
  }
  if (nChildren < 1)
  {
    return 1;
  }

  double* idList = new double[nChildren];
  cgio_children_ids(cgioNum, parentId, 1, nChildren, &len, idList);
  if (len != nChildren)
  {
    delete[] idList;
    std::cerr << "Mismatch in number of children and child IDs read" << std::endl;
    return 1;
  }

  nId = 0;
  for (n = 0; n < nChildren; n++)
  {

    if (cgio_get_label(cgioNum, idList[n], nodeLabel))
    {
      return 1;
    }

    if (name != NULL && cgio_get_name(cgioNum, idList[n], nodeName))
    {
      return 1;
    }

    if (0 == strcmp(nodeLabel, label) && (name == NULL || 0 == strcmp(nodeName, name)))
    {
      *id = idList[n];
      nId = 1;
    }
    else
    {
      cgio_release_id(cgioNum, idList[n]);
    }
    if (nId != 0)
    {
      break;
    }
  }
  n++;
  while (n < nChildren)
  {
    cgio_release_id(cgioNum, idList[n]);
    n++;
  }

  if (nId < 1)
  {
    *id = 0.0;
    ier = 1;
  }

  delete[] idList;
  return ier;
}

//------------------------------------------------------------------------------
int get_section_connectivity(const int cgioNum, const double cgioSectionId, const int dim,
  const cgsize_t* srcStart, const cgsize_t* srcEnd, const cgsize_t* srcStride,
  const cgsize_t* memStart, const cgsize_t* memEnd, const cgsize_t* memStride,
  const cgsize_t* memDim, vtkIdType* localElements)
{
  const char* connectivityPath = "ElementConnectivity";
  double cgioElemConnectId;
  char dataType[3];
  std::size_t sizeOfCnt = 0;

  cgio_get_node_id(cgioNum, cgioSectionId, connectivityPath, &cgioElemConnectId);
  cgio_get_data_type(cgioNum, cgioElemConnectId, dataType);

  if (strcmp(dataType, "I4") == 0)
  {
    sizeOfCnt = sizeof(int);
  }
  else if (strcmp(dataType, "I8") == 0)
  {
    sizeOfCnt = sizeof(cglong_t);
  }
  else
  {
    std::cerr << "ElementConnectivity data_type unknown\n";
  }

  if (sizeOfCnt == sizeof(vtkIdType))
  {
    if (cgio_read_data(cgioNum, cgioElemConnectId, srcStart, srcEnd, srcStride, dim, memDim,
          memStart, memEnd, memStride, (void*)localElements) != CG_OK)
    {
      char message[81];
      cgio_error_message(message);
      std::cerr << "cgio_read_data :" << message;
      return 1;
    }
  }
  else
  {
    // Need to read into temp array to convert data
    cgsize_t nn = 1;
    for (int ii = 0; ii < dim; ii++)
    {
      nn *= memDim[ii];
    }
    if (sizeOfCnt == sizeof(int))
    {
      int* data = new int[nn];
      if (data == 0)
      {
        std::cerr << "Allocation failed for temporary connectivity array\n";
      }

      if (cgio_read_data(cgioNum, cgioElemConnectId, srcStart, srcEnd, srcStride, dim, memDim,
            memStart, memEnd, memStride, (void*)data) != CG_OK)
      {
        delete[] data;
        char message[81];
        cgio_error_message(message);
        std::cerr << "cgio_read_data :" << message;
        return 1;
      }
      for (cgsize_t n = 0; n < nn; n++)
      {
        localElements[n] = static_cast<vtkIdType>(data[n]);
      }
      delete[] data;
    }
    else if (sizeOfCnt == sizeof(cglong_t))
    {
      cglong_t* data = new cglong_t[nn];
      if (data == 0)
      {
        std::cerr << "Allocation failed for temporary connectivity array\n";
        return 1;
      }
      if (cgio_read_data(cgioNum, cgioElemConnectId, srcStart, srcEnd, srcStride, dim, memDim,
            memStart, memEnd, memStride, (void*)data) != CG_OK)
      {
        delete[] data;
        char message[81];
        cgio_error_message(message);
        std::cerr << "cgio_read_data :" << message;
        return 1;
      }
      for (cgsize_t n = 0; n < nn; n++)
      {
        localElements[n] = static_cast<vtkIdType>(data[n]);
      }
      delete[] data;
    }
  }
  cgio_release_id(cgioNum, cgioElemConnectId);
  return 0;
}

//------------------------------------------------------------------------------
int GetVTKElemType(
  CGNS_ENUMT(ElementType_t) elemType, bool& higherOrderWarning, bool& cgnsOrderFlag)
{
  int cellType;
  higherOrderWarning = false;
  cgnsOrderFlag = false;
  //
  switch (elemType)
  {
    case CGNS_ENUMV(NODE):
      cellType = VTK_VERTEX;
      break;
    case CGNS_ENUMV(BAR_2):
      cellType = VTK_LINE;
      break;
    case CGNS_ENUMV(BAR_3):
      cellType = VTK_QUADRATIC_EDGE;
      higherOrderWarning = true;
      break;
    // case CGNS_ENUMV(BAR_4):
    //  cellType = VTK_CUBIC_LINE;
    //  higherOrderWarning = true;
    //  break;
    case CGNS_ENUMV(TRI_3):
      cellType = VTK_TRIANGLE;
      break;
    case CGNS_ENUMV(TRI_6):
      cellType = VTK_QUADRATIC_TRIANGLE;
      higherOrderWarning = true;
      break;
    case CGNS_ENUMV(QUAD_4):
      cellType = VTK_QUAD;
      break;
    case CGNS_ENUMV(QUAD_8):
      cellType = VTK_QUADRATIC_QUAD;
      higherOrderWarning = true;
      break;
    case CGNS_ENUMV(QUAD_9):
      cellType = VTK_BIQUADRATIC_QUAD;
      higherOrderWarning = true;
      break;
    case CGNS_ENUMV(TETRA_4):
      cellType = VTK_TETRA;
      break;
    case CGNS_ENUMV(TETRA_10):
      cellType = VTK_QUADRATIC_TETRA;
      higherOrderWarning = true;
      break;
    case CGNS_ENUMV(PYRA_5):
      cellType = VTK_PYRAMID;
      break;
    case CGNS_ENUMV(PYRA_14):
      cellType = VTK_QUADRATIC_PYRAMID;
      higherOrderWarning = true;
      break;
    case CGNS_ENUMV(PENTA_6):
      cellType = VTK_WEDGE;
      break;
    case CGNS_ENUMV(PENTA_15):
      cellType = VTK_QUADRATIC_WEDGE;
      higherOrderWarning = true;
      cgnsOrderFlag = true;
      break;
    case CGNS_ENUMV(PENTA_18):
      cellType = VTK_BIQUADRATIC_QUADRATIC_WEDGE;
      higherOrderWarning = true;
      cgnsOrderFlag = true;
      break;
    case CGNS_ENUMV(HEXA_8):
      cellType = VTK_HEXAHEDRON;
      break;
    case CGNS_ENUMV(HEXA_20):
      cellType = VTK_QUADRATIC_HEXAHEDRON;
      higherOrderWarning = true;
      cgnsOrderFlag = true;
      break;
    case CGNS_ENUMV(HEXA_27):
      cellType = VTK_TRIQUADRATIC_HEXAHEDRON;
      higherOrderWarning = true;
      cgnsOrderFlag = true;
      break;
    default:
      cellType = VTK_EMPTY_CELL;
      break;
  }
  return cellType;
}
//----------------------------------------------------------------------
// static const int NULL_translate[27] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
//                                  16,17,18,19,20,21,22,23,24,25,26};

// CGNS --> VTK ordering of Elements
// static const int NODE_ToVTK[1] = { 0 };
//
// static const int BAR_2_ToVTK[2] = { 0, 1 };
//
// static const int BAR_3_ToVTK[3] = { 0, 1, 2 };
//
// static const int BAR_4_ToVTK[4] = { 0, 1, 2, 3 };
//
// static const int TRI_3_ToVTK[3] = { 0, 1, 2 };
//
// static const int TRI_6_ToVTK[6] = { 0, 1, 2, 3, 4, 5 };
//
// static const int QUAD_4_ToVTK[4] = { 0, 1, 2, 3 };
//
// static const int QUAD_8_ToVTK[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
//
// static const int QUAD_9_ToVTK[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
//
// static const int TETRA_4_ToVTK[4] = { 0, 1, 2, 3 };
//
// static const int TETRA_10_ToVTK[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
//
// static const int PYRA_5_ToVTK[5] = { 0, 1, 2, 3, 4 };
//
// static const int PYRA_14_ToVTK[14] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
//
// static const int PENTA_6_ToVTK[6] = { 0, 1, 2, 3, 4, 5 };

static const int PENTA_15_ToVTK[15] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 13, 14, 9, 10, 11 };

static const int PENTA_18_ToVTK[18] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 13, 14, 9, 10, 11, 15, 16,
  17 };

// static const int HEXA_8_ToVTK[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

static const int HEXA_20_ToVTK[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 16, 17, 18, 19, 12, 13,
  14, 15 };

static const int HEXA_27_ToVTK[27] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 16, 17, 18, 19, 12, 13,
  14, 15, 24, 22, 21, 23, 20, 25, 26 };
//------------------------------------------------------------------------------
inline const int* getTranslator(const int cellType)
{
  switch (cellType)
  {
    case VTK_VERTEX:
    case VTK_LINE:
    case VTK_QUADRATIC_EDGE:
    case VTK_CUBIC_LINE:
    case VTK_TRIANGLE:
    case VTK_QUADRATIC_TRIANGLE:
    case VTK_QUAD:
    case VTK_QUADRATIC_QUAD:
    case VTK_BIQUADRATIC_QUAD:
    case VTK_TETRA:
    case VTK_QUADRATIC_TETRA:
    case VTK_PYRAMID:
    case VTK_QUADRATIC_PYRAMID:
    case VTK_WEDGE:
      return NULL;
    case VTK_QUADRATIC_WEDGE:
      return CGNSRead::PENTA_15_ToVTK;
    case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
      return CGNSRead::PENTA_18_ToVTK;
    case VTK_HEXAHEDRON:
      return NULL;
    case VTK_QUADRATIC_HEXAHEDRON:
      return CGNSRead::HEXA_20_ToVTK;
    case VTK_TRIQUADRATIC_HEXAHEDRON:
      return CGNSRead::HEXA_27_ToVTK;
    default:
      return NULL;
  }
}

//------------------------------------------------------------------------------
void CGNS2VTKorder(const vtkIdType size, const int* cells_types, vtkIdType* elements)
{
  const int maxPointsPerCells = 27;
  int tmp[maxPointsPerCells];
  const int* translator;
  vtkIdType pos = 0;
  for (vtkIdType icell = 0; icell < size; ++icell)
  {
    translator = getTranslator(cells_types[icell]);
    vtkIdType numPointsPerCell = elements[pos];
    pos++;
    if (translator != NULL)
    {
      for (vtkIdType ip = 0; ip < numPointsPerCell; ++ip)
      {
        tmp[ip] = elements[translator[ip] + pos];
      }
      for (vtkIdType ip = 0; ip < numPointsPerCell; ++ip)
      {
        elements[pos + ip] = tmp[ip];
      }
    }
    pos += numPointsPerCell;
  }
}

//------------------------------------------------------------------------------
void CGNS2VTKorderMonoElem(const vtkIdType size, const int cell_type, vtkIdType* elements)
{
  const int maxPointsPerCells = 27;

  int tmp[maxPointsPerCells];
  const int* translator;
  translator = getTranslator(cell_type);
  if (translator == NULL)
  {
    return;
  }

  vtkIdType pos = 0;
  for (vtkIdType icell = 0; icell < size; ++icell)
  {
    vtkIdType numPointsPerCell = elements[pos];
    pos++;
    for (vtkIdType ip = 0; ip < numPointsPerCell; ++ip)
    {
      tmp[ip] = elements[translator[ip] + pos];
    }
    for (vtkIdType ip = 0; ip < numPointsPerCell; ++ip)
    {
      elements[pos + ip] = tmp[ip];
    }
    pos += numPointsPerCell;
  }
}

//------------------------------------------------------------------------------
bool testValidVector(const CGNSVector& item)
{
  // apply some logic and return true or false
  return (item.numComp == 0);
}

//------------------------------------------------------------------------------
void fillVectorsFromVars(std::vector<CGNSRead::CGNSVariable>& vars,
  std::vector<CGNSRead::CGNSVector>& vectors, const int physicalDim)
{
  // get number of scalars and vectors
  const std::size_t nvar = vars.size();
  std::size_t len;
  char_33 name;

  for (std::size_t n = 0; n < nvar; ++n)
  {
    vars[n].isComponent = false;
    vars[n].xyzIndex = 0;
  }

  for (std::size_t n = 0; n < nvar; ++n)
  {
    len = strlen(vars[n].name) - 1;
    switch (vars[n].name[len])
    {
      case 'X':
        vars[n].xyzIndex = 1;
        vars[n].isComponent = true;
        break;
      case 'Y':
        vars[n].xyzIndex = 2;
        vars[n].isComponent = true;
        break;
      case 'Z':
        vars[n].xyzIndex = 3;
        vars[n].isComponent = true;
        break;
    }
    if (vars[n].isComponent == true)
    {
      strcpy(name, vars[n].name);
      name[len] = '\0';
      std::vector<CGNSRead::CGNSVector>::iterator iter = CGNSRead::getVectorFromName(vectors, name);
      if (iter != vectors.end())
      {
        iter->numComp += vars[n].xyzIndex;
        iter->xyzIndex[vars[n].xyzIndex - 1] = (int)n;
      }
      else
      {
        CGNSRead::CGNSVector newVector;
        newVector.xyzIndex[0] = -1;
        newVector.xyzIndex[1] = -1;
        newVector.xyzIndex[2] = -1;
        newVector.numComp = vars[n].xyzIndex;
        newVector.xyzIndex[vars[n].xyzIndex - 1] = (int)n;
        strcpy(newVector.name, name);
        vectors.push_back(newVector);
      }
    }
  }

  // Detect and tag invalid vector :
  bool invalid = false;
  for (std::vector<CGNSRead::CGNSVector>::iterator iter = vectors.begin(); iter != vectors.end();
       ++iter)
  {
    // Check if number of component agrees with phys_dim
    if (((physicalDim == 3) && (iter->numComp != 6)) ||
      ((physicalDim == 2) && (iter->numComp != 3)))
    {
      for (int index = 0; index < physicalDim; index++)
      {
        int nv = iter->xyzIndex[index];
        if (nv >= 0)
        {
          vars[nv].isComponent = false;
        }
      }
      iter->numComp = 0;
      invalid = true;
    }
    // Check if a variable is present with a similar
    // name as the vector being built
    if (CGNSRead::isACGNSVariable(vars, iter->name) == true)
    {
      // vtkWarningMacro ( "Warning, vector " << iter->name
      //                  << " can't be assembled." << std::endl );
      for (int index = 0; index < physicalDim; index++)
      {
        int n = iter->xyzIndex[index];
        if (n >= 0)
        {
          vars[n].isComponent = false;
        }
      }
      iter->numComp = 0;
      invalid = true;
    }
    if (iter->numComp > 0)
    {
      // Check if DataType_t are identical for all components
      if ((vars[iter->xyzIndex[0]].dt != vars[iter->xyzIndex[1]].dt) ||
        (vars[iter->xyzIndex[0]].dt != vars[iter->xyzIndex[physicalDim - 1]].dt))
      {
        for (int index = 0; index < physicalDim; index++)
        {
          int n = iter->xyzIndex[index];
          if (n >= 0)
          {
            vars[n].isComponent = false;
          }
        }
        iter->numComp = 0;
        invalid = true;
      }
    }
  }
  // Remove invalid vectors
  if (invalid == true)
  {
    vectors.erase(
      std::remove_if(vectors.begin(), vectors.end(), CGNSRead::testValidVector), vectors.end());
  }
}

//------------------------------------------------------------------------------
bool vtkCGNSMetaData::Parse(const char* cgnsFileName)
{

  if (!cgnsFileName)
  {
    return false;
  }

  if (this->LastReadFilename == cgnsFileName)
  {
    return true;
  }

  int cgioNum;
  int ier;
  double rootId;
  char nodeLabel[CGIO_MAX_NAME_LENGTH + 1];

  // use cgio routine to open the file
  if (cgio_open_file(cgnsFileName, CGIO_MODE_READ, CG_FILE_NONE, &cgioNum) != CG_OK)
  {
    cgio_error_exit("cgio_file_open");
  }
  if (cgio_get_root_id(cgioNum, &rootId) != CG_OK)
  {
    cgio_error_exit("cgio_get_root_id");
  }

  // Get base id list :
  std::vector<double> baseIds;
  ier = readBaseIds(cgioNum, rootId, baseIds);
  if (ier != 0)
  {
    return false;
  }

  if (this->baseList.size() > 0)
  {
    this->baseList.clear();
  }
  this->baseList.resize(baseIds.size());
  // Read base list
  for (std::size_t numBase = 0; numBase < baseIds.size(); numBase++)
  {
    // base names for later selection
    readBaseCoreInfo(cgioNum, baseIds[numBase], this->baseList[numBase]);

    std::vector<double> baseChildId;

    getNodeChildrenId(cgioNum, baseIds[numBase], baseChildId);

    std::size_t nzones = 0;
    std::size_t nn;
    for (nzones = 0, nn = 0; nn < baseChildId.size(); ++nn)
    {
      if (cgio_get_label(cgioNum, baseChildId[nn], nodeLabel) != CG_OK)
      {
        return false;
      }

      if (strcmp(nodeLabel, "Zone_t") == 0)
      {
        if (nzones < nn)
        {
          baseChildId[nzones] = baseChildId[nn];
        }
        nzones++;
      }
      else if (strcmp(nodeLabel, "Family_t") == 0)
      {
        readBaseFamily(cgioNum, baseChildId[nn], this->baseList[numBase]);
      }
      else if (strcmp(nodeLabel, "BaseIterativeData_t") == 0)
      {
        readBaseIteration(cgioNum, baseChildId[nn], this->baseList[numBase]);
      }
      else if (strcmp(nodeLabel, "ReferenceState_t") == 0)
      {
        readBaseReferenceState(cgioNum, baseChildId[nn], this->baseList[numBase]);
      }
      else
      {
        cgio_release_id(cgioNum, baseChildId[nn]);
      }
    }
    this->baseList[numBase].nzones = static_cast<int>(nzones);

    if (this->baseList[numBase].times.size() < 1)
    {
      // If no time information were found
      // just put default values
      this->baseList[numBase].steps.clear();
      this->baseList[numBase].times.clear();
      this->baseList[numBase].steps.push_back(0);
      this->baseList[numBase].times.push_back(0.0);
    }

    if (nzones > 0)
    {
      // variable name and more, based on first zone only
      readZoneInfo(cgioNum, baseChildId[0], this->baseList[numBase]);
    }
  }

  // Same Timesteps in all root nodes
  // or separated time range by root nodes
  // timesteps need to be sorted for each root node
  this->GlobalTime.clear();
  for (std::size_t numBase = 0; numBase < baseList.size(); numBase++)
  {
    if (numBase == 0)
    {
      this->GlobalTime = this->baseList[numBase].times;
      continue;
    }
    const std::vector<double>& times = this->baseList[numBase].times;
    if (times.front() > this->GlobalTime.back())
    {
      this->GlobalTime.insert(this->GlobalTime.end(), times.begin(), times.end());
    }

    if (times.back() < this->GlobalTime.front())
    {
      this->GlobalTime.insert(this->GlobalTime.begin(), times.begin(), times.end());
    }
  }

  this->LastReadFilename = cgnsFileName;
  cgio_close_file(cgioNum);
  return true;
}

//------------------------------------------------------------------------------
vtkCGNSMetaData::vtkCGNSMetaData()
{
}
//------------------------------------------------------------------------------
vtkCGNSMetaData::~vtkCGNSMetaData()
{
}
//------------------------------------------------------------------------------
void vtkCGNSMetaData::PrintSelf(std::ostream& os)
{
  os << "--> vtkCGNSMetaData" << std::endl;
  os << "LastReadFileName: " << this->LastReadFilename << std::endl;
  os << "Base information:" << std::endl;
  for (std::size_t b = 0; b < this->baseList.size(); b++)
  {
    os << "  Base name: " << this->baseList[b].name << std::endl;
    os << "    number of zones: " << this->baseList[b].nzones << std::endl;
    os << "    number of time steps: " << this->baseList[b].times.size() << std::endl;
    os << "    use unsteady grid: " << this->baseList[b].useGridPointers << std::endl;
    os << "    use unsteady flow: " << this->baseList[b].useFlowPointers << std::endl;

    for (int i = 0; i < this->baseList[b].PointDataArraySelection.GetNumberOfArrays(); ++i)
    {
      os << "      Vertex :: ";
      os << this->baseList[b].PointDataArraySelection.GetArrayName(i) << std::endl;
    }
    for (int i = 0; i < this->baseList[b].CellDataArraySelection.GetNumberOfArrays(); ++i)
    {
      os << "      Cell :: ";
      os << this->baseList[b].CellDataArraySelection.GetArrayName(i) << std::endl;
    }

    os << "    Family Number: " << this->baseList[b].family.size() << std::endl;
    for (std::size_t fam = 0; fam < this->baseList[b].family.size(); fam++)
    {
      os << "      Family: " << this->baseList[b].family[fam].name
         << " is BC: " << this->baseList[b].family[fam].isBC << std::endl;
    }

    os << "    Reference State:" << std::endl;
    std::map<std::string, double>::iterator iter;
    for (iter = this->baseList[b].referenceState.begin();
         iter != this->baseList[b].referenceState.end(); iter++)
    {
      os << "  Variable: " << iter->first;
      os << "  Value: " << iter->second << std::endl;
    }
  }
}

//------------------------------------------------------------------------------
static void BroadcastCGNSString(vtkMultiProcessController* ctrl, CGNSRead::char_33& str)
{
  int len = 33;
  ctrl->Broadcast(&len, 1, 0);
  ctrl->Broadcast(&str[0], len, 0);
}

//------------------------------------------------------------------------------
static void BroadcastString(vtkMultiProcessController* controller, std::string& str, int rank)
{
  unsigned long len = static_cast<unsigned long>(str.size()) + 1;
  controller->Broadcast(&len, 1, 0);
  if (len)
  {
    if (rank)
    {
      std::vector<char> tmp;
      tmp.resize(len);
      controller->Broadcast(&(tmp[0]), len, 0);
      str = &tmp[0];
    }
    else
    {
      const char* start = str.c_str();
      std::vector<char> tmp(start, start + len);
      controller->Broadcast(&tmp[0], len, 0);
    }
  }
}
//------------------------------------------------------------------------------
static void BroadcastDoubleVector(
  vtkMultiProcessController* controller, std::vector<double>& dvec, int rank)
{
  unsigned long len = static_cast<unsigned long>(dvec.size());
  controller->Broadcast(&len, 1, 0);
  if (rank)
  {
    dvec.resize(len);
  }
  if (len)
  {
    controller->Broadcast(&dvec[0], len, 0);
  }
}
//------------------------------------------------------------------------------
static void BroadcastIntVector(
  vtkMultiProcessController* controller, std::vector<int>& ivec, int rank)
{
  unsigned long len = static_cast<unsigned long>(ivec.size());
  controller->Broadcast(&len, 1, 0);
  if (rank)
  {
    ivec.resize(len);
  }
  if (len)
  {
    controller->Broadcast(&ivec[0], len, 0);
  }
}
//------------------------------------------------------------------------------
static void BroadcastSelection(
  vtkMultiProcessController* controller, CGNSRead::vtkCGNSArraySelection& selection, int rank)
{
  unsigned long len = static_cast<unsigned long>(selection.size());
  controller->Broadcast(&len, 1, 0);
  if (rank == 0)
  {
    std::map<std::string, bool>::iterator ite;
    int tmp;
    for (ite = selection.begin(); ite != selection.end(); ++ite)
    {
      unsigned long len2 = static_cast<unsigned long>(ite->first.size()) + 1;
      controller->Broadcast(&len2, 1, 0);
      if (len2)
      {
        const char* start = ite->first.c_str();
        std::vector<char> tmpVector(start, start + len2);
        controller->Broadcast(&tmpVector[0], len2, 0);
      }
      tmp = (int)ite->second;
      controller->Broadcast(&tmp, 1, 0);
    }
  }
  else
  {
    unsigned long i;
    for (i = 0; i < len; ++i)
    {
      std::string key;
      int tmp;
      CGNSRead::BroadcastString(controller, key, rank);
      selection[key] = false;
      controller->Broadcast(&tmp, 1, 0);
      selection[key] = (tmp != 0);
    }
  }
}

//------------------------------------------------------------------------------
static void BroadcastRefState(
  vtkMultiProcessController* controller, std::map<std::string, double>& refInfo, int rank)
{
  unsigned long len = static_cast<unsigned long>(refInfo.size());
  controller->Broadcast(&len, 1, 0);
  if (rank == 0)
  {
    std::map<std::string, double>::iterator ite;
    for (ite = refInfo.begin(); ite != refInfo.end(); ++ite)
    {
      unsigned long len2 = static_cast<unsigned long>(ite->first.size()) + 1;
      controller->Broadcast(&len2, 1, 0);
      if (len2)
      {
        const char* start = ite->first.c_str();
        std::vector<char> tmp(start, start + len2);
        controller->Broadcast(&tmp[0], len2, 0);
      }
      controller->Broadcast(&ite->second, 1, 0);
    }
  }
  else
  {
    for (unsigned long i = 0; i < len; ++i)
    {
      std::string key;
      CGNSRead::BroadcastString(controller, key, rank);
      refInfo[key] = 0.0;
      controller->Broadcast(&refInfo[key], 1, 0);
    }
  }
}

//------------------------------------------------------------------------------
static void BroadcastFamilies(vtkMultiProcessController* controller,
  std::vector<CGNSRead::FamilyInformation>& famInfo, int rank)
{
  unsigned long len = static_cast<unsigned long>(famInfo.size());
  controller->Broadcast(&len, 1, 0);
  if (rank != 0)
  {
    famInfo.resize(len);
  }
  std::vector<CGNSRead::FamilyInformation>::iterator ite;
  for (ite = famInfo.begin(); ite != famInfo.end(); ++ite)
  {
    BroadcastCGNSString(controller, ite->name);
    int flags = 0;
    if (rank == 0)
    {
      if (ite->isBC == true)
      {
        flags = 1;
      }
      controller->Broadcast(&flags, 1, 0);
    }
    else
    {
      controller->Broadcast(&flags, 1, 0);
      if ((flags & 1) != 0)
      {
        ite->isBC = true;
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkCGNSMetaData::Broadcast(vtkMultiProcessController* controller, int rank)
{
  unsigned long len = static_cast<unsigned long>(this->baseList.size());
  controller->Broadcast(&len, 1, 0);
  if (rank != 0)
  {
    this->baseList.resize(len);
  }
  std::vector<CGNSRead::BaseInformation>::iterator ite;
  for (ite = this->baseList.begin(); ite != baseList.end(); ++ite)
  {
    CGNSRead::BroadcastCGNSString(controller, ite->name);
    controller->Broadcast(&ite->cellDim, 1, 0);
    controller->Broadcast(&ite->physicalDim, 1, 0);
    controller->Broadcast(&ite->baseNumber, 1, 0);
    controller->Broadcast(&ite->nzones, 1, 0);

    int flags = 0;
    if (rank == 0)
    {
      if (ite->useGridPointers == true)
      {
        flags = 1;
      }
      if (ite->useFlowPointers == true)
      {
        flags = (flags | 2);
      }
      controller->Broadcast(&flags, 1, 0);
    }
    else
    {
      controller->Broadcast(&flags, 1, 0);
      if ((flags & 1) != 0)
      {
        ite->useGridPointers = true;
      }
      if ((flags & 2) != 0)
      {
        ite->useFlowPointers = true;
      }
    }

    CGNSRead::BroadcastRefState(controller, ite->referenceState, rank);
    CGNSRead::BroadcastFamilies(controller, ite->family, rank);

    CGNSRead::BroadcastSelection(controller, ite->PointDataArraySelection, rank);
    CGNSRead::BroadcastSelection(controller, ite->CellDataArraySelection, rank);

    BroadcastIntVector(controller, ite->steps, rank);
    BroadcastDoubleVector(controller, ite->times, rank);
  }
  CGNSRead::BroadcastString(controller, this->LastReadFilename, rank);
  BroadcastDoubleVector(controller, this->GlobalTime, rank);
}
}
