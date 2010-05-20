/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVisItDatabase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVisItDatabase -- Object for manipulating VisIt database plugins.
// .SECTION Description:
//
// This class is essentially a short avt pipeline providing low
// level interface to visit database plugins, encapsulating
// visit data structures and avt internals. An important feature
// is that this class can open any visit database and get a 
// corresponding vtk object.
//
#ifndef __vtkVisItDatabase_h
#define __vtkVisItDatabase_h

#include "vtkObject.h"
#include <vtkstd/string>// no comment
#include <vtkstd/vector>// no comment

// define this for cerr status.
// #define vtkVisitDatabaseBridgeDEBUG

class vtkDataObject;
class avtGenericDatabase;
class avtMeshMetaData;
class avtDatabaseMetaData;
class DatabasePluginManager;

class vtkDataSet;
class vtkMutableDirectedGraph;

class VTK_EXPORT vtkVisItDatabase : public vtkObject
{
public:
  static vtkVisItDatabase *New();
  vtkTypeMacro(vtkVisItDatabase, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the file name to open.
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);
  // Description:
  // Get/Set the active time step.
  vtkGetMacro(ActiveTimeStep,int);
  vtkSetMacro(ActiveTimeStep,int);
  // Description:
  // Get/Set the plugin path.
  vtkGetStringMacro(PluginPath);
  vtkSetStringMacro(PluginPath);
  // Description:
  // Get/Set the plugin id.
  vtkGetStringMacro(PluginId);
  vtkSetStringMacro(PluginId);
  // Description:
  // Get/Set the plugin type to Serial or Parallel.
  void SetLibTypeToParallel(){ this->SetLibType(PARALLEL_LIBS); }
  void SetLibTypeToSerial(){ this->SetLibType(SERIAL_LIBS); }
  vtkSetMacro(LibType,int);
  vtkGetMacro(LibType,int);

  // Description:
  // Load a visit plugin based on the PluginId, PluginPath and
  // LibraryType setting, preparing the object for reading files.
  int Configure()
    { return this->LoadPlugin(); }
  // Description:
  // Load the plugin, before calling this method, you must
  // Set the plugin path, its id and its build type.
  int LoadPlugin();
  // Description:
  // Load visit's libraries that are needed by all plugins.
  int LoadVisitLibraries();
  // Description:
  // Unload the currently loaded plugin. If any database is open,
  // it will be closed first.
  void UnloadPlugin();

  // Description:
  // Open the database. Plugin should have previsouly been loaded and
  // FileName previously set.
  int OpenDatabase();
  // Description:
  // Close the open database, database and file attributes are reset.
  void CloseDatabase();

  // Description:
  // Returns true if the plugin path and id have been set. If so then
  // supplied with a file name the database can attempt reads.
  bool Configured()
    { return this->PluginPath && this->PluginId; }
  // Description:
  // Returns true if a database is currently open. If the file name
  // changes then the database should be closed and re-opened.
  bool Opened()
    { return this->Database!=NULL; }

  // Description:
  // Update both mesh and database meta data. Should be called
  // when either active time step or active mesh changes.
  int UpdateMetaData();

  // Description:
  // Return true if the dataset is amr or composite.
  bool ProducesAMRData(int meshId);
  bool ProducesMultiBlockData(int meshId);
  bool ProducesStructuredData(int meshId);
  bool ProducesUnstructuredData(int meshId);

  // Description:
  // Return the number of meshes available.
  int GetNumberOfMeshes();
  bool HasMultipleMeshes();
  // Description:
  // Return the number of blocks available.
  int GetNumberOfBlocks(int meshId);
  // Description:
  // Return true if the curent database can do domain decomposition.
  bool DecomposesDomain();

  // Description:
  // Construct a view of the database in the passed in graph. The graph is a
  // forset of "mesh" trees. The depth of the mesh tree is usually 3.
  // Mesh->[domains->[...] variables->[...] materials->[...] groups->[...]]
  int GenerateDatabaseView(vtkMutableDirectedGraph *g);
  int GenerateMeshView(int meshId, int rootid, vtkMutableDirectedGraph *g);

