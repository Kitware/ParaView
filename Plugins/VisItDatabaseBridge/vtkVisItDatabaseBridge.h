/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVisItDatabaseBridge.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVisItDatabaseBridge -- Connects the VTK pipeline to the vtkVisItDatabase class.
// .SECTION Description
//
// Implements the VTK style pipeline and manipulates and instance of
// vtkVisItDatabase so that any VisIt database plugin may be used to
// provide data stroed on disk to VTK.
//
// .SECTION See Also
// vtkVisItDatabase

#ifndef vtkVisItDatabaseBridge_h
#define vtkVisItDatabaseBridge_h

#include "vtkDataObjectAlgorithm.h"
#include <vtkstd/vector>// no comment

// define this for cerr status.
// #define vtkVisItDatabaseBridgeDEBUG

class vtkVisItDatabase;
class vtkDataArraySelection;
class vtkCallbackCommand;

class vtkVisItDatabaseBridge : public vtkDataObjectAlgorithm
{
public:
  static vtkVisItDatabaseBridge *New();
  vtkTypeMacro(vtkVisItDatabaseBridge,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  // Description:
  // Get/Set the file to read.
  void SetFileName(const char *file);
  vtkGetStringMacro(FileName);
  // Description
  // Determine if the file can be read.
  int CanReadFile(const char *file);
  // Description:
  // Get/Set the path to the database plugin (.so's) directory. This
  // typically needs only to be done once right after construction.
  void SetPluginPath(const char *path);
  vtkGetStringMacro(PluginPath);
  // Description:
  // Get/Set the Visit plugin id. This typically needs to be done
  // only once right after construction.
  void SetPluginId(const char *id);
  vtkGetStringMacro(PluginId);

  // Description:
  // Clear the current U.I. settings and restore to default
  // values.
  void InitializeUI();
  // Description:
  // Set a list of mesh ids that will be read. It's designed to
  // work with the limited capability of PV3 client server patterns
  // thus the first element of the array is expected to be the meshId
  // A second method is provided for general purpose use where
  // meshId is specified as a parameter.
  void SetMeshIds(int *ids);
  void SetMeshIds(int n, int *ids);
  void SetNumberOfMeshes(int nMeshes);
  // Description:
  // The following methods set a list of the ids of various properties to 
  // be read during the RequestData phase of pipeline execution. The interface
  // is designed to work with the limited capability of PV3 client server
  // patterns, The first two elements of the array are expected to be
  // the meshId and the number of ids passed. A second interface is
  // provided for general programatic use where meshId and number of ids
  // are specified as parameters. See GetDatabaseView for how to get the
  // available ids.
  void SetArrayIds(int *ids);
  void SetArrayIds(int meshId, int n, int *ids);
  //
  void SetExpressionIds(int *ids);
  void SetExpressionIds(int meshId, int n, int *ids);
  //
  void SetBlockSSIds(int *ids);
  void SetBlockSSIds(int meshId, int n, int *ids);
  //
  void SetAssemblySSIds(int *ids);
  void SetAssemblySSIds(int meshId, int n, int *ids);
  //
  void SetDomainSSIds(int *ids);
  void SetDomainSSIds(int meshId, int n, int *ids);
  //
  void SetMaterialSSIds(int *ids);
  void SetMaterialSSIds(int meshId, int n, int *ids);
  //
  void SetSpeciesSSIds(int *ids);
  void SetSpeciesSSIds(int meshId, int n, int *ids);

  // Description:
  // Set various SIL set operations. The deafult is for intersection
  // so that the highest level abstraction is applied. Union can 
  // be used to combine things in a more flixible manner.
  void SetDomainToBlockSetOperation(int op);
  void SetDomainToAssemblySetOperation(int op);
  void SetDomainToMaterialSetOperation(int op);

  // Description:
  // Sets modified if array selection changes.
  static void SelectionModifiedCallback( vtkObject*,
                                         unsigned long,
                                         void* clientdata,
                                         void* );


protected:
  // Description:
  // Set up pipeline for composite data, we may or may not have it.
  vtkExecutive* CreateDefaultExecutive();
  // Description:
  // Read the dataset.
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  // Description:
  // Create the appropriate output dataset.
  int RequestDataObject(vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output);
  // Description:
  // Extract meta-data from the file that is to be read.
  int RequestInformation(vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output);

  vtkVisItDatabaseBridge();
  ~vtkVisItDatabaseBridge();

private:
  vtkVisItDatabaseBridge(const vtkVisItDatabaseBridge &); // Not implemented
  void operator=(const vtkVisItDatabaseBridge &); // Not implemented
  //
  void Clear();
  int LoadVisitPlugin();
  int UpdateVisitSource();
  //BTX
  void DistributeBlocks(
        int procId,
        int nProcs,
        int nBlocks,
        vtkstd::vector<int> &blockIds);
  //ETX

private:
  vtkVisItDatabase *VisitSource;
  char *FileName;          // Name of data file to load.
  char *PluginId;          // Name of the plugin to use to load this file.
  char *PluginPath;        // Where to load the plugin from.
  int UpdateDatabase;      // If set then database will re-loaded.
  int UpdatePlugin;        // If set then plugin will be re-loaded.
  int UpdateDatabaseView;
  /// PV Interface.
  //BTX
  vtkstd::vector<int>  MeshIds;                        // List of mesh ids to process.
  vtkstd::vector<vtkstd::vector<int> > ArrayIds;       // For erach mesh in MeshSSIds list
  vtkstd::vector<vtkstd::vector<int> > ExpressionIds;  // these list the category elements
  vtkstd::vector<vtkstd::vector<int> > DomainSSIds;    // to include in the output.
  vtkstd::vector<vtkstd::vector<int> > BlockSSIds;
  vtkstd::vector<vtkstd::vector<int> > AssemblySSIds;
  vtkstd::vector<vtkstd::vector<int> > MaterialSSIds;
  vtkstd::vector<vtkstd::vector<int> > SpeciesSSIds;
  //
  vtkstd::vector<vtkstd::vector<vtkstd::string> > ArrayNames;
  vtkstd::vector<vtkstd::vector<vtkstd::string> > ExpressionNames;
  //ETX
  int DomainBlockSSOp;     // These contol how SIL sets are formed
  int DomainAssemblySSOp;  // prior to making the data request.
  int DomainMaterialSSOp;
  int DomainSpeciesSSOp;

  ///
  vtkCallbackCommand *SelectionObserver;
  int BlockModified;
  int DatabaseViewMTime;
};

#endif
