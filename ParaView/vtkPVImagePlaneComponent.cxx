/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImagePlaneComponent.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkPVImagePlaneComponent.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkImageClip.h"
#include "vtkExtractVOI.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkLookupTable.h"
#include "vtkTexture.h"

//------------------------------------------------------------------------------
vtkPVImagePlaneComponent* vtkPVImagePlaneComponent::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVImagePlaneComponent");
  if(ret)
    {
    return (vtkPVImagePlaneComponent*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVImagePlaneComponent;
}




//------------------------------------------------------------------------------
vtkPVImagePlaneComponent::vtkPVImagePlaneComponent()
{
  this->Input = NULL;
  this->Clip = vtkImageClip::New();
  this->Extract = vtkExtractVOI::New();
  this->Extract->SetInput(this->Clip->GetOutput());
  this->PlaneSource = vtkPlaneSource::New();
  this->PlaneSource->SetXResolution(1);
  this->PlaneSource->SetYResolution(1);
  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInput(this->PlaneSource->GetOutput());
  this->Texture = vtkTexture::New();
  this->Texture->SetInput(this->Extract->GetOutput());
  this->Texture->InterpolateOff();
  this->Texture->MapColorScalarsThroughLookupTableOn();
  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetTexture(this->Texture);
  this->Actor->VisibilityOff();
  
  this->PlaneExtent[0] = this->PlaneExtent[2] = 0;
  this->PlaneExtent[4] = this->PlaneExtent[5] = 0;
  this->PlaneExtent[1] = this->PlaneExtent[3] = -1;
  this->PlaneAxis = 2;
  
  this->Piece = 0;
  this->NumberOfPieces = 1;
  
  vtkLookupTable *satLut = vtkLookupTable::New();
  satLut->SetTableRange (0, 2000);
  satLut->SetHueRange (.6, .6);
  satLut->SetSaturationRange (0, 1);
  satLut->SetValueRange (1, 1);
  satLut->Build();

  this->SetLookupTable(satLut);
  satLut->Delete();
}

//------------------------------------------------------------------------------
vtkPVImagePlaneComponent::~vtkPVImagePlaneComponent()
{
  this->SetInput(NULL);
}

//------------------------------------------------------------------------------
void vtkPVImagePlaneComponent::SetInput(vtkImageData *input)
{
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }
  if (input)
    {
    input->Register(this);
    this->Input = input;
    }
  this->Clip->SetInput(input);
  
  this->Recompute();
}


//------------------------------------------------------------------------------
void vtkPVImagePlaneComponent::SetPlaneExtent(int xMin, int xMax, 
					      int yMin, int yMax, 
					      int zMin, int zMax)
{
  this->PlaneExtent[0] = xMin;
  this->PlaneExtent[1] = xMax;
  this->PlaneExtent[2] = yMin;
  this->PlaneExtent[3] = yMax;
  this->PlaneExtent[4] = zMin;
  this->PlaneExtent[5] = zMax;
  
  this->Recompute();
}


//------------------------------------------------------------------------------
void vtkPVImagePlaneComponent::SetLookupTable(vtkLookupTable *lut)
{
  this->Texture->SetLookupTable(lut);
}

//------------------------------------------------------------------------------
vtkLookupTable *vtkPVImagePlaneComponent::GetLookupTable()
{
  return this->Texture->GetLookupTable();
}


//------------------------------------------------------------------------------
void vtkPVImagePlaneComponent::SetPiece(int piece, int numPieces)
{
  this->Piece = piece;
  this->NumberOfPieces = numPieces;
}


//------------------------------------------------------------------------------
void vtkPVImagePlaneComponent::SetPosition(int val)
{
  int *wholeExt;
  
  this->Input->UpdateInformation();
  wholeExt = this->Input->GetWholeExtent();
  
  if (val < wholeExt[2*this->PlaneAxis])
    {
    val = wholeExt[2*this->PlaneAxis];
    }
  if (val > wholeExt[2*this->PlaneAxis+1])
    {
    val = wholeExt[2*this->PlaneAxis+1];
    }
  
  this->PlaneExtent[this->PlaneAxis*2] = val;
  this->PlaneExtent[this->PlaneAxis*2+1] = val;
  
  this->Recompute();
}


