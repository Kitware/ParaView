#include <iostream>
#include <string>
#include <vector>
#include <map>
using namespace std;

#include "vtkVisItDatabase.h"
#include "vtkVisItDatabaseBridge.h"
#include "vtkDataSet.h"



//=============================================================================

int main(int argc, char **argv)
{
  const char pluginDir[]="/home/burlen/ext2/v3/visit1.10.0/src/plugins\0";
  const char *pluginId;
  const char *fileName;

  int exId=0;
  if (argc>1)
    {
    exId=atoi(argv[1]);
    }
  else
    {
    cerr << "One of the following datasets can be selected by specifying its ID." << endl;
    cerr << "0               1F5A.pdb" << endl;
    cerr << "1               SCHL.v5d" << endl;
    cerr << "2               test.3d.double.hdf5" << endl;
    cerr << "3               bigsil.silo" << endl;
    cerr << "4               poly3d.silo" << endl;
    cerr << "5               noise.silo" << endl;
    cerr << "6               tire.silo" << endl;
    cerr << "7               specmix_quad.silo" << endl;
    return 1;
    }
  // Select which file to load from some examples:
  // PDB: STSD unstructured
  // V5D: MTSD recilinear
  // 
  switch (exId)
    {
    case 0:
      pluginId="ProteinDataBank_1.0";
      fileName="/home/burlen/ext2/visit/1F5A.pdb";
      break;
    case 1:
      pluginId="Vis5D_1.0";
      fileName="/home/burlen/ext2/visit/datasets/SCHL.v5d";
      break;
    case 2:
      pluginId="Chombo_1.0";
      fileName="/home/burlen/ext2/visit/datasets/test.3d.double.hdf5";
      break;
    case 3:
      pluginId="Silo_1.0";
      fileName="/home/burlen/ext2/visit/datasets/bigsil.silo";
      break;
    case 4:
      pluginId="Silo_1.0";
      fileName="/home/burlen/ext2/visit/datasets/poly3d.silo";
      break;
    case 5:
      pluginId="Silo_1.0";
      fileName="/home/burlen/ext2/visit/datasets/noise.silo";
      break;
    case 6:
      pluginId="Silo_1.0";
      fileName="/home/burlen/ext2/visit/datasets/visit/data/tire.silo";
      break;
    case 7:
      pluginId="Silo_1.0";
      fileName="/home/burlen/ext2/visit/datasets/specmix_quad.silo";
      break;
    default:
      cerr << "Unsupported example id: " << exId << endl;
      return 1;
      break;
    }

  // Low level access.
//   vtkVisItDatabaseSource *Db=vtkVisItDatabaseSource::New();
//   Db->SetPluginPath(pluginDir);
//   Db->SetPluginId(pluginId);
//   Db->LoadPlugin();
//   Db->SetFileName(fileName);
//   Db->OpenDatabase();
//   Db->UpdateMetaData();
//   Db->Print(cerr);
//   Db->Delete();
// 
//   vtkDataSet *d=Db->GetDataSet();
//   Db->UpdateData(d);
//   d->Print(cerr);
//   Db->Delete();
//   d->Delete();

  // High level VTK pipeline access
  vtkVisItDatabaseBridge *dbb=vtkVisItDatabaseBridge::New();
  dbb->SetPluginPath(pluginDir);
  dbb->SetPluginId(pluginId);
  dbb->SetFileName(fileName);
  dbb->UpdateInformation();

//   const int nMeshes=dbb->GetNumberOfMeshArrays();
//   for (int i=0; i<nMeshes; ++i)
//     {
//     const char *mesh=dbb->GetMeshArrayName(i);
//     dbb->SelectMeshArray(mesh);
//     }
//   const int nArrays=dbb->GetNumberOfDataArrays();
//   for (int i=0; i<nArrays; ++i)
//     {
//     const char *array=dbb->GetDataArrayName(i);
//     dbb->SelectDataArray(array);
//     }
//   vtkDataObject *d=dbb->GetOutput();
  dbb->Print(cerr);
//   d->Update();
//   d->Print(cerr);

  dbb->Delete();

  return 0;
}

