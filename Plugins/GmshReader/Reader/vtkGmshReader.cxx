/*=========================================================================

  Program:   ParaView
  Module:    vtkGmshReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// -------------------------------------------------------------------
// ParaViewGmshReaderPlugin - Copyright (C) 2015 Cenaero
//
// See the Copyright.txt and License.txt files provided
// with ParaViewGmshReaderPlugin for license information.
//
// -------------------------------------------------------------------

#include "vtkGmshReader.h"
#include "vtkGmshIncludes.h"

#include "vtkByteSwap.h"
#include "vtkCellType.h" //added for constants such as VTK_TETRA etc...
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <vtksys/SystemTools.hxx>

#include <map>
#include <sstream>
#include <string>
#include <vector>

//-----------------------------------------------------------------------------
struct vtkGmshReaderInternal
{
  vtkGmshReaderInternal(int adaptLevel = 0, double adaptTol = -0.0001)
    : AdaptTolerance(adaptTol)
    , AdaptLevel(adaptLevel)
  {
  }

  void ClearConnectivity()
  {
    this->Connectivity.clear();
    std::vector<vectInt>().swap(Connectivity);
  }

  void ClearCellType()
  {
    this->CellType.clear();
    std::vector<int>().swap(CellType);
  }

  void ClearCoords()
  {
    this->Coords.clear();
    std::vector<PCoords>().swap(Coords);
  }

  void ClearValues()
  {
    this->Values.clear();
    std::vector<PValues>().swap(Values);
  }

  void ClearData()
  {
    this->ClearConnectivity();
    this->ClearCellType();
    this->ClearCoords();
    this->ClearValues();
  }

  void ClearMimargv(char** mimargv, size_t numarg)
  {
    for (size_t i = 0; i < numarg; i++)
    {
      delete[] mimargv[i];
    }
    delete[] mimargv;
  }

  double AdaptTolerance;
  int AdaptLevel;
  std::vector<std::string> FieldPathPattern;
  std::vector<vectInt> Connectivity; // connectivity (vector of vector)
  std::vector<int> CellType;         // topology
  std::vector<PCoords> Coords;       // coordinates
  std::vector<PValues> Values;       // nodal values (either scalar or vector)
};

//-----------------------------------------------------------------------------
namespace
{
std::string commonPrefix(const std::string& str1, const std::string& str2)
{
  return std::string(str1.begin(), std::mismatch(str1.begin(), str1.end(), str2.begin()).first);
}
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGmshReader);

//-----------------------------------------------------------------------------
vtkGmshReader::vtkGmshReader()
{
  this->GeometryFileName = nullptr;
  this->XMLFileName = nullptr;
  this->Internal = new vtkGmshReaderInternal;
  this->SetNumberOfInputPorts(0);
}

//-----------------------------------------------------------------------------
vtkGmshReader::~vtkGmshReader()
{
  this->SetGeometryFileName(nullptr);
  this->SetXMLFileName(nullptr);
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkGmshReader::ClearFieldInfo()
{
  this->Internal->FieldPathPattern.clear();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkGmshReader::SetFieldInfoPath(const std::string& addToPath)
{
  this->Internal->FieldPathPattern.push_back(addToPath);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkGmshReader::GetSizeFieldPathPattern()
{
  return static_cast<int>(this->Internal->FieldPathPattern.size());
}

//-----------------------------------------------------------------------------
void vtkGmshReader::SetAdaptInfo(int adaptLevel, double adaptTol)
{
  if (this->Internal->AdaptLevel != adaptLevel || this->Internal->AdaptTolerance != adaptTol)
  {
    this->Internal->AdaptLevel = adaptLevel;
    this->Internal->AdaptTolerance = adaptTol;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
int vtkGmshReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  // get the data object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Allocate 10000 cells by default. If this is not enough, allocated mem is doubled.
  output->Allocate(10000, 10000);

  if (!this->GeometryFileName)
  {
    vtkErrorMacro("GeometryFileName is missing.");
    return 0;
  }

  int firstVertexNo = 0;
  return this->ReadGeomAndFieldFile(firstVertexNo, output);
}

//-----------------------------------------------------------------------------
void vtkGmshReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "GeometryFileName: " << (this->GeometryFileName ? this->GeometryFileName : "(none)")
     << endl;
}

//-----------------------------------------------------------------------------
// firstVertexNo is useful when reading multiple geom files and coalescing
// them into one, ReadGeomfile can then be called repeatedly from Execute with
// firstVertexNo forming consecutive series of vertex numbers
int vtkGmshReader::ReadGeomAndFieldFile(int& firstVertexNo, vtkUnstructuredGrid* output)
{
  vtkDebugMacro("Entering vtkGmshReader::ReadGeomAndFieldFile(): partID=" << PartID << " TimeStep="
                                                                          << TimeStep);

  std::vector<std::string> mshfiles;
  mshfiles.push_back("program_name");

  // Geometry file added to mshfiles
  if (vtksys::SystemTools::FileExists(this->GeometryFileName, true))
  {
    mshfiles.push_back(std::string(this->GeometryFileName));
  }
  else
  {
    vtkErrorMacro("File " << this->GeometryFileName << " does not exit - Check path");
    return 0;
  }

  // Variables for vtk
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();

  std::string debugmsg;

  bool readMeshOnly = (this->Internal->FieldPathPattern.size() == 0);
  bool skipDataSet = false;

  if (!readMeshOnly)
  {
    std::vector<std::string>::iterator itfieldpath = this->Internal->FieldPathPattern.begin();
    std::vector<std::string>::iterator itfieldpathend = this->Internal->FieldPathPattern.end();

    for (; itfieldpath != itfieldpathend; ++itfieldpath)
    {
      // ParaView sorts the fields by alphabetical order
      std::string str_field_name(itfieldpath->c_str());
      std::string pIdentifier;

      pIdentifier = "[partID]";
      this->ReplaceAllStringPattern(str_field_name, pIdentifier, PartID);

      // Test the old format with 6 digits and zero padding (000001, etc)
      if (PartID < 1e7)
      {
        pIdentifier = "[zeroPadPartID]";
        std::ostringstream paddedFileID;
        paddedFileID << std::setw(6) << std::setfill('0') << PartID;
        this->ReplaceAllStringPattern(str_field_name, pIdentifier, paddedFileID.str());
      }

      pIdentifier = "[step]";
      this->ReplaceAllStringPattern(str_field_name, pIdentifier, TimeStep);

      // Returns the directory path specified in the xml file without the geom file name
      std::string fpath = vtksys::SystemTools::GetFilenamePath(str_field_name);

      // Check if str_field_name is an absolute path or a relative path
      if (fpath.empty() || !vtksys::SystemTools::FileIsFullPath(fpath.c_str()))
      {
        // Returns the path of the xml file
        std::string xmlpath = vtksys::SystemTools::GetFilenamePath(this->XMLFileName);
        if (!xmlpath.empty())
        {
          // Prepend relative path
          str_field_name = xmlpath + "/" + str_field_name;
        }
      }

      if (this->GetDebug())
      {
        debugmsg += "\nmshfiles field path: " + str_field_name;
      }

      // Field files added to mshfiles
      if (vtksys::SystemTools::FileExists(str_field_name, true))
      {
        mshfiles.push_back(str_field_name);
      }
      else
      {
        skipDataSet = true;
      }
    }
  }

  if (this->GetDebug())
  {
    vtkDebugMacro(<< debugmsg);
    debugmsg.clear();
  }

  size_t numarg = mshfiles.size();
  // Print the msh files
  if (this->GetDebug())
  {
    for (size_t i = 0; i < numarg; i++)
    {
      debugmsg += "\nGmsh arg " + ToString(i) + ": " + mshfiles[i];
    }
    vtkDebugMacro(<< debugmsg);
    debugmsg.clear();
  }

  // If readMeshOnly is false and field data file is not present (e.g., partial set of files with
  // non-continuous file ids in the case of boundary output for which only a few ranks produced a
  // non-empty data file), then skip the rest and return 1.
  // The field data files are read more quickly in this way.
  // The drawback of this approach is that if the user specifies a wrong path for the field data
  // in the xml file, there is no error message any more (silent mistake vs false error).
  if (skipDataSet)
    return 1;

  // Gmsh requires char** for the initialization, with first char * = program name
  char** mimargv = new char*[numarg];
  for (size_t i = 0; i < numarg; i++)
  {
    mimargv[i] = new char[mshfiles[i].size() + 1];
    std::copy(mshfiles[i].begin(), mshfiles[i].end(), mimargv[i]);
    mimargv[i][mshfiles[i].size()] = '\0';
  }

  // Clear the CTX->files object, which usually keeps the name of all msh files ever loaded under
  // the same paraview session
  CTX::instance()->files.clear();

  // Read the mesh information and store them in a GModel object
  new GModel();
  GmshInitialize(numarg, mimargv);             // Critical for mixed 2D-3D meshes with 2D data
  opt_mesh_ignore_periodicity(0, GMSH_SET, 1); // Critical for distrubuted  meshes with periodic
                                               // boundaries. Must be called after GmshInitialize()
  opt_mesh_partition_create_topology(0, GMSH_SET, 0.); // For safety
  GModel* testGModel = GModel::current();              // Retrieve the static GModel object
  // Gmsh automatically resets the name of the model from CTX::instance()->files[0] (see
  // Common/CommandLine.cpp:1157 in the Gmsh source directory), which is the first model ever read.
  // Therefore, update the name of the model based on mshfiles[1].
  // This is critical when several mshi files are loaded simultaneously (and CTX::instamce()->files
  // is not cleared).
  testGModel->setFileName(mshfiles[1]);
  // Open the mesh file - same as OpenProject(meshfilename);
  OpenProject(testGModel->getFileName());

  if (this->GetDebug())
  {
    debugmsg += "\nTotal number of 2D and 3D elements in the msh mesh file: " +
      ToString(testGModel->getNumMeshElements());
    debugmsg +=
      "\nTotal number of nodes in the msh mesh file: " + ToString(testGModel->getNumMeshVertices());
  }

  if (readMeshOnly)
  {
    // treat only mesh info
    std::set<int> node_list;
    std::set<int> elm_list;
    int isize;

    // Check if there 3D elements in the mesh
    if (testGModel->getNumRegions() > 0)
    {
      for (GModel::riter it = testGModel->firstRegion(); it != testGModel->lastRegion(); ++it)
      {
        isize = (*it)->tetrahedra.size();
        if (isize > 0)
        {
          if (this->GetDebug())
          {
            debugmsg += "\nThere are " + ToString(isize) + " tetrahedra in this region";
          }
          for (int i = 0; i < isize; i++)
          {
            int num_vertices = (*it)->tetrahedra[i]->getNumVertices();
            elm_list.insert((*it)->tetrahedra[i]->getNum());
            for (int j = 0; j < num_vertices; j++)
            {
              node_list.insert((*it)->tetrahedra[i]->getVertex(j)->getNum());
            }
          }
        }

        isize = (*it)->hexahedra.size();
        if (isize > 0)
        {
          if (this->GetDebug())
          {
            debugmsg += "\nThere are " + ToString(isize) + " hexahedra in this region";
          }

          for (int i = 0; i < isize; i++)
          {
            int num_vertices = (*it)->hexahedra[i]->getNumVertices();
            elm_list.insert((*it)->hexahedra[i]->getNum());
            for (int j = 0; j < num_vertices; j++)
            {
              node_list.insert((*it)->hexahedra[i]->getVertex(j)->getNum());
            }
          }
        }

        isize = (*it)->prisms.size();
        if (isize > 0)
        {
          if (this->GetDebug())
          {
            debugmsg += "\nThere are " + ToString(isize) + " prisms in this region";
          }

          for (int i = 0; i < isize; i++)
          {
            int num_vertices = (*it)->prisms[i]->getNumVertices();
            elm_list.insert((*it)->prisms[i]->getNum());
            for (int j = 0; j < num_vertices; j++)
            {
              node_list.insert((*it)->prisms[i]->getVertex(j)->getNum());
            }
          }
        }

        isize = (*it)->pyramids.size();
        if (isize > 0)
        {
          if (this->GetDebug())
          {
            debugmsg += "\nThere are " + ToString(isize) + " pyramids in this region";
          }

          for (int i = 0; i < isize; i++)
          {
            int num_vertices = (*it)->pyramids[i]->getNumVertices();
            elm_list.insert((*it)->pyramids[i]->getNum());
            for (int j = 0; j < num_vertices; j++)
            {
              node_list.insert((*it)->pyramids[i]->getVertex(j)->getNum());
            }
          }
        }

        isize = (*it)->polyhedra.size();
        if (isize > 0)
        {
          vtkErrorMacro("There are " << (*it)->polyhedra.size() << " polyhedra in this mesh\n");
        }
      }
    }

    // Check for 2D elements if there is no 3D element
    else if (testGModel->getNumFaces() > 0)
    {

      for (GModel::fiter it = testGModel->firstFace(); it != testGModel->lastFace(); ++it)
      {

        isize = (*it)->triangles.size();
        if (isize > 0)
        {
          if (this->GetDebug())
          {
            debugmsg += "\nThere are " + ToString(isize) + " triangles in this region";
          }

          for (int i = 0; i < isize; i++)
          {
            int num_vertices = (*it)->triangles[i]->getNumVertices();
            elm_list.insert((*it)->triangles[i]->getNum());
            for (int j = 0; j < num_vertices; j++)
            {
              node_list.insert((*it)->triangles[i]->getVertex(j)->getNum());
            }
          }
        }

        isize = (*it)->quadrangles.size();
        if (isize > 0)
        {
          if (this->GetDebug())
          {
            debugmsg += "\nThere are " + ToString(isize) + " quadrangles in this region";
          }

          for (int i = 0; i < isize; i++)
          {
            int num_vertices = (*it)->quadrangles[i]->getNumVertices();
            elm_list.insert((*it)->quadrangles[i]->getNum());
            for (int j = 0; j < num_vertices; j++)
            {
              node_list.insert((*it)->quadrangles[i]->getVertex(j)->getNum());
            }
          }
        }

        isize = (*it)->polygons.size();
        if (isize > 0)
        {
          vtkErrorMacro("There are " << isize << " polygons in this mesh\n");
        }
      }
    }

    if (this->GetDebug())
    {
      vtkDebugMacro(<< debugmsg);
      debugmsg.clear();
    }

    // insert nodes and their coordinates
    for (std::set<int>::iterator jt = node_list.begin(); jt != node_list.end(); ++jt)
    {
      MVertex* myvertex = testGModel->getMeshVertexByTag(*jt);
      points->InsertPoint(*jt, myvertex->x(), myvertex->y(), myvertex->z());
    }
    output->SetPoints(points.Get());
    node_list.clear();
    std::set<int>().swap(node_list);

    // insert elements with their connectivity and type
    MElement* myelement;
    for (std::set<int>::iterator jt = elm_list.begin(); jt != elm_list.end(); ++jt)
    {
      myelement = testGModel->getMeshElementByTag(*jt);
      int num_vertices = myelement->getNumVertices();
      std::vector<vtkIdType> nodes(num_vertices);
      for (int i = 0; i < num_vertices; i++)
      {
        nodes[i] = myelement->getVertexVTK(i)->getNum();
      }
      int cell_type = myelement->getTypeForVTK();
      output->InsertNextCell(cell_type, num_vertices, &nodes[0]);
    }

    elm_list.clear();
    std::set<int>().swap(elm_list);
    testGModel->destroyMeshCaches(); // Keep memory water mark as low as possible
  }
  else // Read field data and merge them with the mesh data
  {
    // merge field data
    for (size_t i = 2; i < numarg; i++)
    {
      // Initialize datafilename string
      std::string datafilename(mimargv[i]);
      // Merge Field msh files
      // Prototype:
      //   int MergeFile(const std::string &fileName, bool warnIfMissing = false,
      //                 bool setBoundingBox = true, bool importPhysicalsInOnelab = true,
      //                 int partitionToRead = --1);
      MergeFile(datafilename, false, false, false, PartID - 1);
    }

    if (PView::list.empty())
    {
      vtkErrorMacro("PView list is empty");
      this->Internal->ClearMimargv(mimargv, numarg);
      GmshFinalize();
      return 0;
    }

    // Adapt setting
    int level = this->Internal->AdaptLevel;
    double tol = this->Internal->AdaptTolerance;

    // Declare pointer  to retrieve corresponding static data
    PViewData* testPViewData = nullptr;
    PViewDataGModel* testPViewDataGModel = nullptr;
    globalVTKData* globVTKData = nullptr;
    int activeScalars = 0, activeVectors = 0, activeTensors = 0;

    // Determine from the PViewData objects the number of
    // - fields and
    // - interpolation schemes (=regions in the domain with a different polynomial order)
    typedef std::set<std::string> stringset;
    std::map<std::string, stringset> schemeAndPviewMapSet;

    int numOfComps;
    for (size_t iview = 0; iview < PView::list.size(); iview++)
    {
      testPViewData = PView::list[iview]->getData(); // get access to _data from view #i.
      numOfComps = testPViewData->getNumComponents(0, 0, 0);
      std::string PViewName = testPViewData->getName();
      std::string interpolationSchemeName = testPViewData->getInterpolationSchemeName();
      schemeAndPviewMapSet[interpolationSchemeName].insert(PViewName);
    }

    // Build schemeAndPviewMapSet
    // For example, for 4 views Pressure_p_4, Pressure_p_3, Velocity_p_4, Velocity_p_3, we get the
    // following data structure:
    //   Map size: 2
    //   Map: DGInterpolationScheme_level_0 - Number of PViews: 2
    //     PView: Pressure_p_4
    //     PView: Velocity_p_4
    //   Map: DGInterpolationScheme_level_1 - Number of PViews: 2
    //     PView: Pressure_p_3
    //     PView: Velocity_p_3
    // Note that the views are sorted by alphabetical order and their name must be unique (within
    // their interpolation scheme)
    int mapSize = schemeAndPviewMapSet.size();

    if (this->GetDebug())
    {
      debugmsg = "\nMap size: " + ToString(schemeAndPviewMapSet.size());
    }

    int* nPViewPerScheme = new int[mapSize];
    int mapId = 0;
    for (std::map<std::string, stringset>::iterator it = schemeAndPviewMapSet.begin();
         it != schemeAndPviewMapSet.end(); ++it)
    {
      nPViewPerScheme[mapId] = (*it).second.size();
      if (this->GetDebug())
      {
        debugmsg +=
          "\nMap: " + (*it).first + " - Number of PViews:" + ToString(nPViewPerScheme[mapId]);
      }
      mapId++;
      for (stringset::iterator jt = (*it).second.begin(); jt != (*it).second.end(); ++jt)
      {
        if (this->GetDebug())
        {
          debugmsg += "\n  PView: " + (*jt);
        }
      }
    }
    vtkDebugMacro(<< debugmsg);
    if (this->GetDebug())
    {
      debugmsg.clear();
    }

    // Check the size of the map. Each interpolation scheme should include
    // the same number of views
    if (mapSize > 1)
    {
      // More than one interpolation scheme
      for (int i = 1; i < mapSize; i++)
      {
        if (nPViewPerScheme[i] != nPViewPerScheme[i - 1])
        {
          vtkErrorMacro("Number of PViews per interpolation scheme is not consistent: "
            << nPViewPerScheme[i] << " " << nPViewPerScheme[i - 1]);
          this->Internal->ClearMimargv(mimargv, numarg);
          GmshFinalize();
          return 0;
        }
      }
    }

    // Build schemeAndPviewMapVect (std::vector) from schemeAndPviewMapSet (std::set)
    // for faster and easier data access
    // Note that views are now sorted by alphabetical order
    typedef std::vector<std::string> stringvect;
    std::map<std::string, stringvect> schemeAndPviewMapVect;
    for (std::map<std::string, stringset>::iterator it = schemeAndPviewMapSet.begin();
         it != schemeAndPviewMapSet.end(); ++it)
    {
      for (stringset::iterator jt = (*it).second.begin(); jt != (*it).second.end(); ++jt)
      {
        schemeAndPviewMapVect[(*it).first.c_str()].push_back((*jt).c_str());
      }
    }

    // Free memory from schemeAndPviewMapSet
    schemeAndPviewMapSet.clear();

    // Extract the number of fields from the first interplation scheme (should be the same for all
    // interplation scheme)
    int nFields = nPViewPerScheme[0];
    delete[] nPViewPerScheme;

    // Find common substrings from PViewData names and store them in a vector
    // This will be used as a mapping in ParaView so that the same fields (if several different
    // order) have the same name
    std::vector<std::string> fieldNameMapping;
    fieldNameMapping.resize(nFields);
    if (mapSize > 1)
    { // More than one interplation scheme
      std::map<std::string, stringvect>::iterator it1 = schemeAndPviewMapVect.begin();
      std::map<std::string, stringvect>::iterator it2 = schemeAndPviewMapVect.begin();
      ++it2; // Next element in the map
      for (; it2 != schemeAndPviewMapVect.end(); ++it2)
      {
        stringvect::iterator jt1 = (*it1).second.begin();
        int iview = 0;
        for (stringvect::iterator jt2 = (*it2).second.begin(); jt2 != (*it2).second.end(); ++jt2)
        {
          std::string commonName = ::commonPrefix(*jt1, *jt2);
          fieldNameMapping[iview] = commonName;
          ++jt1;
          iview++;
        }
      }
      //  Remove trailing characters such as _p_ (interpolation order)
      for (int i = 0; i < nFields; i++)
      {
        fieldNameMapping[i] = fieldNameMapping[i].substr(0, fieldNameMapping[i].find("_p_"));
      }
    }
    else
    {
      // Only one interpolation scheme, remove trailing characters such
      // as _p_ (interpolation order)
      int iview = 0;
      for (stringvect::iterator it = schemeAndPviewMapVect.begin()->second.begin();
           it != schemeAndPviewMapVect.begin()->second.end(); ++it)
      {
        fieldNameMapping[iview] = (*it).substr(0, (*it).find("_p_"));
        iview++;
      }
    }

    // Check value of adaptation tolerance
    if (tol >= 0 && nFields > 1)
    {
      vtkWarningMacro(
        "Changing adaptation tolerance to a negative value because more than one field are loaded");
      tol = -0.0001;
    }

    //   Map size: 2
    //   Map: DGInterpolationScheme_level_0 - Number of PViews: 2
    //     PView: Pressure_p_4
    //     PView: Velocity_p_4
    //   Map: DGInterpolationScheme_level_1 - Number of PViews: 2
    //     PView: Pressure_p_3
    //     PView: Velocity_p_3

    // Now loop over the fields first (can correspond to several views with different order)
    // following alphabetical order of the views objects, adapt and insert in VTK data structure
    for (int iField = 0; iField < nFields; iField++)
    {
      int localFirstVertexNo = 0;
      int num_nodes_field = 0;
      int num_cells_field = 0;
      std::string fieldNameParaView = fieldNameMapping[iField];

      // Loop over the interpolation orders (pressure_p_4, pressure_p_3, etc)
      for (std::map<std::string, stringvect>::iterator it = schemeAndPviewMapVect.begin();
           it != schemeAndPviewMapVect.end(); ++it)
      {
        vtkDebugMacro(
          "Considering scheme: " << (*it).first << " - PView: " << (*it).second[iField]);

        // Find the view in Gmsh data structure (order may differ so loop over all the views until
        // the target one is found)
        for (size_t iview = 0; iview < PView::list.size(); iview++)
        {
          testPViewData = PView::list[iview]->getData(); // get access to _data from view #iview.
          std::string PViewName = testPViewData->getName();
          std::string interpolationSchemeName = testPViewData->getInterpolationSchemeName();

          // If view found, then adapt and convert the data from Gmsh to VTK
          if (PViewName.compare((*it).second[iField].c_str()) == 0 &&
            interpolationSchemeName.compare((*it).first.c_str()) == 0)
          {
            numOfComps = testPViewData->getNumComponents(0, 0, 0);

            if (this->GetDebug())
            {
              debugmsg = "\nName of the view in Gmsh: " + PViewName + " - number of components: " +
                ToString(numOfComps) + " - interpolation schme: " + interpolationSchemeName +
                "\nName of the view in ParaView: " + fieldNameParaView;
            }

            int timestep = -1; // Initialization to -1 very important

            testPViewDataGModel = dynamic_cast<PViewDataGModel*>(
              testPViewData); // get access to step info (through _step) from view #i
            timestep = testPViewDataGModel->getFirstNonEmptyTimeStep(
              timestep + 1); // find next time step, starting from 0 thanks to initialization to -1

            if (this->GetDebug())
            {
              debugmsg += "\nTime step of current view: " + ToString(timestep) +
                " - Time step size in PViewDataGModel: " +
                ToString(testPViewDataGModel->getNumTimeSteps());
            }
            vtkDebugMacro(<< debugmsg);
            if (this->GetDebug())
            {
              debugmsg.clear();
            }

            // Adapt and generate the GmshS object globVTKData
            if (testPViewData->getAdaptiveData())
            {
              // Clean any existing adaptive data that could have been pre-allcoated
              testPViewData->destroyAdaptiveData();
            }

            // Allocate the _adaptiveData object in Gmsh
            testPViewData->initAdaptiveDataLight(timestep, level, tol);
            testPViewData->getAdaptiveData()->upBuildStaticData(true);
            testPViewData->getAdaptiveData()->upWriteVTK(false);
            testPViewData->getAdaptiveData()->changeResolutionForVTK(timestep, level, tol);
            testPViewData->destroyAdaptiveData();

            // Accumulate the data from globVTKData into local vtkGmshReader data
            // if this view is associated with the first interpolation scheme

            // Coordinates
            int num_nodes_view = globalVTKData::vtkGlobalCoords.size();
            num_nodes_field += num_nodes_view;

            if (this->GetDebug())
            {
              vtkDebugMacro("\nNumber of nodes in the adapted view: "
                << ToString(num_nodes_view)
                << " - localFirstVertexNo: " << ToString(localFirstVertexNo)
                << " - Number of nodes in the field: " << ToString(num_nodes_field));
            }

            if (iField == 0)
            { // Get the mesh info only from the first fields (may involved  several views)
              for (std::vector<PCoords>::iterator itField = globalVTKData::vtkGlobalCoords.begin();
                   itField != globalVTKData::vtkGlobalCoords.end(); ++itField)
              {
                this->Internal->Coords.push_back(*itField);
              }
            }
            globVTKData->clearGlobalCoords();

            // Connectivity
            int num_cells_view = globalVTKData::vtkGlobalConnectivity.size();
            num_cells_field += num_cells_view;
            if (iField == 0)
            { // Get the mesh info only from the first fields (may involved  several views)
              for (std::vector<vectInt>::iterator itField =
                     globalVTKData::vtkGlobalConnectivity.begin();
                   itField != globalVTKData::vtkGlobalConnectivity.end(); ++itField)
              {
                vectInt VectInt;
                for (std::vector<int>::iterator jt = itField->begin(); jt != itField->end(); ++jt)
                {
                  VectInt.push_back(*jt + localFirstVertexNo);
                }
                this->Internal->Connectivity.push_back(VectInt);
              }
            }
            localFirstVertexNo += num_nodes_view;
            globVTKData->clearGlobalConnectivity();

            // cell type
            if (iField == 0)
            { // Get the mesh info only from the first fields (may involved  several views)
              for (std::vector<int>::iterator itField = globalVTKData::vtkGlobalCellType.begin();
                   itField != globalVTKData::vtkGlobalCellType.end(); ++itField)
              {
                this->Internal->CellType.push_back(*itField);
              }
            }
            globVTKData->clearGlobalCellType();

            // Nodal values - Need to store them for all fields
            for (std::vector<PValues>::iterator itField = globalVTKData::vtkGlobalValues.begin();
                 itField != globalVTKData::vtkGlobalValues.end(); ++itField)
            {
              this->Internal->Values.push_back(*itField);
            }
            globVTKData->clearGlobalValues();

            // Free the memory of the View in Gmsh
            delete PView::list[iview];
            break;

          } // if view found
        }   // Loop over the views
      }     // Loop over interplation schemes

      // From here, the field (pressure, velocity, etc) should be complete
      // Add it now to the ParaView data structure
      if (iField == 0)
      {
        // Insert coordinates
        int id = 0;
        for (std::vector<PCoords>::iterator itCoords = this->Internal->Coords.begin();
             itCoords != this->Internal->Coords.end(); ++itCoords)
        {
          points->InsertPoint(
            id + firstVertexNo, (*itCoords).c[0], (*itCoords).c[1], (*itCoords).c[2]);
          id++;
        }
        output->SetPoints(points.Get());
        this->Internal->ClearCoords();

        // Insert connectivity and cell type
        id = 0;
        std::vector<int>::iterator celltypeit = this->Internal->CellType.begin();
        for (std::vector<vectInt>::iterator itCell = this->Internal->Connectivity.begin();
             itCell != this->Internal->Connectivity.end(); ++itCell)
        {
          int num_vertices = itCell->size();
          std::vector<vtkIdType> nodes(num_vertices);

          int jd = 0;
          for (std::vector<int>::iterator jt = itCell->begin(); jt != itCell->end(); ++jt)
          {
            nodes[jd] = *jt + firstVertexNo;
            jd++;
          }

          int cell_type = *celltypeit;
          output->InsertNextCell(cell_type, num_vertices, &nodes[0]);

          id++;
          celltypeit++;
        }

        this->Internal->ClearConnectivity();
        this->Internal->ClearCellType();
      }

      const char* paraviewFieldTag = fieldNameParaView.c_str();

      vtkDataSetAttributes* field;
      field = output->GetPointData();

      vtkDataArray* dataArray =
        vtkDoubleArray::New(); // Only double are currently supported in Gmsh

      int noOfDatas = this->Internal->Values.size();

      dataArray->SetName(paraviewFieldTag);
      dataArray->SetNumberOfComponents(numOfComps);
      dataArray->SetNumberOfTuples(noOfDatas);

      vtkDebugMacro("\nInserting data in ParaView:"
        << "\n Name of the field: " << paraviewFieldTag << "\n Number of nodes in the field: "
        << num_nodes_field << "\n Number of elements in the field: " << num_cells_field
        << "\n Number of components in the field: " << numOfComps
        << "\n Number of data: " << noOfDatas);

      switch (numOfComps)
      {
        case 1:
        {
          if (!activeScalars)
          {
            field->SetActiveScalars(paraviewFieldTag);
          }
          else
          {
            activeScalars = 1;
          }
          int id = 0;
          for (std::vector<PValues>::iterator it = this->Internal->Values.begin();
               it != this->Internal->Values.end(); ++it)
          {
            dataArray->SetTuple1(id, (*it).v[0]);
            id++;
          }
          break;
        }
        case 3:
        {
          if (!activeVectors)
          {
            field->SetActiveVectors(paraviewFieldTag);
          }
          else
          {
            activeVectors = 1;
          }
          int id = 0;
          for (std::vector<PValues>::iterator it = this->Internal->Values.begin();
               it != this->Internal->Values.end(); ++it)
          {
            dataArray->SetTuple3(id, (*it).v[0], (*it).v[1], (*it).v[2]);
            id++;
          }
          break;
        }
        case 9:
        {
          if (!activeTensors)
          {
            field->SetActiveTensors(paraviewFieldTag);
          }
          else
          {
            activeTensors = 1;
          }

          int id = 0;
          for (std::vector<PValues>::iterator it = this->Internal->Values.begin();
               it != this->Internal->Values.end(); ++it)
          {
            dataArray->SetTuple9(id, (*it).v[0], (*it).v[1], (*it).v[2], (*it).v[3], (*it).v[4],
              (*it).v[5], (*it).v[6], (*it).v[7], (*it).v[8]);
            id++;
          }
          break;
        }
        default:
        {
          vtkErrorMacro("number of components [" << numOfComps << "] NOT supported in ParaView");
          dataArray->Delete();
          dataArray = nullptr;
          break;
        }
      }

      if (dataArray)
      {
        field->AddArray(dataArray);
        dataArray->Delete();
        dataArray = nullptr;
      }

      this->Internal->ClearData();
    } // Loop over the fields
  }

  // Free memory of mimargv
  this->Internal->ClearMimargv(mimargv, numarg);

  // Clean Gmsh data
  GmshFinalize();

  return 1;
} // end of ReadGeomAndFieldFile

//----------------------------------------------------------------------------
template <class T>
void vtkGmshReader::ReplaceAllStringPattern(
  std::string& input, const std::string& pIdentifier, const T& target)
{
  vtkDebugMacro("Entering vtkGmshMetaReader::ReplaceStringPatternByInt()");

  size_t foundPID = input.find(pIdentifier);
  while (foundPID != std::string::npos)
  {
    std::string base = input.substr(0, foundPID);
    std::string ext = input.substr(foundPID + pIdentifier.size(), input.size());
    std::ostringstream fStr;
    fStr << base << target << ext;
    input = fStr.str();
    foundPID = input.find(pIdentifier);
  }
}

// Explicit instantiations
template void vtkGmshReader::ReplaceAllStringPattern(
  std::string& input, const std::string& pIdentifier, const int& target);

template void vtkGmshReader::ReplaceAllStringPattern(
  std::string& input, const std::string& pIdentifier, const std::string& target);