//------------------------------------------------------------------------------
// What we really need is something like an UpdateInformation method.
// We need to compute the extent covered by the piece, and intersect this with 
// the plane extent.  If they are non intersecting then the actor must be made
// invisible.
void vtkPVImagePlaneComponent::Recompute()
{
  int pieceExt[6];
  float x, y, z;
  float *spacing, *origin;
  
  if (this->Input == NULL)
    {
    return;
    }
  this->Input->UpdateInformation();
  origin = this->Input->GetOrigin();
  spacing = this->Input->GetSpacing();
  
  // Determine which axis is being displayed
  if (this->PlaneExtent[0] == this->PlaneExtent[1])
    {
    this->PlaneAxis = 0;
    }
  if (this->PlaneExtent[2] == this->PlaneExtent[3])
    {
    this->PlaneAxis = 1;
    }
  if (this->PlaneExtent[4] == this->PlaneExtent[5])
    {
    this->PlaneAxis = 2;
    }
  
  
  this->ComputePieceExtent(this->Piece, this->NumberOfPieces, 
			   this->Input->GetWholeExtent(), pieceExt);
  
  if (pieceExt[0] > this->PlaneExtent[1] || 
      pieceExt[1] < this->PlaneExtent[0] ||
      pieceExt[2] > this->PlaneExtent[3] ||
      pieceExt[3] < this->PlaneExtent[2] ||
      pieceExt[4] > this->PlaneExtent[5] ||
      pieceExt[5] < this->PlaneExtent[4])
    { // no overlap
    this->Actor->SetVisibility(0);
    return;
    }

  this->Actor->SetVisibility(1);
  
  // clip
  if (pieceExt[0] < this->PlaneExtent[0])
    {
    pieceExt[0] = this->PlaneExtent[0];
    }
  if (pieceExt[1] > this->PlaneExtent[1])
    {
    pieceExt[1] = this->PlaneExtent[1];
    }
  if (pieceExt[2] < this->PlaneExtent[2])
    {
    pieceExt[2] = this->PlaneExtent[2];
    }
  if (pieceExt[3] > this->PlaneExtent[3])
    {
    pieceExt[3] = this->PlaneExtent[3];
    }
  if (pieceExt[4] < this->PlaneExtent[4])
    {
    pieceExt[4] = this->PlaneExtent[4];
    }
  if (pieceExt[5] > this->PlaneExtent[5])
    {
    pieceExt[5] = this->PlaneExtent[5];
    }
  
  // Get the right data for the texture map.
  this->Extract->SetVOI(pieceExt);
  
  // Place the plane in the correct position.
  x = origin[0] + (float)(pieceExt[0]) * spacing[0];
  y = origin[1] + (float)(pieceExt[2]) * spacing[1];
  z = origin[2] + (float)(pieceExt[4]) * spacing[2];
  this->PlaneSource->SetOrigin(x, y, z);

  if (this->PlaneAxis == 0)
    {
    x = origin[0] + (float)(pieceExt[0]) * spacing[0];
    y = origin[1] + (float)(pieceExt[3]) * spacing[1];
    z = origin[2] + (float)(pieceExt[4]) * spacing[2];
    this->PlaneSource->SetPoint1(x, y, z);
    x = origin[0] + (float)(pieceExt[0]) * spacing[0];
    y = origin[1] + (float)(pieceExt[2]) * spacing[1];
    z = origin[2] + (float)(pieceExt[5]) * spacing[2];
    this->PlaneSource->SetPoint2(x, y, z);
    }
  if (this->PlaneAxis == 1)
    {
    x = origin[0] + (float)(pieceExt[1]) * spacing[0];
    y = origin[1] + (float)(pieceExt[2]) * spacing[1];
    z = origin[2] + (float)(pieceExt[4]) * spacing[2];
    this->PlaneSource->SetPoint1(x, y, z);
    x = origin[0] + (float)(pieceExt[0]) * spacing[0];
    y = origin[1] + (float)(pieceExt[2]) * spacing[1];
    z = origin[2] + (float)(pieceExt[5]) * spacing[2];
    this->PlaneSource->SetPoint2(x, y, z);
    }  
  if (this->PlaneAxis == 2)
    {
    x = origin[0] + (float)(pieceExt[1]) * spacing[0];
    y = origin[1] + (float)(pieceExt[2]) * spacing[1];
    z = origin[2] + (float)(pieceExt[4]) * spacing[2];
    this->PlaneSource->SetPoint1(x, y, z);
    x = origin[0] + (float)(pieceExt[0]) * spacing[0];
    y = origin[1] + (float)(pieceExt[3]) * spacing[1];
    z = origin[2] + (float)(pieceExt[4]) * spacing[2];
    this->PlaneSource->SetPoint2(x, y, z);
    }

  cerr << "PieceExt: " << pieceExt[0] << ", " << pieceExt[1] << ", " 
       << pieceExt[2] << ", " << pieceExt[3] << ", " 
       << pieceExt[4] << ", "  << pieceExt[5] << endl;
  
}


