/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChomboReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkChomboReader.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"

vtkCxxRevisionMacro(vtkChomboReader, "1.3");
vtkStandardNewMacro(vtkChomboReader);

//----------------------------------------------------------------------------
vtkChomboReader::vtkChomboReader()
{
  this->FileName = 0;

  this->vtkSource::SetNthOutput(0, vtkHierarchicalBoxDataSet::New());
  this->Outputs[0]->Delete();

  this->Dimensionality = 0;
  this->NumberOfLevels = 0;
  this->NumberOfComponents = 0;

  this->BoxDataType = -1;
}

//----------------------------------------------------------------------------
vtkChomboReader::~vtkChomboReader()
{
  this->SetFileName(0);
  if ( this->BoxDataType >= 0 )
    {
    H5Tclose( this->BoxDataType );
    }
}

//----------------------------------------------------------------------------
void vtkChomboReader::CreateBoxDataType()
{
  if ( this->BoxDataType >= 0)
    {
    int size = H5Tget_size(this->BoxDataType);
    int dimensionality = size/2/sizeof(int);
    if (dimensionality == this->Dimensionality)
      {
      return;
      }
    H5Tclose( this->BoxDataType );
    }

  hid_t boxType = H5Tcreate(H5T_COMPOUND, 2*this->Dimensionality*sizeof(int));
  char loCornerComponentNames[3][5] = {"lo_i", "lo_j", "lo_k"};
  char hiCornerComponentNames[3][5] = {"hi_i", "hi_j", "hi_k"};

  int i;
  for( i=0; i<this->Dimensionality; i++ )
    {
    H5Tinsert( boxType, 
               loCornerComponentNames[i],
               i*sizeof(int),
               H5T_NATIVE_INT );
    }
  for( i=0; i<this->Dimensionality; i++ )
    {
    H5Tinsert( boxType, 
               hiCornerComponentNames[i],
               (this->Dimensionality+i) * sizeof(int),
               H5T_NATIVE_INT );
    }
  this->BoxDataType = boxType;

}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* vtkChomboReader::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkHierarchicalBoxDataSet*>(this->Outputs[0]);
  
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* vtkChomboReader::GetOutput(int idx)
{
  return static_cast<vtkHierarchicalBoxDataSet*>(this->vtkSource::GetOutput(idx)); 
}

//----------------------------------------------------------------------------
int vtkChomboReader::GetStringAttribute( hid_t locID, 
                                         const char* attrName,
                                         vtkstd::string& val)
{
  hid_t attribute = H5Aopen_name( locID, attrName );
  if ( attribute < 0 )
    {
    vtkErrorMacro( << "Failed to open attribute " << attrName);
    return 0;
    }

  hid_t atype  = H5Aget_type(attribute);
  size_t size = H5Tget_size(atype);
  char* str = new char[size+1];
  if ( H5Aread(attribute, atype, str ) != 0)
    {
    vtkErrorMacro( << "Failed to read attribute " << attrName );
    H5Aclose( attribute );
    delete [] str;
    str = 0;
    return 0;
    }
  str[size] = 0;
  val = str;
  delete[] str;
  return 1;
}

//----------------------------------------------------------------------------
int vtkChomboReader::GetRealTypeAttribute( hid_t locID, 
                                           const char* attrName, 
                                           double& val )
{
  hid_t attribute = H5Aopen_name( locID, attrName );
  if ( attribute < 0 )
    {
    vtkErrorMacro( << "Failed to open attribute " << attrName );
    return 0;
    }

  if ( this->RealType == vtkChomboReader::FLOAT )
    {
    float tmp;
    if( H5Aread( attribute, H5T_NATIVE_FLOAT, &tmp ) != 0 )
      {
      vtkErrorMacro( << "Failed to read attribute " << attrName );
      H5Aclose( attribute );
      return 0;
      }
    val = tmp;
    }
  else
    {
    double tmp;
    if( H5Aread( attribute, H5T_NATIVE_DOUBLE, &tmp ) != 0 )
      {
      vtkErrorMacro( << "Failed to read attribute " << attrName );
      H5Aclose( attribute );
      return 0;
      }
    val = tmp;
    }
  H5Aclose( attribute );
  return 1;
}

