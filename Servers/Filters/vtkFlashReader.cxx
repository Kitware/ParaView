/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFlashReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*****************************************************************************
*
* Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
* Produced at the Lawrence Livermore National Laboratory
* LLNL-CODE-400124
* All rights reserved.
*
* This file was adapted from the VisIt Flash reader (avtFLASHFileFormat). For
* details, see https://visit.llnl.gov/.  The full copyright notice is contained
* in the file COPYRIGHT located at the root of the VisIt distribution or at
* http://www.llnl.gov/visit/copyright.html.
*
*****************************************************************************/
#define H5_USE_16_API

#include "vtkFlashReader.h"

#include "vtkByteSwap.h"
#include "vtkPoints.h"
#include "vtkDataSet.h"
#include "vtkPolyData.h"
#include "vtkCellData.h"
#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkDataSetAttributes.h"

#include "vtkIntArray.h"
#include "vtkCharArray.h"
#include "vtkShortArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkDataArraySelection.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCallbackCommand.h"

#include <hdf5.h>    // for the HDF data loading engine

#include <vtkstd/algorithm> // for 'find()'
#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/string>
#include <vtkstd/vector>

vtkStandardNewMacro( vtkFlashReader );

// ============================================================================

#define  FLASH_READER_MAX_DIMS     3
#define  FLASH_READER_LEAF_BLOCK   1
#define  FLASH_READER_FLASH3_FFV8  8
#define  FLASH_READER_FLASH3_FFV9  9

int      vtkFlashReader::NumberOfInstances = 0;

typedef  struct tagFlashReaderIntegerScalar
{
  char   Name[20];                // name  of the integer scalar
  int    Value;                   // value of the integer scalar
} FlashReaderIntegerScalar;   
 
typedef  struct tagFlashReaderDoubleScalar
{
  char   Name[20];                // name  of the real scalar
  double Value;                   // value of the real scalar
} FlashReaderDoubleScalar;

typedef  struct tagFlashReaderSimulationParameters
{
  int    NumberOfBlocks;          // number of all blocks
  int    NumberOfTimeSteps;       // number of time steps
  int    NumberOfXDivisions;      // number of divisions per block along x axis
  int    NumberOfYDivisions;      // number of divisions per block along y axis
  int    NumberOfZDivisions;      // number of divisions per block along z axis
  double Time;                    // the time of this step
  double TimeStep;                // time interval
  double RedShift;   
} FlashReaderSimulationParameters;

typedef  struct tagBlock
{
  int    Index;                   // Id of the block
  int    Level;                   // LOD level
  int    Type;                    // a leaf block?
  int    ParentId;                // Id  of the parent block
  int    ChildrenIds[8];          // Ids of the children    blocks
  int    NeighborIds[6];          // Ids of the neighboring blocks
  int    ProcessorId;             // Id  of the processor
  int    MinGlobalDivisionIds[3]; // first (global) division index
  int    MaxGlobalDivisionIds[3]; // last  (global) division index
  double Center[3];               // center of the block
  double MinBounds[3];            // lower left  of the bounding box
  double MaxBounds[3];            // upper right of the bounding box
} Block;

typedef  struct tagFlashReaderSimulationInformation
{
  int    FileFormatVersion;
  char   SetupCall[400];
  char   FileCreationTime[80];
  char   FlashVersion[80];
  char   BuildData[80];
  char   BuildDirectory[80];
  char   build_machine[80];
  char   CFlags[400];
  char   FFlags[400];
  char   SetupTimeStamp[80];
  char   BuildTimeStamp[80];
} FlashReaderSimulationInformation;

static vtkstd::string GetSeparatedParticleName( const vtkstd::string & variable )
{
  vtkstd::string sepaName = variable;
  
  if ( sepaName.length() > 9 && sepaName.substr(0,9) == "particle_" )
    {
    sepaName = vtkstd::string( "Particles/" ) + sepaName.substr( 9 );
    }
  else
    {
    sepaName = vtkstd::string( "Particles/" ) + sepaName;
    }
  
  return sepaName;
}


// ----------------------------------------------------------------------------
//                     Class  vtkFlashReaderInternal (begin)                         
// ----------------------------------------------------------------------------


class vtkFlashReaderInternal
{
public:
  vtkFlashReaderInternal()  { this->Init(); }
  ~vtkFlashReaderInternal() { this->Init(); }
  
  int      NumberOfBlocks;            // number of ALL blocks
  int      NumberOfLevels;            // number of levels
  int      FileFormatVersion;         // version of file format
  int      NumberOfParticles;         // number of particles
  int      NumberOfLeafBlocks;        // number of leaf blocks
  int      NumberOfDimensions;        // number of dimensions
  int      NumberOfProcessors;        // number of processors
  int      HaveProcessorsInfo;        // processor Ids available? 
  int      BlockGridDimensions[3];    // number of grid points
  int      BlockCellDimensions[3];    // number of divisions
  int      NumberOfChildrenPerBlock;  // number of children  per block
  int      NumberOfNeighborsPerBlock; // number of neighbors per block
 
  char *   FileName;                  // Flash data file name
  hid_t    FileIndex;                 // file handle
  double   MinBounds[3];              // lower left  of the bounding-box
  double   MaxBounds[3];              // upper right of the bounding box
  FlashReaderSimulationParameters     SimulationParameters;   // CFD simulation
  FlashReaderSimulationInformation    SimulationInformation;  // CFD simulation
  
  // blocks
  vtkstd::vector< Block >             Blocks;
  vtkstd::vector<  int  >             LeafBlocks;
  vtkstd::vector< vtkstd::string >    AttributeNames;
  
  // particles
  vtkstd::string                      ParticleName;
  vtkstd::vector< hid_t >             ParticleAttributeTypes;
  vtkstd::vector< vtkstd::string >    ParticleAttributeNames;
  vtkstd::map< vtkstd::string, int >  ParticleAttributeNamesToIds;
  
  
  int      GetCycle();
  double   GetTime();
  
  void     Init();
  void     SetFileName( char * fileName ) { this->FileName = fileName; }
  
  void     ReadMetaData();
  void     ReadProcessorIds();
  void     ReadDoubleScalars( hid_t fileIndx );
  void     ReadIntegerScalars( hid_t fileIndx );
  void     ReadVersionInformation( hid_t fileIndx );
  void     ReadSimulationParameters
           ( hid_t fileIndx, bool bTmCycle = false ); // time and cycle only
  void     GetBlockMinMaxGlobalDivisionIds();
           
  void     ReadBlockTypes();
  void     ReadBlockBounds();
  void     ReadBlockCenters();
  void     ReadBlockStructures();
  void     ReadRefinementLevels();
  void     ReadDataAttributeNames();
           
  void     ReadParticlesComponent
           ( hid_t dataIndx, const char * compName, double * dataBuff );
  void     ReadParticleAttributes();
  void     ReadParticleAttributesFLASH3();
};

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::Init()
{
  this->FileName  = NULL;
  this->FileIndex = -1;
  this->MinBounds[0] = 
  this->MinBounds[1] = 
  this->MinBounds[2] = VTK_DOUBLE_MAX;
  this->MaxBounds[0] = 
  this->MaxBounds[1] = 
  this->MaxBounds[2] =-VTK_DOUBLE_MAX;
  
  this->NumberOfBlocks = 0;
  this->NumberOfLevels = 0;
  this->FileFormatVersion  =-1;
  this->NumberOfParticles  = 0;
  this->NumberOfLeafBlocks = 0;  
  this->NumberOfDimensions = 0;
  this->NumberOfProcessors = 0;
  this->HaveProcessorsInfo = 0;
  this->BlockGridDimensions[0] = 1;
  this->BlockGridDimensions[1] = 1;
  this->BlockGridDimensions[2] = 1;
  this->BlockCellDimensions[0] = 1;
  this->BlockCellDimensions[1] = 1;
  this->BlockCellDimensions[2] = 1;
  this->NumberOfChildrenPerBlock  = 0;
  this->NumberOfNeighborsPerBlock = 0;
  
  this->Blocks.clear();
  this->LeafBlocks.clear();
  this->AttributeNames.clear();
  
  this->ParticleName = "";
  this->ParticleAttributeTypes.clear();
  this->ParticleAttributeNames.clear();
  this->ParticleAttributeNamesToIds.clear();
}

// ----------------------------------------------------------------------------
int vtkFlashReaderInternal::GetCycle()
{
  const bool bTmCycle = true;
  
  hid_t      fileIndx = H5Fopen( this->FileName, H5F_ACC_RDONLY, H5P_DEFAULT );
  if ( fileIndx < 0 )
    {
    return -VTK_INT_MAX;
    }
    
  this->ReadVersionInformation( fileIndx );
  this->ReadSimulationParameters( fileIndx, bTmCycle );
  H5Fclose( fileIndx );
  
  return this->SimulationParameters.NumberOfTimeSteps;
}