//------------------------------------------------------------------------------
// A separate object must be made to compute this extent.
void vtkPVImagePlaneComponent::ComputePieceExtent(int piece, int numPieces, 
						  int *wholeExt, int *pieceExt)
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    pieceExt[idx] = wholeExt[idx];
    }
  this->SplitExtent(piece, numPieces, pieceExt);
}

#define VTK_MINIMUM_PIECE_SIZE 4

//----------------------------------------------------------------------------
int vtkPVImagePlaneComponent::SplitExtent(int piece, int numPieces,
					  int *ext)
{
  int numPiecesInFirstHalf;
  int size[3], mid, splitAxis;

  // keep splitting until we have only one piece.
  // piece and numPieces will always be relative to the current ext. 
  while (numPieces > 1)
    {
    // Get the dimensions for each axis.
    size[0] = ext[1]-ext[0];
    size[1] = ext[3]-ext[2];
    size[2] = ext[5]-ext[4];
    // choose the biggest axis
    if (size[2] >= size[1] && size[2] >= size[0] && 
	size[2]/2 >= VTK_MINIMUM_PIECE_SIZE)
      {
      splitAxis = 2;
      }
    else if (size[1] >= size[0] && size[1]/2 >= VTK_MINIMUM_PIECE_SIZE)
      {
      splitAxis = 1;
      }
    else if (size[0]/2 >= VTK_MINIMUM_PIECE_SIZE)
      {
      splitAxis = 0;
      }
    else
      {
      // signal no more splits possible
      splitAxis = -1;
      }

    if (splitAxis == -1)
      {
      // can not split any more.
      if (piece == 0)
        {
        // just return the remaining piece
        numPieces = 1;
        }
      else
        {
        // the rest must be empty
        return 0;
        }
      }
    else
      {
      // split the chosen axis into two pieces.
      numPiecesInFirstHalf = (numPieces / 2);
      mid = (size[splitAxis] * numPiecesInFirstHalf / numPieces) 
	+ ext[splitAxis*2];
      if (piece < numPiecesInFirstHalf)
        {
        // piece is in the first half
        // set extent to the first half of the previous value.
        ext[splitAxis*2+1] = mid;
        // piece must adjust.
        numPieces = numPiecesInFirstHalf;
        }
      else
        {
        // piece is in the second half.
        // set the extent to be the second half. (two halves share points)
        ext[splitAxis*2] = mid;
        // piece must adjust
        numPieces = numPieces - numPiecesInFirstHalf;
        piece -= numPiecesInFirstHalf;
        }
      }
    } // end of while

  return 1;
}