//----------------------------------------------------------------------------
int vtkChomboReader::GetIntAttribute( hid_t locID, const char* attrName, int& val )
{
  hid_t attribute = H5Aopen_name( locID, attrName );
  if ( attribute < 0 )
    {
    vtkErrorMacro( << "Failed to open attribute " << attrName);
    H5Aclose( attribute );
    return 0;
    }

  if( H5Aread( attribute, H5T_NATIVE_INT, &val ) != 0 )
    {
    vtkErrorMacro( << "Failed to read attribute " << attrName );
    H5Aclose( attribute );
    return 0;
    }
  H5Aclose( attribute );
  return 1;
}

//----------------------------------------------------------------------------
int vtkChomboReader::ReadBoxes( hid_t locID , vtkstd::vector<vtkAMRBox>& boxes )
{
  hid_t boxdataset, boxdataspace, memdataspace;
  hsize_t dims[1], maxdims[1];

  boxdataset = H5Dopen( locID, "boxes" );
  if( boxdataset < 0 )
    {
    vtkErrorMacro( << "Failed to open dataset boxes"  );
    return 0;
    }
  boxdataspace = H5Dget_space( boxdataset );
  if( boxdataspace < 0 )
    {
    vtkErrorMacro( << "Failed to open dataspace for dataset boxes" );
    H5Dclose( boxdataset );
    return 0;
    }

  H5Sget_simple_extent_dims( boxdataspace, dims, maxdims );
  memdataspace = H5Screate_simple( 1, dims, 0 );

  int numBoxes = dims[0];

  boxes.resize(numBoxes);
  vtkstd::vector< int > buf( numBoxes*2*this->Dimensionality );
  this->CreateBoxDataType();
  if ( H5Dread(boxdataset, this->BoxDataType, memdataspace,
               boxdataspace, H5P_DEFAULT, &buf[0] ) < 0 )
    {
    vtkErrorMacro( << "Could not read box information." );
    H5Dclose( boxdataset );
    H5Sclose( boxdataspace );
    H5Sclose( memdataspace );
    return 0;
    }
    

  for(int i=0; i<numBoxes; i++)
    {
    int idx = i*2*this->Dimensionality;
    boxes[i] = vtkAMRBox(this->Dimensionality, 
                         &buf[idx], 
                         &buf[idx+this->Dimensionality]);
    }

  H5Dclose( boxdataset );
  H5Sclose( boxdataspace );
  H5Sclose( memdataspace );

  return 1;
}

