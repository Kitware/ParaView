#include "vtkFloatArray.h"
#include "vtkSpyPlotBlock.h"
#include "vtkSpyPlotIStream.h"

#define MinBlockBound(i) this->XYZArrays[i]->GetTuple1(0)
#define MaxBlockBound(i)  \
this->XYZArrays[i]->GetTuple1(this->XYZArrays[i]->GetNumberOfTuples()-1)
#define coutVector6(x) (x)[0] << " " << (x)[1] << " " << (x)[2] << " " \
<< (x)[3] << " " << (x)[4] << " " << (x)[5]
#define coutVector3(x) (x)[0] << " " << (x)[1] << " " << (x)[2]


//-----------------------------------------------------------------------------
vtkSpyPlotBlock::vtkSpyPlotBlock() :
  Allocated(0), Active(0), Level(0)
{
  this->XYZArrays[0] = 0;
  this->XYZArrays[1] = 0;
  this->XYZArrays[2] = 0;
  this->DebugMode = 0;
}

//-----------------------------------------------------------------------------
vtkSpyPlotBlock::~vtkSpyPlotBlock()
{
  if (!this->Allocated)
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
  this->DebugMode = mode;
}

//-----------------------------------------------------------------------------
unsigned char vtkSpyPlotBlock::GetDebug() const
{
  return this->DebugMode;
}

//-----------------------------------------------------------------------------
const char *vtkSpyPlotBlock::GetClassName() const
{
  return "vtkSpyPlotBlock";;
}

