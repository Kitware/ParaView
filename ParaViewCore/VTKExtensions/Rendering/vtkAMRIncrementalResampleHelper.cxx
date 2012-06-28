/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRIncrementalResampleHelper.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRIncrementalResampleHelper.h"

#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkFieldData.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkStructuredData.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridPartitioner.h"
#include "vtkXMLImageDataWriter.h"

// C++ includes
#include <set>
#include <cassert>

#define IMIN(ext) ext[0]
#define IMAX(ext) ext[1]
#define JMIN(ext) ext[2]
#define JMAX(ext) ext[3]
#define KMIN(ext) ext[4]
#define KMAX(ext) ext[5]

//-----------------------------------------------------------------------------
// INTERNAL CLASSES
//-----------------------------------------------------------------------------
class vtkAMRDomainParameters
{
public:
  double GlobalOrigin[3];
  double RootSpacing[3];
  int RefinementRatio;
};

class vtkAMRProcessedBlocksList : public std::set<int> {};
//------------------------------------------------------------------------------


vtkStandardNewMacro(vtkAMRIncrementalResampleHelper);

//------------------------------------------------------------------------------
vtkAMRIncrementalResampleHelper::vtkAMRIncrementalResampleHelper()
{
  this->Controller          = vtkMultiProcessController::GetGlobalController();
  this->ProcessedBlocks     = new vtkAMRProcessedBlocksList;
  this->Metadata        = NULL;
  this->AMRData        = NULL;
  this->Grid                = NULL;
}

//------------------------------------------------------------------------------
vtkAMRIncrementalResampleHelper::~vtkAMRIncrementalResampleHelper()
{
  if( this->Grid != NULL )
    {
    this->Grid->Delete();
    }

  if( this->ProcessedBlocks != NULL )
    {
    this->ClearProcessedBlocks();
    delete this->ProcessedBlocks;
    }
}

//------------------------------------------------------------------------------
void vtkAMRIncrementalResampleHelper::PrintSelf(ostream &oss, vtkIndent indent)
{
  this->Superclass::PrintSelf( oss, indent );
}

//------------------------------------------------------------------------------
void vtkAMRIncrementalResampleHelper::Initialize(vtkOverlappingAMR* amrmetadata)
{
  assert("pre: AMR metadata is NULL!" && (amrmetadata != NULL) );
  this->Metadata = amrmetadata;
  this->ClearProcessedBlocks();
}

//------------------------------------------------------------------------------
void vtkAMRIncrementalResampleHelper::UpdateROI(
    double bounds[6], int N[3])
{
  // STEP 0: Delete previous ROI
  if( this->Grid != NULL )
    {
    this->Grid->Delete();
    }
  this->Grid = vtkUniformGrid::New();

  // STEP 1: Clear all processed blocks
  this->ClearProcessedBlocks();

  // STEP 2: Compute grid parameters
  double L[3];
  for( int i=0; i < 3; ++i )
    {
    this->NumberOfSamples[i] = N[i];
    this->ROIBounds[i*2]     = bounds[i*2];
    this->ROIBounds[i*2+1]   = bounds[i*2+1];
    L[i] = this->ROIBounds[i*2+1] - this->ROIBounds[i*2];
    this->h[i] = L[i]/this->NumberOfSamples[i];
    }

  // STEP 3: Partition the grid to the number of processes.
  if( this->Controller->GetNumberOfProcesses( ) > 1 )
    {
  // TODO: Call vtkUniformGridPartitioner to partition the output grid
  // to the number of processes.
    }
  else
    {
    double origin[3];
    origin[0] = this->ROIBounds[0]; // xmin
    origin[1] = this->ROIBounds[2]; // ymin
    origin[2] = this->ROIBounds[4]; // zmin
    this->Grid->SetOrigin(origin);
    this->Grid->SetSpacing( this->h );
    this->Grid->SetDimensions(this->NumberOfSamples);
    }

  this->InitializeGrid();
  this->Controller->Barrier();
}