  // Description:
  // Get scalar, vector, and tensor data array names available.
  int EnumerateDataArrays(vtkstd::vector<vtkstd::vector<vtkstd::string> > &names);
  int EnumerateDataArrays(int meshId, vtkstd::vector<vtkstd::string> &names);
  int EnumerateDataArrays(vtkstd::vector<vtkstd::string> &names);
  // Description:
  // Given a mesh Id and a list of data arrays, and list to 
  // append resutls to, append only those arrays defined 
  // on the given mesh.
  int RestrictDataArraysToMesh(
        int meshId,
        vtkstd::vector<vtkstd::string> &possibles,
        vtkstd::vector<vtkstd::string> &actuals);
  // Description:
  // Get expression names available.
  int EnumerateExpressions(vtkstd::vector<vtkstd::vector<vtkstd::string> > &names);
  int EnumerateExpressions(int meshId, vtkstd::vector<vtkstd::string> &names);
  int EnumerateExpressions(vtkstd::vector<vtkstd::string> &names);
  // Description:
  // Get mesh descriptor the named mesh or for all present.
  int EnumerateMeshes(vtkstd::vector<vtkstd::string> &Meshdesc);
  int GenerateMeshDescriptor(int meshId, vtkstd::string &meshdesc);
  // Description:
  // Get the time steps available.
  int EnumerateTimeSteps(vtkstd::vector<double> &steps);
  // Description:
  // Return the descriptive string of the type of VisIt mesh in 
  // the database. If you want the actual type see GetDataObjectType.
  const char *GetMeshType(int meshId);
  // Description:
  // Return the type of VTK dataset needed to read in the data.
  // If no meshId is specified then the root type is returned.
  const char *GetDataObjectType();
  const char *GetDataObjectType(int meshId);
  // Description:
  // Return a new instance of the VTK data object needed to read 
  // the data. If no mesh is specified the root type is returned.
  // The resulting objects are not configured. The typical use case
  // will be to  first call GetDataObject then call ConfigureDataObject
  // on the result. This has been done to support ParaView pipeline 
  // where construction and configuration are distinct.
  vtkDataObject *GetDataObject();
  vtkDataObject *GetDataObject(int meshId);
  // Description:
  // Configure the dataset passed in. If it is of the wrong
  // type an error will occur. If no meshId is specified then
  // the entire tree is configured.
  int ConfigureDataObject(vtkstd::vector<int> &meshIds, vtkDataObject *output);
  int ConfigureDataObject(int meshId, vtkDataObject *output);

  // Description:
  // Read the file/files and extract vtk data sets. The list of arrays 
  // is expected to be valid on the specified mesh. The blockIds list 
  // conatins blocks to read. The meshIds list the meshes to read.
  // It is the caller responsibility to pass in the appropriate VTK data
  // object and to be sure meshIds, blockIds, and arrays are valid.
  // See GetDataObject, ConfigureDataObject, RestrictDataArraysToMesh.
  int ReadData(
        vtkstd::vector<int> &meshIds,
        vtkstd::vector<vtkstd::vector<vtkstd::string> >&arrays,
        vtkstd::vector<vtkstd::vector<int> > &availableDomains,
        vtkstd::vector<vtkstd::vector<int> > &selectedDomainSSIds,
        vtkstd::vector<vtkstd::vector<int> > &selectedMaterialSSIds,
        vtkDataObject *dataobject);
  int ReadData(
        int meshId,
        vtkstd::vector<vtkstd::string> &arrays,
        vtkstd::vector<int> &availableDomains,
        vtkstd::vector<int> &selectedDomainSSIds,
        vtkstd::vector<int> &selectedMaterialSSIds,
        vtkDataObject *dataobject);

  // Description:
  // Initialize the object state.
  void Initialize();
  void InitializeDataAttributes();
  void InitializeDatabaseAttributes();
  void InitializeConfiguration();


private:
  vtkVisItDatabase();
  ~vtkVisItDatabase();
private:
  char *PluginPath;      // Path to visit database .so's
  char *PluginId;        // Name_version of the plugin to load.
  char *FileName;        // Path to file to open.
  int PluginLoaded;      // Set when load succeeds.
  int ActiveTimeStep;    // Which time state to read.
  int LibType;           // Serial or Parallel visit libraries?
  enum {SERIAL_LIBS=0,PARALLEL_LIBS=1};
  //
  DatabasePluginManager *Manager;
  avtGenericDatabase *Database;
  const avtDatabaseMetaData *DatabaseMetaData;
  //
  static const char *DbTypeStr[];
  static const char *MeshTypeStr[];
};

#endif