//-----------------------------------------------------------------------------
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
void vtkSpyPlotBlock::GetRealBounds(double rbounds[6],
                                    int assumeAMR) const
{
  int i, j = 0;
  double minV, maxV;
  if (assumeAMR)
    {
    double spacing, maxVm, minV;
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
  if (!this->VectorsFixedForGhostCells) 
    {
    fixOffset = 1;
    }

  for (i = 0; i < 3; i++)
    {
    if (this->Dimensions[i] > 1)
      {
      rbounds[j++] = this->XYZArrays[i]->GetTuple1(fixOffset);
      rbounds[j++] = this->XYZArrays[i]->GetTuple1(this->Dimensions[i]+fixOffset-2);
      continue;
      }
    rbounds[j++] = 0;
    rbounds[j++] = 0;
    }
}
     
//-----------------------------------------------------------------------------
void vtkSpyPlotBlock::GetVectors(vtkDataArray *coordinates[3]) const
{
  coordinates[0] = this->XYZArrays[0];
  coordinates[1] = this->XYZArrays[1];
  coordinates[2] = this->XYZArrays[2];
}

//-----------------------------------------------------------------------------
int vtkSpyPlotBlock::GetAMRInformation(double globalBounds[6],
                                       int *level, 
                                       double spacing[3],
                                       double origin[3], 
                                       int extents[6],
                                       int realExtents[6], 
                                       int realDims[3]) const
{
  *level = this->Level;
  this->GetExtents(extents);
  int i, j, hasBadCells = 0;
 
  double minV, maxV;
  for (i = 0, j = 0; i < 3; i++, j++)
    {
    minV = MinBlockBound(i);
    maxV = MaxBlockBound(i);
    spacing[i] = (maxV  - minV) / this->Dimensions[i];
    
    if (this->Dimensions[i] == 1)
      {
      realExtents[j++] = 0;
      realExtents[j++] = 1;
      realDims[i] = 1;
      continue;
      }

    if (minV < globalBounds[j])
      {
      realExtents[j] = 1;
      origin[i] = minV + spacing[i];
      hasBadCells = 1;
      if (!this->VectorsFixedForGhostCells)
        {
        --extents[j+1];
        }
      }
    else 
      {
      realExtents[j] = 0;
      origin[i] = minV;
      }
    
    if (maxV > globalBounds[++j])
      {
      realExtents[j] = this->Dimensions[i] - 1;      
      hasBadCells = 1;
      if (!this->VectorsFixedForGhostCells)
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
int vtkSpyPlotBlock::FixInformation(double globalBounds[6],
                                    int extents[6],
                                    int realExtents[6], 
                                    int realDims[3],
                                    vtkDataArray *ca[3]
  )
{
  
  this->GetExtents(extents);

  int i, j, hasBadGhostCells = 0;
  int vectorsWereFixed = 0;
 
  /*cout  << "Dims: " << this->Dimensions[0] << " " 
    << this->Dimensions[1] << " " << this->Dimensions[2] << endl;
  */
  vtkDebugMacro( "Vectors for block: ");
  vtkDebugMacro( "  X: " << this->XYZArrays[0]->GetNumberOfTuples() );
  vtkDebugMacro( "  Y: " << this->XYZArrays[1]->GetNumberOfTuples() );
  vtkDebugMacro( "  Z: " << this->XYZArrays[2]->GetNumberOfTuples() );
  vtkDebugMacro( << __LINE__ << " BadGhostCells:" 
                 << coutVector6(this->RemovedBadGhostCells) );
  vtkDebugMacro( << __LINE__ << " Dims: " << coutVector3(this->Dimensions) );
  vtkDebugMacro( << __LINE__ << " Bool: " << this->VectorsFixedForGhostCells );

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
    if(this->RemovedBadGhostCells[j] || 
       (minV < globalBounds[j]) && !this->VectorsFixedForGhostCells)
      {
      realExtents[j]=1;
      --extents[j+1];
      hasBadGhostCells=1;
      this->RemovedBadGhostCells[j] = 1;
      if (!this->VectorsFixedForGhostCells)
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
    
    if(this->RemovedBadGhostCells[j] || 
       (maxV >globalBounds[j]) && !this->VectorsFixedForGhostCells)
      {
      realExtents[j] = this->Dimensions[i] - 1;      
      --extents[j];
      hasBadGhostCells=1;
      this->RemovedBadGhostCells[j] = 1;
      if (!this->VectorsFixedForGhostCells)
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
    this->VectorsFixedForGhostCells = 1;
    }
  vtkDebugMacro( << __LINE__ << " BadGhostCells:" 
                 << coutVector6(this->RemovedBadGhostCells) );
  return hasBadGhostCells;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotBlock::Read(vtkSpyPlotIStream *stream)
{

  // Read in the dimensions of the block
  if (!stream->ReadInt32s(this->Dimensions, 3))
    {
    vtkErrorMacro("Could not read in block's dimensions");
    return 0;
    }
  // Read in the allocation state of the block
  if (!stream->ReadInt32s(&(this->Allocated), 1))
    {
    vtkErrorMacro("Could not read in block's allocated state");
    return 0;
    }
  // Read in the active state of the block
  if (!stream->ReadInt32s(&(this->Active), 1))
    {
    vtkErrorMacro("Could not read in block's active state");
    return 0;
    }
  // Read in the level of the block
  if (!stream->ReadInt32s(&(this->Level), 1))
    {
    vtkErrorMacro("Could not read in block's level");
    return 0;
    }

  if ( this->Allocated )
    {
    this->XYZArrays[0] = vtkFloatArray::New();
    this->XYZArrays[0]->SetNumberOfTuples(this->Dimensions[0]+1);
    this->XYZArrays[1] = vtkFloatArray::New();
    this->XYZArrays[1]->SetNumberOfTuples(this->Dimensions[1]+1);
    this->XYZArrays[2] = vtkFloatArray::New();
    this->XYZArrays[2]->SetNumberOfTuples(this->Dimensions[2]+1);
    }
  else
    {
    this->XYZArrays[0] = 0;
    this->XYZArrays[1] = 0;
    this->XYZArrays[2] = 0;
    }
  // Clear the bad cell info
  this->RemovedBadGhostCells[0] = 0;
  this->RemovedBadGhostCells[1] = 0;
  this->RemovedBadGhostCells[2] = 0;
  this->RemovedBadGhostCells[3] = 0;
  this->RemovedBadGhostCells[4] = 0;
  this->RemovedBadGhostCells[5] = 0;
  this->VectorsFixedForGhostCells = 0;
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

 