//------------------------------------------------------------------------------
void vtkAMRIncrementalResampleHelper::InitializeGrid()
{
  assert("pre: AMR metdata has not been set!" && (this->Metadata != NULL) );
  assert("pre: AMR data has not been set!" && (this->AMRData != NULL) );
  assert("pre: Grid instance is NULL!" && (this->Grid != NULL) );

  // STEP 0: Initialize the grid fields
  vtkUniformGrid *refGrid = this->GetReferenceGrid();
  assert("pre: Grid should not be NULL" && (refGrid != NULL) );
  this->Grid->GetPointData()->CopyAllOn();
  this->Grid->GetPointData()->CopyAllocate(
    refGrid->GetCellData(), this->Grid->GetNumberOfPoints());
  for (int cc=0; cc < this->Grid->GetPointData()->GetNumberOfArrays(); cc++)
    {
    this->Grid->GetPointData()->GetArray(cc)->SetNumberOfTuples(
      this->Grid->GetNumberOfPoints());
    }
  //this->InitializeGridFields(
  //   this->Grid->GetPointData(),this->Grid->GetNumberOfPoints(),
  //   refGrid->GetCellData());


  // STEP 1: Create donor level array that store for each grid point the level
  // of the block that they got the value from
  vtkIntArray *donorLevelArray = vtkIntArray::New();
  donorLevelArray->SetName( "DonorLevel" );
  donorLevelArray->SetNumberOfComponents( 1 );
  donorLevelArray->SetNumberOfTuples( this->Grid->GetNumberOfPoints() );

  // STEP 2: Initialize the donorLevel Array to -1
  for(vtkIdType pntIdx=0; pntIdx < this->Grid->GetNumberOfPoints(); ++pntIdx )
    {
    donorLevelArray->SetValue(pntIdx,-1);
    } // END for all grid points

  this->Grid->GetPointData()->AddArray( donorLevelArray );
  donorLevelArray->Delete();

  // STEP 2: Process current AMR data in the tree
  this->Update();
}

//------------------------------------------------------------------------------
void vtkAMRIncrementalResampleHelper::InitializeGridFields(
    vtkFieldData *F, vtkIdType size, vtkFieldData *src)
{
  assert( "pre: field data is NULL!" && (F != NULL) );
  assert( "pre: source cell data is NULL" && (src != NULL) );

  for( int arrayIdx=0; arrayIdx < src->GetNumberOfArrays(); ++arrayIdx )
   {
   int dataType        = src->GetArray( arrayIdx )->GetDataType();
   vtkDataArray *array = vtkDataArray::CreateDataArray( dataType );
   assert( "pre: failed to create array!" && (array != NULL) );

   array->SetName( src->GetArray(arrayIdx)->GetName() );
   array->SetNumberOfComponents(
            src->GetArray(arrayIdx)->GetNumberOfComponents() );
   array->SetNumberOfTuples( size );
   assert( "post: array size mismatch" &&
           (array->GetNumberOfTuples() == size) );

   F->AddArray( array );
   array->Delete();
   }

}