//----------------------------------------------------------------------------
void vtkChomboReader::ExecuteInformation()
{
  this->Boxes.clear();
  this->Offsets.clear();
  this->DXs.clear();

  hid_t fileHID = H5Fopen( this->FileName, H5F_ACC_RDONLY, H5P_DEFAULT);
  if( fileHID < 0 )
    {
    vtkErrorMacro( << "Failed to open file " << this->FileName );
    return;
    }

  hid_t globalGroupId = H5Gopen( fileHID, "Chombo_global" );
  if( globalGroupId < 0 )
    {
    vtkErrorMacro( << "Failed to open group Chombo_global");
    H5Fclose( fileHID );
    return;
    }

  if (!this->GetIntAttribute( globalGroupId, "SpaceDim", this->Dimensionality))
    {
    return;
    }

  hid_t attribute = H5Aopen_name( globalGroupId, "testReal" );
  if ( attribute < 0 )
    {
    vtkErrorMacro( << "Failed to open attribute testReal");
    H5Gclose( globalGroupId );
    H5Fclose( fileHID );
    return;
    }

  hid_t datatype = H5Aget_type( attribute );
  if( H5Tget_precision( datatype ) == H5Tget_precision( H5T_NATIVE_FLOAT ) )
    {
    this->RealType = vtkChomboReader::FLOAT;
    } 
  else if( H5Tget_precision( datatype ) == H5Tget_precision( H5T_NATIVE_DOUBLE ) )
    {
    this->RealType = vtkChomboReader::DOUBLE;
    } 
  else
    {
    vtkErrorMacro( << "Illegal datatype. Only float and double are supported." );
    H5Aclose( attribute );
    H5Gclose( globalGroupId );
    H5Fclose( fileHID );
    return;
    }

  H5Aclose( attribute );

  H5Gclose( globalGroupId );

  hid_t rootGroupId = H5Gopen( fileHID, "/" );
  if( rootGroupId < 0 )
    {
    vtkErrorMacro( << "Failed to open group /");
    H5Fclose( fileHID );
    return;
    }
  
  if (!this->GetIntAttribute( rootGroupId, "num_components", this->NumberOfComponents))
    {
    H5Gclose( rootGroupId );
    H5Fclose( fileHID );
    return;
    }

  this->ComponentNames.resize(this->NumberOfComponents);
  for(int i=0; i<this->NumberOfComponents; i++)
    {
    ostrstream attrName;
    attrName << "component_" << i << ends;
    if (!this->GetStringAttribute( rootGroupId, attrName.str(), this->ComponentNames[i] ) )
      {
      delete[] attrName.str();
      H5Gclose( rootGroupId );
      H5Fclose( fileHID );
      return;
      }
    delete[] attrName.str();
    }

  int numLevels;

  if (!this->GetIntAttribute( rootGroupId, "num_levels", numLevels))
    {
    H5Gclose( rootGroupId );
    H5Fclose( fileHID );
    return;
    }
  this->NumberOfLevels = numLevels;

  H5Gclose( rootGroupId );

  if ( numLevels > 0 )
    {
    typedef vtkstd::vector<vtkAMRBox> LevelBoxesType;
    typedef vtkstd::vector<LevelBoxesType> BoxesType;
    
    this->Boxes.resize(numLevels);
    this->DXs.resize(numLevels);
    this->Offsets.resize(numLevels);
    
    int levelId;
    for (levelId=0; levelId<numLevels; levelId++)
      {
      ostrstream levelName;
      levelName << "/level_" << levelId << ends;
      hid_t levelGroupId = H5Gopen( fileHID, levelName.str() );
      delete[] levelName.str();
      if( levelGroupId < 0 )
        {
        vtkErrorMacro( << "Failed to open group /level_" << levelId);
        H5Fclose( fileHID );
        return;
        }
      
      if (!this->GetRealTypeAttribute(levelGroupId, "dx", this->DXs[levelId]))
        {
        vtkErrorMacro( << "Failed to read dx for level " << levelId );
        H5Gclose(levelGroupId);
        H5Fclose( fileHID );
        return;
        }

      LevelBoxesType& boxes = this->Boxes[levelId];
      if (!this->ReadBoxes(levelGroupId, boxes))
        {
        vtkErrorMacro( << "Failed to read level " << levelId << " boxes");
        H5Gclose(levelGroupId);
        H5Fclose( fileHID );
        return;
        }
      
      int numBoxes = boxes.size();
      if ( numBoxes > 0 )
        {
        LevelOffsetsType& offsets = this->Offsets[levelId];
        offsets.resize(numBoxes);
        offsets[0] = 0;
        for(int boxId=1; boxId<numBoxes; boxId++)
          {
          vtkAMRBox& box = boxes[boxId-1];
          offsets[boxId] = offsets[boxId-1] + this->NumberOfComponents*box.GetNumberOfCells();
          }
        }
      H5Gclose(levelGroupId);
      }
    }

  vtkHierarchicalBoxDataSet* output = this->GetOutput();
  output->SetMaximumNumberOfPieces(1);

  H5Fclose( fileHID );
}

