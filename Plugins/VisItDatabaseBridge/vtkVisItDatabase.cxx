/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVisItDatabase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVisItDatabase.h"
#include "vtkVisItDatabaseBridgeTypes.h"
#include "PrintUtils.h"
// VTK
#include "vtkObjectFactory.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkUniformGrid.h"
#include "vtkAMRBox.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkStringArray.h"
#include "vtkIntArray.h"
// VisIt
#include "PluginManager.h"
#include "DatabasePluginManager.h"
#include "DatabasePluginInfo.h"
#include "VisItException.h"
#include "TimingsManager.h"
#include "avtDatabase.h"
#include "avtDatabaseFactory.h"
#include "avtDatabaseMetaData.h"
#include "avtMeshMetaData.h"
#include "avtOriginatingSource.h"
#include "avtSourceFromDatabase.h"
#include "avtGenericDatabase.h"
#include "avtExpressionEvaluatorFilter.h"
#include "avtSIL.h"
#include "avtSILSet.h"
#include "avtSILCollection.h"
#include "avtIntervalTree.h"
#include "avtMetaData.h"
#include "avtTypes.h"
// System
#ifdef UNIX
#include "dlfcn.h"
#endif

#include <vtkstd/algorithm>
using vtkstd::count;
using vtkstd::string;
using vtkstd::vector;

//*****************************************************************************
void EnumerateSILSetLinks(
        const avtSIL *sil,
        int ssIdx,
        vector<int> &links)
{
  const avtSILSet_p set=sil->GetSILSet(ssIdx);
  // Recursivley grow a list of sil set ids linked to by this
  // set. If there are no links out, the set's own id is returned.
  const vector<int> &collections=set->GetRealMapsOut();
  const size_t nCollections=collections.size();
  if (nCollections)
    {
    for (size_t i=0; i<nCollections; ++i)
      {
      const int collectionId=collections[i];
      const avtSILCollection_p collection=sil->GetSILCollection(collectionId);
      const vector<int> &subsets=collection->GetSubsetList();
      const size_t nSubsets=subsets.size();
      for (size_t ii=0; ii<nSubsets; ++ii)
        {
        const avtSILSet_p subset=sil->GetSILSet(subsets[ii]);
        EnumerateSILSetLinks(sil,subsets[ii],links);
        }
      }
    }
  else
    {
    links.push_back(ssIdx);
    }
}

//*****************************************************************************
vtkIdType ReverseLookup(vtkIntArray *array, int value)
{
  vtkIdType nTups=array->GetNumberOfTuples();
  int *data=array->GetPointer(0);
  for (vtkIdType id=0; id<nTups; ++id)
    {
    if (data[0]==value)
      {
      return id;
      }
    ++data;
    }

  return -1;
}

//*****************************************************************************
// Convert from a rectilinear to a uniform grid. VisIt use rectilinear for
// AMR.
int RectilinearToUniformGrid(
        vtkRectilinearGrid *rg,
        vtkUniformGrid *&ug,
        vtkAMRBox &box)
{
  // Construct a uniform grid matching the passed rectilinear grid.
  vtkDataArray *X=rg->GetXCoordinates();
  vtkDataArray *Y=rg->GetYCoordinates();
  vtkDataArray *Z=rg->GetZCoordinates();

  double O[3];
  O[0]=*X->GetTuple(0);
  O[1]=*X->GetTuple(1);
  O[2]=*X->GetTuple(2);

  double dX[3];
  dX[0]=*X->GetTuple(1)-*X->GetTuple(0);
  dX[1]=*Y->GetTuple(1)-*Y->GetTuple(0);
  dX[2]=*Z->GetTuple(1)-*Z->GetTuple(0);

  int exts[6]={0};
  exts[1]=X->GetNumberOfTuples()-1;
  exts[3]=Y->GetNumberOfTuples()-1;
  exts[5]=Z->GetNumberOfTuples()-1;

  box.SetGridSpacing(dX);
  box.SetDimensions(exts);
  box.SetDataSetOrigin(O);
 // TODO we don't know the data set origin here, nor do we know
 // which cells this box covers, so this isn't right. Under VisIt's
 // AMR scheme we don't have this info. If we had the dataset origin 
 // we can compute the cells we own using our origin, that might be 
 // a possibility.Or we may use visits ghost arrays for our visablility
 // arrays, but the latter will leave the amr boxes incorrect. ??

  if (ug==NULL)
    {
    ug=vtkUniformGrid::New();
    }
  ug->Initialize(&box);

  // Copy the data arrays.
  ug->GetPointData()->ShallowCopy(rg->GetPointData());
  ug->GetCellData()->ShallowCopy(rg->GetCellData());

  return 1;
}



vtkCxxRevisionMacro(vtkVisItDatabase, "1.5");
vtkStandardNewMacro(vtkVisItDatabase);

//-----------------------------------------------------------------------------
vtkVisItDatabase::vtkVisItDatabase()
{
  // Needed to make visit run.
  SystemTimingsManager *tm=new SystemTimingsManager;
  visitTimer = tm->Initialize(".visit.log");
  delete tm;
  // Configuration.
  this->LibType=0;
  this->PluginPath=0;
  this->PluginId=0;
  // Manager.
  this->Manager=new DatabasePluginManager;
  this->PluginLoaded=0;
  // File attributes.
  this->FileName=0;
  this->ActiveTimeStep=0;
  this->Database=0;
  this->DatabaseMetaData=0;
}

//-----------------------------------------------------------------------------
vtkVisItDatabase::~vtkVisItDatabase()
{
  this->Initialize();
  delete this->Manager;
}