// ----------------------------------------------------------------------------
double vtkFlashReaderInternal::GetTime()
{
    const bool bTmCycle = true;
    
    hid_t      fileIndx = H5Fopen( this->FileName, H5F_ACC_RDONLY, H5P_DEFAULT);
    if ( fileIndx < 0 )
      {
      return -VTK_DOUBLE_MAX;
      }
      
    this->ReadVersionInformation( fileIndx );
    this->ReadSimulationParameters( fileIndx, bTmCycle );
    H5Fclose( fileIndx );
    
    return this->SimulationParameters.Time;
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadMetaData()
{
  if ( this->FileIndex >= 0 )
    {
    return;
    }
  
  // file handle
  this->FileIndex = H5Fopen( this->FileName, H5F_ACC_RDONLY, H5P_DEFAULT );
  if ( this->FileIndex < 0 )
    {
    vtkGenericWarningMacro( "Failed to open file " << this->FileName << 
                            "." << endl );
    return;
    }

  // file format version
  this->ReadVersionInformation( this->FileIndex );
  if ( this->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
    {
    this->ReadParticleAttributes();       // FLASH2 version
    }
  else 
    {
    this->ReadParticleAttributesFLASH3(); // FLASH3 version
    }

  // block structures
  this->ReadBlockStructures();
  if ( this->NumberOfParticles == 0 && this->NumberOfBlocks == 0 )
    {
    vtkGenericWarningMacro( "Invalid Flash file, without any " <<
                            "block/particle." << endl );
    return;
    }

  // obtain further information about blocks
  if ( this->NumberOfBlocks > 0 )
    {
    this->ReadBlockBounds();
    this->ReadRefinementLevels();
    this->ReadSimulationParameters( this->FileIndex );
    this->ReadDataAttributeNames();
    this->GetBlockMinMaxGlobalDivisionIds();
    this->ReadBlockTypes();
    this->ReadBlockCenters();
    this->ReadProcessorIds();
    }
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadProcessorIds()
{
  hid_t rootIndx = H5Gopen( this->FileIndex, "/" );
  if ( rootIndx < 0 )
    {
    vtkGenericWarningMacro( "Failed to open the root group" << endl );
    return;
    }
    
  hsize_t numbObjs;
  herr_t  errorIdx = H5Gget_num_objs( rootIndx, &numbObjs );
  if ( errorIdx < 0 )
    {
    vtkGenericWarningMacro( "Failed to get the number of objects " <<
                            "in the root group" << endl );
    return;
    }
    
  hsize_t        objIndex;
  vtkstd::string sObjName = "processor number";
  char           namefromfile[17];
  for ( objIndex = 0; objIndex < numbObjs; objIndex ++ )
    {
    ssize_t objsize = H5Gget_objname_by_idx( rootIndx, objIndex, NULL, 0 );
    if ( objsize == 16 )
      {
      H5Gget_objname_by_idx( rootIndx, objIndex, namefromfile, 17 );
      vtkstd::string tempstr = namefromfile;
      if ( tempstr == sObjName ) // if this file contains processor numbers
        {
        this->HaveProcessorsInfo = 1;
        }
      }
    }
  H5Gclose( rootIndx );

  if ( this->HaveProcessorsInfo )
    {
    // Read the processor number description for the blocks
    hid_t procnumId = H5Dopen( this->FileIndex, "processor number" );
    if ( procnumId < 0 )
      {
      vtkGenericWarningMacro( "Processor Id information not found." << endl );
      }

    hid_t procnumSpaceId = H5Dget_space( procnumId );
  
    hsize_t procnum_dims[1];
    hsize_t procnum_ndims = H5Sget_simple_extent_dims
                            ( procnumSpaceId, procnum_dims, NULL );

    if (  static_cast<int> ( procnum_ndims   ) != 1 ||
          static_cast<int> ( procnum_dims[0] ) != this->NumberOfBlocks  )
      {
      vtkGenericWarningMacro( "Error with getting the number of " <<
                              "processor Ids." << endl );
      }

    hid_t procnum_raw_data_type = H5Dget_type( procnumId );
    hid_t procnum_data_type = H5Tget_native_type
                              ( procnum_raw_data_type, H5T_DIR_ASCEND );
  
    int * procnum_array = new int [ this->NumberOfBlocks ];
    H5Dread( procnumId, procnum_data_type, H5S_ALL, 
             H5S_ALL, H5P_DEFAULT, procnum_array );

    int highProcessor = -1;
    for (int b = 0; b < this->NumberOfBlocks; b ++ )
      {
      int pnum = procnum_array[b];
      if ( pnum > highProcessor ) 
        {
        highProcessor = pnum;
        this->NumberOfProcessors ++;
        }
      this->Blocks[b].ProcessorId = pnum;
      }

    H5Tclose( procnum_data_type );
    H5Tclose( procnum_raw_data_type );
    H5Sclose( procnumSpaceId );
    H5Dclose( procnumId );

    delete[] procnum_array;
    procnum_array = NULL;
    }
  else
    {
    this->NumberOfProcessors = 1;
    for ( int b = 0; b < this->NumberOfBlocks; b ++ )
      {
      this->Blocks[b].ProcessorId = 0;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadDoubleScalars( hid_t fileIndx )
{
  // Should only be used for FLASH3 files

  if ( this->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
    {
    vtkGenericWarningMacro( "Error with the format version." << endl );
    return;
    }

  hid_t realScalarsId = H5Dopen( fileIndx, "real scalars" );
  //
  // Read the real scalars
  //
  if ( realScalarsId < 0 )
    {
    vtkGenericWarningMacro( "Real scalars not found in FLASH3." << endl );
    return;
    }

  hid_t spaceId = H5Dget_space( realScalarsId );
  if ( spaceId < 0 )
    {
    vtkGenericWarningMacro( "Failed to get the real scalars space." << endl );
    return;
    }

  hsize_t scalarDims[10];
  H5Sget_simple_extent_dims( spaceId, scalarDims, NULL );

  int nScalars = scalarDims[0];

  hid_t datatype = H5Tcreate
                   (  H5T_COMPOUND,  sizeof( FlashReaderDoubleScalar )  );

  hid_t string20 = H5Tcopy( H5T_C_S1 );
  H5Tset_size( string20, 20 );

  H5Tinsert(  datatype, "name",  
              HOFFSET( FlashReaderDoubleScalar, Name  ), string20  );
  H5Tinsert(  datatype, "value", 
              HOFFSET( FlashReaderDoubleScalar, Value ), H5T_NATIVE_DOUBLE  );

  FlashReaderDoubleScalar * rs = new FlashReaderDoubleScalar[ nScalars ];
  H5Dread(realScalarsId, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, rs);

  for ( int i = 0; i < nScalars; i ++ )
    { 
    if (  strncmp( rs[i].Name, "time", 4 ) == 0  )
      {
      this->SimulationParameters.Time = rs[i].Value;
      }
    } 
    
  delete [] rs;
  rs = NULL;
  
  H5Tclose( string20 );
  H5Tclose( datatype );
  H5Sclose( spaceId  );
  H5Dclose( realScalarsId );
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadIntegerScalars( hid_t fileIndx )
{
  // Should only be used for FLASH3 files

  if ( this->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
    {
    vtkGenericWarningMacro( "Error with the format version." << endl );
    return;
    }

  hid_t intScalarsId = H5Dopen( fileIndx, "integer scalars" );
  
  // Read the integer scalars
  if ( intScalarsId < 0 )  
    {
    vtkGenericWarningMacro( "Integer scalars not found in FLASH3." << endl );
    return;
    }

  hid_t spaceId = H5Dget_space( intScalarsId );
  if ( spaceId < 0 )
    {
    vtkGenericWarningMacro( "Failed to get the integer scalars space." << endl );
    return;
    }

  hsize_t  scalarDims[1];
  H5Sget_simple_extent_dims( spaceId, scalarDims, NULL );
  int   nScalars = scalarDims[0];

  hid_t datatype = H5Tcreate
                   (  H5T_COMPOUND,  sizeof( FlashReaderIntegerScalar )  );

  hid_t string20 = H5Tcopy( H5T_C_S1 );
  H5Tset_size( string20, 20 );

  H5Tinsert(  datatype, "name",  
              HOFFSET( FlashReaderIntegerScalar, Name  ), string20       );
  H5Tinsert(  datatype, "value", 
              HOFFSET( FlashReaderIntegerScalar, Value ), H5T_NATIVE_INT );
              
  FlashReaderIntegerScalar * is = new FlashReaderIntegerScalar [ nScalars ];
  H5Dread( intScalarsId, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, is );

  for ( int i = 0; i < nScalars; i ++ )
    { 
    if (  strncmp( is[i].Name, "nxb", 3 ) == 0  )
      {
      this->SimulationParameters.NumberOfXDivisions = is[i].Value;
      }
    else 
    if (  strncmp( is[i].Name, "nyb", 3 ) == 0  )
      {
      this->SimulationParameters.NumberOfYDivisions = is[i].Value;
      }
    else 
    if (  strncmp( is[i].Name, "nzb", 3 ) == 0  )
      {
      this->SimulationParameters.NumberOfZDivisions = is[i].Value;
      }
    else 
    if (  strncmp( is[i].Name, "globalnumblocks", 15 ) == 0  )
      {
      this->SimulationParameters.NumberOfBlocks = is[i].Value;
      }
    else 
    if (  strncmp( is[i].Name, "nstep", 5 ) == 0  )
      {
      this->SimulationParameters.NumberOfTimeSteps = is[i].Value;
      }
    } 
  
  delete [] is;
  is = NULL;
  
  H5Tclose( string20 );
  H5Tclose( datatype );
  H5Sclose( spaceId  );
  H5Dclose( intScalarsId );
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadVersionInformation( hid_t fileIndx )
{
  // temporarily disable error reporting
  H5E_auto_t   old_errorfunc;
  void       * old_clientdata = NULL;
  H5Eget_auto( &old_errorfunc, &old_clientdata );
  H5Eset_auto( NULL, NULL );

  // If this is a FLASH3 Particles file, or a FLASH3 file with particles, 
  // then it will have the "particle names" field.  If, in addition, it's a
  // file format version (FFV) 9 file, it can have "file format version" and 
  // "sim info", so further checking is needed.  Further checking is also
  // needed for non-particle files.  So...further checking all around.

  int flash3_particles = 0;   //  Init to false
  hid_t h5_PN = H5Dopen( fileIndx, "particle names" );
  if ( h5_PN >= 0 )
    {
    flash3_particles = 1;
    H5Dclose( h5_PN );
    }

  // Read the file format version  (<= 7 means FLASH2)
  hid_t h5_FFV = H5Dopen( fileIndx, "file format version" );

  if ( h5_FFV < 0 )
    {
    hid_t h5_SI = H5Dopen( fileIndx, "sim info" );
    if ( h5_SI < 0 )
      {
      if ( flash3_particles == 1 )
        {
        this->FileFormatVersion = FLASH_READER_FLASH3_FFV8;
        }
      else
        {
        this->FileFormatVersion = 7;
        }
      }
    else
      {
      // Read the "sim info" components
      hid_t si_type = H5Tcreate(  H5T_COMPOUND,  
                                  sizeof( FlashReaderSimulationInformation )  );
      H5Tinsert(  si_type, "file format version", 
                  HOFFSET( FlashReaderSimulationInformation, FileFormatVersion ), 
                  H5T_STD_I32LE  );
      H5Tinsert(  si_type, "setup call",          
                  HOFFSET( FlashReaderSimulationInformation, SetupCall ),          
                  H5T_STRING  );
      H5Tinsert(  si_type, "file creation time",  
                  HOFFSET( FlashReaderSimulationInformation, FileCreationTime ),  
                  H5T_STRING  );
      H5Tinsert(  si_type, "flash version",       
                  HOFFSET( FlashReaderSimulationInformation, FlashVersion ),       
                  H5T_STRING  );
      H5Tinsert(  si_type, "build date",          
                  HOFFSET( FlashReaderSimulationInformation, BuildData ),          
                  H5T_STRING  );
      H5Tinsert(  si_type, "build dir",           
                  HOFFSET( FlashReaderSimulationInformation, BuildDirectory ),           
                  H5T_STRING  );
      H5Tinsert(  si_type, "build machine",       
                  HOFFSET( FlashReaderSimulationInformation, build_machine ),       
                  H5T_STRING  );
      H5Tinsert(  si_type, "cflags",              
                  HOFFSET(FlashReaderSimulationInformation, CFlags ),              
                  H5T_STRING  );
      H5Tinsert(  si_type, "fflags",              
                  HOFFSET( FlashReaderSimulationInformation, FFlags ),              
                  H5T_STRING  );
      H5Tinsert(  si_type, "setup time stamp",    
                  HOFFSET( FlashReaderSimulationInformation, SetupTimeStamp ),    
                  H5T_STRING  );
      H5Tinsert(  si_type, "build time stamp",    
                  HOFFSET( FlashReaderSimulationInformation, BuildTimeStamp ),    
                  H5T_STRING  );
  
      H5Dread( h5_SI,   si_type,     H5S_ALL, 
               H5S_ALL, H5P_DEFAULT, &this->SimulationInformation );
  
      H5Tclose( si_type );
      H5Dclose( h5_SI );

      // FileFormatVersion is readin as little-endian. On BE machines, we need to
      // ensure that it's swapped back to right order.
      // The following will have no effect on LE machines.
      vtkByteSwap::SwapLE(&this->SimulationInformation.FileFormatVersion);
      this->FileFormatVersion = this->SimulationInformation.FileFormatVersion;
      }
      
    // turn back on error reporting
    H5Eset_auto( old_errorfunc, old_clientdata );
    old_clientdata = NULL;
    return;
    }

  if ( flash3_particles == 1 )
    {
    this->FileFormatVersion = FLASH_READER_FLASH3_FFV8;
    }
  else
    {
    // FLASH 2 has file format version available in global attributes.
    H5Dread( h5_FFV,  H5T_NATIVE_INT, H5S_ALL, 
             H5S_ALL, H5P_DEFAULT,    &this->FileFormatVersion );
    }

  H5Dclose( h5_FFV );
  
  // turn back on error reporting
  H5Eset_auto( old_errorfunc, old_clientdata );
  old_clientdata = NULL;
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadSimulationParameters
  ( hid_t fileIndx, bool bTmCycle )
{
  if ( this->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
    {
    // Read the simulation parameters
    hid_t simparamsId = H5Dopen( fileIndx, "simulation parameters" );
    if ( simparamsId < 0 )
      {
      vtkGenericWarningMacro( "Simulation parameters unavailable." << endl );
      }

    hid_t sp_type = H5Tcreate(  H5T_COMPOUND, 
                                sizeof( FlashReaderSimulationParameters )  );
                                
    H5Tinsert(  sp_type, "total blocks",   
                HOFFSET( FlashReaderSimulationParameters, NumberOfBlocks ), 
                H5T_NATIVE_INT  );
    H5Tinsert(  sp_type, "time",           
                HOFFSET( FlashReaderSimulationParameters, Time ),         
                H5T_NATIVE_DOUBLE  );
    H5Tinsert(  sp_type, "timestep",       
                HOFFSET( FlashReaderSimulationParameters, TimeStep ),     
                H5T_NATIVE_DOUBLE  );
    H5Tinsert(  sp_type, "redshift",       
                HOFFSET( FlashReaderSimulationParameters, RedShift ),     
                H5T_NATIVE_DOUBLE  );
    H5Tinsert(  sp_type, "number of steps",
                HOFFSET( FlashReaderSimulationParameters, NumberOfTimeSteps ),       
                H5T_NATIVE_INT  );
    H5Tinsert(  sp_type, "nxb",            
                HOFFSET( FlashReaderSimulationParameters, NumberOfXDivisions ),          
                H5T_NATIVE_INT  );
    H5Tinsert(  sp_type, "nyb",            
                HOFFSET( FlashReaderSimulationParameters, NumberOfYDivisions ),          
                H5T_NATIVE_INT  );
    H5Tinsert(  sp_type, "nzb",            
                HOFFSET( FlashReaderSimulationParameters, NumberOfZDivisions ),          
                H5T_NATIVE_INT  );

    H5Dread( simparamsId, sp_type,     H5S_ALL, 
             H5S_ALL,     H5P_DEFAULT, &this->SimulationParameters );

    H5Tclose( sp_type );
    H5Dclose( simparamsId );
    }
  else
    {
    this->ReadIntegerScalars( fileIndx );
    this->ReadDoubleScalars ( fileIndx );
    }

  if ( bTmCycle )
    {
    return;
    }

  // Sanity check: size of the gid array better match number of blocks
  //               reported in the simulation parameters
  if ( this->SimulationParameters.NumberOfBlocks != this->NumberOfBlocks )
    {
    vtkGenericWarningMacro( "Inconsistency in the number of blocks." << endl );
    return;
    }

  if ( this->SimulationParameters.NumberOfXDivisions == 1 )
    {
    this->BlockGridDimensions[0] = 1;
    this->BlockCellDimensions[0] = 1;
    }
  else
    {
    this->BlockGridDimensions[0] = 
    this->SimulationParameters.NumberOfXDivisions + 1;
    this->BlockCellDimensions[0] = 
    this->SimulationParameters.NumberOfXDivisions;
    }

  if ( this->SimulationParameters.NumberOfYDivisions == 1 )
    {
    this->BlockGridDimensions[1] = 1;
    this->BlockCellDimensions[1] = 1;
    }
  else
    {
    this->BlockGridDimensions[1] = 
    this->SimulationParameters.NumberOfYDivisions + 1;
    this->BlockCellDimensions[1] = 
    this->SimulationParameters.NumberOfYDivisions;
    }

  if ( this->SimulationParameters.NumberOfZDivisions == 1 )
    {
    this->BlockGridDimensions[2] = 1;
    this->BlockCellDimensions[2] = 1;
    }
  else
    {
    this->BlockGridDimensions[2] = 
    this->SimulationParameters.NumberOfZDivisions + 1;
    this->BlockCellDimensions[2] = 
    this->SimulationParameters.NumberOfZDivisions;
    }
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::GetBlockMinMaxGlobalDivisionIds()
{
  double problemsize[3] = { this->MaxBounds[0] - this->MinBounds[0],
                            this->MaxBounds[1] - this->MinBounds[1],
                            this->MaxBounds[2] - this->MinBounds[2] };
                            
  for ( int b = 0; b < this->NumberOfBlocks; b ++ )
    {
    Block & B = this->Blocks[b];

    for ( int d = 0; d < 3; d ++ )
      {
      if ( d < this->NumberOfDimensions )
        {
        double factor = problemsize[d] / ( B.MaxBounds[d] - B.MinBounds[d] );
        double start  = ( B.MinBounds[d] - this->MinBounds[d] ) / problemsize[d];

        double beg = this->BlockCellDimensions[d] * start * factor;
        double end = this->BlockCellDimensions[d] * start * factor + 
                     this->BlockCellDimensions[d];
        this->Blocks[b].MinGlobalDivisionIds[d] = int( beg + 0.5 );
        this->Blocks[b].MaxGlobalDivisionIds[d] = int( end + 0.5 );
        }
      else
        {
        this->Blocks[b].MinGlobalDivisionIds[d] = 0;
        this->Blocks[b].MaxGlobalDivisionIds[d] = 0;
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadBlockTypes()
{
  // Read the node type description for the blocks
  hid_t nodetypeId = H5Dopen( this->FileIndex, "node type" );
  if ( nodetypeId < 0 )
    {
    vtkGenericWarningMacro( "Block types not found." << endl );
    return;
    }

  hid_t nodetypeSpaceId = H5Dget_space( nodetypeId );
  
  hsize_t nodetype_dims[1];
  hsize_t nodetype_ndims = H5Sget_simple_extent_dims
                           ( nodetypeSpaceId, nodetype_dims, NULL );

  if (  static_cast<int> ( nodetype_ndims   ) != 1 ||
        static_cast<int> ( nodetype_dims[0] ) != this->NumberOfBlocks  )
    {
    vtkGenericWarningMacro( "Inconsistency in the number of blocks." << endl );
    return;
    }

  hid_t nodetype_raw_data_type = H5Dget_type( nodetypeId );
  hid_t nodetype_data_type = H5Tget_native_type
                             ( nodetype_raw_data_type, H5T_DIR_ASCEND );
  
  int * nodetype_array = new int [ this->NumberOfBlocks ];
  H5Dread( nodetypeId, nodetype_data_type, H5S_ALL, 
           H5S_ALL,    H5P_DEFAULT,        nodetype_array );

  this->NumberOfLeafBlocks = 0;
  for ( int b = 0; b < this->NumberOfBlocks; b ++ )
    {
    int ntype = nodetype_array[b];
    this->Blocks[b].Type = ntype;
    if ( ntype == FLASH_READER_LEAF_BLOCK )
      {
      this->NumberOfLeafBlocks ++;
      this->LeafBlocks.push_back( b );
      }
    }
    
  delete [] nodetype_array;
  nodetype_array = NULL;

  H5Tclose( nodetype_data_type );
  H5Tclose( nodetype_raw_data_type );
  H5Sclose( nodetypeSpaceId );
  H5Dclose( nodetypeId );
}

// ----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadBlockBounds()
{
  // Read the bounding box description for the blocks
  hid_t bboxId = H5Dopen( this->FileIndex, "bounding box" );
  if ( bboxId < 0 )
    {
    vtkGenericWarningMacro( "Blocks bounding info not found." << endl );
    return;
    }

  hid_t  bboxSpaceId = H5Dget_space( bboxId );
  hsize_t bbox_dims[3];
  hsize_t bbox_ndims = H5Sget_simple_extent_dims
                       ( bboxSpaceId, bbox_dims, NULL );

  if ( this->FileFormatVersion <= FLASH_READER_FLASH3_FFV8 )
    {
    if (  static_cast<int> ( bbox_ndims   ) != 3 ||
          static_cast<int> ( bbox_dims[0] ) != this->NumberOfBlocks ||
          static_cast<int> ( bbox_dims[1] ) != this->NumberOfDimensions ||
          static_cast<int> ( bbox_dims[2] ) != 2  )
      {
      vtkGenericWarningMacro( "Error with number of blocks " <<
                              "or number of dimensions." << endl );
      return;
      }

    double * bbox_array = new double [ this->NumberOfBlocks * 
                                       this->NumberOfDimensions * 2 ];
    H5Dread( bboxId,  H5T_NATIVE_DOUBLE, H5S_ALL, 
             H5S_ALL, H5P_DEFAULT,       bbox_array );
    
    this->MinBounds[0] = VTK_DOUBLE_MAX;
    this->MinBounds[1] = VTK_DOUBLE_MAX;
    this->MinBounds[2] = VTK_DOUBLE_MAX;
    this->MaxBounds[0] =-VTK_DOUBLE_MAX;
    this->MaxBounds[1] =-VTK_DOUBLE_MAX;
    this->MaxBounds[2] =-VTK_DOUBLE_MAX;
    
    for (int b=0; b<this->NumberOfBlocks; b++)
      {
      double * bbox_line = &bbox_array[ this->NumberOfDimensions * 2 * b ];
      for ( int d = 0; d < 3; d ++ )
        {
        if ( d + 1 <= this->NumberOfDimensions )
          {
          this->Blocks[b].MinBounds[d] = bbox_line[ d * 2 + 0 ];
          this->Blocks[b].MaxBounds[d] = bbox_line[ d * 2 + 1 ];
          }
        else
          {
          this->Blocks[b].MinBounds[d] = 0;
          this->Blocks[b].MaxBounds[d] = 0;
          }
    
        if ( this->Blocks[b].MinBounds[0] < this->MinBounds[0] )
          {
          this->MinBounds[0] = this->Blocks[b].MinBounds[0];
          }
          
        if ( this->Blocks[b].MinBounds[1] < this->MinBounds[1] )
          {
          this->MinBounds[1] = this->Blocks[b].MinBounds[1];
          }
          
        if ( this->Blocks[b].MinBounds[2] < this->MinBounds[2] )
          {
          this->MinBounds[2] = this->Blocks[b].MinBounds[2];
          }
    
        if ( this->Blocks[b].MaxBounds[0] > this->MaxBounds[0] )
          {
          this->MaxBounds[0] = this->Blocks[b].MaxBounds[0];
          }
          
        if ( this->Blocks[b].MaxBounds[1] > this->MaxBounds[1] )
          {
          this->MaxBounds[1] = this->Blocks[b].MaxBounds[1];
          }
          
        if ( this->Blocks[b].MaxBounds[2] > this->MaxBounds[2] )
          {
          this->MaxBounds[2] = this->Blocks[b].MaxBounds[2];
          }
        }
        
      bbox_line = NULL;
      }
    
    delete[] bbox_array;
    bbox_array = NULL;
    }
  else 
  if ( this->FileFormatVersion == FLASH_READER_FLASH3_FFV9 )
    {
    if (  static_cast<int> ( bbox_ndims   ) != 3 ||
          static_cast<int> ( bbox_dims[0] ) != this->NumberOfBlocks  ||
          static_cast<int> ( bbox_dims[1] ) != FLASH_READER_MAX_DIMS ||
          static_cast<int> ( bbox_dims[2] ) != 2  )
      {
      vtkGenericWarningMacro( "Error with number of blocks." << endl );
      return;
      }
    
    double * bbox_array = new double [ this->NumberOfBlocks * 
                                       FLASH_READER_MAX_DIMS * 2];
    H5Dread( bboxId,  H5T_NATIVE_DOUBLE, H5S_ALL, 
             H5S_ALL, H5P_DEFAULT,       bbox_array );
    
    this->MinBounds[0] = VTK_DOUBLE_MAX;
    this->MinBounds[1] = VTK_DOUBLE_MAX;
    this->MinBounds[2] = VTK_DOUBLE_MAX;
    this->MaxBounds[0] =-VTK_DOUBLE_MAX;
    this->MaxBounds[1] =-VTK_DOUBLE_MAX;
    this->MaxBounds[2] =-VTK_DOUBLE_MAX;
    
    for ( int b = 0; b < this->NumberOfBlocks; b ++ )
      {
      double * bbox_line = &bbox_array[ FLASH_READER_MAX_DIMS * 2 * b ];
      
      for ( int d = 0; d < 3; d ++ )
        {
        this->Blocks[b].MinBounds[d] = bbox_line[ d * 2 + 0 ];
        this->Blocks[b].MaxBounds[d] = bbox_line[ d * 2 + 1 ];
    
        if ( this->Blocks[b].MinBounds[0] < this->MinBounds[0] )
          {
          this->MinBounds[0] = this->Blocks[b].MinBounds[0];
          }
            
        if ( this->Blocks[b].MinBounds[1] < this->MinBounds[1] )
          {
          this->MinBounds[1] = this->Blocks[b].MinBounds[1];
          }
            
        if ( this->Blocks[b].MinBounds[2] < this->MinBounds[2] )
          {
          this->MinBounds[2] = this->Blocks[b].MinBounds[2];
          }
    
        if ( this->Blocks[b].MaxBounds[0] > this->MaxBounds[0] )
          {
          this->MaxBounds[0] = this->Blocks[b].MaxBounds[0];
          }
            
        if ( this->Blocks[b].MaxBounds[1] > this->MaxBounds[1] )
          {
          this->MaxBounds[1] = this->Blocks[b].MaxBounds[1];
          }
            
        if ( this->Blocks[b].MaxBounds[2] > this->MaxBounds[2] )
          {
          this->MaxBounds[2] = this->Blocks[b].MaxBounds[2];
          }
        }
        
      bbox_line = NULL;
      }    
      
    delete[] bbox_array;
    bbox_array = NULL;
    }

  H5Sclose(bboxSpaceId);
  H5Dclose(bboxId);
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadBlockCenters()
{
  // Read the coordinates description for the blocks
  hid_t coordinatesId = H5Dopen( this->FileIndex, "coordinates" );
  if ( coordinatesId < 0 )
    {
    vtkGenericWarningMacro( "Block centers not found." << endl );
    return;
    }

  hid_t coordinatesSpaceId = H5Dget_space( coordinatesId );

  hsize_t coordinates_dims[2];
  hsize_t coordinates_ndims = H5Sget_simple_extent_dims
                              ( coordinatesSpaceId, coordinates_dims, NULL );

  if ( this->FileFormatVersion <= FLASH_READER_FLASH3_FFV8 )
    {
    if (  static_cast<int> ( coordinates_ndims   ) != 2 ||
          static_cast<int> ( coordinates_dims[0] ) != this->NumberOfBlocks ||
          static_cast<int> ( coordinates_dims[1] ) != this->NumberOfDimensions  )
      {
      vtkGenericWarningMacro( "Error with number of blocks or " << 
                              "number of dimensions." << endl );
      return;
      }

    double * coordinates_array = new double [ this->NumberOfBlocks * 
                                              this->NumberOfDimensions ];
    H5Dread( coordinatesId, H5T_NATIVE_DOUBLE, H5S_ALL, 
             H5S_ALL,       H5P_DEFAULT,       coordinates_array );

    for ( int b = 0; b < this->NumberOfBlocks; b ++ )
      {
      double * coords = &coordinates_array[ this->NumberOfDimensions * b ];
      
      if ( this->NumberOfDimensions == 1 )
        {
        this->Blocks[b].Center[0] = coords[0];
        this->Blocks[b].Center[1] = 0.0;
        this->Blocks[b].Center[2] = 0.0;
        }
      else 
      if ( this->NumberOfDimensions == 2 )
        {
        this->Blocks[b].Center[0] = coords[0];
        this->Blocks[b].Center[1] = coords[1];
        this->Blocks[b].Center[2] = 0.0;
        }
      else 
      if ( this->NumberOfDimensions == 3 )
        {
        this->Blocks[b].Center[0] = coords[0];
        this->Blocks[b].Center[1] = coords[1];
        this->Blocks[b].Center[2] = coords[2];
        }
        
      coords = NULL;
      }
    
    delete [] coordinates_array;
    coordinates_array = NULL;
    }
  else 
  if ( this->FileFormatVersion == FLASH_READER_FLASH3_FFV9 )
    {
    if (  static_cast<int> ( coordinates_ndims   ) != 2 ||
          static_cast<int> ( coordinates_dims[0] ) != this->NumberOfBlocks ||
          static_cast<int> ( coordinates_dims[1] ) != FLASH_READER_MAX_DIMS  )
      {
      vtkGenericWarningMacro( "Error with number of blocks." << endl );
      return;
      }

    double * coordinates_array = new double [ this->NumberOfBlocks * 
                                              FLASH_READER_MAX_DIMS ];
    H5Dread( coordinatesId, H5T_NATIVE_DOUBLE, H5S_ALL, 
             H5S_ALL,       H5P_DEFAULT,       coordinates_array );

    for ( int b = 0; b < this->NumberOfBlocks; b ++ )
      {
      double * coords = &coordinates_array[ FLASH_READER_MAX_DIMS * b ];
      this->Blocks[b].Center[0] = coords[0];
      this->Blocks[b].Center[1] = coords[1];
      this->Blocks[b].Center[2] = coords[2];
      coords = NULL;
      }
    
    delete [] coordinates_array;
    coordinates_array = NULL;
    }

  H5Sclose( coordinatesSpaceId );
  H5Dclose( coordinatesId );
}

// ----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadBlockStructures()
{
  // temporarily disable error reporting
  H5E_auto_t  old_errorfunc;
  void      * old_clientdata = NULL;
  H5Eget_auto( &old_errorfunc, &old_clientdata );
  H5Eset_auto( NULL, NULL );

  // Read the "gid" block connectivity description
  hid_t gidId = H5Dopen( this->FileIndex, "gid" );

  // turn back on error reporting
  H5Eset_auto( old_errorfunc, old_clientdata );

  if ( gidId < 0 )
    {
    this->NumberOfBlocks = 0;
    old_clientdata = NULL;
    return;
    }

  hid_t gidSpaceId = H5Dget_space( gidId );
  
  hsize_t gid_dims[2];
  hsize_t gid_ndims =  H5Sget_simple_extent_dims( gidSpaceId, gid_dims, NULL );
  if ( gid_ndims != 2 )
    {
    vtkGenericWarningMacro( "Error with reading block connectivity." << endl );
    return;
    }

  this->NumberOfBlocks = gid_dims[0];
  switch ( gid_dims[1] )
    {
    case 5:
      this->NumberOfDimensions        = 1;
      this->NumberOfChildrenPerBlock  = 2;
      this->NumberOfNeighborsPerBlock = 2;
      break;
    
    case 9:
      this->NumberOfDimensions        = 2;
      this->NumberOfChildrenPerBlock  = 4;
      this->NumberOfNeighborsPerBlock = 4;
      break;
    
    case 15:
      this->NumberOfDimensions        = 3;
      this->NumberOfChildrenPerBlock  = 8;
      this->NumberOfNeighborsPerBlock = 6;
      break;
    
    default:
      vtkGenericWarningMacro( "Invalid block connectivity." << endl );
      break;
    }

  hid_t gid_raw_data_type = H5Dget_type( gidId );
  hid_t gid_data_type = H5Tget_native_type( gid_raw_data_type, H5T_DIR_ASCEND );
  
  int * gid_array = new int [ this->NumberOfBlocks * gid_dims[1] ];
  H5Dread( gidId,   gid_data_type, H5S_ALL, 
           H5S_ALL, H5P_DEFAULT,   gid_array );

  // convert to an easier-to-grok format
  this->Blocks.resize( this->NumberOfBlocks );
  for ( int b = 0; b < this->NumberOfBlocks; b ++ )
    {
    int * gid_line = &gid_array[ gid_dims[1] * b ];
    int   pos = 0;
    int   n;

    this->Blocks[b].Index = b + 1;  // 1-origin IDs
    
    for ( n = 0; n < 6; n ++ )
      {
      this->Blocks[b].NeighborIds[n] = -32;
      }
    
    for ( n = 0; n < this->NumberOfNeighborsPerBlock; n ++ )
      {
      this->Blocks[b].NeighborIds[n] = gid_line[ pos ++ ];
      }
      
    this->Blocks[b].ParentId = gid_line[ pos ++ ];
    
    for ( n = 0; n < 8; n ++ )
      {
      this->Blocks[b].ChildrenIds[n] = -1;
      }
      
    for ( n = 0; n < this->NumberOfChildrenPerBlock; n ++ )
      {
      this->Blocks[b].ChildrenIds[n] = gid_line[ pos ++ ];
      }
      
    gid_line = NULL;
    }
    
  delete[] gid_array;
  gid_array = NULL;

  H5Tclose( gid_data_type );
  H5Tclose( gid_raw_data_type );
  H5Sclose( gidSpaceId );
  H5Dclose( gidId );
}

// ----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadRefinementLevels()
{
  // Read the bounding box description for the blocks
  hid_t refinementId = H5Dopen( this->FileIndex, "refine level" );
  if ( refinementId < 0 )
    {
    vtkGenericWarningMacro( "Refinement levels not found." << endl );
    return;
    }

  hid_t refinementSpaceId = H5Dget_space( refinementId );
  
  hsize_t refinement_dims[1];
  hsize_t refinement_ndims = H5Sget_simple_extent_dims
                             ( refinementSpaceId, refinement_dims, NULL );

  if (  static_cast<int> ( refinement_ndims   ) != 1 ||
        static_cast<int> ( refinement_dims[0] ) != this->NumberOfBlocks  )
    {
    vtkGenericWarningMacro( "Error with number of blocks" << endl );
    return;
    }

  hid_t refinement_raw_data_type = H5Dget_type( refinementId );
  hid_t refinement_data_type = H5Tget_native_type
                               ( refinement_raw_data_type, H5T_DIR_ASCEND );
  
  int * refinement_array = new int [ this->NumberOfBlocks ];
  H5Dread( refinementId, refinement_data_type, H5S_ALL, 
           H5S_ALL,      H5P_DEFAULT,          refinement_array );

  for ( int b = 0; b < this->NumberOfBlocks; b ++ )
    {
    int level = refinement_array[b];
    this->Blocks[b].Level = level;
    if ( level > this->NumberOfLevels )
      {
      this->NumberOfLevels = level;
      }
    }
    
  delete[] refinement_array;
  refinement_array = NULL;

  H5Tclose( refinement_data_type );
  H5Tclose( refinement_raw_data_type );
  H5Sclose( refinementSpaceId );
  H5Dclose( refinementId );
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadDataAttributeNames()
{
  hid_t unknownsId = H5Dopen( this->FileIndex, "unknown names" );
  if ( unknownsId < 0 )
    {
    vtkGenericWarningMacro( "Data attributes not found." << endl );
    return;
    }

  hid_t unkSpaceId = H5Dget_space( unknownsId );
  
  hsize_t unk_dims[2];
  hsize_t unk_ndims =  H5Sget_simple_extent_dims( unkSpaceId, unk_dims, NULL );
  if ( unk_ndims != 2 || unk_dims[1] != 1 )
    {
    vtkGenericWarningMacro( "Error with reading data attributes." << endl );
    return;
    }

  hid_t unk_raw_data_type = H5Dget_type( unknownsId );
  int length = (int)(H5Tget_size( unk_raw_data_type ));

  int nvars = unk_dims[0];
  char * unk_array = new char [ nvars * length ];

  H5Dread( unknownsId, unk_raw_data_type, H5S_ALL, 
           H5S_ALL,    H5P_DEFAULT,       unk_array );

  this->AttributeNames.resize( nvars );
  char * tmpstring = new char [ length + 1 ];
  for ( int v = 0; v < nvars; v ++ )
    {
    for ( int c = 0; c < length; c ++ )
      {
      tmpstring[c] = unk_array[ v * length + c ];
      }
    tmpstring[ length ] = '\0';

    this->AttributeNames[v] = tmpstring;
    }
    
  delete [] unk_array;
  delete [] tmpstring;
  unk_array = NULL;
  tmpstring = NULL;

  H5Tclose( unk_raw_data_type );
  H5Sclose( unkSpaceId );
  H5Dclose( unknownsId );
}

// ----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadParticlesComponent
  ( hid_t dataIndx, const char * compName, double * dataBuff )
{
  if ( !compName || this->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
    {
    vtkGenericWarningMacro( "Invalid component name of particles or " <<
                            "non FLASH3_FFV8 file format." << endl );
    return;
    }

  hsize_t    spaceIdx = H5Dget_space( dataIndx ); // data space index
  hsize_t    thisSize = this->NumberOfParticles;
  hsize_t    spaceMem = H5Screate_simple( 1, &thisSize, NULL );
  int        attrIndx = this->ParticleAttributeNamesToIds[ compName ];
  
  hsize_t    theShift[2] = { 0, attrIndx };
  hsize_t    numReads[2] = { this->NumberOfParticles, 1 }; 
  H5Sselect_hyperslab ( spaceIdx, H5S_SELECT_SET, theShift, 
                        NULL,     numReads,       NULL );
  H5Dread( dataIndx, H5T_NATIVE_DOUBLE, spaceMem, 
           spaceIdx, H5P_DEFAULT,       dataBuff ); 
 
  H5Sclose( spaceIdx );
  H5Sclose( spaceMem );
}

// ----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadParticleAttributes()
{
  // temporarily disable error reporting
  H5E_auto_t  old_errorfunc;
  void      * old_clientdata = NULL;
  H5Eget_auto( &old_errorfunc, &old_clientdata );
  H5Eset_auto( NULL, NULL );

  // find the particle variable (if it exists)
  hid_t pointId;
  this->ParticleName = "particle tracers";
  pointId = H5Dopen( this->FileIndex, this->ParticleName.c_str() );
  if ( pointId < 0 )
    {
    this->ParticleName = "tracer particles";
    pointId = H5Dopen( this->FileIndex, this->ParticleName.c_str() );
    }

  // turn back on error reporting
  H5Eset_auto( old_errorfunc, old_clientdata );

  if ( pointId < 0 )
    {
    this->NumberOfParticles = 0;
    old_clientdata = NULL;
    return;
    }

  hid_t pointSpaceId = H5Dget_space( pointId );

  hsize_t p_dims[100];
  hsize_t p_ndims =  H5Sget_simple_extent_dims( pointSpaceId, p_dims, NULL );
  if ( p_ndims != 1 )
    {
    vtkGenericWarningMacro( "Error with number of data attributes." << endl );
    }

  this->NumberOfParticles = p_dims[0];

  hid_t point_raw_type = H5Dget_type( pointId );
  int numMembers = H5Tget_nmembers( point_raw_type );
  for ( int i = 0; i < numMembers; i ++ )
    {
    char  * member_name = H5Tget_member_name( point_raw_type, i );
    vtkstd::string nice_name = GetSeparatedParticleName( member_name );
    hid_t  member_raw_type = H5Tget_member_type( point_raw_type, i );
    hid_t  member_type = H5Tget_native_type( member_raw_type, H5T_DIR_ASCEND );
    int    index = (int)(this->ParticleAttributeTypes.size());
    
    if (  strcmp( member_name, "particle_x" )  &&
          strcmp( member_name, "particle_y" )  &&
          strcmp( member_name, "particle_z" )  
       )
      { 
      if (  H5Tequal( member_type, H5T_NATIVE_DOUBLE ) > 0  )
        {
        this->ParticleAttributeTypes.push_back( H5T_NATIVE_DOUBLE );
        this->ParticleAttributeNames.push_back( member_name );
        this->ParticleAttributeNamesToIds[ nice_name ] = index;
        }
      else 
      if (  H5Tequal( member_type, H5T_NATIVE_INT ) > 0  )
        {
        this->ParticleAttributeTypes.push_back( H5T_NATIVE_INT );
        this->ParticleAttributeNames.push_back( member_name );
        this->ParticleAttributeNamesToIds[ nice_name ] = index;
        }
      else
        {
        vtkGenericWarningMacro( "Only DOUBLE and INT supported." << endl );
        }
      }

    // We read the particles before the grids.  Just in case we
    // don't have any grids, take a stab at the problem dimension
    // based purely on the existence of various data members.
    // This will be overwritten by the true grid topological
    // dimension if the grid exists.
    if (  strcmp( member_name, "particle_x" ) == 0 && 
          this->NumberOfDimensions < 1  )
      {
      this->NumberOfDimensions = 1;
      }
      
    if (  strcmp( member_name, "particle_y" ) == 0 && 
          this->NumberOfDimensions < 2  )
      {
      this->NumberOfDimensions = 2;
      }
      
    if (  strcmp( member_name, "particle_z" ) == 0 && 
          this->NumberOfDimensions < 3  )
      {
      this->NumberOfDimensions = 3;
      }

    member_name = NULL;
    
    H5Tclose( member_type );
    H5Tclose( member_raw_type );
    }

  H5Tclose( point_raw_type );
  H5Sclose( pointSpaceId );
  H5Dclose( pointId );
}

// ----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadParticleAttributesFLASH3()
{
  // Should only be used for FLASH3 files
  if ( this->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
    {
    return;
    }

  // temporarily disable error reporting
  H5E_auto_t  old_errorfunc;
  void      * old_clientdata = NULL;
  H5Eget_auto( &old_errorfunc, &old_clientdata );
  H5Eset_auto( NULL, NULL );

  hid_t pnameId = H5Dopen( this->FileIndex, "particle names" );

  // turn back on error reporting
  H5Eset_auto( old_errorfunc, old_clientdata );

  if ( pnameId < 0 )
    {
    this->NumberOfParticles = 0; 
    old_clientdata = NULL;
    return;
    }
  
  hid_t pnamespace = H5Dget_space( pnameId );
  hsize_t dims[10];
  hsize_t ndims =  H5Sget_simple_extent_dims( pnamespace, dims, NULL );

  // particle names ndims should be 2, and if the second dim isn't 1,
  // need to come up with a way to handle it!
  if ( ndims != 2 || dims[1] != 1 ) 
    {
    if ( ndims != 2 )
      {
      vtkGenericWarningMacro( "FLASH3 expecting particle names ndims of 2, got "  
                              << ndims << endl );
      }
    if ( dims[1] != 1 )
      {
      vtkGenericWarningMacro( "FLASH3 expecting particle names dims[1] of 1, got "  
                              << dims[1] << endl );
      }
    }

  int numNames = dims[0];

  // create the right-size string, and a char array to read the data into
  hid_t string24 = H5Tcopy( H5T_C_S1 );
  H5Tset_size( string24, 24 );
  char * cnames = new char [ 24 * numNames ];
  H5Dread( pnameId, string24, H5S_ALL, H5S_ALL, H5P_DEFAULT, cnames );

  // Convert the single string to individual variable names.
  vtkstd::string  snames( cnames );
  delete[] cnames;
  cnames = NULL;
  
  for ( int i = 0; i < numNames; i ++ )
    { 
    vtkstd::string name = snames.substr( i * 24, 24 );
    
    int sp = (int)(name.find_first_of(' '));
    if ( sp < 24 )
      {
      name = name.substr( 0, sp );
      }
    
    if (  name != "particle_x" &&
          name != "particle_y" &&
          name != "particle_z" 
       )  
      {
      vtkstd::string nice_name = GetSeparatedParticleName( name );
      this->ParticleAttributeTypes.push_back( H5T_NATIVE_DOUBLE );
      this->ParticleAttributeNames.push_back( name );
      this->ParticleAttributeNamesToIds[ nice_name ] = i;
      }

    // We read the particles before the grids.  Just in case we
    // don't have any grids, take a stab at the problem dimension
    // based purely on the existence of various data members.
    // This will be overwritten by the true grid topological
    // dimension if the grid exists.
    if ( name == "posx" && this->NumberOfDimensions < 1 )
      {
      this->NumberOfDimensions = 1;
      }
      
    if ( name == "posy" && this->NumberOfDimensions < 2 )
      {
      this->NumberOfDimensions = 2;
      }
      
    if ( name == "posz" && this->NumberOfDimensions < 3 )
      {
      this->NumberOfDimensions = 3;
      }
    } 
    
  H5Tclose( string24 );
  H5Sclose( pnamespace );
  H5Dclose( pnameId );
  
  // Read particle dimensions and particle HDFVarName 

  // temporarily disable error reporting
  H5Eget_auto( &old_errorfunc, &old_clientdata );
  H5Eset_auto( NULL, NULL );

  // find the particle variable (if it exists)
  hid_t pointId;
  this->ParticleName = "particle tracers";
  pointId = H5Dopen( this->FileIndex, this->ParticleName.c_str() );
  if ( pointId < 0 )
    {
    this->ParticleName = "tracer particles";
    pointId = H5Dopen( this->FileIndex, this->ParticleName.c_str() );
    }

  // turn back on error reporting
  H5Eset_auto( old_errorfunc, old_clientdata );

  // Doesn't exist?  No problem -- we just don't have any particles
  if ( pointId < 0 )
    {
    vtkGenericWarningMacro( "FLASH3 no tracer particles" << endl );
    this->NumberOfParticles = 0;
    old_clientdata = NULL;
    return;
    }

  hid_t pointSpaceId = H5Dget_space( pointId );

  hsize_t p_dims[10];
  hsize_t p_ndims =  H5Sget_simple_extent_dims( pointSpaceId, p_dims, NULL );
  if ( p_ndims != 2 )
    {
    vtkGenericWarningMacro( "FLASH3, expecting particle tracer ndims of 2, got"
                            << p_ndims << endl );
    }
  this->NumberOfParticles = p_dims[0];

  H5Sclose( pointSpaceId );
  H5Dclose( pointId );
}


// ----------------------------------------------------------------------------
//                     Class  vtkFlashReaderInternal ( end )                         
// ----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int vtkFlashReader::UpdateMetaData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  //int* timeStepRange = uniReader->GetTimeStepRange();
  //num_time_steps=timeStepRange[1] + 1;
  //this->TimeStepRange[1]=timeStepRange[1];
  
  //if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  //  {
    //vtkInformation* outInfo = outputVector->GetInformationObject(0);
    //double* timeArray = uniReader->GetTimeArray();
    //outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
    //             timeArray,
    //             num_time_steps);
    //double timeRange[2];
    //timeRange[0] = timeArray[0];
    //timeRange[1] = timeArray[num_time_steps-1];
    //outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), 
    //             timeRange, 2);
  //  }

  //if (!this->TimeRequestedFromPipeline)
  //  {
  //  this->CurrentTimeStep=this->TimeStep;
  //  }
  //if(this->CurrentTimeStep<0||this->CurrentTimeStep>=num_time_steps)
  //  {
  //  vtkErrorMacro("TimeStep set to " << this->CurrentTimeStep << " outside of the range 0 - " << (num_time_steps-1) << ". Use 0.");
  //  this->CurrentTimeStep=0;
  //  }

  // Set the reader to read the first time step.
  //uniReader->SetCurrentTimeStep(this->CurrentTimeStep);

  // Fields
  int fieldsCount = this->GetNumberOfCellArrays();
  vtkDebugMacro("Number of fields: " << fieldsCount);
  
  vtkstd::set<vtkstd::string> fileFields;
  
  int field;
  for(field=0; field<fieldsCount; ++field)
    {
    const char*fieldName=this->CellDataArraySelection->GetArrayName(field);
    vtkDebugMacro("Field #" << field << ": " << fieldName);
    fileFields.insert(fieldName);
    }
  // Now remove the existing array that were not found in the file.
  field=0;
  // the trick is that GetNumberOfArrays() may change at each step.
  while(field<this->CellDataArraySelection->GetNumberOfArrays())
    {
    if(fileFields.find(this->CellDataArraySelection->GetArrayName(field))==fileFields.end())
      {
      this->CellDataArraySelection->RemoveArrayByIndex(field);
      }
    else
      {
      ++field;
      }
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkFlashReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
void vtkFlashReader::SetCellArrayStatus(const char* name, int status)
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
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
void vtkFlashReader::SelectionModifiedCallback(vtkObject*, unsigned long,
                                                 void* clientdata, void*)
{
  static_cast<vtkFlashReader*>(clientdata)->Modified();
}


//-----------------------------------------------------------------------------
vtkFlashReader::vtkFlashReader()
{
  this->NumberOfRoots = 1;
  this->MergeXYZComponents = 1;

  this->CellDataArraySelection = vtkDataArraySelection::New();
  // Setup the selection callback to modify this object when an array
  // selection is changed.
  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(
    &vtkFlashReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->CellDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                            this->SelectionObserver);

  this->FileName = NULL;
  this->Internal = new vtkFlashReaderInternal;
  this->MaximumNumberOfBlocks = -1;
  this->LoadParticles   = 1;
  this->LoadMortonCurve = 0;
  this->BlockOutputType = 0;

  
  this->SetNumberOfInputPorts( 0 );

  // do HDF5 library initialization on consturction of first instance
  if ( vtkFlashReader::NumberOfInstances == 0 )
    {
    vtkDebugMacro( "Initializing HDF5 Library ..." << endl );
    H5open();
    H5Eset_auto( NULL, NULL );
    }
  vtkFlashReader::NumberOfInstances ++;
  
  this->Point1[0] = 0.0;
  this->Point1[1] = 0.0;
  this->Point1[2] = 0.0;
}

//-----------------------------------------------------------------------------

vtkFlashReader::~vtkFlashReader()
{
  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->CellDataArraySelection->Delete();

  if ( this->FileName )
    {
    delete [] this->FileName;
    this->FileName = NULL;
    } 
  
  delete this->Internal;
  this->Internal = NULL;

  // handle HDF5 library termination on descrution of last instance
  vtkFlashReader::NumberOfInstances --;
  if ( vtkFlashReader::NumberOfInstances == 0 )
    {
    vtkDebugMacro( "Finalizing HDF5 Library ..." << endl );
    H5close();
    }
}

// ----------------------------------------------------------------------------
void vtkFlashReader::SetFileName( const char * fileName )
{   
  if (    fileName 
       && strcmp( fileName, "" )
       && ( ( this->FileName == NULL ) || strcmp( fileName, this->FileName ) )
     )
    {
    if ( this->FileName )
      {
      delete [] this->FileName;
      this->FileName = NULL;
      this->Internal->SetFileName( NULL );
      }
      
    this->FileName = new char[  strlen( fileName ) + 1  ];
    strcpy( this->FileName, fileName );
    this->FileName[ strlen( fileName ) ] = '\0';
    
    this->Internal->SetFileName( this->FileName );
    
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetNumberOfBlocks()
{
  this->Internal->ReadMetaData();
  return this->Internal->NumberOfBlocks;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetNumberOfLevels()
{
  this->Internal->ReadMetaData();
  return this->Internal->NumberOfLevels;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetFileFormatVersion()
{
  this->Internal->ReadMetaData();
  return this->Internal->FileFormatVersion;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetNumberOfParticles()
{
  this->Internal->ReadMetaData();
  return this->Internal->NumberOfParticles;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetNumberOfLeafBlocks()
{
  this->Internal->ReadMetaData();
  return this->Internal->NumberOfLeafBlocks;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetNumberOfDimensions()
{
  this->Internal->ReadMetaData();
  return this->Internal->NumberOfDimensions;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetNumberOfProcessors()
{
  this->Internal->ReadMetaData();
  return this->Internal->NumberOfProcessors;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::HaveProcessorsInformation()
{
  this->Internal->ReadMetaData();
  return this->Internal->HaveProcessorsInfo;
}

// ----------------------------------------------------------------------------
const int * vtkFlashReader::GetDimensionsPerBlock()
{
  this->Internal->ReadMetaData();
  return this->Internal->BlockGridDimensions;
}

// ----------------------------------------------------------------------------
void vtkFlashReader::GetDimensionsPerBlock( int BlckDims[3] )
{
  this->Internal->ReadMetaData();
  BlckDims[0] = this->Internal->BlockGridDimensions[0];
  BlckDims[1] = this->Internal->BlockGridDimensions[1];
  BlckDims[2] = this->Internal->BlockGridDimensions[2];
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetNumberOfChildrenPerBlock()
{
  this->Internal->ReadMetaData();
  return this->Internal->NumberOfChildrenPerBlock;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetNumberOfNeighborsPerBlock()
{
  this->Internal->ReadMetaData();
  return this->Internal->NumberOfNeighborsPerBlock;
}

// ----------------------------------------------------------------------------
void vtkFlashReader::GetBounds( double dataBBox[6] )
{
  this->Internal->ReadMetaData();
  
  dataBBox[0] = this->Internal->MinBounds[0];
  dataBBox[2] = this->Internal->MinBounds[1];
  dataBBox[4] = this->Internal->MinBounds[2];
  
  dataBBox[1] = this->Internal->MaxBounds[0];
  dataBBox[3] = this->Internal->MaxBounds[1];
  dataBBox[5] = this->Internal->MaxBounds[2];
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetBlockFlashId( int blockIdx )
{
  this->Internal->ReadMetaData();
  
  if ( blockIdx < 0 || blockIdx >= this->Internal->NumberOfBlocks )
    {
    return -1;
    }
    
  return this->Internal->Blocks[ blockIdx ].Index;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetBlockLevel( int blockIdx )
{
  this->Internal->ReadMetaData();
  
  if ( blockIdx < 0 || blockIdx >= this->Internal->NumberOfBlocks )
    {
    return -1;
    }
    
  return this->Internal->Blocks[ blockIdx ].Level;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetBlockType( int blockIdx )
{
  this->Internal->ReadMetaData();
  
  if ( blockIdx < 0 || blockIdx >= this->Internal->NumberOfBlocks )
    {
    return -1;
    }
    
  return this->Internal->Blocks[ blockIdx ].Type;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetBlockParentId( int blockIdx )
{
  this->Internal->ReadMetaData();
  
  if ( blockIdx < 0 || blockIdx >= this->Internal->NumberOfBlocks )
    {
    return -2;
    }
    
  return this->Internal->Blocks[ blockIdx ].ParentId;
}

// ----------------------------------------------------------------------------
const int * vtkFlashReader::GetBlockChildrenIds( int blockIdx )
{
  this->Internal->ReadMetaData();
  
  if ( blockIdx < 0 || blockIdx >= this->Internal->NumberOfBlocks )
    {
    return NULL;
    }
    
  return this->Internal->Blocks[ blockIdx ].ChildrenIds;
}

// ----------------------------------------------------------------------------
void vtkFlashReader::GetBlockChildrenIds( int blockIdx, int childIds[8] )
{
  static int invalids[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
  int *      indxsPtr    = invalids;
  
  this->Internal->ReadMetaData();
 
  if ( blockIdx >= 0 && blockIdx < this->Internal->NumberOfBlocks )
    {
    indxsPtr = this->Internal->Blocks[ blockIdx ].ChildrenIds;
    }
    
  for ( int i = 0; i < 8; i ++ )
    {
    childIds[i] = indxsPtr[i];
    }
    
  indxsPtr = NULL;
}

// ----------------------------------------------------------------------------
const int * vtkFlashReader::GetBlockNeighborIds( int blockIdx )
{
  this->Internal->ReadMetaData();
  
  if ( blockIdx < 0 || blockIdx >= this->Internal->NumberOfBlocks )
    {
    return NULL;
    }
    
  return this->Internal->Blocks[ blockIdx ].NeighborIds;
}

// ----------------------------------------------------------------------------
void vtkFlashReader::GetBlockNeighborIds( int blockIdx, int neighbrs[6] )
{
  static int invalids[6] = { -32, -32, -32, -32, -32, -32 };
  int *      indxsPtr    = invalids;
  
  this->Internal->ReadMetaData();
 
  if ( blockIdx >= 0 && blockIdx < this->Internal->NumberOfBlocks )
    {
    indxsPtr = this->Internal->Blocks[ blockIdx ].NeighborIds;
    }
    
  for ( int i = 0; i < 6; i ++ )
    {
    neighbrs[i] = indxsPtr[i];
    }
    
  indxsPtr = NULL;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetBlockProcessorId( int blockIdx )
{
  this->Internal->ReadMetaData();
  
  if ( blockIdx < 0 || blockIdx >= this->Internal->NumberOfBlocks )
    {
    return -1;
    }
    
  return this->Internal->Blocks[ blockIdx ].ProcessorId;
}

// ----------------------------------------------------------------------------
const double * vtkFlashReader::GetBlockCenter( int blockIdx )
{
  this->Internal->ReadMetaData();
  
  if ( blockIdx < 0 || blockIdx >= this->Internal->NumberOfBlocks )
    {
    return NULL;
    }
    
  return this->Internal->Blocks[ blockIdx ].Center;
}

// ----------------------------------------------------------------------------
void vtkFlashReader::GetBlockCenter( int blockIdx, double blockMid[3] )
{
  static double invalids[3] = { -VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX, 
                                -VTK_DOUBLE_MAX };
  double *      indxsPtr    = invalids;
  
  this->Internal->ReadMetaData();
 
  if ( blockIdx >= 0 && blockIdx < this->Internal->NumberOfBlocks )
    {
    indxsPtr = this->Internal->Blocks[ blockIdx ].Center;
    }
    
  for ( int i = 0; i < 3; i ++ )
    {
    blockMid[i] = indxsPtr[i];
    }
    
  indxsPtr = NULL;
}

// ----------------------------------------------------------------------------
void vtkFlashReader::GetBlockBounds( int blockIdx, double dataBBox[6] )
{
  dataBBox[0] = dataBBox[2] = dataBBox[4] = VTK_DOUBLE_MAX;
  dataBBox[1] = dataBBox[3] = dataBBox[5] =-VTK_DOUBLE_MAX;
  
  this->Internal->ReadMetaData();
  
  if ( blockIdx < 0 || blockIdx >= this->Internal->NumberOfBlocks )
    {
    return;
    }
  
  dataBBox[0] = this->Internal->Blocks[ blockIdx ].MinBounds[0];
  dataBBox[2] = this->Internal->Blocks[ blockIdx ].MinBounds[1];
  dataBBox[4] = this->Internal->Blocks[ blockIdx ].MinBounds[2];
  
  dataBBox[1] = this->Internal->Blocks[ blockIdx ].MaxBounds[0];
  dataBBox[3] = this->Internal->Blocks[ blockIdx ].MaxBounds[1];
  dataBBox[5] = this->Internal->Blocks[ blockIdx ].MaxBounds[2];
}

//-----------------------------------------------------------------------------
int vtkFlashReader::GetLeafBlockId( int leafIndx )
{
  this->Internal->ReadMetaData();
  
  int  blockIdx = -1;
  if ( leafIndx >= 0 && leafIndx < this->Internal->NumberOfLeafBlocks )
    {
    blockIdx = this->Internal->LeafBlocks[ leafIndx ] - 1;
    }
    
  return blockIdx;
}

//-----------------------------------------------------------------------------
int vtkFlashReader::GetLeafBlockIdFlash( int leafIndx )
{
  this->Internal->ReadMetaData();
  
  int  blockIdx = -1;
  if ( leafIndx >= 0 && leafIndx < this->Internal->NumberOfLeafBlocks )
    {
    blockIdx = this->Internal->LeafBlocks[ leafIndx ];
    }
    
  return blockIdx;
}

//-----------------------------------------------------------------------------
int vtkFlashReader::IsLeafBlock( int blockIdx )
{
  this->Internal->ReadMetaData();
  
  if ( blockIdx < 0 || blockIdx >= this->Internal->NumberOfBlocks )
    {
    return 0;
    }
  
  return !(  this->Internal->Blocks[ blockIdx ].Type  -  1  );
}

//-----------------------------------------------------------------------------
int vtkFlashReader::IsIntermediateBlock( int blockIdx )
{
  this->Internal->ReadMetaData();
  
  if ( blockIdx < 0 || blockIdx >= this->Internal->NumberOfBlocks )
    {
    return 0;
    }
  
  return !(  this->Internal->Blocks[ blockIdx ].Type  -  2  );
}

//-----------------------------------------------------------------------------
int vtkFlashReader::GetNumberOfBlockAttributes()
{
  this->Internal->ReadMetaData();
  return static_cast< int > ( this->Internal->AttributeNames.size() );
}

//-----------------------------------------------------------------------------
const char * vtkFlashReader::GetBlockAttributeName( int attrIndx )
{
  this->Internal->ReadMetaData();
  int  numAttrs = static_cast< int > ( this->Internal->AttributeNames.size() );
  
  return ( attrIndx < 0 || attrIndx >= numAttrs ) 
         ? NULL
         : (  this->Internal->AttributeNames[ attrIndx ].c_str()  );
}

//-----------------------------------------------------------------------------
int vtkFlashReader::IsBlockAttribute( const char * attrName )
{
  int  numAttrs = 0;
  int  attrIndx = -1;
  
  if ( attrName )
    {
    this->Internal->ReadMetaData();
    numAttrs = static_cast< int > ( this->Internal->AttributeNames.size() );
    for (  int i = 0;  ( i < numAttrs ) && ( attrIndx == -1 );  i ++  )
      {
      attrIndx = (  this->Internal->AttributeNames[i] == attrName  ) ? i : -1;
      }
    }
    
  return attrIndx;
}

//-----------------------------------------------------------------------------
const char * vtkFlashReader::GetParticleName()
{
  this->Internal->ReadMetaData();
  return ( this->Internal->ParticleName == "" )
         ? NULL : this->Internal->ParticleName.c_str();
}

//-----------------------------------------------------------------------------
int vtkFlashReader::GetNumberOfParticleAttributes()
{
  this->Internal->ReadMetaData();
  return static_cast< int > ( this->Internal->ParticleAttributeNames.size() );
}

//-----------------------------------------------------------------------------
const char * vtkFlashReader::GetParticleAttributeName( int attrIndx )
{
  this->Internal->ReadMetaData();
  int  numAttrs = static_cast< int > 
                  ( this->Internal->ParticleAttributeNames.size() );
 
  if ( attrIndx < 0 || attrIndx >= numAttrs ) 
    {
    return NULL;
    }
  
  vtkstd::map< vtkstd::string, int >::iterator i;
  for (   i  = this->Internal->ParticleAttributeNamesToIds.begin(); 
          i != this->Internal->ParticleAttributeNamesToIds.end() &&
         (  ( *i ).second != attrIndx  );   i ++   );
    
  return ( *i ).first.c_str();
}

//-----------------------------------------------------------------------------
int vtkFlashReader::IsParticleAttribute( const char * attrName )
{
  int  attrIndx = -1;
  if ( attrName )
    {
    this->Internal->ReadMetaData();
    int  numAttrs = static_cast< int > 
                    ( this->Internal->ParticleAttributeNames.size() );
    int  tempIndx = this->Internal->
                    ParticleAttributeNamesToIds[  vtkstd::string( attrName )  ];
    attrIndx = ( tempIndx > 0 && tempIndx < numAttrs ) ? tempIndx : ( -1 );
    }
    
  return attrIndx;
}

//-----------------------------------------------------------------------------
void vtkFlashReader::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  
  os << indent << "FileName: "        << this->FileName        << "\n";
  os << indent << "BlockOutputType: " << this->BlockOutputType << "\n";
  if ( this->CellDataArraySelection )
    {
    os << "CellDataArraySelection:" << endl;
    this->CellDataArraySelection->PrintSelf(os, indent.GetNextIndent());
    }

  os << "MergeXYZComponents: ";
  if(this->MergeXYZComponents)
    {
    os << "true"<<endl;
    }
  else
    {
    os << "false"<<endl;
    }
}

// ----------------------------------------------------------------------------
int vtkFlashReader::FillOutputPortInformation
  (  int vtkNotUsed( port ),  vtkInformation * info  )
{
  info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet" );
  return 1;
}

//-----------------------------------------------------------------------------
// Start with the root node.
// If we are under the limit, then split the leaf with the highest rank.
// The map contains zero based global indexes. Chilren from methods return 1 based.
void vtkFlashReader::GenerateBlockMap()
{
  this->Internal->ReadMetaData();

  // Choose which roots will be loaded on this process.
  int numProcs = 1;
  this->MyProcessId = 0;
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if (controller)
    {
    numProcs = controller->GetNumberOfProcesses();
    this->MyProcessId = controller->GetLocalProcessId();
    }
 
  this->ToGlobalBlockMap.clear();
  this->BlockRank.clear();
  this->BlockProcess.clear();
  // Add the selected roots.
  int numGlobalBlocks = this->Internal->NumberOfBlocks;
  int root = 0;
  for (int i = 0; i < numGlobalBlocks; ++i)
    {
    if (this->GetBlockLevel(i) == 1)
      {
      this->AddBlockToMap(i);
      // Set the process for this root.
      this->BlockProcess.push_back(root * numProcs / this->NumberOfRoots);
      ++root;
      }
    }
  
  while (((int)(this->ToGlobalBlockMap.size())+7) <= this->MaximumNumberOfBlocks ||
    this->MaximumNumberOfBlocks < 0)
    { // Find the block with the highest rank.
    int bestIdx = 0;
    double bestRank = -1.0;
    int numBlocks = (int)(this->BlockRank.size());
    for (int j = 0; j < numBlocks; ++j)
      {
      double rank = this->BlockRank[j];
      if (rank > bestRank)
        {
        bestRank = rank;
        bestIdx = j;
        }
      }
    if (bestRank < 0.0)
      {
      return;
      }
    // Now we have the highest ranked block.  Refine it.
    int blockId = this->ToGlobalBlockMap[bestIdx];
    int process = this->BlockProcess[bestIdx]; // Child inherits process.
    // Remove the parent from the list so it will not be loaded.
    this->ToGlobalBlockMap.erase(this->ToGlobalBlockMap.begin()+bestIdx);
    this->BlockRank.erase(this->BlockRank.begin()+bestIdx);
    this->BlockProcess.erase(this->BlockProcess.begin()+bestIdx);
    // Add the children of the best block.
    int* children = this->Internal->Blocks[blockId].ChildrenIds;
    for (int j = 0; j < 8; ++j)
      {
      this->AddBlockToMap(children[j]-1);
      this->BlockProcess.push_back(process);
      }
    }
}      

void vtkFlashReader::AddBlockToMap(int globalId)
{
  int* children = this->Internal->Blocks[globalId].ChildrenIds;
  double rank = -1.0; // Cannot split leaves.
  if (children[0] >= 0)
    {
    double bounds[6];
    this->GetBlockBounds(globalId, bounds);
    // If the block contains the rank then use a large rank.
    // Only one leaf block should contain a point.
    if ((this->Point1[0] > bounds[0] && this->Point1[0] < bounds[1] &&
         this->Point1[1] > bounds[2] && this->Point1[1] < bounds[3] &&
         this->Point1[2] > bounds[4] && this->Point1[2] < bounds[5]) ||
        (this->Point2[0] > bounds[0] && this->Point2[0] < bounds[1] &&
         this->Point2[1] > bounds[2] && this->Point2[1] < bounds[3] &&
         this->Point2[2] > bounds[4] && this->Point2[2] < bounds[5]))
      {
      rank = VTK_LARGE_FLOAT;
      }
    else
      { // Compute inverse of minimum distance.
      double x = 0.0;
      if (this->Point1[0] < bounds[0]) {x = bounds[0]-this->Point1[0];}
      else if (this->Point1[0] > bounds[1]) {x = this->Point1[0]-bounds[1];}
      double y = 0.0;
      if (this->Point1[1] < bounds[2]) {y = bounds[2]-this->Point1[1];}
      else if (this->Point1[1] > bounds[3]) {y = this->Point1[1]-bounds[3];}
      double z = 0.0;
      if (this->Point1[2] < bounds[4]) {z = bounds[4]-this->Point1[2];}
      else if (this->Point1[2] > bounds[5]) {z = this->Point1[2]-bounds[5];}
      double dist = sqrt(x*x + y*y + z*z);
      rank = (dist == 0) ? VTK_LARGE_FLOAT:(1.0/dist);

      x = 0.0;  y = 0.0; z = 0.0;
      if (this->Point2[0] < bounds[0]) {x = bounds[0]-this->Point2[0];}
      else if (this->Point2[0] > bounds[1]) {x = this->Point2[0]-bounds[1];}
      if (this->Point2[1] < bounds[2]) {y = bounds[2]-this->Point2[1];}
      else if (this->Point2[1] > bounds[3]) {y = this->Point2[1]-bounds[3];}
      if (this->Point2[2] < bounds[4]) {z = bounds[4]-this->Point2[2];}
      else if (this->Point2[2] > bounds[5]) {z = this->Point2[2]-bounds[5];}
      dist = sqrt(x*x + y*y + z*z);
      double rank2 = (dist == 0) ? VTK_LARGE_FLOAT:(1.0/dist);
      if (rank2 > rank) 
        {
        rank = rank2;
        }
      }
    }
  this->BlockRank.push_back(rank);
  this->ToGlobalBlockMap.push_back(globalId);
}


//-----------------------------------------------------------------------------
int vtkFlashReader::RequestData( vtkInformation * vtkNotUsed( request ),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector  * outputVector )
{
  vtkInformation *       outInf = outputVector->GetInformationObject( 0 );
  vtkMultiBlockDataSet * output = vtkMultiBlockDataSet::SafeDownCast
                         (  outInf->Get( vtkDataObject::DATA_OBJECT() )  );
  
  this->Internal->ReadMetaData();
  this->GenerateBlockMap();
  
  // Save meta data from all blocks and a map from global to loaded ids.  
  // I am saving global ids because I do not want to require that all ancestors
  // of leaves have to be loaded.  However, I need ancestor meta data to traverse
  // the tree.  We could change this so that pruned whole branches are not
  // in the metadata. 
  int numBlocks = this->Internal->NumberOfBlocks;
  vtkIntArray* levelArray = vtkIntArray::New();
  levelArray->SetName("BlockLevel");
  levelArray->SetNumberOfTuples(numBlocks);
  output->GetFieldData()->AddArray(levelArray);
  vtkIntArray* parentArray = vtkIntArray::New();
  parentArray->SetName("BlockParent");
  parentArray->SetNumberOfTuples(numBlocks);
  output->GetFieldData()->AddArray(parentArray);
  vtkIntArray* childrenArray = vtkIntArray::New();
  childrenArray->SetName("BlockChildren");
  childrenArray->SetNumberOfComponents(8);
  childrenArray->SetNumberOfTuples(numBlocks);
  output->GetFieldData()->AddArray(childrenArray);
  vtkIntArray* neighborArray = vtkIntArray::New();
  neighborArray->SetName("BlockNeighbors");
  neighborArray->SetNumberOfComponents(6);
  neighborArray->SetNumberOfTuples(numBlocks);
  output->GetFieldData()->AddArray(neighborArray);  
  vtkIntArray* globalToLocalMapArray = vtkIntArray::New();
  globalToLocalMapArray->SetName("GlobalToLocalMap");
  globalToLocalMapArray->SetNumberOfTuples(numBlocks);
  output->GetFieldData()->AddArray(globalToLocalMapArray);
  vtkIntArray* localToGlobalMapArray = vtkIntArray::New();
  localToGlobalMapArray->SetName("LocalToGlobalMap");
  output->GetFieldData()->AddArray(localToGlobalMapArray);
  for ( int j = 0; j < numBlocks; j++ )
    {
    globalToLocalMapArray->SetValue(j,-32);
    levelArray->SetValue(j,this->GetBlockLevel(j));
    parentArray->SetValue(j,this->GetBlockParentId(j));
    int childrenIds[8];
    this->GetBlockChildrenIds(j, childrenIds);
    for (int i = 0; i < 8; ++i) 
      {
      if (childrenIds[i] > 0) {--childrenIds[i];}
      } 
    childrenArray->SetTupleValue(j,childrenIds);
    int neighborIds[6];
    this->GetBlockNeighborIds(j, neighborIds);
    for (int i = 0; i < 6; ++i) 
      {
      if (neighborIds[i] > 0) {--neighborIds[i];}
      }
    neighborArray->SetTupleValue(j, neighborIds);
    }

  numBlocks = (int)(this->ToGlobalBlockMap.size());  
  for ( int j = 0; j < numBlocks; j ++ )
    {
    // Change GlobalToLocalMapArray to reflect this block was loaded.
    int globalId = this->ToGlobalBlockMap[j];
    globalToLocalMapArray->SetValue(globalId,j);
    localToGlobalMapArray->InsertNextValue(globalId);
    // blocks not loaded because they are ancestors will be marked with -1
    while (globalId != 0) // root
      {
      globalId = parentArray->GetValue(globalId) - 1;
      if (globalToLocalMapArray->GetValue(globalId) != -32)
        {
        break;
        }
      globalToLocalMapArray->SetValue(globalId, -1);
      }
    this->GetBlock( j, output );
    }
   
  int   blockIdx = (int)(this->ToGlobalBlockMap.size());
  if (this->LoadParticles)
    {
    this->GetParticles( blockIdx, output );
    }
  if (this->LoadMortonCurve)
    {
    this->GetMortonCurve( blockIdx, output );
    }
  outInf = NULL;
  output = NULL;
  levelArray->Delete();
  levelArray = 0;
  
  return 1;
}

// ----------------------------------------------------------------------------
void vtkFlashReader::GetBlock( int blockMapIdx, vtkMultiBlockDataSet * multiBlk )
{
  this->Internal->ReadMetaData();
  int blockIdx = this->ToGlobalBlockMap[blockMapIdx];
  bool processOwns = (this->MyProcessId == this->BlockProcess[blockMapIdx]);
  
  if ( multiBlk == NULL || blockIdx < 0 || 
       blockIdx >= this->Internal->NumberOfBlocks )
    {
    vtkDebugMacro( "Invalid block index or vtkMultiBlockDataSet NULL" << endl );
    return;
    }
  
  int                  bSuccess = 0;
  vtkDataSet         * pDataSet = NULL;
  vtkImageData       * imagData = NULL;
  vtkRectilinearGrid * rectGrid = NULL;
  
  if (processOwns)
    {
    if ( this->BlockOutputType == 0 ) // take each block as a vtkImageData
      {
      imagData = vtkImageData::New();
      pDataSet = imagData;
      bSuccess = this->GetBlock( blockIdx, imagData );
      }
    else                              // take each clock as a vtkRectilinearGrid
      {
      rectGrid = vtkRectilinearGrid::New();
      pDataSet = rectGrid;
      bSuccess = this->GetBlock( blockIdx, rectGrid );
      }
    }
  
  if (  bSuccess == 1  )
    {
    char     blckName[100];
    sprintf( blckName, "Block%03d_Level%d_Type%d", 
             this->Internal->Blocks[blockIdx].Index, 
             this->Internal->Blocks[blockIdx].Level, 
             this->Internal->Blocks[blockIdx].Type );
    multiBlk->SetBlock( blockMapIdx, pDataSet );
    multiBlk->GetMetaData( blockMapIdx )
            ->Set( vtkCompositeDataSet::NAME(), blckName );
    }
    
  pDataSet = NULL;
  
  if ( imagData )
    {
    imagData->Delete();
    imagData = NULL;
    }
  
  if ( rectGrid )
    {
    rectGrid->Delete();
    rectGrid = NULL;
    }
}

// ----------------------------------------------------------------------------
int  vtkFlashReader::GetBlock( int blockIdx, vtkImageData * imagData )
{
  this->Internal->ReadMetaData();
  
  if ( imagData == NULL || blockIdx < 0 || 
       blockIdx >= this->Internal->NumberOfBlocks )
    {
    vtkDebugMacro( "Invalid block index or vtkImageData NULL" << endl );
    return 0;
    }
  
  int     i;
  double  blockMin[3];
  double  blockMax[3];
  double  spacings[3];
  
  for ( i = 0; i < 3; i ++ )
    {
    blockMin[i] =   this->Internal->Blocks[ blockIdx ].MinBounds[i];
    blockMax[i] =   this->Internal->Blocks[ blockIdx ].MaxBounds[i]; 
    spacings[i] = ( this->Internal->BlockGridDimensions[i] > 1   )
                ? ( blockMax[i] - blockMin[i] ) / 
                  ( this->Internal->BlockGridDimensions[i] - 1.0 )
                :   1.0;
    }
  
  imagData->SetDimensions( this->Internal->BlockGridDimensions );
  imagData->SetOrigin ( blockMin[0], blockMin[1], blockMin[2] );
  imagData->SetSpacing( spacings[0], spacings[1], spacings[2] );
  
  // attach the data attributes to the grid
  int   numAttrs = static_cast < int > 
                   ( this->Internal->AttributeNames.size() );
  for ( i = 0; i < numAttrs; i ++ )
    {
    const char* name = this->Internal->AttributeNames[i].c_str();
    if (this->GetCellArrayStatus(name))
      {
      this->GetBlockAttribute(name, blockIdx, imagData );
      }
    }

  // vectorize
  if (this->MergeXYZComponents)
    {
    this->MergeVectors(imagData->GetCellData()); 
    }
    
  return 1;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetBlock( int blockIdx, vtkRectilinearGrid * rectGrid )
{
  this->Internal->ReadMetaData();
  
  if ( rectGrid == NULL || blockIdx < 0 || 
       blockIdx >= this->Internal->NumberOfBlocks )
    {
    vtkDebugMacro( "Invalid block index or vtkRectilinearGrid NULL" << endl );
    return 0;
    }
  
  int       i, j;
  vtkDoubleArray * theCords[3] = { NULL, NULL, NULL };
  
  for ( j = 0; j < 3; j ++ )
    {
    theCords[j] = vtkDoubleArray::New();
    theCords[j]->SetNumberOfTuples( this->Internal->BlockGridDimensions[j] );
    
    if ( this->Internal->BlockGridDimensions[j] == 1 )
      {
      // dimension degeneration
      theCords[j]->SetComponent( 0, 0, 0.0 );
      }
    else
      {  
      // set the one-dimensional coordinates
      double blockMin = this->Internal->Blocks[blockIdx].MinBounds[j];
      double blockMax = this->Internal->Blocks[blockIdx].MaxBounds[j]; 
      double cellSize = ( blockMax - blockMin ) / 
                        ( this->Internal->BlockGridDimensions[j] - 1.0 );
      for ( i = 0; i < this->Internal->BlockGridDimensions[j]; i ++ )
        {
        theCords[j]->SetComponent( i, 0, blockMin + i * cellSize );
        }
      }
    }
  
  // link the coordinates to the rectilinear grid 
  rectGrid->SetDimensions( this->Internal->BlockGridDimensions );
  rectGrid->SetXCoordinates( theCords[0] );
  rectGrid->SetYCoordinates( theCords[1] );
  rectGrid->SetZCoordinates( theCords[2] );
  theCords[0]->Delete();
  theCords[1]->Delete();
  theCords[2]->Delete();
  theCords[0] = NULL;
  theCords[1] = NULL;
  theCords[2] = NULL;
  
  // attach the data attributes to the grid
  int   numAttrs = static_cast < int > 
                   ( this->Internal->AttributeNames.size() );
  for ( i = 0; i < numAttrs; i ++ )
    {
    this->GetBlockAttribute( this->Internal->AttributeNames[i].c_str(), 
                             blockIdx, rectGrid );
    }
    
  return 1;
}

// ----------------------------------------------------------------------------
void vtkFlashReader::GetBlockAttribute( const char * atribute, int blockIdx, 
                                        vtkDataSet * pDataSet )
{
  // this function must be called by GetBlock( ... )
  this->Internal->ReadMetaData();
  
  if ( atribute == NULL || blockIdx < 0  ||
       pDataSet == NULL || blockIdx >= this->Internal->NumberOfBlocks )
    {
    vtkDebugMacro( "Data attribute name or vtkDataSet NULL, or " <<
                   "invalid block index." << endl );
    return;
    }
  // remove the prefix ("mesh_blockandlevel/" or "mesh_blockandproc/") to get
  // the actual attribute name
  vtkstd::string  tempName = atribute;
  size_t          slashPos = tempName.find( "/" );
  vtkstd::string  attrName = tempName.substr ( slashPos + 1 );
  hid_t           dataIndx = H5Dopen
                             ( this->Internal->FileIndex, attrName.c_str() );
  
  if ( dataIndx < 0 )
    {
    vtkErrorMacro( "Invalid attribute name." << endl );
    return;
    }

  hid_t    spaceIdx = H5Dget_space( dataIndx );
  hsize_t  dataDims[4]; // dataDims[0] == number of blocks
  hsize_t  numbDims = H5Sget_simple_extent_dims( spaceIdx, dataDims, NULL );

  if ( numbDims != 4 )
    {
    vtkErrorMacro( "Error with reading the data dimensions." << endl );
    return;
    }

  int      numTupls = dataDims[1] * dataDims[2] * dataDims[3];
  hsize_t  startVec[5];
  hsize_t  stridVec[5];
  hsize_t  countVec[5];
  
  startVec[0] = blockIdx;
  startVec[1] = 0;
  startVec[2] = 0;
  startVec[3] = 0;
  
  stridVec[0] = 1;
  stridVec[1] = 1;
  stridVec[2] = 1;
  stridVec[3] = 1;
  
  countVec[0] = 1;
  countVec[1] = dataDims[1];
  countVec[2] = dataDims[2];
  countVec[3] = dataDims[3];
  
  // file space index
  hid_t      filSpace = H5Screate_simple( 4, dataDims, NULL );
  H5Sselect_hyperslab ( filSpace, H5S_SELECT_SET, startVec, 
                        stridVec, countVec,       NULL );

  startVec[0] = 0;
  startVec[1] = 0;
  startVec[2] = 0;
  startVec[3] = 0;
  
  stridVec[0] = 1;
  stridVec[1] = 1;
  stridVec[2] = 1;
  stridVec[3] = 1;
  
  countVec[0] = 1;
  countVec[1] = dataDims[1];
  countVec[2] = dataDims[2];
  countVec[3] = dataDims[3];
  
  hid_t      memSpace = H5Screate_simple( 4, dataDims, NULL );
  H5Sselect_hyperslab ( memSpace, H5S_SELECT_SET, startVec, 
                        stridVec, countVec,       NULL );

  vtkDoubleArray   * dataAray = vtkDoubleArray::New();
  dataAray->SetName( atribute );
  dataAray->SetNumberOfTuples( numTupls );
  double           * arrayPtr = static_cast < double * > 
                                (  dataAray->GetPointer( 0 )  );

  int    i;
  hid_t  hRawType = H5Dget_type( dataIndx );
  hid_t  dataType = H5Tget_native_type( hRawType, H5T_DIR_ASCEND );
  
  if (  H5Tequal( dataType, H5T_NATIVE_DOUBLE )  )
    {
    H5Dread( dataIndx, dataType,    memSpace, 
             filSpace, H5P_DEFAULT, arrayPtr );
    }
  else 
  if (  H5Tequal( dataType, H5T_NATIVE_FLOAT )  )
    {   
    float * dataFlts = new float [ numTupls ];
    H5Dread( dataIndx, dataType,    memSpace, 
             filSpace, H5P_DEFAULT, dataFlts );
    for ( i = 0; i < numTupls; i ++ )
      {
      arrayPtr[i] = dataFlts[i];
      }
    delete [] dataFlts;
    dataFlts = NULL;
    }
  else 
  if (  H5Tequal( dataType, H5T_NATIVE_INT )  )
    {
    int * dataInts = new int [ numTupls ];
    H5Dread( dataIndx, dataType,    memSpace, 
             filSpace, H5P_DEFAULT, dataInts );
    for ( i = 0; i < numTupls; i ++ )
      {
      arrayPtr[i] = dataInts[i];
      }
    delete[] dataInts;
    dataInts = NULL;
    }
  else 
  if (  H5Tequal( dataType, H5T_NATIVE_UINT )  )
    {
    unsigned int * unsgnInt = new unsigned int [ numTupls ];
    H5Dread( dataIndx, dataType,    memSpace, 
             filSpace, H5P_DEFAULT, unsgnInt );
    for ( i = 0; i < numTupls; i ++ )
      {
      arrayPtr[i] = unsgnInt[i];
      }
    delete[] unsgnInt;
    unsgnInt = NULL;
    }
  else
    {
    vtkErrorMacro( "Invalid data attribute type." << endl );
    }

  H5Sclose( filSpace );
  H5Sclose( memSpace );
  H5Sclose( spaceIdx );
  H5Tclose( dataType );
  H5Tclose( hRawType );
  H5Dclose( dataIndx );

  pDataSet->GetCellData()->AddArray ( dataAray );
  
  dataAray->Delete();
  dataAray = NULL;
  arrayPtr = NULL;
}

// ----------------------------------------------------------------------------
void vtkFlashReader::GetParticles( int & blockIdx, 
                                   vtkMultiBlockDataSet * multiBlk )
{ 
  this->Internal->ReadMetaData();
  
  hid_t       dataIndx = H5Dopen( this->Internal->FileIndex, 
                                  this->Internal->ParticleName.c_str() );
  
  if ( blockIdx < 0 || dataIndx < 0 || !multiBlk )
    {
    vtkDebugMacro( "Particles not found or vtkMultiBlockDataSet NULL" << endl );
    return;
    }

  vtkPolyData * polyData = vtkPolyData::New();
  if (  this->GetParticles( polyData ) == 1  )
    {
    multiBlk->SetBlock( blockIdx, polyData );
    multiBlk->GetMetaData( blockIdx )
            ->Set( vtkCompositeDataSet::NAME(), 
                   this->Internal->ParticleName.c_str() );  
    }   
 
  polyData->Delete();
  polyData = NULL;
  blockIdx ++;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetParticles( vtkPolyData * polyData )
{
  this->Internal->ReadMetaData();
  
  hid_t       dataIndx = H5Dopen( this->Internal->FileIndex, 
                                  this->Internal->ParticleName.c_str() );
  
  if ( dataIndx < 0 || !polyData )
    {
    vtkDebugMacro( "Particles not found or vtkPolyData NULL" << endl );
    return 0;
    }

  char        cordName[20];
  char        xyzChars[3] = { 'x', 'y', 'z' };
  hid_t       theTypes[3];
  vtkPoints * ptCoords = vtkPoints::New( VTK_DOUBLE );
  ptCoords->SetNumberOfPoints( this->Internal->NumberOfParticles );
  
  double    * cordsBuf = new double [ this->Internal->NumberOfParticles ];
  double    * cordsPtr = static_cast< double * > 
                         (  ptCoords->GetVoidPointer( 0 )  );
  memset(  cordsPtr,  0,  sizeof( double ) * 3 * 
                          this->Internal->NumberOfParticles  );
  
  if ( this->Internal->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
    {
    theTypes[0] = H5Tcreate(  H5T_COMPOUND,  sizeof( double )  );
    theTypes[1] = H5Tcreate(  H5T_COMPOUND,  sizeof( double )  );
    theTypes[2] = H5Tcreate(  H5T_COMPOUND,  sizeof( double )  );
    H5Tinsert( theTypes[0], "particle_x", 0, H5T_NATIVE_DOUBLE );
    H5Tinsert( theTypes[1], "particle_y", 0, H5T_NATIVE_DOUBLE );
    H5Tinsert( theTypes[2], "particle_z", 0, H5T_NATIVE_DOUBLE );
    }
    
  for ( int j = 0; j < this->Internal->NumberOfDimensions; j ++ )
    {
      if ( this->Internal->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
        {
        H5Dread( dataIndx, theTypes[j], H5S_ALL,H5S_ALL,H5P_DEFAULT, cordsBuf );
        }
      else 
        {
        sprintf( cordName, "Particles/pos%c", xyzChars[j] );
        this->Internal->ReadParticlesComponent( dataIndx, cordName, cordsBuf );
        }
        
      for ( int i = 0; i < this->Internal->NumberOfParticles; i ++ )
        {
        cordsPtr[ ( i << 1 ) + i + j ] = cordsBuf[i];
        }
    }
    
  delete [] cordsBuf;
  cordsBuf = NULL;
  cordsPtr = NULL;

  if ( this->Internal->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
    {
    H5Tclose( theTypes[0] );
    H5Tclose( theTypes[1] );
    H5Tclose( theTypes[2] );
    }
  H5Dclose( dataIndx );
  
  // fill the vtkPolyData with points and cells (vertices) 
  vtkCellArray * theVerts = vtkCellArray::New();
  polyData->SetPoints( ptCoords );
  polyData->SetVerts( theVerts );
  for ( vtkIdType cellPtId = 0; 
        cellPtId < this->Internal->NumberOfParticles; cellPtId ++ )
    {
    theVerts->InsertNextCell( 1, &cellPtId );
    }
  
  // attach the cell data attributes to the vtkPolyData
  for ( vtkstd::vector< vtkstd::string >::iterator
        attrIter  = this->Internal->ParticleAttributeNames.begin();
        attrIter != this->Internal->ParticleAttributeNames.end(); attrIter ++ )
    {
    if (  ( *attrIter ) != "particle_x"  &&
          ( *attrIter ) != "particle_y"  &&
          ( *attrIter ) != "particle_z"
       )
      {
      // skip the coordinates
      this->GetParticlesAttribute
      (   GetSeparatedParticleName(  ( *attrIter )  ).c_str(),   polyData   );
      }
    }   
  
  // memory deallocation
  theVerts->Delete();
  ptCoords->Delete();
  theVerts = NULL;
  ptCoords = NULL;
  
  return 1;
}

// ----------------------------------------------------------------------------
void vtkFlashReader::GetParticlesAttribute( const char  * atribute, 
                                            vtkPolyData * polyData )
{
  // this function must be called by GetParticles( ... )
  this->Internal->ReadMetaData();
  
  if (  !polyData || !atribute || 
        !this->Internal->ParticleAttributeNamesToIds.count( atribute )  )
    {
    vtkErrorMacro( "Invalid attribute name of particles or " <<
                   "vtkPolyData NULL." << endl );
    return;
    }
  
  hid_t            dataIndx = H5Dopen( this->Internal->FileIndex, 
                                       this->Internal->ParticleName.c_str() );
  int              attrIndx = this->Internal
                                  ->ParticleAttributeNamesToIds[ atribute ];
  hid_t            attrType = this->Internal->ParticleAttributeTypes[ attrIndx ];
  vtkstd::string   attrName = this->Internal->ParticleAttributeNames[ attrIndx ];
  
  if ( attrType != H5T_NATIVE_INT && attrType != H5T_NATIVE_DOUBLE )
    {
    vtkErrorMacro( "Invalid attribute data type of particles." << endl );
    return;
    }
  
  vtkDoubleArray * dataAray = vtkDoubleArray::New();
  dataAray->SetName( atribute );
  dataAray->SetNumberOfTuples( this->Internal->NumberOfParticles );
  double         * arrayPtr = static_cast< double * > 
                              (  dataAray->GetPointer( 0 )  );

  if ( attrType == H5T_NATIVE_DOUBLE )
    {
    if ( this->Internal->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
      {
      hid_t      dataType = H5Tcreate(  H5T_COMPOUND,  sizeof( double )  );
      H5Tinsert( dataType, attrName.c_str(), 0, H5T_NATIVE_DOUBLE );
      H5Dread  ( dataIndx, dataType, H5S_ALL,H5S_ALL, H5P_DEFAULT, arrayPtr );
      H5Tclose ( dataType );
      }
    else
      {
      this->Internal->ReadParticlesComponent( dataIndx, atribute, arrayPtr );
      }
    }
  else 
  if ( attrType == H5T_NATIVE_INT )
    {
    hid_t      dataType = H5Tcreate(  H5T_COMPOUND,  sizeof( int )  );
    H5Tinsert( dataType, attrName.c_str(), 0, H5T_NATIVE_INT );

    int      * dataInts = new int[ this->Internal->NumberOfParticles ];
    H5Dread  ( dataIndx, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataInts );

    for ( int i = 0; i < this->Internal->NumberOfParticles; i ++ )
      {
      arrayPtr[i] = dataInts[i];
      }
    delete [] dataInts;
    dataInts = NULL;
    H5Tclose( dataType );
    }

  H5Dclose(dataIndx);

  polyData->GetCellData()->AddArray( dataAray );
  
  dataAray->Delete();
  dataAray = NULL;
  arrayPtr = NULL;
}

// ----------------------------------------------------------------------------
void vtkFlashReader::GetMortonCurve( int & blockIdx, 
                                     vtkMultiBlockDataSet * multiBlk )
{   
  if ( blockIdx < 0 || !multiBlk )
    {
    vtkErrorMacro( "vtkMultiBlockDataSet NULL or an invalid block index " 
                   << "assigned to the Morton curve." << endl );
    return;
    }
    
  vtkPolyData * polyData = vtkPolyData::New();
  
  if (  this->GetMortonCurve( polyData ) == 1  )
    {
    multiBlk->SetBlock( blockIdx, polyData );
    multiBlk->GetMetaData( blockIdx )
            ->Set( vtkCompositeDataSet::NAME(), "MortonCurve" );
            
    blockIdx ++;
    }
          
  polyData->Delete();
  polyData = NULL;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetMortonCurve( vtkPolyData * polyData )
{
  this->Internal->ReadMetaData();
  
  if ( this->Internal->NumberOfBlocks < 1 || !polyData )
    {
    vtkErrorMacro( "no any block found or vtkPolyData NULL." << endl );
    return 0;
    }
    
  int            i;
  int            bSuccess = 0;
  int            numbPnts = 0;
  vtkPoints    * curvePts = vtkPoints::New();
  vtkCellArray * theLines = vtkCellArray::New();

  // retrieve the center of each leaf block and duplicate it (except for the
  // first leaf block) and then connect these centers successively, two points
  // per line segment
  for ( numbPnts = 0, i = 0; i < this->Internal->NumberOfBlocks; i ++ )
    {
    if ( this->Internal->Blocks[i].Type == FLASH_READER_LEAF_BLOCK )
      {
      curvePts->InsertPoint( numbPnts, this->Internal->Blocks[i].Center[0], 
                                       this->Internal->Blocks[i].Center[1],
                                       this->Internal->Blocks[i].Center[2] );
      numbPnts ++;
      
      if ( numbPnts != 1 )
        {
        // duplicate each internal point beginning with the second point
        curvePts->InsertPoint( numbPnts, this->Internal->Blocks[i].Center[0], 
                                         this->Internal->Blocks[i].Center[1], 
                                         this->Internal->Blocks[i].Center[2] );
        numbPnts ++;
        }
      }
    }
    
  // # ( numbPnts - 2 ) and # ( numbPnts - 1 ) refer to the very last point
  // and hence the final connection occurs between # (numbPnts - 3 ) and
  // # ( numbPnts - 2 ) 
  for ( i = 0; i < numbPnts - 2; i += 2 )
    {
    theLines->InsertNextCell( 2 );
    theLines->InsertCellPoint( i );
    theLines->InsertCellPoint( i + 1 );
    }

  if ( numbPnts )
    {
    bSuccess = 1;
    polyData->SetPoints( curvePts );
    polyData->SetLines ( theLines );
    }
          
  theLines->Delete();
  curvePts->Delete();
  theLines = NULL;
  curvePts = NULL;
  
  return bSuccess;
}

// ----------------------------------------------------------------------------
int vtkFlashReader::GetMortonSegment( int blockIdx, vtkPolyData * polyData )
{
  this->Internal->ReadMetaData();
  
  // A morton curve is something like a z-order curve that connects leaf blocks
  // by their centers successively. This function links the given leaf block,
  // if so, with its neighboring leaf blocks using two line segments.
  
  if ( polyData == NULL || blockIdx < 0 )
    {
    vtkDebugMacro( "vtkPolyData NULL, unable to hold Morton curve." << endl );
    return 0;
    }
  
  vtkstd::vector< int >::iterator i = 
                            find( this->Internal->LeafBlocks.begin(), 
                                  this->Internal->LeafBlocks.end(), blockIdx );
  if ( i == this->Internal->LeafBlocks.end() )
    {
    vtkDebugMacro( "A leaf block expected." << endl );
    return 0;
    }
    
  vtkPoints    * linePnts = vtkPoints::New();
  vtkCellArray * theLines = vtkCellArray::New();
  
  if (  i == this->Internal->LeafBlocks.begin()  )
    {
    linePnts->InsertPoint(  0,  this->Internal->Blocks[  blockIdx  ].Center[0], 
                                this->Internal->Blocks[  blockIdx  ].Center[1], 
                                this->Internal->Blocks[  blockIdx  ].Center[2]  );
    linePnts->InsertPoint(  1,  this->Internal->Blocks[ *( i + 1 ) ].Center[0], 
                                this->Internal->Blocks[ *( i + 1 ) ].Center[1], 
                                this->Internal->Blocks[ *( i + 1 ) ].Center[2]  );
                                
    theLines->InsertNextCell ( 2 );
    theLines->InsertCellPoint( 0 );
    theLines->InsertCellPoint( 1 );
    }
  else 
  if (  i == ( this->Internal->LeafBlocks.end() - 1 )  )
    {
    linePnts->InsertPoint(  0,  this->Internal->Blocks[ *( i - 1 ) ].Center[0], 
                                this->Internal->Blocks[ *( i - 1 ) ].Center[1], 
                                this->Internal->Blocks[ *( i - 1 ) ].Center[2]  );
    linePnts->InsertPoint(  1,  this->Internal->Blocks[  blockIdx  ].Center[0], 
                                this->Internal->Blocks[  blockIdx  ].Center[1], 
                                this->Internal->Blocks[  blockIdx  ].Center[2]  );
                                
    theLines->InsertNextCell ( 2 );
    theLines->InsertCellPoint( 0 );
    theLines->InsertCellPoint( 1 );
    }
  else
    {
    // duplicate internal points
    linePnts->InsertPoint(  0,  this->Internal->Blocks[ *( i - 1 ) ].Center[0], 
                                this->Internal->Blocks[ *( i - 1 ) ].Center[1], 
                                this->Internal->Blocks[ *( i - 1 ) ].Center[2]  );
    linePnts->InsertPoint(  1,  this->Internal->Blocks[  blockIdx  ].Center[0], 
                                this->Internal->Blocks[  blockIdx  ].Center[1], 
                                this->Internal->Blocks[  blockIdx  ].Center[2]  );
    linePnts->InsertPoint(  2,  this->Internal->Blocks[  blockIdx  ].Center[0], 
                                this->Internal->Blocks[  blockIdx  ].Center[1], 
                                this->Internal->Blocks[  blockIdx  ].Center[2]  );
    linePnts->InsertPoint(  3,  this->Internal->Blocks[ *( i + 1 ) ].Center[0], 
                                this->Internal->Blocks[ *( i + 1 ) ].Center[1], 
                                this->Internal->Blocks[ *( i + 1 ) ].Center[2]  );
                                
    theLines->InsertNextCell ( 2 );
    theLines->InsertCellPoint( 0 );
    theLines->InsertCellPoint( 1 );
    
    theLines->InsertNextCell ( 2 );
    theLines->InsertCellPoint( 2 );
    theLines->InsertCellPoint( 3 );
    }

  polyData->SetPoints(linePnts);
  polyData->SetLines(theLines);
  
  theLines->Delete();
  linePnts->Delete();
  theLines = NULL;
  linePnts = NULL;
  
  return 1;
}

/*/ ----------------------------------------------------------------------------
void vtkFlashReader::GetCurve( const char * curvName, int & blockIdx, 
                               vtkMultiBlockDataSet * multiBlk )
{
  if (  curvName == NULL || strlen( curvName ) <= 7 || 
        strncmp( curvName, "curves/", 7 )  )
    {
    vtkDebugMacro( "Invalid curve name or vtkMultiBlockDataSet NULL." << endl );
    return;
    }
    
  this->Internal->ReadMetaData();
    
  // Connect the center (note that only x coordinate is used) of each cell
  // along the x axis to form a line, which is then saved to a 1D rectlinear
  // grid. Attached to this 1D grid (line) is a point data attribute specified
  // by the latter sub-string of 'curvName'.

  // obtain the scalar value at each sample from the file
  int            numSamps = this->Internal->BlockCellDimensions[0] * 
                            this->Internal->NumberOfBlocks;
  //float        * sampVals = new float [ numSamps ];
  double        * sampVals = new double [ numSamps ];
  vtkstd::string dataName = vtkstd::string( curvName ).substr( 7 ); 
  hid_t          dataIndx = H5Dopen
                            ( this->Internal->FileIndex, dataName.c_str() );
  if ( dataIndx < 0 )
    {
    vtkDebugMacro( << curvName << " not found in the file." << endl );
    delete [] sampVals;
    sampVals = NULL;
    return;
    }
  //H5Dread ( dataIndx, H5T_NATIVE_FLOAT, H5S_ALL,
  H5Dread ( dataIndx, H5T_NATIVE_DOUBLE, H5S_ALL,
            H5S_ALL,  H5P_DEFAULT,      sampVals );
  H5Dclose( dataIndx );

  // determine the sampling rate --- the smallest step size of all blocks
  int       b,  i;
  double    dataSize = this->Internal->MaxBounds[0] - 
                       this->Internal->MinBounds[0];
  double    stepSize = dataSize;
  for ( b = 0; b < this->Internal->NumberOfBlocks; b ++ )
    {
    double  blockMin = this->Internal->Blocks[b].MinBounds[0];
    double  blockMax = this->Internal->Blocks[b].MaxBounds[0];
    double  cellSize = ( blockMax - blockMin ) / 
                       this->Internal->BlockCellDimensions[0];
    stepSize = ( cellSize < stepSize ) ? cellSize : stepSize;
    }
  int       numSteps = static_cast < int > ( dataSize / stepSize + 0.5 );

  // allocate memory for the samples and initialize the arrays
  // (note that each sample is located at the center of a cell)
  int      * ptLevels = new int    [ numSteps ];
  double   * sampleXs = new double [ numSteps ];
  double   * dataVals = new double [ numSteps ];
  for ( i = 0; i < numSteps; i ++ )
    {
    ptLevels[i] = -1;
    sampleXs[i] = this->Internal->MinBounds[0] + ( i + 0.5 ) * stepSize;
    dataVals[i] = 0.0; // scalar point data attribute value, to be determined
    }

  // the position of the starting sample of all blocks
  double    samp0Pos = this->Internal->MinBounds[0] + stepSize * 0.5;
  
  // iterate over each block to process its cells
  for ( b = 0; b < this->Internal->NumberOfBlocks; b ++ )
    {
    // block-based information
    double  blockMin = this->Internal->Blocks[b].MinBounds[0];
    double  blockMax = this->Internal->Blocks[b].MaxBounds[0];
    double  blckSize = blockMax - blockMin;
    double  blckStep = blckSize / this->Internal->BlockCellDimensions[0];
    int     firstIdx = int(  ( blockMin - samp0Pos ) / stepSize + 0.99999  );
    int     lastIndx = int(  ( blockMax - samp0Pos ) / stepSize            );
    
    firstIdx = ( firstIdx < 0 || firstIdx >= numSteps ) ? 0 : firstIdx;
    lastIndx = ( lastIndx < 0 || lastIndx >= numSteps ) 
               ? numSteps - 1 : lastIndx;

    for ( i = firstIdx; i <= lastIndx; i ++ )
      {
      if ( this->Internal->Blocks[b].Level < ptLevels[i] )
        {
        continue;
        }

      // the block-based index of the sample / cell-center
      int   cellIndx = int(  ( sampleXs[i] - blockMin )  /  blckStep  );
      if (  cellIndx < 0  ||  
            cellIndx >= this->Internal->BlockCellDimensions[0]  )
        {
        continue;
        }

      dataVals[i] = sampVals[ b * this->Internal->BlockCellDimensions[0] + 
                              cellIndx ];
      ptLevels[i] = this->Internal->Blocks[b].Level;
      }
    }

  // create a 1D rectilinear grid (line) to connect all sample points and
  // attach a scalar value as the point data attribute to each sample
  vtkImageData * sampLine = vtkImageData::New();
  this->Create1DRectilinearGrid( sampLine, numSteps, VTK_DOUBLE );
  vtkDataArray       * smpCords = sampLine->GetXCoordinates();
  vtkDoubleArray     * dataAray = vtkDoubleArray::New();
  dataAray->SetNumberOfComponents( 1 );
  dataAray->SetNumberOfTuples( numSteps );
  dataAray->SetName( curvName );
  sampLine->GetPointData()->SetScalars( dataAray );
  
  for ( i = 0; i < numSteps; i ++ )
    { 
    smpCords->SetComponent(i, 0, sampleXs[i] );
    dataAray->SetValue( i, dataVals[i] );
    }
    
  // insert this rectilinear grid to the multi-block dataset
  multiBlk->SetBlock( blockIdx, sampLine );
  multiBlk->GetMetaData( blockIdx )
          ->Set( vtkCompositeDataSet::NAME(), curvName );
          
  sampLine->Delete();
  dataAray->Delete();
  delete[] sampVals;
  delete[] sampleXs;
  delete[] dataVals;
  delete[] ptLevels;
  sampLine = NULL;  
  dataAray = NULL;
  smpCords = NULL;
  sampVals = NULL;
  sampleXs = NULL;
  dataVals = NULL;
  ptLevels = NULL;
  
  blockIdx ++;
}

// ----------------------------------------------------------------------------
// This function initilizes a vtkImageData rectGrid with an allocated
// one-dimensional array of x-coordinates (of size nXCoords and of type 
// dataType: VTK_DOUBLE, VTK_INT, et al) to be filled.
void vtkFlashReader::Create1DRectilinearGrid( vtkImageData * rectGrid,
                                              int nXCoords, int dataType )
{
  if ( !rectGrid || nXCoords <= 0 )
    {
    vtkDebugMacro( "Invalid dimension or vtkImageData NULL." << endl );
    return;
    }
    
  vtkDataArray * xCordAry = NULL;
  vtkDataArray * yzCoords = NULL;
  rectGrid->SetDimensions( nXCoords, 1, 1 );
  
  if ( dataType == VTK_DOUBLE )
    {
    xCordAry = vtkDoubleArray::New();
    yzCoords = vtkDoubleArray::New();
    }
  else 
  if ( dataType == VTK_FLOAT )
    {
    xCordAry = vtkFloatArray::New();
    yzCoords = vtkFloatArray::New();
    }
  else 
  if ( dataType == VTK_INT )
    {
    xCordAry = vtkIntArray::New();
    yzCoords = vtkIntArray::New();
    }
  else 
  if ( dataType == VTK_SHORT )
    {
    xCordAry = vtkShortArray::New();
    yzCoords = vtkShortArray::New();
    }
  else 
  if ( dataType == VTK_CHAR )
    {
    xCordAry = vtkCharArray::New();
    yzCoords = vtkCharArray::New();
    }
  else
    {
    vtkDebugMacro( "Data type not yet supported." << endl );
    return;
    }
    
  xCordAry->SetNumberOfComponents( 1 );
  xCordAry->SetNumberOfTuples( nXCoords );
  
  yzCoords->SetNumberOfComponents( 1 );
  yzCoords->SetNumberOfTuples( 1 );
  yzCoords->SetTuple1( 0, 0.0 );
  
  rectGrid->SetXCoordinates( xCordAry ); 
  rectGrid->SetYCoordinates( yzCoords ); 
  rectGrid->SetZCoordinates( yzCoords ); 
  
  xCordAry->Delete(); 
  yzCoords->Delete(); 
  xCordAry = NULL;
  yzCoords = NULL;
}//*/



//-----------------------------------------------------------------------------
void vtkFlashReader::MergeVectors(vtkDataSetAttributes* da)
{
  int numArrays = da->GetNumberOfArrays();
  int idx;
  vtkDataArray *a1, *a2, *a3;
  int flag = 1;

  // Loop merging arrays.
  // Since we are modifying the list of arrays that we are traversing,
  // merge one set of arrays at a time.
  while (flag)
    {
    flag = 0;  
    for (idx = 0; idx < numArrays-1 && !flag; ++idx)
      {
      a1 = da->GetArray(idx);
      a2 = da->GetArray(idx+1);
      if (idx+2 < numArrays)
        {
        a3 = da->GetArray(idx+2);
        if (this->MergeVectors(da, a1, a2, a3))
          {
          flag = 1;
          continue;
          }    
        if (this->MergeVectors(da, a3, a2, a1))
          {
          flag = 1;
          continue;
          }
        }
      if (this->MergeVectors(da, a1, a2))
        {
        flag = 1;
        continue;
        }
      if (this->MergeVectors(da, a2, a1))
        {
        flag = 1;
        continue;
        }
      }
    }
}

//-----------------------------------------------------------------------------
// The templated execute function handles all the data types.
template <class T>
void vtkMergeVectorComponents(vtkIdType length,
                              T *p1, T *p2, T *p3, T *po)
{
  vtkIdType idx;
  if (p3)
    {
    for (idx = 0; idx < length; ++idx)
      {
      *po++ = *p1++;
      *po++ = *p2++;
      *po++ = *p3++;
      }
    }
  else
    {
    for (idx = 0; idx < length; ++idx)
      {
      *po++ = *p1++;
      *po++ = *p2++;
      *po++ = (T)0;
      }
    }
}

//-----------------------------------------------------------------------------
int vtkFlashReader::MergeVectors(vtkDataSetAttributes* da, 
                                   vtkDataArray * a1, vtkDataArray * a2, vtkDataArray * a3)
{
  int prefixFlag = 0;

  if (a1 == 0 || a2 == 0 || a3 == 0)
    {
    return 0;
    }
  if(a1->GetNumberOfTuples() != a2->GetNumberOfTuples() ||
     a1->GetNumberOfTuples() != a3->GetNumberOfTuples())
    { // Sanity check.  Should never happen.
    return 0;
    }
  if(a1->GetDataType()!=a2->GetDataType()||a1->GetDataType()!=a3->GetDataType())
    {
    return 0;
    }
  if(a1->GetNumberOfComponents()!=1 || a2->GetNumberOfComponents()!=1 ||
     a3->GetNumberOfComponents()!=1)
    {
    return 0;
    }
  const char *n1, *n2, *n3;
  size_t e1, e2, e3;
  n1 = a1->GetName();
  n2 = a2->GetName();
  n3 = a3->GetName();
  if (n1 == 0 || n2 == 0 || n3 == 0)
    {
    return 0;
    }  
  e1 = strlen(n1)-1;
  e2 = strlen(n2)-1;
  e3 = strlen(n3)-1;
  if(e1!=e2 || e1 != e3)
    {
    return 0;
    }
  if (strncmp(n1+1,n2+1,e1)==0 && strncmp(n1+1,n3+1,e1)==0)
    { // Trailing characters are the same. Check for prefix XYZ.
    if ((n1[0]!='X' || n2[0]!='Y' || n3[0]!='Z') &&
        (n1[0]!='x' || n2[0]!='y' || n3[0]!='z'))
      { // Since last characters are the same, there is no way
      // the names can have postfix XYZ.
      return 0;
      }
    prefixFlag = 1;
    }
  else
    { // Check for postfix case.
    if (strncmp(n1,n2,e1)!=0 || strncmp(n1,n3,e1)!=0)
      { // Not pre or postfix.
      return 0;
      }
    if ((n1[e1]!='X' || n2[e2]!='Y' || n3[e3]!='Z') &&
        (n1[e1]!='x' || n2[e2]!='y' || n3[e3]!='z'))
      { // Tails are the same, but postfix not XYZ.
      return 0;
      }
    }
  // Merge the arrays.
  vtkDataArray* newArray = a1->NewInstance();
  newArray->SetNumberOfComponents(3);
  vtkIdType length = a1->GetNumberOfTuples();
  newArray->SetNumberOfTuples(length);
  void *p1 = a1->GetVoidPointer(0);
  void *p2 = a2->GetVoidPointer(0);
  void *p3 = a3->GetVoidPointer(0);
  void *pn = newArray->GetVoidPointer(0);
  switch (a1->GetDataType())
    {
    vtkTemplateMacro(
      vtkMergeVectorComponents( length, 
                                (VTK_TT*)p1,
                                (VTK_TT*)p2,
                                (VTK_TT*)p3,
                                (VTK_TT*)pn));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return 0;
    }
  if (prefixFlag)
    {
    newArray->SetName(n1+1);
    }
  else
    {
    char* name = new char[e1+2];
    strncpy(name,n1,e1);
    name[e1] = '\0';
    newArray->SetName(name);
    delete [] name;
    }
  da->RemoveArray(n1);    
  da->RemoveArray(n2);    
  da->RemoveArray(n3);
  da->AddArray(newArray);
  newArray->Delete();    
  return 1;
}

//-----------------------------------------------------------------------------
int vtkFlashReader::MergeVectors(vtkDataSetAttributes* da, 
                                   vtkDataArray * a1, vtkDataArray * a2)
{
  int prefixFlag = 0;

  if (a1 == 0 || a2 == 0)
    {
    return 0;
    }
  if (a1->GetNumberOfTuples() != a2->GetNumberOfTuples())
    { // Sanity check.  Should never happen.
    return 0;
    }
  if (a1->GetDataType() != a2->GetDataType())
    {
    return 0;
    }
  if (a1->GetNumberOfComponents() != 1 || a2->GetNumberOfComponents() != 1)
    {
    return 0;
    }
  const char *n1, *n2;
  size_t e1, e2;
  n1 = a1->GetName();
  n2 = a2->GetName();
  if (n1 == 0 || n2 == 0)
    {
    return 0;
    }  
  e1 = strlen(n1)-1;
  e2 = strlen(n2)-1;
  if (e1 != e2 )
    {
    return 0;
    }
  if ( strncmp(n1+1,n2+1,e1) == 0)
    { // Trailing characters are the same. Check for prefix XYZ.
    if ((n1[0]!='X' || n2[0]!='Y') && (n1[0]!='x' || n2[0]!='y'))
      { // Since last characters are the same, there is no way
      // the names can have postfix XYZ.
      return 0;
      }
    prefixFlag = 1;
    }
  else
    { // Check for postfix case.
    if (strncmp(n1,n2,e1) != 0)
      { // Not pre or postfix.
      return 0;
      }
    if ((n1[e1]!='X' || n2[e2]!='Y') && (n1[e1]!='x' || n2[e2]!='y'))
      { // Tails are the same, but postfix not XYZ.
      return 0;
      }
    }
  // Merge the arrays.
  vtkDataArray* newArray = a1->NewInstance();
  // Creae the third componnt and set to 0.
  newArray->SetNumberOfComponents(3);
  vtkIdType length = a1->GetNumberOfTuples();
  newArray->SetNumberOfTuples(length);
  void *p1 = a1->GetVoidPointer(0);
  void *p2 = a2->GetVoidPointer(0);
  void *pn = newArray->GetVoidPointer(0);
  switch (a1->GetDataType())
    {
    vtkTemplateMacro(
      vtkMergeVectorComponents( length, 
                                (VTK_TT*)p1,
                                (VTK_TT*)p2,
                                (VTK_TT*)0,
                                (VTK_TT*)pn));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return 0;
    }
  if (prefixFlag)
    {
    newArray->SetName(n1+1);
    }
  else
    {
    char* name = new char[e1+2];
    strncpy(name,n1,e1);
    name[e1] = '\0';
    newArray->SetName(name);
    delete [] name;
    }
  da->RemoveArray(n1);    
  da->RemoveArray(n2);    
  da->AddArray(newArray);
  newArray->Delete();    
  return 1;
}

//-----------------------------------------------------------------------------
void vtkFlashReader::SetMergeXYZComponents(int merge)
{
  if ( merge == this->MergeXYZComponents )
    {
    return;
    }
  this->MergeXYZComponents = merge;
  this->Modified();
}


//-----------------------------------------------------------------------------
// Count the number of roots for parallelism.
// Add the available atrributes to information.
int vtkFlashReader::RequestInformation(vtkInformation *request,
                                         vtkInformationVector **inputVector,
                                         vtkInformationVector *outputVector)
{
  if(!this->Superclass::RequestInformation(request,inputVector,outputVector))
    {
    return 0;
    }
  // I copied this from the SpyPlot reader. It should not be necessary.
  //struct stat fs;
  //if(stat(this->FileName,&fs)!=0)
  //  {
  //  vtkErrorMacro("Cannot find file " << this->FileName);
  //  return 0;
  //  }

  // Count the roots.
  this->NumberOfRoots = 0;
  this->Internal->ReadMetaData();
  int numGlobalBlocks = this->Internal->NumberOfBlocks;
  for (int i = 0; i < numGlobalBlocks; ++i)
    {
    if (this->GetBlockLevel(i) == 1)
      {
      ++this->NumberOfRoots;
      }
    }
  vtkInformation *info=outputVector->GetInformationObject(0);
  info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),this->NumberOfRoots);

  // Superclass must call this  I do not know how the CellDataArraySelection gets filled out.
  // It works for selecitng arrays from the Paraview Gui so ....
  // I cannot call it here because CellDataArraySelection is empty and crashes.
  //return this->UpdateMetaData(request, outputVector);
  return -1;
}
