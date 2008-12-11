#include "vtkFloatArray.h"
#include "vtkSpyPlotBlock.h"
#include "vtkSpyPlotIStream.h"
#include "vtkByteSwap.h"
#include "vtkBoundingBox.h"
#include <assert.h>

#define MinBlockBound(i) this->XYZArrays[i]->GetTuple1(0)
#define MaxBlockBound(i)  \
this->XYZArrays[i]->GetTuple1(this->XYZArrays[i]->GetNumberOfTuples()-1)


#define coutVector6(x) (x)[0] << " " << (x)[1] << " " << (x)[2] << " " \
<< (x)[3] << " " << (x)[4] << " " << (x)[5]
#define coutVector3(x) (x)[0] << " " << (x)[1] << " " << (x)[2]

//-----------------------------------------------------------------------------
vtkSpyPlotBlock::vtkSpyPlotBlock() :
  Level(0)
{
  this->Level = 0;
  this->XYZArrays[0]=this->XYZArrays[1]=this->XYZArrays[2]=NULL;
  this->Dimensions[0]=this->Dimensions[1]=this->Dimensions[2]=0;
  this->SavedExtents[0]=this->SavedExtents[2]=this->SavedExtents[4]=1;
  this->SavedExtents[1]=this->SavedExtents[3]=this->SavedExtents[5]=0;
  this->SavedRealExtents[0]=this->SavedRealExtents[2]=this->SavedRealExtents[4]=1;
  this->SavedRealExtents[1]=this->SavedRealExtents[3]=this->SavedRealExtents[5]=0;
  this->SavedRealDims[0]=this->SavedRealDims[2]=this->SavedRealDims[4]=1;
  this->SavedRealDims[1]=this->SavedRealDims[3]=this->SavedRealDims[5]=0;
  //
  this->Status.Active = 0;
  this->Status.Allocated = 0;
  this->Status.Fixed = 0;
  this->Status.Debug = 0;
  this->Status.AMR = 0;
}

//-----------------------------------------------------------------------------
vtkSpyPlotBlock::~vtkSpyPlotBlock()
{
  if (!this->IsAllocated()) 
    {
    return;
    }
  this->XYZArrays[0]->Delete();
  this->XYZArrays[1]->Delete();
  this->XYZArrays[2]->Delete();
}
  
  
//-----------------------------------------------------------------------------
void vtkSpyPlotBlock::SetDebug(unsigned char mode)
{
  if (mode)
    {
    this->Status.Debug = 1;
    }
  else 
    {
    this->Status.Debug = 0;
    }
}

//-----------------------------------------------------------------------------
unsigned char vtkSpyPlotBlock::GetDebug() const
{
  return this->Status.Debug;
}

//-----------------------------------------------------------------------------
const char *vtkSpyPlotBlock::GetClassName() const
{
  return "vtkSpyPlotBlock";
}

//-----------------------------------------------------------------------------
// world space
void vtkSpyPlotBlock::GetBounds(double bounds[6])const
{
  bounds[0] = MinBlockBound(0);
  bounds[1] = MaxBlockBound(0);
  bounds[2] = MinBlockBound(1);
  bounds[3] = MaxBlockBound(1);
  bounds[4] = MinBlockBound(2);
  bounds[5] = MaxBlockBound(2);
}