//-----------------------------------------------------------------------------
void vtkVisItDatabase::Initialize()
{
  // Clear currently loaded plugin and its associated state.
  this->UnloadPlugin();
  // Clear opened file and attributes.
  this->SetFileName(0);
  this->InitializeDataAttributes();
  // Clear configuration.
  this->InitializeConfiguration();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabase::InitializeConfiguration()
{
  this->LibType=SERIAL_LIBS;
  this->SetPluginPath(0);
  this->SetPluginId(0);
}

//-----------------------------------------------------------------------------
void vtkVisItDatabase::InitializeDataAttributes()
{
  this->ActiveTimeStep=0;
}

//-----------------------------------------------------------------------------
void vtkVisItDatabase::InitializeDatabaseAttributes()
{
  this->DatabaseMetaData=0;
}

//-----------------------------------------------------------------------------
void vtkVisItDatabase::UnloadPlugin()
{
  // First close the open database (if any). It's not valid
  // without an open plugin.
  this->CloseDatabase();
  // Unload the loaded (if any) plugin.
  if (this->PluginLoaded)
    {
    this->Manager->UnloadPlugins();
    this->PluginLoaded=0;
    }
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::LoadVisitLibraries()
{
  // Need to select the right libs, _ser or _par in the list below.
  // For example:
  // const char *dblib
  //   = (this->LibType==SERIAL_LIBS?"libdatabase_ser.so":"libdatabase_par.so");

  // TODO: FIXME
  // Problem: Typically when visit database plugins are
  // loaded by visit all internal visit libraries have been loaded,
  // however paraview knows nothing of visit libraries and they aren't
  // linked in. To cheat the system we can load them ourselves into
  // the dynamic loaders global search space.
  const char *visitlibs[]={
    "liblightweight_visit_vtk.so",
    "libvisit_vtk.so",
    "libavtexceptions.so",
    "libplotter_ser.so",
    "libavtview.so",
    "libavtddf_ser.so",
    "libavtfilters_ser.so",
    "libavtivp_ser.so",
    "libavtmath_ser.so",
    "libavtshapelets_ser.so",
    "libavtwriter_ser.so",
    "libbow.so",
    "libcomm.so",
    "libdbatts.so",
    "libdatabase_ser.so",
    "libexpressions_ser.so",
    "libexpr.so",
    "libmir_ser.so",
    "libmisc.so",
    "libparser.so",
    "libpipeline_ser.so",
    "libplugin.so",
    "libproxybase.so",
    "libquery_ser.so",
    "libstate.so",
    "libutility.so"};
  const int nVisitLibs=26;
  int ok=1;
  for (int i=0; i<nVisitLibs; ++i)
    {
    #ifdef UNIX
    ok&=dlopen(visitlibs[i],RTLD_GLOBAL|RTLD_LAZY)?1:0;
    #endif
    }

  if (!ok)
    {
    vtkWarningMacro("Failed to laod visit's libraries.");
    return 0;
    }

  return 1;
}


//-----------------------------------------------------------------------------
int vtkVisItDatabase::LoadPlugin()
{
  if (!this->PluginPath || !this->PluginId)
    {
    vtkWarningMacro("Failed to load plugin. "
        << "Plugin path " << (this->PluginPath?"has":"has not") << " been set. "
        << "Plugin id=" << (this->PluginId?"has":"has not") << " been set.");
    return 0;
    }

  this->UnloadPlugin();

  try
    {
    this->LoadVisitLibraries();
    this->Manager->Initialize(
                PluginManager::Engine,
                this->LibType,
                this->PluginPath);
    int pidx=this->Manager->GetAllIndex(this->PluginId);
    if (pidx<0)
      {
      VisItException e("Plugin un-available.");
      throw e;
      }
    this->Manager->LoadSinglePluginNow(this->PluginId);
    this->PluginLoaded=1;
    }
  catch(VisItException &e)
    {
    vtkWarningMacro(
        "Failed to initialize plugin manager with "
        << "plugin path : " << this->PluginPath << " and "
        << "plugin Id : " << this->PluginId << ". "
        << e.Message().c_str());
    return 0;
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkVisItDatabase::CloseDatabase()
{
  delete this->Database;
  this->Database=0;
  this->InitializeDatabaseAttributes();
  this->InitializeDataAttributes();
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::OpenDatabase()
{
  if (!this->PluginLoaded)
    {
    vtkWarningMacro(
        "Failed to open database. Plugin has not been loaded.");
    return 0;
    }

  this->CloseDatabase();

  try
    {
    vector<string> vs;
    avtDatabase *db=avtDatabaseFactory::FileList(
            this->Manager,
            &this->FileName,1,
            this->ActiveTimeStep,vs,
            this->PluginId,
            false,false);
    this->Database=dynamic_cast<avtGenericDatabase *>(db);
    }
  catch(VisItException &e)
    {
    vtkWarningMacro(
        "Failed to open the database "
        << "with file name " << this->FileName << ":" << this->ActiveTimeStep << " "
        << "using plugin id " << this->PluginId << ". "
        << e.Message().c_str());
    return 0;
    }

  return this->Database!=NULL;
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::UpdateMetaData()
{
  if (!this->Database)
    {
    vtkWarningMacro("Failed to update meta-data. "
      << "Database " << (this->Database?"is":"is not") << " open.");
    return 0;
    }

  try
    {
    this->DatabaseMetaData
      = this->Database->GetMetaData(this->ActiveTimeStep);
    }
  catch(VisItException &e)
    {
    vtkWarningMacro(
      "Failed to update meta-data. " << e.Message().c_str());
    return 0;
    }

  return 1;
}

//-----------------------------------------------------------------------------
bool vtkVisItDatabase::HasMultipleMeshes()
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  return this->DatabaseMetaData->GetNumMeshes()>1;
}

//-----------------------------------------------------------------------------
bool vtkVisItDatabase::ProducesMultiBlockData(int meshId)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const avtMeshMetaData *meshMetaData
    = this->DatabaseMetaData->GetMesh(meshId);

  return meshMetaData->numBlocks>1;
}

//-----------------------------------------------------------------------------
bool vtkVisItDatabase::ProducesAMRData(int meshId)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const avtMeshMetaData *meshMetaData
    = this->DatabaseMetaData->GetMesh(meshId);

  return meshMetaData->meshType==AVT_AMR_MESH;
}

//-----------------------------------------------------------------------------
bool vtkVisItDatabase::ProducesUnstructuredData(int meshId)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const avtMeshMetaData *meshMetaData
    = this->DatabaseMetaData->GetMesh(meshId);

  return meshMetaData->meshType==AVT_UNSTRUCTURED_MESH
      || meshMetaData->meshType==AVT_POINT_MESH
      || meshMetaData->meshType==AVT_SURFACE_MESH
      || meshMetaData->meshType==AVT_CSG_MESH;
}

//-----------------------------------------------------------------------------
bool vtkVisItDatabase::ProducesStructuredData(int meshId)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const avtMeshMetaData *meshMetaData
    = this->DatabaseMetaData->GetMesh(meshId);

  return meshMetaData->meshType==AVT_CURVILINEAR_MESH
      || meshMetaData->meshType==AVT_RECTILINEAR_MESH;
}



//-----------------------------------------------------------------------------
const char *vtkVisItDatabase::GetMeshType(int meshId)
{
  // NOTE this is used for U.I. See GetDataObjectType
  // if you are really interested in getting the type.
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const avtMeshMetaData *meshMetaData
    = this->DatabaseMetaData->GetMesh(meshId);

  switch (meshMetaData->meshType)
    {
    case AVT_RECTILINEAR_MESH:
      return "rectilinear";
      break;

    case AVT_CURVILINEAR_MESH:
      return "curvilinear";
      break;

    case AVT_UNSTRUCTURED_MESH:
      return "unstructured";
      break;

    case AVT_POINT_MESH:
      return "point";
      break;

    case AVT_SURFACE_MESH:
      return "surface";
      break;

    case AVT_CSG_MESH:
      return "CSG";
      break;

    case AVT_AMR_MESH:
      return "AMR";
      break;
    }

  vtkWarningMacro("Unexpected mesh encountered.");
  return 0;
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::ConfigureDataObject(
        vector<int> &meshIds,
        vtkDataObject *dataobject)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const int nMeshes=this->DatabaseMetaData->GetNumMeshes();
  if (nMeshes==1)
    {
    // We have a single mesh in this file, and we don't want 
    // a composite data set unless it necessary so we treat this as
    // a special case.
    this->ConfigureDataObject(0,dataobject);
    return 1;
    }

  // Multiple meshes in this file so we need a composite data set
  // to hold them. On top of that we have to configure each mesh.
  vtkMultiBlockDataSet *mbds=dynamic_cast<vtkMultiBlockDataSet *>(dataobject);
  if (!mbds)
    {
    vtkWarningMacro("Data type error. Failed to configure multi-mesh.");
    return 0;
    }
  mbds->Initialize();
  mbds->SetNumberOfBlocks(nMeshes);
  const size_t nMeshIds=meshIds.size();
  for (int i=0; i<nMeshIds; ++i)
    {
    const avtMeshMetaData *meshMetaData=this->DatabaseMetaData->GetMesh(meshIds[i]);
    vtkDataObject *meshObject=this->GetDataObject(meshIds[i]);
    this->ConfigureDataObject(meshIds[i],meshObject);
    mbds->SetBlock(meshIds[i],meshObject);
    meshObject->Delete();
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::ConfigureDataObject(
        int meshId,
        vtkDataObject *dataset)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const avtMeshMetaData *meshMetaData
    = this->DatabaseMetaData->GetMesh(meshId);

  // We have to handle two special cases: AMR and MultiBlock data.
  // for both the datasets will be configured but have null
  // leaves.
  // First special case is AMR...
  if (meshMetaData->meshType==AVT_AMR_MESH)
    {
    vtkHierarchicalBoxDataSet *hbds=dynamic_cast<vtkHierarchicalBoxDataSet *>(dataset);
    if (!hbds)
      {
      vtkWarningMacro("Data type error. Failed to configure AMR mesh.");
      return 0;
      }
    hbds->Initialize();
    int nLevels=meshMetaData->numGroups;
    hbds->SetNumberOfLevels(nLevels);
    for (int levelId=0; levelId<nLevels; ++levelId)
      {
      int nBlocks
        = count(meshMetaData->groupIds.begin(),meshMetaData->groupIds.end(),levelId);
      hbds->SetNumberOfDataSets(levelId,nBlocks);
      }
    return 1;
    }
  // Second special case is MultiBlock.
  const int nBlocks=meshMetaData->numBlocks;
  if (nBlocks>1)
    {
    vtkMultiBlockDataSet *mbds=dynamic_cast<vtkMultiBlockDataSet *>(dataset);
    mbds->Initialize();
    mbds->SetNumberOfBlocks(nBlocks);
    return 1;
    }

  // All other data are single VTK data set, and need no configuration.
  return 1;
}

//-----------------------------------------------------------------------------
const char *vtkVisItDatabase::GetDataObjectType()
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const int nMeshes=this->DatabaseMetaData->GetNumMeshes();
  if (nMeshes==1)
    {
    // We have a single mesh in this file, and we don't want 
    // a composite data set unless it necessary so we treat this as
    // a special case.
    return this->GetDataObjectType(0);
    }

  // Multiple meshes in this file so we need a composite data set
  // to hold them.
  return "vtkMultiBlockDataSet";
}

//-----------------------------------------------------------------------------
const char *vtkVisItDatabase::GetDataObjectType(int meshId)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const avtMeshMetaData *meshMetaData
    = this->DatabaseMetaData->GetMesh(meshId);

  // We have to handle two special cases: AMR and MultiBlock data.
  // for both the datasets will be configured but have null
  // leaves.
  // First special case is AMR...
  if (meshMetaData->meshType==AVT_AMR_MESH)
    {
    return "vtkHierarchicalBoxDataSet";
    }
  // Second special case is MultiBlock.
  const int nBlocks=meshMetaData->numBlocks;
  if (nBlocks>1)
    {
    return "vtkMultiBlockDataSet";
    }

  // The rest are standard VTK types.
  switch (meshMetaData->meshType)
    {
    case AVT_RECTILINEAR_MESH:
      return "vtkRectilinearGrid";
      break;

    case AVT_CURVILINEAR_MESH:
      return "vtkStructuredGrid";
      break;

    case AVT_UNSTRUCTURED_MESH:
      return "vtkUnstructuredGrid";
      break;

    case AVT_POINT_MESH:
      return "vtkPolyData";
      break;

    case AVT_SURFACE_MESH:
      return "vtkPolyData";
      break;

    case AVT_CSG_MESH:
      return "vtkUnstructuredGrid";
      break;
    }

  vtkWarningMacro("Unexpected mesh encountered.");
  return 0;
}

