/*=========================================================================

  Program:   ParaView
  Module:    vtkHDF5RawImageReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHDF5RawImageReader.h"

#include "vtkCallbackCommand.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkLongArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkShortArray.h"
#include "vtkPVFiltersConfig.h"


// Include ordering of these four files is very sensitive on HP-UX.
#include <vtkstd/vector>
#include <vtkstd/string>
#include <sys/stat.h>
#include <hdf5.h>

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkHDF5RawImageReader, "1.12");
vtkStandardNewMacro(vtkHDF5RawImageReader);

//----------------------------------------------------------------------------
// HDF5 traversal helpers.
class vtkHDF5RawImageReaderDataSet
{
public:
  vtkstd::string name;
  int dims[3];
  int rank;
  int format;
};

class vtkHDF5RawImageReaderDataSetsType: public vtkstd::vector<vtkHDF5RawImageReaderDataSet> {};

extern "C"
int vtkHDF5RawImageReaderVisit(hid_t, const char*, void* opdata);
int vtkHDF5RawImageReaderTraverseGroup(vtkHDF5RawImageReader*,
                                       hid_t loc_id, const char*);

//----------------------------------------------------------------------------
// Functions to convert between HDF5 and VTK dimensions ordering.
void vtkHDF5RawImageReaderHDF5toVTK(int rank, const hsize_t* h5dims, int* dims)
{
  int i=0;
  for(; i < rank; ++i)
    {
    dims[i] = static_cast<int>(h5dims[rank-i-1]);
    }
  for(; i < 3; ++i)
    {
    dims[i] = 0;
    }
}

void vtkHDF5RawImageReaderVTKtoHDF5(int rank, const int* dims, hsize_t* h5dims)
{
  for(int i=0; i < rank; ++i)
    {
    h5dims[rank-i-1] = static_cast<hsize_t>(dims[i]);
    }
}

void vtkHDF5RawImageReaderVTKtoHDF5(int rank, const int* dims, hssize_t* h5dims)
{
  for(int i=0; i < rank; ++i)
    {
    h5dims[rank-i-1] = static_cast<hssize_t>(dims[i]);
    }
}

//----------------------------------------------------------------------------
// Select internal implementations for data types in the HDF5 files.
#if VTK_SIZEOF_SHORT == 2
# define VTK_HDF5_TYPE_INT16 VTK_SHORT
#else
# error "No native integer type is 16 bits."
#endif

#if VTK_SIZEOF_SHORT == 4
# define VTK_HDF5_TYPE_INT32 VTK_SHORT
#elif VTK_SIZEOF_INT == 4
# define VTK_HDF5_TYPE_INT32 VTK_INT
#elif VTK_SIZEOF_LONG == 4
# define VTK_HDF5_TYPE_INT32 VTK_LONG
#else
# error "No native integer type is 32 bits."
#endif

//----------------------------------------------------------------------------
vtkHDF5RawImageReader::vtkHDF5RawImageReader()
{
  // Copied from vtkImageDataReader constructor:
  this->SetOutput(vtkImageData::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty.
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();

  this->FileName = 0;
  
  this->SetToEmptyExtent(this->WholeExtent);
  this->SetToEmptyExtent(this->UpdateExtent);
  
  this->Stride[0] = 1;
  this->Stride[1] = 1;
  this->Stride[2] = 1;
  
  this->Rank = 3;
  
  this->InformationError = 1;
  
  this->AvailableDataSets = new vtkHDF5RawImageReaderDataSetsType;
  
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();
  
  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkHDF5RawImageReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                             this->SelectionObserver);
  this->CellDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                            this->SelectionObserver);
}

//----------------------------------------------------------------------------
vtkHDF5RawImageReader::~vtkHDF5RawImageReader()
{
  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->CellDataArraySelection->Delete();
  this->PointDataArraySelection->Delete();
  delete this->AvailableDataSets;
  this->SetFileName(0);
}

//----------------------------------------------------------------------------
void vtkHDF5RawImageReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: " << (this->FileName?this->FileName:"(none)") << "\n";
  os << indent << "Stride: "
     << this->Stride[0] << " "
     << this->Stride[1] << " "
     << this->Stride[2] << "\n";
  if(this->PointDataArraySelection)
    {
    os << indent << "PointDataArraySelection: " << this->PointDataArraySelection;
    os << "\n";
    }
  else
    {
    os << indent << "PointDataArraySelection: (none)\n";
    }
  if(this->CellDataArraySelection)
    {
    os << indent << "CellDataArraySelection: " << this->CellDataArraySelection;
    os << "\n";
    }
  else
    {
    os << indent << "CellDataArraySelection: (none)\n";
    }
}

//----------------------------------------------------------------------------
void vtkHDF5RawImageReader::SetOutput(vtkImageData *output)
{
  this->Superclass::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
vtkImageData* vtkHDF5RawImageReader::GetOutput()
{
  if(this->NumberOfOutputs < 1)
    {
    return 0;
    }
  return static_cast<vtkImageData*>(this->Outputs[0]);
}

//----------------------------------------------------------------------------
vtkImageData* vtkHDF5RawImageReader::GetOutput(int idx)
{
  return static_cast<vtkImageData*>(this->Superclass::GetOutput(idx));
}

//----------------------------------------------------------------------------
void vtkHDF5RawImageReader::ExecuteInformation()
{
  this->InformationError = 1;
  this->GetOutput()->Initialize();
  if(!this->FileName)
    {
    vtkErrorMacro("No FileName set!");
    return;
    }

  // Make sure the file exists.
  struct stat fs;
  if(stat(this->FileName, &fs) != 0)
    {
    vtkErrorMacro("File \"" << this->FileName << "\" does not exist.");
    return;
    }

  // Open the file and traverse it to get the list of supported data
  // sets.
  this->AvailableDataSets->erase(this->AvailableDataSets->begin(),
                                 this->AvailableDataSets->end());
  hid_t file = H5Fopen (this->FileName, H5F_ACC_RDONLY, H5P_DEFAULT);
  if(file < 0)
    {
    vtkErrorMacro("Could not open file \"" << this->FileName << "\".");
    return;
    }
  vtkHDF5RawImageReaderTraverseGroup(this, file, "/");
  H5Fclose(file);
  
  // Make sure we can read the file.
  if(this->AvailableDataSets->empty())
    {
    this->SetToEmptyExtent(this->WholeExtent);
    this->GetOutput()->SetExtent(this->WholeExtent);
    vtkErrorMacro("File \"" << this->FileName
                  << "\" contains no supported data sets.");
    return;
    }
  
  this->SetDataArraySelections(this->PointDataArraySelection);
  
  double spacing[3] = {this->Stride[0], this->Stride[1], this->Stride[2]};
  this->GetOutput()->SetWholeExtent(this->WholeExtent);
  this->GetOutput()->SetSpacing(spacing);
  this->InformationError = 0;
}

//----------------------------------------------------------------------------
void vtkHDF5RawImageReader::Execute()
{
  // Make sure there were no errors from ExecuteInformation().
  this->GetOutput()->Initialize();
  if(this->InformationError)
    {
    this->GetOutput()->SetExtent(1, 0, 1, 0, 1, 0);
    return;
    }  
  this->GetOutput()->GetUpdateExtent(this->UpdateExtent);
  
  // Re-open the file.
  hid_t file = H5Fopen (this->FileName, H5F_ACC_RDONLY, H5P_DEFAULT);
  if(file < 0)
    {
    vtkErrorMacro("Could not open file \"" << this->FileName << "\".");
    return;
    }
  
  int result = 1;
  hid_t read_memspace = H5S_ALL;
  hid_t read_dataspace = H5S_ALL;
  int space_opened = 0;
  int count[3] =
    {
      this->UpdateExtent[1]-this->UpdateExtent[0]+1,
      this->UpdateExtent[3]-this->UpdateExtent[2]+1,
      this->UpdateExtent[5]-this->UpdateExtent[4]+1
    };
  
  if(!(this->UpdateExtentIsWholeExtent() && (this->Stride[0] == 1) &&
      (this->Stride[1] == 1) && (this->Stride[2] == 1)))
    {
    hssize_t h_start[3]  = {0,0,0};
    hsize_t  h_count[3]  = {0,0,0};
    hsize_t  h_stride[3] = {0,0,0};
    hsize_t  h_total[3]  = {0,0,0};
    
    int start[3] =
      {
        this->UpdateExtent[0]*this->Stride[0],
        this->UpdateExtent[2]*this->Stride[1],
        this->UpdateExtent[4]*this->Stride[2]
      };
    
    // Convert from HDF5 to VTK dimension ordering.
    vtkHDF5RawImageReaderVTKtoHDF5(this->Rank, start, h_start);
    vtkHDF5RawImageReaderVTKtoHDF5(this->Rank, count, h_count);
    vtkHDF5RawImageReaderVTKtoHDF5(this->Rank, this->Stride, h_stride);
    vtkHDF5RawImageReaderVTKtoHDF5(this->Rank, this->Total, h_total);
    
    hid_t dataspace = H5Screate_simple(this->Rank, h_total, 0);
    if(H5Sselect_hyperslab(dataspace, H5S_SELECT_SET,
                              h_start, h_stride, h_count, 0) < 0)
      {
      vtkErrorMacro("Cannot select file hyperslab.");
      result = 0;
      }
    hid_t memspace = H5Screate_simple(this->Rank, h_count, 0);
    hssize_t moffset[3] = {0, 0, 0};
    if(H5Sselect_hyperslab(memspace, H5S_SELECT_SET,
                              moffset, 0, h_count, 0) < 0)
      {
      vtkErrorMacro("Cannot select memory hyperslab.");
      result = 0;
      }
    read_memspace = memspace;
    read_dataspace = dataspace;
    space_opened = 1;
    }
  
  // Read each array.
  for(vtkHDF5RawImageReaderDataSetsType::const_iterator
        dsi = this->AvailableDataSets->begin();
      dsi != this->AvailableDataSets->end(); ++dsi)
    {
    const vtkHDF5RawImageReaderDataSet& ds = *dsi;
    
    // Only read enabled arrays.
    if(!this->PointDataArrayIsEnabled(&ds))
      {
      continue;
      }
    
    const char* name = ds.name.c_str();    
    vtkDataArray* data = 0;
    hid_t datatype = 0;
    switch (ds.format)
      {
      case VTK_DOUBLE:
        data = vtkDoubleArray::New();
        datatype = H5T_NATIVE_DOUBLE;
        break;
      case VTK_FLOAT:
        data = vtkFloatArray::New();
        datatype = H5T_NATIVE_FLOAT;
        break;
      case VTK_INT:
        data = vtkIntArray::New();
        datatype = H5T_NATIVE_INT;
        break;
      case VTK_LONG:
        data = vtkLongArray::New();
        datatype = H5T_NATIVE_LONG;
        break;
      case VTK_SHORT:
        data = vtkShortArray::New();
        datatype = H5T_NATIVE_SHORT;
        break;
      default:
        // This should never happen.  No other formats will have been set.
        result = 0;
        continue;
      }
    
    // Set name from hdf5 file.
    data->SetName(name);
    
    // Allocate memory.
    data->SetNumberOfComponents(1);  
    data->SetNumberOfTuples(count[0] * count[1] * count[2]);
    
    vtkstd::string full = "/";
    full += name;
    hid_t dataset = H5Dopen(file, full.c_str());
    if(dataset >= 0)
      {
      // Read data.
      if(H5Dread(dataset, datatype, read_memspace, read_dataspace,
                    H5P_DEFAULT, data->GetVoidPointer(0)) >= 0)
        {
        this->GetOutput()->GetPointData()->AddArray(data);
        }
      else
        {
        if(dataset < 0)
          {
          vtkErrorMacro("Could not open dataset \"" << name
                        << "\" in file, but it was previously opened "
                        << "successfully.");
          }
        result = 0;
        }
      
      H5Dclose(dataset);
      }
    else
      {
      result = 0;
      }
    data->Delete();
    }
  
  if(space_opened)
    {
    H5Sclose(read_dataspace);
    H5Sclose(read_memspace);
    }
  H5Fclose(file);
  
  if(result)
    {
    // We filled the exact update extent in the output.
    this->GetOutput()->SetExtent(this->UpdateExtent);  
    }
  else
    {
    vtkErrorMacro("Problems when reading file.");
    this->GetOutput()->SetExtent(1, 0, 1, 0, 1, 0);
    }
}

//----------------------------------------------------------------------------
int vtkHDF5RawImageReader::UpdateExtentIsWholeExtent()
{
  return ((this->UpdateExtent[0] == this->WholeExtent[0]) &&
          (this->UpdateExtent[1] == this->WholeExtent[1]) &&
          (this->UpdateExtent[2] == this->WholeExtent[2]) &&
          (this->UpdateExtent[3] == this->WholeExtent[3]) &&
          (this->UpdateExtent[4] == this->WholeExtent[4]) &&
          (this->UpdateExtent[5] == this->WholeExtent[5]));
}

//----------------------------------------------------------------------------
// Small structure to hold user data for the traversal.
struct vtkHDF5RawImageReaderTraverseData
{
  vtkHDF5RawImageReader* self;
  const char* path;
};

//----------------------------------------------------------------------------
int vtkHDF5RawImageReaderTraverseGroup(vtkHDF5RawImageReader* self,
                                       hid_t loc_id, const char* name)
{
  vtkHDF5RawImageReaderTraverseData data = {self, name};
  return H5Giterate(loc_id, const_cast<char*>(name), 0,
                    &vtkHDF5RawImageReaderVisit, &data);
}

//----------------------------------------------------------------------------
int vtkHDF5RawImageReaderVisit(hid_t loc_id, const char* name, void* opdata)
{
  vtkHDF5RawImageReaderTraverseData* data =
    static_cast<vtkHDF5RawImageReaderTraverseData*>(opdata);
  
  // Construct the full name of the HDF5 object.
  vtkstd::string full = data->path;
  if(full != "/")
    {
    full += "/";
    }
  full += name;
  
  // Get the object's information.
  H5G_stat_t statbuf;
  if(H5Gget_objinfo(loc_id, full.c_str(), 0, &statbuf) < 0)
    {
    // Error getting information about this object.  Do not traverse
    // this sub-tree.
    return 0;
    }
  
  // Handle each object type.
  if(statbuf.type == H5G_GROUP)
    {
    // Found a nested group.  Recursively handle it.
#ifdef VTK_HDF5_DEBUG
    cout << "Found group: " << full.c_str() << endl;
#endif
    vtkHDF5RawImageReaderTraverseGroup(data->self, loc_id, full.c_str());
    }
  else if(statbuf.type == H5G_DATASET)
    {
    // Found a data set.  See if it is supported.
    vtkHDF5RawImageReaderDataSet ds;
    ds.name = full.substr(1);
    hid_t dataset = H5Dopen(loc_id, full.c_str());
    if(dataset < 0)
      {
      return 0;
      }
    hid_t datatype = H5Dget_type(dataset);
    if(datatype < 0)
      {
      H5Dclose(dataset);
      return 0;
      }
    int precision = H5Tget_precision(datatype);
    H5T_class_t dclass = H5Tget_class(datatype);
    if(dclass == H5T_FLOAT && precision == 32)
      {
      ds.format = VTK_FLOAT;
      }
    else if(dclass == H5T_FLOAT && precision == 64)
      {
      ds.format = VTK_DOUBLE;
      }
    else if(dclass == H5T_INTEGER && precision == 16)
      {
      ds.format = VTK_HDF5_TYPE_INT16;
      }
    else if(dclass == H5T_INTEGER && precision == 32)
      {
      ds.format = VTK_HDF5_TYPE_INT32;
      }
    else
      {
      H5Tclose(datatype);
      H5Dclose(dataset);
      return 0;
      }
    hid_t dataspace = H5Dget_space(dataset);
    hsize_t dims[3] = {0,0,0};
    ds.rank = H5Sget_simple_extent_ndims(dataspace);
    if((ds.rank < 1) || (ds.rank > 3))
      {
      H5Sclose(dataspace);
      H5Tclose(datatype);
      H5Dclose(dataset);
      return 0;
      }
    if(H5Sget_simple_extent_dims(dataspace, dims, 0) < 0)
      {
      H5Sclose(dataspace);
      H5Tclose(datatype);
      H5Dclose(dataset);
      return 0;
      }
    H5Sclose(dataspace);
    H5Tclose(datatype);
    H5Dclose(dataset);
    
    // Convert from HDF5 to VTK dimension ordering.
    vtkHDF5RawImageReaderHDF5toVTK(ds.rank, dims, ds.dims);
    
    // Found a dataset that we support.  Tell the
    // vtkHDF5RawImageReader instance about it.
    vtkHDF5RawImageReaderAddDataSet(data->self, &ds);
#ifdef VTK_HDF5_DEBUG
    cout << "Found supported dataset: " << full.c_str() << endl;
#endif
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkHDF5RawImageReader::AddDataSet(vtkHDF5RawImageReaderDataSet* ds)
{
  if(this->AvailableDataSets->empty())
    {
    this->AvailableDataSets->push_back(*ds);
    this->Rank = ds->rank;
    this->Total[0] = ds->dims[0];
    this->Total[1] = ds->dims[1];
    this->Total[2] = ds->dims[2];
    this->ConvertDimsToExtent(this->Rank, this->Total, this->WholeExtent);
    }
  else if((ds->rank == this->Rank) && (ds->dims[0] == this->Total[0]) &&
          (ds->dims[1] == this->Total[1]) && (ds->dims[2] == this->Total[2]))
    {
    this->AvailableDataSets->push_back(*ds);
    }
}

//----------------------------------------------------------------------------
void vtkHDF5RawImageReaderAddDataSet(vtkHDF5RawImageReader* reader,
                                     vtkHDF5RawImageReaderDataSet* ds)
{
  reader->AddDataSet(ds);
}

//----------------------------------------------------------------------------
void vtkHDF5RawImageReader::SetToEmptyExtent(int* extent)
{
  extent[0] = 0;
  extent[1] = -1;
  extent[2] = 0;
  extent[3] = -1;
  extent[4] = 0;
  extent[5] = -1;  
}

//----------------------------------------------------------------------------
void vtkHDF5RawImageReader::ConvertDimsToExtent(int rank, const int* dims,
                                                int* extent)
{
  this->SetToEmptyExtent(extent);
  for(int i=0;i < rank; ++i)
    {
    int dim = dims[i];
    int stride = this->Stride[i];
    extent[2*i+1] = (dim+stride-1)/stride - 1;
    }
}

//----------------------------------------------------------------------------
char** vtkHDF5RawImageReader::CreateStringArray(int numStrings)
{
  char** strings = new char*[numStrings];
  int i;
  for(i=0; i < numStrings; ++i)
    {
    strings[i] = 0;
    }
  return strings;
}

//----------------------------------------------------------------------------
void vtkHDF5RawImageReader::DestroyStringArray(int numStrings, char** strings)
{
  int i;
  for(i=0; i < numStrings; ++i)
    {
    if(strings[i])
      {
      delete [] strings[i];
      }
    }
  delete strings;
}

//----------------------------------------------------------------------------
int vtkHDF5RawImageReader::PointDataArrayIsEnabled(
  const vtkHDF5RawImageReaderDataSet* ds)
{
  const char* name = ds->name.c_str();
  return (name && this->PointDataArraySelection->ArrayIsEnabled(name));
}

//----------------------------------------------------------------------------
int vtkHDF5RawImageReader::CellDataArrayIsEnabled(
  const vtkHDF5RawImageReaderDataSet* ds)
{
  const char* name = ds->name.c_str();
  return (name && this->CellDataArraySelection->ArrayIsEnabled(name));
}

//----------------------------------------------------------------------------
void vtkHDF5RawImageReader::SetDataArraySelections(vtkDataArraySelection* sel)
{
  int numArrays = this->AvailableDataSets->size();
  if(!numArrays)
    {
    sel->SetArrays(0, 0);
    return;
    }
  
  char** names = this->CreateStringArray(numArrays);
  int i;
  for(i=0;i < numArrays;++i)
    {
    const char* name = (*this->AvailableDataSets)[i].name.c_str();
    names[i] = new char[ strlen(name)+1 ];
    strcpy(names[i], name);
    }
  sel->SetArrays(names, numArrays);
  this->DestroyStringArray(numArrays, names);
}

//----------------------------------------------------------------------------
void
vtkHDF5RawImageReader::SelectionModifiedCallback(vtkObject*, unsigned long,
                                                 void* clientdata, void*)
{
  static_cast<vtkHDF5RawImageReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
int vtkHDF5RawImageReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkHDF5RawImageReader::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkHDF5RawImageReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkHDF5RawImageReader::SetPointArrayStatus(const char* name, int status)
{
  if(status)
    {
    this->PointDataArraySelection->EnableArray(name);
    }
  else
    {
    this->PointDataArraySelection->DisableArray(name);
    }
}

//----------------------------------------------------------------------------
int vtkHDF5RawImageReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkHDF5RawImageReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkHDF5RawImageReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkHDF5RawImageReader::SetCellArrayStatus(const char* name, int status)
{
  if(status)
    {
    this->CellDataArraySelection->EnableArray(name);
    }
  else
    {
    this->CellDataArraySelection->DisableArray(name);
    }
}

//----------------------------------------------------------------------------
int vtkHDF5RawImageReader::CanReadFile(const char* name)
{
H5E_BEGIN_TRY {
  hid_t file = H5Fopen (name, H5F_ACC_RDONLY, H5P_DEFAULT);
  if(file >= 0)
    {
    H5Fclose(file);
    return 1;
    }
} H5E_END_TRY;
  return 0;
}