// 2nd Approach: Compute rcvExtent and DonorExtent to avoid calling
// ComputeStructuredCoords for each point, as well as looping through all
// the points.
//    // STEP 3: Compute rcvExtent and donorExtent within the given box
//    int rcvExtent[6];
//    this->ComputeExtent(
//       this->Grid,
//       const_cast<double*>(iBox.GetMinPoint()),
//       const_cast<double*>(iBox.GetMaxPoint()),
//       rcvExtent);
//
//    int donorExtent[6];
//    this->ComputeExtent(
//        donorGrid,
//        const_cast<double*>(iBox.GetMinPoint()),
//        const_cast<double*>(iBox.GetMaxPoint()),
//        donorExtent );
//
//    // NOTE: rcvExtent & donorExtent may not always be of the same size (?)
//    for( int i=IMIN(rcvExtent); i <= IMAX(rcvExtent); ++i )
//      {
//      for( int j=JMIN(rcvExtent); j <= JMAX(rcvExtent); ++j )
//        {
//        for( int k=IMIN(rcvExtent); k <= KMAX(rcvExtent); ++k )
//          {
//
//          } // END for all k
//        } // END for all j
//      } // END for all i
//------------------------------------------------------------------------------
void vtkAMRIncrementalResampleHelper::TransferSolutionFromGrid(
    vtkUniformGrid *donorGrid, int level)
{
  assert("pre: donor grid should not be NULL" && (donorGrid != NULL) );
  assert("pre: level >= 0" && (level >= 0) );
  assert("pre: Receiver grid is NULL!" && (this->Grid != NULL) );
  assert("pre: Receiver grid must have a DonorLevel array" &&
    (this->Grid->GetPointData()->HasArray("DonorLevel") ) );

  // STEP 0: Construct the intersection box instance between the two grids,
  // initialized to the receiver grid.
  vtkBoundingBox iBox;
  iBox.SetBounds( this->Grid->GetBounds() );

  // STEP 1: Construct the donor box instance
  vtkBoundingBox donorBox;
  donorBox.SetBounds( donorGrid->GetBounds() );

  // STEP 2: Get pointer to the donor level array
  vtkIntArray *donorLevel =
    vtkIntArray::SafeDownCast(
      this->Grid->GetPointData()->GetArray("DonorLevel") );
  assert("pre: donor level array is NULL" && (donorLevel != NULL) );
  assert("pre: donor level array is out-of-bounds" &&
    (donorLevel->GetNumberOfTuples()==this->Grid->GetNumberOfPoints()));

  // STEP 3: Check if the grids are intersecting
  if (!iBox.IntersectBox( donorBox ))
    {
    // nothing to do, boxes don't intersect.
    return;
    }
  // STEP 4: Declare/acquire some attributes computed in each loop
  int ijk[3];
  double pcoords[3];
  int dataDescription =
    vtkStructuredData::GetDataDescription(donorGrid->GetDimensions());

  bool data_changed = false;
  // STEP 5: Loop through all the points and foreach point do:
  // 1. Check if the value at the point is a lower resolution
  // 2. If at a lower res, find the donorCell that contains the pnt
  // 3. Override the data at the point with higher res values
  // 4. Update the donorLevel array with the level of this donor grid
  for (vtkIdType pntIdx=0; pntIdx < this->Grid->GetNumberOfPoints(); ++pntIdx )
    {
    if (donorLevel->GetValue( pntIdx ) < level)
      {
      int status = donorGrid->ComputeStructuredCoordinates(
        this->Grid->GetPoint(pntIdx), ijk, pcoords);
      if (status == 1)
        {
        // Compute the cell index
        int cellIdx = vtkStructuredData::ComputeCellId(
          donorGrid->GetDimensions(),ijk,dataDescription);

        // Copy the data
        this->CopyData( this->Grid->GetPointData(), pntIdx,
          donorGrid->GetCellData(), cellIdx );

        // Upgrade donor level
        donorLevel->SetValue(pntIdx, level);

        data_changed = true;
        } // END if point inside grid
      } // END if the value at node is lower level
    } // END for all grid points

  if (data_changed)
    {
    // Mark all scalar arrays modified. This required since otherwise the volume
    // mappers may not realize that the scalars have changed, as well as the
    // scalar range returned by the array may be outdated.
    for (int cc=0; cc < this->Grid->GetPointData()->GetNumberOfArrays(); cc++)
      {
      vtkDataArray* array = this->Grid->GetPointData()->GetArray(cc);
      if (array)
        {
        array->Modified();
        }
      }
    this->Grid->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkAMRIncrementalResampleHelper::CopyData(
    vtkFieldData *target, vtkIdType targetIdx,
    vtkCellData *src, vtkIdType srcIdx )
{
  assert( "pre: target field data is NULL" && (target != NULL) );
  assert( "pre: source field data is NULL" && (src != NULL) );
  assert( "pre: number of arrays does not match" &&
           (target->GetNumberOfArrays() == (src->GetNumberOfArrays() + 1) ) );
  // the extra array is the  "DonorLevel" array.

  for (int arrayIdx=0; arrayIdx < src->GetNumberOfArrays(); ++arrayIdx)
    {
    vtkDataArray *targetArray = target->GetArray( arrayIdx );
    vtkDataArray *srcArray    = src->GetArray( arrayIdx );
    assert( "pre: target array is NULL!" && (targetArray != NULL) );
    assert( "pre: source array is NULL!" && (srcArray != NULL) );
    assert( "pre: targer/source array number of components mismatch!" &&
            (targetArray->GetNumberOfComponents()==
             srcArray->GetNumberOfComponents() ) );
    assert( "pre: target/source array names mismatch!" &&
            (strcmp(targetArray->GetName(),srcArray->GetName()) == 0) );
    assert( "pre: source index is out-of-bounds" &&
            (srcIdx >=0) &&
            (srcIdx < srcArray->GetNumberOfTuples() ) );
    assert( "pre: target index is out-of-bounds" &&
            (targetIdx >= 0) &&
            (targetIdx < targetArray->GetNumberOfTuples() ) );

    for (int c=0 ; c < srcArray->GetNumberOfComponents(); ++c )
      {
      double f = srcArray->GetComponent( srcIdx, c );
      targetArray->SetComponent( targetIdx, c, f );
      } // END for all componenents

    } // END for all arrays
}

//------------------------------------------------------------------------------
void vtkAMRIncrementalResampleHelper::ComputeExtent(
    vtkUniformGrid *G, double min[3], double max[3], int ext[6])
{
  assert("pre: input grid is NULL!" && (G != NULL) );

  int status;
  int ijkmin[3];
  int ijkmax[3];
  double pcoords[3];

  status = G->ComputeStructuredCoordinates(min,ijkmin,pcoords);
  assert("pre: min is outside grid bounds!" && (status == 1) );

  status = G->ComputeStructuredCoordinates(max,ijkmax,pcoords);
  assert("pre: max is outside grid bounds!" && (status == 1) );

  IMIN(ext) = ijkmin[0];
  JMIN(ext) = ijkmin[1];
  KMIN(ext) = ijkmin[2];
  IMAX(ext) = ijkmax[0];
  JMAX(ext) = ijkmax[1];
  KMAX(ext) = ijkmax[2];
}

//------------------------------------------------------------------------------
void vtkAMRIncrementalResampleHelper::Update()
{
  assert("pre: AMR metadata is NULL!" && (this->Metadata != NULL) );
  assert("pre: AMR data is NULL!" && (this->AMRData != NULL) );

  // NOTE: Shouldn't the update start here with the last level, dataIdx that
  // was processed?

  unsigned int compositeIdx = 0;
  unsigned int numLevels = this->AMRData->GetNumberOfLevels();
  for( unsigned int levelIdx=0; levelIdx < numLevels; ++levelIdx )
    {
    unsigned int numDataSets = this->AMRData->GetNumberOfDataSets(levelIdx);
    for(unsigned int dataIdx=0; dataIdx < numDataSets; ++dataIdx )
      {
      vtkUniformGrid *donorGrid = this->AMRData->GetDataSet(levelIdx,dataIdx);
      //int compositeIdx = this->Metadata->GetCompositeIndex(levelIdx,dataIdx);
      if( !this->HasBlockBeenProcessed( compositeIdx) && (donorGrid !=  NULL) )
        {
        this->TransferSolutionFromGrid( donorGrid, static_cast<int>(levelIdx));
        this->MarkBlockAsProcessed( compositeIdx );
        } // END if
      compositeIdx++;
      } // END for all datasets within level
    } // END for all levels



}

//------------------------------------------------------------------------------
bool vtkAMRIncrementalResampleHelper::WriteGrid(const char* filename)
{
  if (this->Grid && filename && filename[0])
    {
    vtkNew<vtkXMLImageDataWriter> imgWriter;
    imgWriter->SetFileName(filename);
    imgWriter->SetInputData(this->Grid);
    imgWriter->Write();
    return true;
    }
  return false;
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkAMRIncrementalResampleHelper::GetReferenceGrid()
{
  assert("pre: AMR data has not been set!" && (this->AMRData != NULL) );

  unsigned int numLevels = this->AMRData->GetNumberOfLevels();
  for( unsigned int level=0; level < numLevels; ++level )
    {
    unsigned int numDataSets = this->AMRData->GetNumberOfDataSets( level );
    for( unsigned int dataIdx=0; dataIdx < numDataSets; ++dataIdx )
      {
      vtkUniformGrid *refGrid = this->AMRData->GetDataSet(level,dataIdx);
      if( refGrid != NULL )
        {
        return( refGrid );
        }
      } // END for all datasets
    } // END for all levels

  // This process has no grids
  return NULL;
}

//------------------------------------------------------------------------------
void vtkAMRIncrementalResampleHelper::MarkBlockAsProcessed(const int blockIdx)
{
  this->ProcessedBlocks->insert(blockIdx);
}

//------------------------------------------------------------------------------
bool vtkAMRIncrementalResampleHelper::HasBlockBeenProcessed(const int blockIdx)
{
  return(this->ProcessedBlocks->find(blockIdx)!=this->ProcessedBlocks->end());
}

//------------------------------------------------------------------------------
void vtkAMRIncrementalResampleHelper::ClearProcessedBlocks()
{
  this->ProcessedBlocks->erase(
      this->ProcessedBlocks->begin(),this->ProcessedBlocks->end());
}