//-----------------------------------------------------------------------------
vtkDataObject *vtkVisItDatabase::GetDataObject()
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const int nMeshes=this->DatabaseMetaData->GetNumMeshes();
  if (nMeshes==1)
    {
    // We have a single mesh in this file, and we don't want 
    // a composite data set unless it necessary so we treat this as
    // a special case.
    return this->GetDataObject(0);
    }

  // Multiple meshes in this file so we need a composite data set
  // to hold them.
  return vtkMultiBlockDataSet::New();
}

//-----------------------------------------------------------------------------
vtkDataObject *vtkVisItDatabase::GetDataObject(int meshId)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const avtMeshMetaData *meshMetaData
    = this->DatabaseMetaData->GetMesh(meshId);

  // We have to handle two special cases: AMR and MultiBlock data.
  // for both the datasets will be configured but have null
  // leaves.
  // First special case is AMR...
  if (meshMetaData->meshType==AVT_AMR_MESH)
    {
    vtkHierarchicalBoxDataSet *hbds=vtkHierarchicalBoxDataSet::New();
    return hbds;
    }
  // Second special case is MultiBlock.
  const int nBlocks=meshMetaData->numBlocks;
  if (nBlocks>1)
    {
    vtkMultiBlockDataSet *mbds=vtkMultiBlockDataSet::New();
    return mbds;
    }

  // The rest are standard VTK types.
  switch (meshMetaData->meshType)
    {
    case AVT_RECTILINEAR_MESH:
      return vtkRectilinearGrid::New();
      break;

    case AVT_CURVILINEAR_MESH:
      return vtkStructuredGrid::New();
      break;

    case AVT_UNSTRUCTURED_MESH:
      return vtkUnstructuredGrid::New();
      break;

    case AVT_POINT_MESH:
      return vtkPolyData::New();
      break;

    case AVT_SURFACE_MESH:
      return vtkPolyData::New();
      break;

    case AVT_CSG_MESH:
      // TODO
      return vtkUnstructuredGrid::New();
      break;
    }

  vtkWarningMacro("Unexpected mesh encountered.");
  return 0;
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::GenerateDatabaseView(vtkMutableDirectedGraph *dbView)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  static int viewMTime=0; // This will be stored in the view and can be
  ++viewMTime;            // used to detect when the view has changed.

  // Attributes are stored vert by vert.
  dbView->Initialize();
  vtkStringArray *uiLabels=vtkStringArray::New();
  uiLabels->SetName("uiLabels");
  dbView->GetVertexData()->AddArray(uiLabels);
  uiLabels->Delete();
  vtkIntArray *ssIds=vtkIntArray::New();
  ssIds->SetName("ssIds");
  dbView->GetVertexData()->AddArray(ssIds);
  ssIds->Delete();
  vtkIntArray *ssRole=vtkIntArray::New();
  ssRole->SetName("ssRole");
  dbView->GetVertexData()->AddArray(ssRole);
  ssRole->Delete();
  vtkIntArray *mt=vtkIntArray::New();
  mt->SetName("viewMTime");
  mt->InsertNextValue(viewMTime);
  dbView->GetVertexData()->AddArray(mt);
  mt->Delete();
  // Edges are given a type. This allows the U.I. to link verts
  // together for inteligent selection and subsetting.
  vtkIntArray *edgeType=vtkIntArray::New();
  edgeType->SetName("EdgeType");
  dbView->GetEdgeData()->AddArray(edgeType);
  edgeType->Delete();

  // Add a dummy node which has an edge to all of the SIL
  // supersets(meshes).
  const int rootId=dbView->AddVertex();
  uiLabels->InsertNextValue("SIL");   // these values are place holders
  ssIds->InsertNextValue(-1);         // with no specific meaning and will
  ssRole->InsertNextValue(-1);        // be ingnored on the client side.

  const int nMeshes=this->DatabaseMetaData->GetNumMeshes();
  for (int meshId=0; meshId<nMeshes; ++meshId)
    {
    this->GenerateMeshView(meshId,rootId,dbView);
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::GenerateMeshView(
        int meshId,
        int rootId,
        vtkMutableDirectedGraph *dbView)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  avtSIL *sil = this->Database->GetSIL(this->ActiveTimeStep);
  if (sil==NULL)
    {
    vtkWarningMacro("Failed to get SIL.");
    return 0;
    }

  // Data for the view.
  // Vertex
  vtkStringArray *uiLabels
    = dynamic_cast<vtkStringArray *>(dbView->GetVertexData()->GetAbstractArray(0));
  vtkIntArray *ssIds
    = dynamic_cast<vtkIntArray *>(dbView->GetVertexData()->GetAbstractArray(1));
  vtkIntArray *ssRole
    = dynamic_cast<vtkIntArray *>(dbView->GetVertexData()->GetAbstractArray(2));
  // Edge
  vtkIntArray *edgeType
    = dynamic_cast<vtkIntArray *>(dbView->GetEdgeData()->GetAbstractArray(0));

  const avtMeshMetaData *meshMetaData=this->DatabaseMetaData->GetMesh(meshId);
  const string &meshName=meshMetaData->name;

  try
    {
    const int supersetId=sil->GetSetIndex(meshName);
    avtSILSet_p superset=sil->GetSILSet(supersetId);

    // Add new tree for this mesh.
    const vtkIdType meshBranchId=dbView->AddChild(rootId);
    string meshLabel;
    this->GenerateMeshDescriptor(meshId,meshLabel);
    uiLabels->InsertNextValue(meshLabel);
    ssIds->InsertNextValue(meshId); // not using supersetId. See below.
    ssRole->InsertNextValue(SIL_USERD_MESH);
    edgeType->InsertNextValue(SIL_EDGE_STRUCTURAL);
    // We have chosen to use mesh index rather than its SSId. The index can be used
    // to identify the mesh's meta data, from which we can get everything we need
    // additionally it provides an id to use to locate the mesh in our multi-block
    // output. Using the SSId would not provide the latter.
    #if defined vtkVisitDatabaseBridgeDEBUG
    cerr << meshName << endl;
    #endif

    // Add a branch for the arrays.
    vector<string> arrays;
    this->EnumerateDataArrays(meshId,arrays);
    const size_t nArrays=arrays.size();
    if (nArrays)
      {
      #if defined vtkVisitDatabaseBridgeDEBUG
      cerr << "\tArrays" << endl;
      #endif

      // Arrays don't explicitly show up in the SIL, I am
      // denoting this by giving them a negative SIL set id
      // idexed from 1. This keeps them from conflicting
      // with real SIL set ids.
      const vtkIdType arrayBranchId=dbView->AddChild(meshBranchId);
      uiLabels->InsertNextValue(string("Arrays"));
      ssIds->InsertNextValue(-static_cast<long>(nArrays));
      ssRole->InsertNextValue(SIL_USERD_ARRAY);
      edgeType->InsertNextValue(SIL_EDGE_STRUCTURAL);

      for (size_t i=0; i<nArrays; ++i)
        {
        #if defined vtkVisitDatabaseBridgeDEBUG
        cerr << "\t\t" << arrays[i] << endl;
        #endif

        dbView->AddChild(arrayBranchId);
        uiLabels->InsertNextValue(arrays[i]);
        ssIds->InsertNextValue(-static_cast<long>(i)-1);
        ssRole->InsertNextValue(SIL_USERD_ARRAY);
        edgeType->InsertNextValue(SIL_EDGE_STRUCTURAL);
        }
      }

    /// TODO Expressions are disabled for PV 3.6 release.
    // 
    // // Add a branch for the expressions
    // vector<string> expressions;
    // this->EnumerateExpressions(meshId,expressions);
    // const size_t nExpressions=expressions.size();
    // if (nExpressions)
    //   {
    //   #if defined vtkVisitDatabaseBridgeDEBUG
    //   cerr << "\tExpressions" << endl;
    //   #endif
    // 
    //   // Expressions don't explicitly show up in the SIL, I am
    //   // denoting this by giving them a negative SIL set id
    //   // and indexing from 1.This keeps them from conflicting
    //   // with real SIL set ids.
    //   const vtkIdType expressionBranchId=dbView->AddChild(meshBranchId);
    //   uiLabels->InsertNextValue("Expressions");
    //   ssIds->InsertNextValue(-nExpressions);
    //   ssRole->InsertNextValue(SIL_USERD_EXPRESSION);
    //   edgeType->InsertNextValue(SIL_EDGE_STRUCTURAL);
    // 
    //   for (size_t i=0; i<nExpressions; ++i)
    //     {
    //     #if defined vtkVisitDatabaseBridgeDEBUG
    //     cerr << "\t\t" << expressions[i] << endl;
    //     #endif
    // 
    //     dbView->AddChild(expressionBranchId);
    //     uiLabels->InsertNextValue(expressions[i]);
    //     ssIds->InsertNextValue(-i-1);
    //     ssRole->InsertNextValue(SIL_USERD_EXPRESSION);
    //     edgeType->InsertNextValue(SIL_EDGE_STRUCTURAL);
    //     }
    //   }

    // Superficially parse the SIL. NOT going to recursively walk the SIL.
    // We will only expose a single link out from each collection linked
    // to by this mesh.
    const vector<int> &collections=superset->GetRealMapsOut();
    const size_t nCollections=collections.size();
    for (size_t i=0; i<nCollections; ++i)
      {
      const int collectionId=collections[i];
      const avtSILCollection_p collection=sil->GetSILCollection(collectionId);
      string label;
      SILCategoryRole role=collection->GetRole();
      switch (role)
        {
        case SIL_BLOCK:
          label="Blocks";
          break;
        case SIL_ASSEMBLY:
          label="Assemblies";
          break;
        case SIL_MATERIAL:
          label="Materials";
          continue; /// TODO materials are disabled for PV 3.6 release.
          break;
        case SIL_SPECIES:
          label="Species";
          continue; /// TODO species are disabled for PV 3.6 release.
          break;

        case SIL_DOMAIN:
          label="Domains";
          break;

        case SIL_BOUNDARY:
        case SIL_TOPOLOGY:
        case SIL_PROCESSOR:
        case SIL_ENUMERATION:
        case SIL_USERD:
        default:
          // Support these categories at some point?
          #if defined vtkVisitDatabaseBridgeDEBUG
          cerr << "\t" << collection->GetCategory() << endl;
          cerr << "\t\t**** ingored ****" << endl;
          #endif
          continue;
          break;
        }
      #if defined vtkVisitDatabaseBridgeDEBUG
      cerr << "\t" << label << endl;
      #endif

      const vector<int> &subsets=collection->GetSubsetList();
      const size_t nSubsets=subsets.size();
      if (nSubsets)
        {
        // Add a branch for this collection.
        const vtkIdType collBranchId=dbView->AddChild(meshBranchId);
        uiLabels->InsertNextValue(label);
        ssIds->InsertNextValue(collectionId);
        ssRole->InsertNextValue(role);
        edgeType->InsertNextValue(SIL_EDGE_STRUCTURAL);

        // Create a leaf for each link out of the collection.
        for (int ii=0; ii<nSubsets; ++ii)
          {
          const avtSILSet_p subset=sil->GetSILSet(subsets[ii]);

          #if defined vtkVisitDatabaseBridgeDEBUG
          cerr << "\t\t" << subset->GetName() << endl;
          #endif

          vtkIdType linkFrom=dbView->AddChild(collBranchId);
          uiLabels->InsertNextValue(subset->GetName());
          ssIds->InsertNextValue(subsets[ii]);
          ssRole->InsertNextValue(role);
          edgeType->InsertNextValue(SIL_EDGE_STRUCTURAL);

          // add link edges.
          vector<int> links;
          EnumerateSILSetLinks(sil,subsets[ii],links);
          size_t nLinks=links.size();
          if (links[0]==subsets[ii])
            {
            // Self links only occur for leaves with no back links.
            continue;
            }
          #if defined vtkVisitDatabaseBridgeDEBUG
          cerr << "\t\t\tLinking " << subsets[ii] << " to " << links << endl;
          #endif
          for (int iii=0; iii<nLinks; ++iii)
            {
            // vtkIdType linkTo=ssIds->LookupValue(links[iii]); TODO not working ??
            vtkIdType linkTo=ReverseLookup(ssIds,links[iii]);
            if (linkTo<1) // NOTE all linked to ids should already be there ?
              {           // It depends on the order of the maps, if you see
                          // see this warning then this piece of code has to move
                          // into its own loop over all sets.
              vtkWarningMacro("Link ssId was not found. May need two pass algorithm.");
              continue;
              }
            #if defined vtkVisitDatabaseBridgeDEBUG
            cerr << uiLabels->GetValue(linkFrom) << " -> " << uiLabels->GetValue(linkTo) << endl;
            #endif
            dbView->AddEdge(linkFrom,linkTo);
            edgeType->InsertNextValue(SIL_EDGE_LINK);
            }
          }
        }
      }
    }
  catch(VisItException &e)
    {
    vtkWarningMacro( << e.Message());
    return NULL;
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::ReadData(
        vector<int> &meshIds,                        // list of mesh ids
        vector<vector<string> > &arrays,             // arrays
        vector<vector<int> > &availableDomains,      // blocks this process is responsible for
        vector<vector<int> > &selectedDomainSSIds,   // blocks that have been selected.
        vector<vector<int> > &selectedMaterialSSIds, // materials that have been selected.
        vtkDataObject *dataobject)                   // root dataobject
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const int nMeshes=this->DatabaseMetaData->GetNumMeshes();
  if (nMeshes==1)
    {
    // Because we have a single mesh in this file, our root dataobject
    // is vanilla VTK. We treat this as a special case.
    this->ReadData(
        meshIds[0],
        arrays[0],
        availableDomains[0],
        selectedDomainSSIds[0],
        selectedMaterialSSIds[0],
        dataobject);
    return 1;
    }

  // Multiple meshes in this file so we have a composite data set
  // to hold them.
  vtkMultiBlockDataSet *mbds=dynamic_cast<vtkMultiBlockDataSet *>(dataobject);
  if (!mbds)
    {
    vtkWarningMacro("Data type error. Failed to read multi-mesh.");
    return 0;
    }
  // Read each mesh
  size_t nActiveMeshes=meshIds.size();
  for (size_t i=0; i<nActiveMeshes; ++i)
    {
    vtkDataObject *meshObject=mbds->GetBlock(meshIds[i]);
    this->ReadData(
            meshIds[i],
            arrays[i],
            availableDomains[i],
            selectedDomainSSIds[i],
            selectedMaterialSSIds[i],
            meshObject);
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::ReadData(
        int meshId,
        vector<string> &arrays, // assumed valid on meshId
        vector<int> &availableDomains,
        vector<int> &selectedDomainSSIds,
        vector<int> &selectedMaterialSSIds,
        vtkDataObject *dataobject)
{
  if (!this->Database
    || !this->DatabaseMetaData)
    {
    // Configuration issues.
    vtkWarningMacro("Failed to read the database. "
      << "Database " << (this->Database?"is":"is not") << " open. "
      << "Database meta-data " 
      << (this->DatabaseMetaData?"has":"has not") << " been read. ");
      return 0;
    }

  try
    {
    // Get the SIL for this time step, from which construct a restriction
    // that defines what we want in this read. The restriction eventually
    // is converted into a data request that seeds the avt pipeline.
    const avtMeshMetaData *meshMetaData=this->DatabaseMetaData->GetMesh(meshId);
    const string &meshName=meshMetaData->name;
    avtSIL *sil = this->Database->GetSIL(this->ActiveTimeStep);
    // sil->Print(cerr);

    // Here we specify which mesh we are interested in, the SIL may expose more
    // than one mesh, however only one may be manipulated at a time.
    avtSILRestriction_p silr = new avtSILRestriction(sil);
    silr->SetTopSet(meshName.c_str());

    // If this mesh contains multiple domains, the user has a subsetting 
    // opportunity. In that case we turn all domains off, and turn on only 
    // the selected ones. However for meshes with only one domain the user
    // is never given a subsetting oportunity, and there will be no selected 
    // domains, so we can't turn off the domains in that case.
    const int nAllDomains=meshMetaData->numBlocks;
    if (nAllDomains>1)
      {
      silr->TurnOffAll();
      }
    // Turn back on selcted domains.
    const size_t nSelectedDomains=selectedDomainSSIds.size();
    if (nSelectedDomains)
      {
      for (int i=0; i<nSelectedDomains; ++i)
        {
        silr->TurnOnSet(selectedDomainSSIds[i]);
        }
      }
    // Restrict to those available on this processes.
    silr->RestrictDomainsForLoadBalance(availableDomains);
    // TODO this might be done better with boolean ops on the multiple
    // restrictions. I tried it briefly and it didn't work.
  
    // M.I.R.
    // // TODO domain decomposition and MIR don't apear to work together.
    // const size_t nSelectedMaterials=selectedMaterialSSIds.size();
    // if (nSelectedMaterials)
    //   {
    //   for (size_t i=0; i<nSelectedMaterials; ++i)
    //     {
    //     silr->TurnOnSet(selectedMaterialSSIds[i]);
    //     }
    //   }
    // cerr << "N Materials: "<<  nSelectedMaterials << endl;

    avtDataRequest_p dreq;
    const char *activeArray;
    const size_t nArrays=arrays.size();
    if (nArrays>0)
      {
      // User has requested data arrays. We have to set the data
      // request variable to one of them and add the rest as
      // secondary variables.
      activeArray=arrays[0].c_str();
      dreq
      = new avtDataRequest(activeArray, this->ActiveTimeStep, silr);
      for (int i=1; i<nArrays; ++i)
        {
        dreq->AddSecondaryVariable(arrays[i].c_str());
        }
      }
    else
      {
      // User has requested no arrays. In this case we have to set
      // the data request variable to the mesh name.
      activeArray=meshName.c_str();
      dreq
      = new avtDataRequest(activeArray, this->ActiveTimeStep, silr);
      }

    // Create an avt pipeline.
    avtSourceFromDatabase dsrc(this->Database,activeArray,0);
    // Get the output.
    avtDataTree_p dtree=this->Database->GetOutput(dreq,&dsrc);

    //   avtExpressionEvaluatorFilter eeval;
    //   eeval.SetInput((avtDataObject *)*dtree);
    //   avtDataObject *dobj=*eeval.GetOutput();
    //   avtDataTree_p dtree=dynamic_cast<avtDataTree *>(dobj);
    // TODO Evaluate expressions.

    // Get the output of the avt pipeline.
    int nDomains;
    vtkDataSet **domains=dtree->GetAllLeaves(nDomains);
    if (nDomains==0)
      {
      delete [] domains;
      return 0;
      }
    vector<int>  domainIds;
    dtree->GetAllDomainIds(domainIds);

    // Copy the newly read data into the output.
    // We have to handle two special cases: AMR and MultiBlock data.
    // First special case is AMR...
    if (meshMetaData->meshType==AVT_AMR_MESH)
      {
      vtkHierarchicalBoxDataSet *hbds=dynamic_cast<vtkHierarchicalBoxDataSet *>(dataobject);
      if (!hbds)
        {
        vtkWarningMacro("Data type error. Failed to read AMR mesh.");
        return 0;
        }

      // TODO loop over the returned blocks,{
      //         convert rectlinear to image data
      //         get group and block id for this block.
      //         insert into the output. }
      //hbds->SetDataSet(meshMetaData->groupIds[blockId[i]],blocks[i]);
      }
    // Second special case is MultiBlock.
    else
    if (nAllDomains>1)
      {
      vtkMultiBlockDataSet *mbds=dynamic_cast<vtkMultiBlockDataSet *>(dataobject);
      for (int i=0; i<nDomains; ++i)
        {
        mbds->SetBlock(domainIds[i],domains[i]);
        }
      return 1;
      }
    // All other data are single VTK data set.
    else
      {
      vtkDataSet *ds=dynamic_cast<vtkDataSet *>(dataobject);
      ds->ShallowCopy(domains[0]);
      }

    // We have to delete the array but ref counted pointers are being
    // used by VisIt so we need not delete elements.
    delete [] domains;
    }
  catch(VisItException &e)
    {
    vtkWarningMacro( << e.Message());
    return NULL;
    }
  return 1;
}



// //-----------------------------------------------------------------------------
// int vtkVisItDatabase::GetMeshExtents(int extents[6])
// {
//   if (!this->MeshMetaData)
//     {
//     vtkWarningMacro("Failed to get mesh extents.");
//     return 0;
//     }
//   
//   
//   
// }

//-----------------------------------------------------------------------------
int vtkVisItDatabase::EnumerateMeshes(
      vtkstd::vector<vtkstd::string> &meshDesc)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const size_t nMeshes=this->DatabaseMetaData->GetNumMeshes();
  size_t descId=meshDesc.size();
  meshDesc.resize(descId+nMeshes);
  for (size_t meshId=0; meshId<nMeshes; ++meshId)
    {
    this->GenerateMeshDescriptor(meshId,meshDesc[descId]);
    ++descId;
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::GenerateMeshDescriptor(
        int meshId,
        vtkstd::string &meshDesc)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const avtMeshMetaData *meshMetaData
    = this->DatabaseMetaData->GetMesh(meshId);

  vtkstd::ostringstream desc;
  desc << "Mesh " << meshMetaData->name << " has "
        << meshMetaData->numBlocks << " "
        << meshMetaData->spatialDimension << "D "
        << this->GetMeshType(meshId) << " dataset"
        << (meshMetaData->numBlocks>1?"s.":".");

  meshDesc=desc.str();

  return 1;
}
//-----------------------------------------------------------------------------
int vtkVisItDatabase::EnumerateDataArrays(
      vtkstd::vector<vtkstd::string> &names)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  // Collect all arrays on all meshes.
  const size_t nScalars=this->DatabaseMetaData->GetNumScalars();
  const size_t nVectors=this->DatabaseMetaData->GetNumVectors();
  const size_t nTensors=this->DatabaseMetaData->GetNumTensors();
  const size_t nSymTensors=this->DatabaseMetaData->GetNumSymmTensors();
  const size_t nMaterials=this->DatabaseMetaData->GetNumMaterials();
  size_t arrayId=names.size();
  const size_t nExpected
    = arrayId+nScalars+nVectors+nTensors+nSymTensors+nMaterials;
  names.resize(nExpected);

  // Scalars...
  for (int i=0; i<nScalars; ++i)
    {
    const avtScalarMetaData *smd=this->DatabaseMetaData->GetScalar(i);
    names[arrayId]=smd->name;
    ++arrayId;
    }
  // Vectors...
  for (int i=0; i<nVectors; ++i)
    {
    const avtVectorMetaData *vmd=this->DatabaseMetaData->GetVector(i);
    names[arrayId]=vmd->name;
    ++arrayId;
    }
  // Tensors...
  for (int i=0; i<nTensors; ++i)
    {
    const avtTensorMetaData *tmd=this->DatabaseMetaData->GetTensor(i);
    names[arrayId]=tmd->name;
    ++arrayId;
    }
  // Symetric Tensors...
  for (int i=0; i<nSymTensors; ++i)
    {
    const avtSymmetricTensorMetaData *stmd=this->DatabaseMetaData->GetSymmTensor(i);
    names[arrayId]=stmd->name;
    ++arrayId;
    }
  // Materials...
  for (int i=0; i<nMaterials; ++i)
    {
    const avtMaterialMetaData *mmd=this->DatabaseMetaData->GetMaterial(i);
    names[arrayId]=mmd->name;
    ++arrayId;
    }
  // Expressions should be considered seperately.
  return 1;
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::EnumerateDataArrays(
      vtkstd::vector<vtkstd::vector<vtkstd::string> > &names)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const int nMeshes=this->DatabaseMetaData->GetNumMeshes();
  names.resize(nMeshes);
  for (int meshId=0; meshId<nMeshes; ++meshId)
    {
    this->EnumerateDataArrays(meshId,names[meshId]);
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::EnumerateDataArrays(
      int meshId,
      vtkstd::vector<vtkstd::string> &names)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }
  // Collect the array names that are valid on this mesh.
  const size_t nScalars=this->DatabaseMetaData->GetNumScalars();
  const size_t nVectors=this->DatabaseMetaData->GetNumVectors();
  const size_t nTensors=this->DatabaseMetaData->GetNumTensors();
  const size_t nSymTensors=this->DatabaseMetaData->GetNumSymmTensors();
  const size_t nMaterials=this->DatabaseMetaData->GetNumMaterials();
  const size_t arrayId=names.size();
  const size_t nExpected
    = arrayId+nScalars+nVectors+nTensors+nSymTensors+nMaterials;
  names.reserve(nExpected);

  const avtMeshMetaData *meshMetaData
    = this->DatabaseMetaData->GetMesh(meshId);
  const string &meshName=meshMetaData->name;

  // Scalars...
  for (int i=0; i<nScalars; ++i)
    {
    const avtScalarMetaData *smd=this->DatabaseMetaData->GetScalar(i);
    if (this->DatabaseMetaData->MeshForVar(smd->name)==meshName)
      {
      names.push_back(smd->name);
      }
    }
  // Vectors...
  for (int i=0; i<nVectors; ++i)
    {
    const avtVectorMetaData *vmd=this->DatabaseMetaData->GetVector(i);
    if (this->DatabaseMetaData->MeshForVar(vmd->name)==meshName)
      {
      names.push_back(vmd->name);
      }
    }
  // Tensors...
  for (int i=0; i<nTensors; ++i)
    {
    const avtTensorMetaData *tmd=this->DatabaseMetaData->GetTensor(i);
    if (this->DatabaseMetaData->MeshForVar(tmd->name)==meshName)
      {
      names.push_back(tmd->name);
      }
    }
  // Symetric Tensors...
  for (int i=0; i<nSymTensors; ++i)
    {
    const avtSymmetricTensorMetaData *stmd=this->DatabaseMetaData->GetSymmTensor(i);
    if (this->DatabaseMetaData->MeshForVar(stmd->name)==meshName)
      {
      names.push_back(stmd->name);
      }
    }
  // Materials...
  for (int i=0; i<nMaterials; ++i)
    {
    const avtMaterialMetaData *mmd=this->DatabaseMetaData->GetMaterial(i);
    if (this->DatabaseMetaData->MeshForVar(mmd->name)==meshName)
      {
      names.push_back(mmd->name);
      }
    }
  // Expressions should be considered seperately.
  return 1;
}

// NOTE this is only required for kitchen sink UI 
//-----------------------------------------------------------------------------
int vtkVisItDatabase::RestrictDataArraysToMesh(
      int meshId,
      vtkstd::vector<vtkstd::string> &possibles,
      vtkstd::vector<vtkstd::string> &actuals)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const string &meshName
    = this->DatabaseMetaData->GetMesh(meshId)->name;

  size_t nPossibles=possibles.size();
  actuals.reserve(nPossibles+actuals.size());

  for (size_t i=0; i<nPossibles; ++i)
    {
    const string &array=possibles[i];
    if (this->DatabaseMetaData->MeshForVar(array)==meshName)
      {
      actuals.push_back(array);
      #if defined vtkVisitDatabaseBridgeDEBUG
      cerr << array << " defined on " << this->DatabaseMetaData->MeshForVar(array)
            << " kept." << endl;
      #endif
      }
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::EnumerateExpressions(
      vtkstd::vector<vtkstd::vector<vtkstd::string> > &names)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const int nMeshes=this->DatabaseMetaData->GetNumMeshes();
  names.resize(nMeshes);
  for (int meshId=0; meshId<nMeshes; ++meshId)
    {
    this->EnumerateExpressions(meshId,names[meshId]);
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::EnumerateExpressions(
      int meshId,
      vtkstd::vector<vtkstd::string> &names)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const avtMeshMetaData *meshMetaData
    = this->DatabaseMetaData->GetMesh(meshId);
  const string &meshName=meshMetaData->name;

  int nExpressions=this->DatabaseMetaData->GetNumberOfExpressions();
  for (int i=0; i<nExpressions; ++i)
    {
    const Expression *expr=this->DatabaseMetaData->GetExpression(i);
    try
      {
      string exprMesh=this->DatabaseMetaData->MeshForVar(expr->GetDefinition());
      if (exprMesh==meshName)
        {
        names.push_back(expr->GetDefinition());
        }
      }
    catch(VisItException &e)
      {
      // TODO Sometimes expressions valid on this mesh put us here...
      //cerr << e.Message() << endl;
      continue;
      }
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::EnumerateExpressions(
      vtkstd::vector<vtkstd::string> &names)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }
  int nExpressions=this->DatabaseMetaData->GetNumberOfExpressions();
  int exprId=names.size();
  names.resize(exprId+nExpressions);
  for (int i=0; i<nExpressions; ++i)
    {
    const Expression *expr=this->DatabaseMetaData->GetExpression(i);
    names[exprId]=expr->GetName();
    ++exprId;
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::EnumerateTimeSteps(
      vtkstd::vector<double> &timeSteps)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  int nSteps=this->DatabaseMetaData->GetNumStates();
  #if defined vtkVisitDatabaseBridgeDEBUG
  cerr << "Number of states is " << nSteps << endl;
  #endif
  if (nSteps>1)
    {
    // Either we have accurate times...
    int hasExt=this->DatabaseMetaData->GetHasTemporalExtents();
    const vector<double> &dbtimes=this->DatabaseMetaData->GetTimes();
    const vector<int> &dbcycles=this->DatabaseMetaData->GetCycles();
    if (dbtimes.size()==nSteps
      && this->DatabaseMetaData->AreAllTimesAccurateAndValid(-1))
      {
      timeSteps=dbtimes;
      }
    // Or, we have accurate cycles...
    else if (dbcycles.size()==nSteps
      && this->DatabaseMetaData->AreAllCyclesAccurateAndValid(-1))
      {
      timeSteps.resize(nSteps);
      // with extents.
      if (hasExt) 
        {
        double delta;
        double text[2]={0.0};
        text[0]=this->DatabaseMetaData->GetMinTemporalExtents();
        text[1]=this->DatabaseMetaData->GetMaxTemporalExtents();
        delta
          = (text[1]-text[0])/static_cast<double>(nSteps-1);
        for (int i=0; i<nSteps; ++i)
          {
          int cycle=dbcycles[i];
          timeSteps[i]=static_cast<double>(cycle)*delta+text[0];
          }
        }
      // without extents.
      else
        {
        for (int i=0; i<nSteps; ++i)
          {
          timeSteps[i]=dbcycles[i];
          }
        }
      }
    // OR, we have neither accurate times nor cycles...
    else
      {
      timeSteps.resize(nSteps);
      double delta;
      double text[2]={0.0};
      // with extents.
      if (hasExt)
        {
        text[0]=this->DatabaseMetaData->GetMinTemporalExtents();
        text[1]=this->DatabaseMetaData->GetMaxTemporalExtents();
        delta
          = (text[1]-text[0])/static_cast<double>(nSteps-1);
        }
      // without extents.
      else
        {
        delta = 1.0/static_cast<double>(nSteps-1);
        }
      // Generate a dummy sequnece.
      for (int i=0; i<nSteps; ++i)
        {
        timeSteps[i]=static_cast<double>(i)*delta+text[0];
        #if defined vtkVisitDatabaseBridgeDEBUG
        cerr << timeSteps[i] << endl;
        #endif
        }
      }
    }
  // No times were found.
  else
    {
    timeSteps.resize(1,0.0);
    }
  return 1;
}

//-----------------------------------------------------------------------------
bool vtkVisItDatabase::DecomposesDomain()
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }
  return this->DatabaseMetaData->GetFormatCanDoDomainDecomposition();
}

//-----------------------------------------------------------------------------
int vtkVisItDatabase::GetNumberOfMeshes()
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  return this->DatabaseMetaData->GetNumMeshes();
}


//-----------------------------------------------------------------------------
int vtkVisItDatabase::GetNumberOfBlocks(int meshId)
{
  if (!this->DatabaseMetaData)
    {
    vtkWarningMacro("No meta data available.");
    return 0;
    }

  const avtMeshMetaData *meshMetaData
    = this->DatabaseMetaData->GetMesh(meshId);

  return meshMetaData->numBlocks;
}

// //-----------------------------------------------------------------------------
// int vtkVisItDatabase::UpdateData(vtkDataObject *dataOut)
// {
//   vector<string> dataArrayNames;
//   this->EnumerateDataArrays(dataArrayNames);
//   return this->UpdateData(dataOut,dataArrayNames);
// }



//-----------------------------------------------------------------------------
void vtkVisItDatabase::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PluginPath:        " << this->PluginPath << endl;
  os << indent << "PluginId:          " << this->PluginId << endl;
  os << indent << "LibType:           " << this->LibType << endl;
  os << indent << "FileName:          " << this->FileName << endl;
  os << indent << "ActiveTimeStep:    " << this->ActiveTimeStep << endl;
  os << indent << "PluginLoaded:      " << this->PluginLoaded << endl;
  if (this->PluginLoaded)
    {
    CommonDatabasePluginInfo *info
      = this->Manager->GetCommonPluginInfo(this->PluginId);
    if (info) 
      {
      os << indent << "PluginName:        " << info->GetName() << endl
         << indent << "Version:           " << info->GetVersion() << endl
         << indent << "Supported exts:    " << this->Manager->PluginFileExtensions(this->PluginId) << endl
         << indent << "Supported files:   " << this->Manager->PluginFilenames(this->PluginId) << endl;
      }
    }
  if (this->DatabaseMetaData)
    {
    this->DatabaseMetaData->Print(os);
    }
}