//-----------------------------------------------------------------------------
void vtkSpyPlotBlock::GetRealBounds(double rbounds[6]) const
{
  int i, j = 0;
  if (this->IsAMR())
    {
    double spacing, maxV, minV;
    for ( i = 0; i < 3; i++)
      {
      if (this->Dimensions[i] > 1)
        {
        minV = MinBlockBound(i);
        maxV = MaxBlockBound(i);
        spacing = (maxV - minV) / this->Dimensions[i];
        rbounds[j++] = minV + spacing;
        rbounds[j++] = maxV - spacing;
        continue;
        }
      rbounds[j++] = 0;
      rbounds[j++] = 0;
      }
    return;
    }

  // If the block has been fixed then the XYZArrays true size is dim - 2
  // (-2 represents the original endpoints being removed) - fixOffset is
  // used to correct for this
  int fixOffset = 0; 
  if (!this->IsFixed()) 
    {
    fixOffset = 1;
    }

  for (i = 0; i < 3; i++)
    {
    if (this->Dimensions[i] > 1)
      {
      rbounds[j++] = this->XYZArrays[i]->GetTuple1(fixOffset);
      rbounds[j++] =
        this->XYZArrays[i]->GetTuple1(this->Dimensions[i]+fixOffset-2);
      continue;
      }
    rbounds[j++] = 0;
    rbounds[j++] = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkSpyPlotBlock::GetSpacing(double spacing[3]) const
{
  double maxV, minV;
  for (int q=0; q<3; ++q)
    {
    minV = MinBlockBound(q);
    maxV = MaxBlockBound(q);
    spacing[q] = (maxV - minV) / this->Dimensions[q];
    }
}

     
//-----------------------------------------------------------------------------
void vtkSpyPlotBlock::GetVectors(vtkDataArray *coordinates[3]) const
{
  assert("Check Block is not AMR" && (!this->IsAMR()));
  coordinates[0] = this->XYZArrays[0];
  coordinates[1] = this->XYZArrays[1];
  coordinates[2] = this->XYZArrays[2];
}

//-----------------------------------------------------------------------------
int vtkSpyPlotBlock::GetAMRInformation(const vtkBoundingBox  &globalBounds,
                                       int *level, 
                                       double spacing[3],
                                       double origin[3], 
                                       int extents[6],
                                       int realExtents[6], 
                                       int realDims[3]) const
{
  assert("Check Block is AMR" && this->IsAMR());
  *level = this->Level;
  this->GetExtents(extents);
  int i, j, hasBadCells = 0;
  const double *minP = globalBounds.GetMinPoint();
  const double *maxP = globalBounds.GetMaxPoint();

  double minV, maxV;
  for (i = 0, j = 0; i < 3; i++, j++)
    {
    minV = MinBlockBound(i);
    maxV = MaxBlockBound(i);

    spacing[i] = (maxV  - minV) / this->Dimensions[i];

    if (this->Dimensions[i] == 1)
      {
      origin[i]=0.0;
      realExtents[j++] = 0;
      realExtents[j++] = 1; //!
      realDims[i] = 1;
      continue;
      }

    if (minV < minP[i])
      {
      realExtents[j] = 1;
      origin[i] = minV + spacing[i];
      hasBadCells = 1;
      if (!this->IsFixed())
        {
        --extents[j+1];
        }
      }
    else 
      {
      realExtents[j] = 0;
      origin[i] = minV;
      }
    ++j;
    if (maxV > maxP[i])
      {
      realExtents[j] = this->Dimensions[i] - 1;
      hasBadCells = 1;
      if (!this->IsFixed())
        {
        --extents[j];
        }
      }
    else
      {
      realExtents[j] = this->Dimensions[i];
      }
    realDims[i] = realExtents[j] - realExtents[j-1];
    }
  return hasBadCells;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotBlock::FixInformation(const vtkBoundingBox &globalBounds,
                                    int extents[6],
                                    int realExtents[6], 
                                    int realDims[3],
                                    vtkDataArray *ca[3]
  )
{
  // This better not be a AMR block!
  assert("Check Block is not AMR" && (!this->IsAMR()));
  const double *minP = globalBounds.GetMinPoint();
  const double *maxP = globalBounds.GetMaxPoint();

  this->GetExtents(extents);

  if (this->Status.Fixed)
    {
    //if a previous run through removed the invalid ghost cells,
    //reuse the extent and geometry information calculated then
    extents[0] = this->SavedExtents[0] ;
    extents[1] = this->SavedExtents[1] ;
    extents[2] = this->SavedExtents[2] ;
    extents[3] = this->SavedExtents[3] ;
    extents[4] = this->SavedExtents[4] ;
    extents[5] = this->SavedExtents[5] ;
    realExtents[0] = this->SavedRealExtents[0] ;
    realExtents[1] = this->SavedRealExtents[1] ;
    realExtents[2] = this->SavedRealExtents[2] ;
    realExtents[3] = this->SavedRealExtents[3] ;
    realExtents[4] = this->SavedRealExtents[4] ;
    realExtents[5] = this->SavedRealExtents[5] ;
    realDims[0] = this->SavedRealDims[0] ;
    realDims[1] = this->SavedRealDims[1] ;
    realDims[2] = this->SavedRealDims[2] ;

    for (int i = 0; i < 3; i++)
      {
      if (this->Dimensions[i] == 1)
        {
          ca[i] = 0;
          continue;
        }
      ca[i] = this->XYZArrays[i];
      }
    return 1;
    }
  

  int i, j, hasBadGhostCells = 0;
  int vectorsWereFixed = 0;
 
  vtkDebugMacro( "Vectors for block: ");
  vtkDebugMacro( "  X: " << this->XYZArrays[0]->GetNumberOfTuples() );
  vtkDebugMacro( "  Y: " << this->XYZArrays[1]->GetNumberOfTuples() );
  vtkDebugMacro( "  Z: " << this->XYZArrays[2]->GetNumberOfTuples() );
  vtkDebugMacro( << __LINE__ << " Dims: " << coutVector3(this->Dimensions) );
  vtkDebugMacro( << __LINE__ << " Bool: " << this->IsFixed() );

  double minV, maxV;
  for (i = 0, j = 0; i < 3; i++, j++)
    {
    if (this->Dimensions[i] == 1)
      {
      realExtents[j++] = 0;
      realExtents[j++] = 1;
      realDims[i] = 1;
      ca[i] = 0;
      continue;
      }

    minV = MinBlockBound(i);
    maxV = MaxBlockBound(i);
    vtkDebugMacro( "Bounds[" << (j) << "] = " << minV 
                   <<" Bounds[" << (j+1) << "] = " << maxV);
    ca[i] = this->XYZArrays[i];
    if (minV < minP[i])
      {
      realExtents[j]=1;
      --extents[j+1];
      hasBadGhostCells=1;
      if (!this->IsFixed())
        {
        this->XYZArrays[i]->RemoveFirstTuple();
        vectorsWereFixed = 1;
        }
      }
    else
      {
      realExtents[j]=0;
      }
    j++;
    
    if (maxV > maxP[i])
      {
      realExtents[j] = this->Dimensions[i] - 1;
      --extents[j];
      hasBadGhostCells=1;
      if (!this->IsFixed())
        {
        this->XYZArrays[i]->RemoveLastTuple();
        vectorsWereFixed = 1;
        }
      }
    else
      {
      realExtents[j] = this->Dimensions[i];
      }
    realDims[i] = realExtents[j] - realExtents[j-1];
    }
  if (vectorsWereFixed)
    {
    this->SavedExtents[0] = extents[0];
    this->SavedExtents[1] = extents[1];
    this->SavedExtents[2] = extents[2];
    this->SavedExtents[3] = extents[3];
    this->SavedExtents[4] = extents[4];
    this->SavedExtents[5] = extents[5];
    this->SavedRealExtents[0] = realExtents[0];
    this->SavedRealExtents[1] = realExtents[1];
    this->SavedRealExtents[2] = realExtents[2];
    this->SavedRealExtents[3] = realExtents[3];
    this->SavedRealExtents[4] = realExtents[4];
    this->SavedRealExtents[5] = realExtents[5];
    this->SavedRealDims[0] = realDims[0];
    this->SavedRealDims[1] = realDims[1];
    this->SavedRealDims[2] = realDims[2];
    this->Status.Fixed = 1;
    }
  return hasBadGhostCells;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotBlock::Read(int isAMR, int fileVersion, vtkSpyPlotIStream *stream)
{

  if (isAMR)
    {
    this->Status.AMR = 1;
    }
  else 
    {
    this->Status.AMR = 0;
    }

  int temp;
  // Read in the dimensions of the block
  if (!stream->ReadInt32s(this->Dimensions, 3))
    {
    vtkErrorMacro("Could not read in block's dimensions");
    return 0;
    }

  // Read in the allocation state of the block
  if (!stream->ReadInt32s(&temp, 1))
    {
    vtkErrorMacro("Could not read in block's allocated state");
    return 0;
    }
  if (temp)
    {
    this->Status.Allocated = 1;
    }
  else
    {
    this->Status.Allocated = 0;
    }

  // Read in the active state of the block
  if (!stream->ReadInt32s(&temp, 1))
    {
    vtkErrorMacro("Could not read in block's active state");
    return 0;
    }
  if (temp)
    {
    this->Status.Active = 1;
    }
  else
    {
    this->Status.Active = 0;
    }

  // Read in the level of the block
  if (!stream->ReadInt32s(&(this->Level), 1))
    {
    vtkErrorMacro("Could not read in block's level");
    return 0;
    }

  //read in bounds, but don't do anything with them
  if (fileVersion>=103)
    {
    int buffer[6];
    if (!stream->ReadInt32s(buffer, 6))
      {
      vtkErrorMacro("Could not read in block's bounding box");
      return 0;
      }
    }

  int i;
  if ( this->IsAllocated() )
    {
    for (i = 0; i < 3; i++)
      {
      if (!this->XYZArrays[i])
        {
        this->XYZArrays[i] = vtkFloatArray::New();
        }
      this->XYZArrays[i]->SetNumberOfTuples(this->Dimensions[i]+1);
      }
    }
  else 
    {
    for (i = 0; i < 3; i++)
      {
      if (this->XYZArrays[i])
        {
        this->XYZArrays[i]->Delete();
        this->XYZArrays[i] = 0;
        }
      }
    }
  this->Status.Fixed = 0;
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotBlock::Scan(vtkSpyPlotIStream *stream, 
                          unsigned char *isAllocated,
                          int fileVersion)
{

  int temp[3];
  // Read in the dimensions of the block
  if (!stream->ReadInt32s(temp, 3))
    {
    vtkGenericWarningMacro("Could not read in block's dimensions");
    return 0;
    }
  // Read in the allocation state of the block
  if (!stream->ReadInt32s(temp, 1))
    {
    vtkGenericWarningMacro("Could not read in block's allocated state");
    return 0;
    }
  if (temp[0])
    {
    *isAllocated = 1;
    }
  else 
    {
    *isAllocated = 0;
    }


  // Read in the active state of the block
  if (!stream->ReadInt32s(temp, 1))
    {
    vtkGenericWarningMacro("Could not read in block's active state");
    return 0;
    }
  
  // Read in the level of the block
  if (!stream->ReadInt32s(temp, 1))
    {
    vtkGenericWarningMacro("Could not read in block's level");
    return 0;
    }

  //read in bounds, but don't do anything with them
  if (fileVersion>=103)
    {
    int buffer[6];
    if (!stream->ReadInt32s(buffer, 6))
      {
      vtkGenericWarningMacro("Could not read in block's bounding box");
      return 0;
      }
    }

  return 1;
}

int vtkSpyPlotBlock::HasObserver(const char *) const
{
  return 0;
}

int vtkSpyPlotBlock::InvokeEvent(const char *, void *) const
{
  return 0;
}

 
//-----------------------------------------------------------------------------
/* SetGeometry - sets the geometric definition of the block's ith direction
   encodeInfo is a runlength delta encoded string of size infoSize

   DeltaRunLengthEncoded Description:
   run-length-encodes the data pointed to by *data, placing
   the result in *out. n is the number of doubles to encode. n_out
   is the number of bytes used for the compression (and stored at
   *out). delta is the smallest change of adjacent values that will
   be accepted (changes smaller than delta will be ignored). 

*/

int vtkSpyPlotBlock::SetGeometry(int dir,
                                 const unsigned char* encodedInfo, 
                                 int infoSize)
{
  int compIndex = 0, inIndex = 0;
  int compSize = this->Dimensions[dir] + 1;
  const unsigned char* ptmp = encodedInfo;

  /* Run-length decode */

  // Get first value
  float val;
  memcpy(&val, ptmp, sizeof(float));
  vtkByteSwap::SwapBE(&val);
  ptmp += 4;

  // Get delta
  float delta;
  memcpy(&delta, ptmp, sizeof(float));
  vtkByteSwap::SwapBE(&delta);
  ptmp += 4;


  if (!this->XYZArrays[dir])
    {
    vtkErrorMacro( "Coordinate Array has not been allocated" );
    return 0;
    }

  float *comp = this->XYZArrays[dir]->GetPointer(0);

  // Now loop around until I get to the end of
  // the input array
  inIndex += 8;
  while ((compIndex<compSize) && (inIndex<infoSize))
    {
    // Okay get the run length
    unsigned char runLength = *ptmp;
    ptmp ++;
    if (runLength < 128)
      {
      ptmp += 4;
      // Now populate the comp data
      int k;
      for (k=0; k<runLength; ++k)
        {
        if ( compIndex >= compSize )
          {
          vtkErrorMacro( "Problem doing RLD decode. "
                         << "Too much data generated. Excpected: " 
                         << compSize );
          return 0;
          }
        comp[compIndex] = val + compIndex*delta;
        compIndex++;
        }
      inIndex += 5;
      }
    else  // runLength >= 128
      {
      int k;
      for (k=0; k<runLength-128; ++k)
        {
        if ( compIndex >= compSize )
          {
          vtkErrorMacro( "Problem doing RLD decode. "
                         << "Too much data generated. Excpected: " 
                         << compSize );
          return 0;
          }
        float nval;
        memcpy(&nval, ptmp, sizeof(float));
        vtkByteSwap::SwapBE(&nval);
        comp[compIndex] = nval + compIndex*delta;
        compIndex++;
        ptmp += 4;
        }
      inIndex += 4*(runLength-128)+1;
      }
    } // while 

  return 1;
} 