//----------------------------------------------------------------------------
void vtkChomboReader::ExecuteData(vtkDataObject* doOutput)
{
  hid_t fileHID = H5Fopen( this->FileName, H5F_ACC_RDONLY, H5P_DEFAULT);
  if( fileHID < 0 )
    {
    vtkErrorMacro( << "Failed to open file " << this->FileName );
    return;
    }

  vtkHierarchicalBoxDataSet* output = 
    vtkHierarchicalBoxDataSet::SafeDownCast(doOutput);

  int outputId=0;
  int numLevels = this->NumberOfLevels;
  output->SetNumberOfLevels(numLevels);
  for (int levelId=0; levelId<numLevels; levelId++)
    {
    double dx = this->DXs[levelId];
    LevelBoxesType& boxes = this->Boxes[levelId];
    LevelOffsetsType& offsets = this->Offsets[levelId];
    int numBoxes = boxes.size();

    output->SetNumberOfDataSets(levelId, numBoxes);

    ostrstream levelName;
    levelName << "/level_" << levelId << ends;
    hid_t levelGroupId = H5Gopen( fileHID, levelName.str() );
    delete[] levelName.str();
    if( levelGroupId < 0 )
      {
      vtkErrorMacro( << "Failed to open group /level_" << levelId);
      H5Fclose( fileHID );
      return;
      }
    
    int ref;
    if (!this->GetIntAttribute(levelGroupId, "ref_ratio", ref))
      {
      vtkErrorMacro( << "Failed to read ref_ratio for level " << levelId );
      H5Gclose(levelGroupId);
      H5Fclose( fileHID );
      return;
      }
    output->SetRefinementRatio(levelId, ref);
      
    hid_t dataset = H5Dopen( levelGroupId, "data:datatype=0" );
    if ( dataset < 0 )
      {
      vtkErrorMacro( << "Failed to open group /level_" << levelId);
      H5Gclose( levelGroupId );
      H5Fclose( fileHID );
      return;
      }

    hid_t fileSpace = H5Dget_space( dataset );

    for (int boxId=0; boxId<numBoxes; boxId++)
      {
      vtkAMRBox& box = boxes[boxId];

      int numPts[3];
      numPts[2] = 1;
      for(int i=0; i<this->Dimensionality; i++)
        {
        numPts[i] = box.HiCorner[i] - box.LoCorner[i] + 2;
        }
      vtkUniformGrid* ug = vtkUniformGrid::New();;
      ug->SetSpacing(dx, dx, dx);
      ug->SetWholeExtent(0, numPts[0]-1, 0, numPts[1]-1, 0, numPts[2]-1);
      ug->SetOrigin(box.LoCorner[0]*dx, box.LoCorner[1]*dx, box.LoCorner[2]*dx);
      ug->SetDimensions(numPts[0], numPts[1], numPts[2]);
      
      output->SetDataSet(levelId, boxId, box, ug);
      ug->Delete();

      hsize_t numItems = box.GetNumberOfCells();
      hssize_t offset = offsets[boxId];
      H5Sselect_hyperslab( fileSpace, H5S_SELECT_SET, &offset, 0, &numItems, 0 );

      hid_t memSpace = H5Screate_simple( 1, &numItems, 0 );

      int fixme;
      vtkDataArray* array;
      if ( this->RealType == vtkChomboReader::FLOAT )
        {
        array = vtkFloatArray::New();
        array->SetNumberOfTuples(numItems);
        float* buf = reinterpret_cast<float*>(array->GetVoidPointer(0));
        if (
          H5Dread( dataset, H5T_NATIVE_FLOAT, memSpace, fileSpace, H5P_DEFAULT, buf ) < 0)
          {
          vtkErrorMacro( << "Error reading FArray." );
          array->Delete();
          H5Sclose( fileSpace );
          H5Dclose( dataset );
          H5Sclose( memSpace );
          H5Gclose( levelGroupId );
          H5Fclose( fileHID );
          return;
          }
        }
      else
        {
        array = vtkDoubleArray::New();
        array->SetNumberOfTuples(numItems);
        double* buf = reinterpret_cast<double*>(array->GetVoidPointer(0));
        if (
          H5Dread( dataset, H5T_NATIVE_DOUBLE, memSpace, fileSpace, H5P_DEFAULT, buf ) < 0)
          {
          vtkErrorMacro( << "Error reading FArray." );
          array->Delete();
          H5Sclose( fileSpace );
          H5Dclose( dataset );
          H5Sclose( memSpace );
          H5Gclose( levelGroupId );
          H5Fclose( fileHID );
          return;
          }
        array->SetName(this->ComponentNames[0].c_str());
        ug->GetCellData()->AddArray(array);
        array->Delete();
        }
      H5Sclose( memSpace );

      outputId++;
      }

    H5Sclose( fileSpace );
    H5Dclose( dataset );
    H5Gclose( levelGroupId );
    }
  
  H5Fclose( fileHID );

  output->GenerateVisibilityArrays();
}

//----------------------------------------------------------------------------
void vtkChomboReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: "
     << (this->FileName? this->FileName:"(none)") << "\n";
  os << indent << "Dimensionality: " << this->Dimensionality << endl;
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
  os << indent << "RealType: ";
  if (this->RealType == vtkChomboReader::FLOAT)
    {
    os << " float" << endl;
    }
  else
    {
    os << " double" << endl;
    }
  os << indent << "NumberOfLevels: " << this->NumberOfLevels << endl;
}
