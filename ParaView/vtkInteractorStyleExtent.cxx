/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleExtent.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkInteractorStyleExtent.h"
#include "vtkPolyDataMapper.h"
#include "vtkOutlineSource.h"
#include "vtkMath.h" 
#include "vtkCellPicker.h"
#include "vtkSphereSource.h"

//----------------------------------------------------------------------------
void vtkInteractorStyleExtentCallback(void *arg)
{
  vtkInteractorStyleExtent *self = (vtkInteractorStyleExtent *)arg;

  self->DefaultCallback(self->GetCallbackType());
}

//----------------------------------------------------------------------------
vtkInteractorStyleExtent::vtkInteractorStyleExtent() 
{
  this->SphereSource = vtkSphereSource::New();
  this->SphereMapper = vtkPolyDataMapper::New();
  this->SphereMapper->SetInput(this->SphereSource->GetOutput());
  this->SphereActor = vtkActor::New();
  this->SphereActor->SetMapper(this->SphereMapper);

  this->Button = -1;
  this->ActiveCorner = -1;
  this->ExtentPtr0 = NULL;
  this->ExtentPtr1 = NULL;
  this->ExtentPtr2 = NULL;

  this->DisplayToExtentMatrix[0] = this->DisplayToExtentMatrixRow0;
  this->DisplayToExtentMatrix[1] = this->DisplayToExtentMatrixRow1;

  this->DisplayToExtentPermutation0 = 0;
  this->DisplayToExtentPermutation1 = 1;
  this->ExtentRemainder0 = 0.0;
  this->ExtentRemainder1 = 0.0;
  this->ExtentRemainder2 = 0.0;

  this->CallbackMethod = vtkInteractorStyleExtentCallback;
  this->CallbackMethodArg = (void *)this;
  this->CallbackMethodArgDelete = NULL;
  this->CallbackType = NULL;

  this->Extent[0] = this->Extent[1] = 0;
  this->Extent[2] = this->Extent[3] = 0;
  this->Extent[4] = this->Extent[5] = 0;

}

//----------------------------------------------------------------------------
vtkInteractorStyleExtent::~vtkInteractorStyleExtent() 
{
  // We must remove the actor from the renderer.
  if (this->ActiveCorner != -1 && this->CurrentRenderer)
    {
    this->CurrentRenderer->RemoveActor(this->SphereActor);
    }
  this->SphereActor->Delete();
  this->SphereActor = NULL;
  this->SphereMapper->Delete();
  this->SphereMapper = NULL;
  this->SphereSource->Delete();
  this->SphereSource = NULL;

  if ((this->CallbackMethodArg)&&(this->CallbackMethodArgDelete))
    {
    (*this->CallbackMethodArgDelete)(this->CallbackMethodArg);
    }
  this->SetCallbackType(NULL);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleExtent::SetExtent(int min0, int max0, int min1, 
                                         int max1, int min2, int max2)
{
  int *wholeExt = this->GetWholeExtent();
  int changedFlag = 0;

  if (wholeExt != NULL)
    {
    if (min0 < wholeExt[0])
      {
      min0 = wholeExt[0];
      }
    if (max0 > wholeExt[1])
      {
      max0 = wholeExt[1];
      }
    if (min1 < wholeExt[2])
      {
      min1 = wholeExt[2];
      }
    if (max1 > wholeExt[3])
      {
      max1 = wholeExt[3];
      }
    if (min2 < wholeExt[4])
      {
      min2 = wholeExt[4];
      }
    if (max2 > wholeExt[5])
      {
      max2 = wholeExt[5];
      }
    }

  if (this->Extent[0] != min0)
    {
    this->Extent[0] = min0;
    changedFlag = 1;
    }
  if (this->Extent[1] != max0)
    {
    this->Extent[1] = max0;
    changedFlag = 1;
    }
  if (this->Extent[2] != min1)
    {
    this->Extent[2] = min1;
    changedFlag = 1;
    }
  if (this->Extent[3] != max1)
    {
    this->Extent[3] = max1;
    changedFlag = 1;
    }
  if (this->Extent[4] != min2)
    {
    this->Extent[4] = min2;
    changedFlag = 1;
    }
  if (this->Extent[5] != max2)
    {
    this->Extent[5] = max2;
    changedFlag = 1;
    }
  
  if (changedFlag)
    {
    this->Modified();
    // Not exactly the callback tytpe I was looking for ...
    // It should be changed to something else.
    this->SetCallbackType("InteractiveRender");
    (*this->CallbackMethod)(this->CallbackMethodArg);
    }

}

//----------------------------------------------------------------------------
void vtkInteractorStyleExtent::SetCallbackMethod(void (*f)(void *), void *arg)
{
  if (f == NULL)
    {
    vtkErrorMacro("A valid callback must be provided");
    return;
    }

  if ( f != this->CallbackMethod || arg != this->CallbackMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->CallbackMethodArg)&&(this->CallbackMethodArgDelete))
      {
      (*this->CallbackMethodArgDelete)(this->CallbackMethodArg);
      }
    this->CallbackMethod = f;
    this->CallbackMethodArg = arg;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleExtent::SetCallbackMethodArgDelete(void (*f)(void *))
{
  if ( f != this->CallbackMethodArgDelete)
    {
    this->CallbackMethodArgDelete = f;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleExtent::DefaultCallback(char *type)
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  // Handle hot spot.  Show a sphere when a sopt is active.
  if (strcmp(type, "ActiveSpotChanged") == 0)
    {
    if (this->OldSpotId == -1)
      {
      this->CurrentRenderer->AddActor(this->SphereActor);
      }
    if (this->CurrentSpotId == -1)
      {
      this->CurrentRenderer->RemoveActor(this->SphereActor);
      }
    this->SphereActor->SetPosition(this->CurrentSpot);
    this->SphereSource->SetRadius(1.0);
    this->CurrentRenderer->GetRenderWindow()->Render();
    }

  if (strcmp(type, "InteractiveRender") == 0)
    {
    this->CurrentRenderer->GetRenderWindow()->Render();
    }
}


//----------------------------------------------------------------------------
void vtkInteractorStyleExtent::OnMouseMove(int vtkNotUsed(ctrl), 
					  int vtkNotUsed(shift),
					  int x, int y) 
{
  if (this->Button == -1)
    {
    this->HandleIndicator(x, y);
    }

  if (this->Button == 0 && this->CurrentSpotId != -1)
    {
    this->TranslateXY(x - this->LastPos[0], y - this->LastPos[1]);
    }
  if (this->Button == 1 && this->CurrentSpotId != -1)
    {
    this->SetCallbackType("InteractiveRender");
    (*this->CallbackMethod)(this);
    }
  if (this->Button == 2 && this->CurrentSpotId != -1)
    {
    this->SetCallbackType("InteractiveRender");
    (*this->CallbackMethod)(this);
    }

  this->LastPos[0] = x;
  this->LastPos[1] = y;
}

//--------------------------------------------------------------------------
// This assumes the grid is up to date.
void vtkInteractorStyleExtent::TranslateXY(int dx, int dy)
{
  double de[3];
  double t0, t1;
  int changeFlag = 0;
  int *ext, temp;
  
  // change dx, dy into de[3] in extent coordinates.
  de[0] = de[1] = de[2] = 0.0;
  t0 = this->DisplayToExtentMatrix[0][0] * dx + 
        this->DisplayToExtentMatrix[0][1] * dy;
  t1 = this->DisplayToExtentMatrix[1][0] * dx + 
        this->DisplayToExtentMatrix[1][1] * dy;
  de[this->DisplayToExtentPermutation0] = t0;
  de[this->DisplayToExtentPermutation1] = t1;

  //vtkErrorMacro("ExtentRemainder: " << this->ExtentRemainder0 << ", "
  //     << this->ExtentRemainder1 << ", " << this->ExtentRemainder2);
  //vtkErrorMacro("ExtentDelta: " << de[0] << ", " << de[1] << ", " << de[2]);

  // Add this to the current extent remainder.
  this->ExtentRemainder0 += de[0];
  this->ExtentRemainder1 += de[1];
  this->ExtentRemainder2 += de[2];

  ext = this->GetWholeExtent();

  // See if we have moved to another grid point.
  if (this->ExtentRemainder0 > 0.5 && *this->ExtentPtr0 < ext[1])
    {
    *(this->ExtentPtr0) += 1.0;
    this->ExtentRemainder0 -= 1.0;
    changeFlag = 1;
    }
  if (this->ExtentRemainder0 < -0.5 && *this->ExtentPtr0 > ext[0])
    {
    *(this->ExtentPtr0) -= 1.0;
    this->ExtentRemainder0 += 1.0;
    changeFlag = 1;
    }

  if (this->ExtentRemainder1 > 0.5 && *this->ExtentPtr1 < ext[3])
    {
    *(this->ExtentPtr1) += 1.0;
    this->ExtentRemainder1 -= 1.0;
    changeFlag = 1;
    }
  if (this->ExtentRemainder1 < -0.5 && *this->ExtentPtr1 > ext[2])
    {
    *(this->ExtentPtr1) -= 1.0;
    this->ExtentRemainder1 += 1.0;
    changeFlag = 1;
    }

  if (this->ExtentRemainder2 > 0.5 && *this->ExtentPtr2 < ext[5])
    {
    *(this->ExtentPtr2) += 1.0;
    this->ExtentRemainder2 -= 1.0;
    changeFlag = 1;
    }
  if (this->ExtentRemainder2 < -0.5 && *this->ExtentPtr2 > ext[4])
    {
    *(this->ExtentPtr2) -= 1.0;
    this->ExtentRemainder2 += 1.0;
    changeFlag = 1;
    }

  
  // Now we check to see if any min > max...
  if (changeFlag)
    {
    //vtkErrorMacro("Extent: " << this->Extent[0] << ", " << this->Extent[1] << ", "
    //     << this->Extent[2] << ", " << this->Extent[3] << ", "
    //     << this->Extent[4] << ", " << this->Extent[5]);
    //vtkErrorMacro("Corner = " << *(this->ExtentPtr0) << ", "
    //     << *(this->ExtentPtr1) << ", " << *(this->ExtentPtr2)); 
    if (this->Extent[0] > this->Extent[1])
      {
      // Swap min max values.
      temp = this->Extent[0];
      this->Extent[0] = this->Extent[1];
      this->Extent[1] = temp;
      if (this->ExtentPtr0 == &(this->Extent[0]))
        { // min has moved to max.
        this->ExtentPtr0 = &(this->Extent[1]);
        this->CurrentSpotId += 1;
        }
      else
        { // max has moved to min.
        this->ExtentPtr0 = &(this->Extent[0]);
        this->CurrentSpotId -= 1;
        }
      }
    if (this->Extent[2] > this->Extent[3])
      {
      // Swap min max values.
      temp = this->Extent[2];
      this->Extent[2] = this->Extent[3];
      this->Extent[3] = temp;
      if (this->ExtentPtr1 == &(this->Extent[2]))
        { // min has moved to max.
        this->ExtentPtr1 = &(this->Extent[3]);
        this->CurrentSpotId += 2;
        }
      else
        { // max has moved to min.
        this->ExtentPtr1 = &(this->Extent[2]);
        this->CurrentSpotId -= 2;
        }
      }
    if (this->Extent[4] > this->Extent[5])
      {
      // Swap min max values.
      temp = this->Extent[4];
      this->Extent[4] = this->Extent[5];
      this->Extent[5] = temp;
      if (this->ExtentPtr2 == &(this->Extent[4]))
        { // min has moved to max.
        this->ExtentPtr2 = &(this->Extent[5]);
        this->CurrentSpotId += 4;
        }
      else
        { // max has moved to min.
        this->ExtentPtr2 = &(this->Extent[4]);
        this->CurrentSpotId -= 4;
        }
      }
    this->GetWorldSpot(this->CurrentSpotId, this->CurrentSpot);
    this->SphereActor->SetPosition(this->CurrentSpot);

    this->ComputeDisplayToExtentMapping();
    this->SetCallbackType("InteractiveRender");
    (*this->CallbackMethod)(this->CallbackMethodArg);
    }
}

//--------------------------------------------------------------------------
// This assumes the grid is up to date.
void vtkInteractorStyleExtent::ComputeDisplayToExtentMapping()
{
  int id;
  double v0[3], v1[3], v2[3];
  double m0, m1, m2;
  double dOrigin[4], d0[4], d1[4], d2[4];
  // For matrix inversion.
  double *extentToDisplayMatrix[2];

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  // Pointers used to manipulate the active extent corner.
  id = this->CurrentSpotId;
  this->ExtentPtr0 = &(this->Extent[0]);
  this->ExtentPtr1 = &(this->Extent[2]);
  this->ExtentPtr2 = &(this->Extent[4]);
  if (id >= 4) 
    {
    this->ExtentPtr2 = &(this->Extent[5]);
    id -= 4;
    }
  if (id >= 2) 
    {
    this->ExtentPtr1 = &(this->Extent[3]);
    id -= 2;
    }
  if (id >= 1) 
    {
    this->ExtentPtr0 = &(this->Extent[1]);
    id -= 1;
    }

  // Get the three axes.
  this->GetSpotAxes(this->CurrentSpotId, v0, v1, v2);

  //vtkErrorMacro("v0 : " << v0[0] << ", " << v0[1] << ", " << v0[2]);
  //vtkErrorMacro("v1 : " << v1[0] << ", " << v1[1] << ", " << v1[2]);
  //vtkErrorMacro("v2 : " << v2[0] << ", " << v2[1] << ", " << v2[2]);

  // Magnitudes for normalization.
  m0 = sqrt(v0[0]*v0[0] + v0[1]*v0[1] + v0[2]*v0[2]);
  m1 = sqrt(v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2]);
  m2 = sqrt(v2[0]*v2[0] + v2[1]*v2[1] + v2[2]*v2[2]);

  //vtkErrorMacro("mag: " << m0 << ", " << m1 << ", " << m2);
  float *pt;

  // Now pick the two axes the are most parallel with the screen.
  // Find the spot in display coords (origin).  I doubt that the
  // Z buffer transformation is linear, so normalize the axes, and
  // put the scale back in for X and Y in display.  d0,1,2 are
  // the axes in display coordinates.

  // We need the relative transformation at the spot.  
  // Add it in world coords, and take it out in display.
  this->CurrentRenderer->SetWorldPoint(this->CurrentSpot[0], 
                   this->CurrentSpot[1], this->CurrentSpot[2], 1.0);
  pt = this->CurrentRenderer->GetWorldPoint();
  //vtkErrorMacro("world origin: " << pt[0] << ", " << pt[1] << ", " << pt[2] << ", " << pt[3]);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(dOrigin);

  // Try axis 0.
  this->CurrentRenderer->SetWorldPoint(this->CurrentSpot[0]+(v0[0]/m0), 
		     this->CurrentSpot[1]+(v0[1]/m0), 
		     this->CurrentSpot[2]+(v0[2]/m0), 1.0);
  pt = this->CurrentRenderer->GetWorldPoint();
  //vtkErrorMacro("world p0: " << pt[0] << ", " << pt[1] << ", " << pt[2] << ", " << pt[3]);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(d0);
  //vtkErrorMacro("display p0 : " << d0[0] << ", " << d0[1] << ", " << d0[2]);
  d0[0] = (d0[0] - dOrigin[0]) * m0;
  d0[1] = (d0[1] - dOrigin[1]) * m0;
  d0[2] = (d0[2] - dOrigin[2]) * m0;
  // The z value will be used to sort the axis.
  d0[2] = d0[2] * d0[2];
  //vtkErrorMacro("d0 : " << d0[0] << ", " << d0[1] << ", " << d0[2]);

  // Try axis 1.
  this->CurrentRenderer->SetWorldPoint(this->CurrentSpot[0]+(v1[0]/m1), 
		     this->CurrentSpot[1]+(v1[1]/m1), 
		     this->CurrentSpot[2]+(v1[2]/m1), 1.0);
  //vtkErrorMacro("d0 : " << d0[0] << ", " << d0[1] << ", " << d0[2]);

  pt = this->CurrentRenderer->GetWorldPoint();
  //vtkErrorMacro("world p1: " << pt[0] << ", " << pt[1] << ", " << pt[2] << ", " << pt[3]);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(d1);
  //vtkErrorMacro("display p1 : " << d1[0] << ", " << d1[1] << ", " << d1[2]);
  d1[0] = (d1[0] - dOrigin[0]) * m1;
  d1[1] = (d1[1] - dOrigin[1]) * m1;
  d1[2] = (d1[2] - dOrigin[2]) * m1;
  // The z value will be used to sort the axis.
  d1[2] = d1[2] * d1[2];
  //vtkErrorMacro("d1 : " << d1[0] << ", " << d1[1] << ", " << d1[2]);

  // Try axis 2.
  this->CurrentRenderer->SetWorldPoint(this->CurrentSpot[0]+(v2[0]/m2), 
		     this->CurrentSpot[1]+(v2[1]/m2), 
		     this->CurrentSpot[2]+(v2[2]/m2), 1.0);
  pt = this->CurrentRenderer->GetWorldPoint();
  //vtkErrorMacro("world p2: " << pt[0] << ", " << pt[1] << ", " << pt[2] << ", " << pt[3]);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(d2);
  //vtkErrorMacro("display p2 : " << d2[0] << ", " << d2[1] << ", " << d2[2]);
  d2[0] = (d2[0] - dOrigin[0]) * m2;
  d2[1] = (d2[1] - dOrigin[1]) * m2;
  d2[2] = (d2[2] - dOrigin[2]) * m2;
  // The z value will be used to sort the axis.
  d2[2] = d2[2] * d2[2];
  //vtkErrorMacro("d2 : " << d2[0] << ", " << d2[1] << ", " << d2[2]);

  // Now take the inverse of the (extent->display)
  // to get (display->extent).  This will be used for interaction.  
  // Note: Constant values are not important because computation
  // is relative to last position (dx, dy, this->Extent0, this->Extent1).

  // Now we brute force sort to find the best two axes.  Save the
  // axis permutation (display xy to extentXYZ (subspace)).
  if (d0[2] <= d2[2] && d1[2] <= d2[2])
    { // XY
    extentToDisplayMatrix[0] = d0;   
    extentToDisplayMatrix[1] = d1;
    this->DisplayToExtentPermutation0 = 0;
    this->DisplayToExtentPermutation1 = 1;
    }
  else if (d0[2] <= d1[2] && d2[2] <= d1[2])
    { // XZ
    extentToDisplayMatrix[0] = d0;   
    extentToDisplayMatrix[1] = d2;
    this->DisplayToExtentPermutation0 = 0;
    this->DisplayToExtentPermutation1 = 2;
    }
  else if (d1[2] <= d0[2] && d2[2] <= d0[2])
    { // YZ
    extentToDisplayMatrix[0] = d1;   
    extentToDisplayMatrix[1] = d2;
    this->DisplayToExtentPermutation0 = 1;
    this->DisplayToExtentPermutation1 = 2;
    }

  if (vtkMath::InvertMatrix(extentToDisplayMatrix, 
                            this->DisplayToExtentMatrix, 2) == 0)
    { // This should not happen (Unless bounding box has zero volume).
    vtkErrorMacro("Could not invert matrix");
    }
  //vtkErrorMacro("Mapping Permute : " << this->DisplayToExtentPermutation0
  //    << ", " << this->DisplayToExtentPermutation1);
  //vtkErrorMacro("Mapping Matrix, row1: " 
  //    << this->DisplayToExtentMatrix[0][0] << ", " 
  //    << this->DisplayToExtentMatrix[0][1] << ", row2: " 
  //    << this->DisplayToExtentMatrix[1][0] << ", " 
  //    << this->DisplayToExtentMatrix[1][1]); 

}



//----------------------------------------------------------------------------
// This method handles display of hot spots.
// When the mouse is passively being moved over objects, this will
// highlight an object to indicate that it can be manipulated with the mouse.
void vtkInteractorStyleExtent::HandleIndicator(int x, int y) 
{
  int spotId, numSpots, bestSpotId;
  float spot[4], bestSpot[4];
  float display[3], point[4];
  float temp, rad, dist2, bestDist2;
  int *size;

  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  numSpots = 8;
  bestDist2 = VTK_LARGE_FLOAT;
  bestSpotId = -1;
  for (spotId = 0; spotId < numSpots; ++spotId)
    {
    this->GetWorldSpot(spotId, spot);
    spot[3] = 1.0;
    this->CurrentRenderer->SetWorldPoint(spot);
    this->CurrentRenderer->WorldToDisplay();
    this->CurrentRenderer->GetDisplayPoint(display);
    temp = (float)x - display[0];
    dist2 = temp * temp;
    temp = (float)y - display[1];
    dist2 += temp * temp;

    if (dist2 < 300.0)
      {
      if (dist2 < bestDist2)
	{
	bestDist2 = dist2;
	bestSpotId = spotId;
	bestSpot[0] = spot[0];
	bestSpot[1] = spot[1];
	bestSpot[2] = spot[2];
	}
      }
    }

  if (this->CurrentSpotId != bestSpotId)
    {
    // We need to keep the old spot Id for the call back ...
    this->OldSpotId = this->CurrentSpotId;
    this->CurrentSpotId = bestSpotId;
    this->CurrentSpot[0] = bestSpot[0];
    this->CurrentSpot[1] = bestSpot[1];
    this->CurrentSpot[2] = bestSpot[2];

    // Compute the size of the sphere 
    // as a fraction of the renderers size.
    if (bestSpotId != -1)
      {
	bestSpot[3] = 1.0;
	this->CurrentRenderer->SetWorldPoint(bestSpot);
	this->CurrentRenderer->WorldToDisplay();
	this->CurrentRenderer->GetDisplayPoint(display);
	size = this->CurrentRenderer->GetSize();
	display[0] += (float)(size[0] + size[1]) / 50.0;
	this->CurrentRenderer->SetDisplayPoint(display);
	this->CurrentRenderer->DisplayToWorld();
	this->CurrentRenderer->GetWorldPoint(point);
	temp = bestSpot[0] - point[0];
	rad = temp * temp;
	temp = bestSpot[1] - point[1];
	rad += temp * temp;
	temp = bestSpot[2] - point[2];
	rad += temp * temp;
	rad = sqrt(rad);
	this->SphereActor->SetScale(rad);
      }

    this->SetCallbackType("ActiveSpotChanged");
    (*this->CallbackMethod)(this->CallbackMethodArg);
    }
}


//----------------------------------------------------------------------------
void vtkInteractorStyleExtent::OnLeftButtonDown(int ctrl, int shift, 
                                                int x, int y) 
{
  this->UpdateInternalState(ctrl, shift, x, y);

  if (this->CurrentSpotId == -1)
    {
    this->Button = -2;
    return;
    }

  this->FindPokedCamera(x, y);

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  this->Button = 0;
  this->ExtentRemainder0 = 0.0;
  this->ExtentRemainder1 = 0.0;
  this->ExtentRemainder2 = 0.0;


  this->ComputeDisplayToExtentMapping();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleExtent::OnLeftButtonUp(int ctrl, int shift, int X, int Y) 
{
  this->UpdateInternalState(ctrl, shift, X, Y);
  
  this->Button = -1;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleExtent::OnMiddleButtonDown(int ctrl, int shift, 
                                                  int X, int Y) 
{
  //
  this->UpdateInternalState(ctrl, shift, X, Y);
  //
  this->FindPokedCamera(X, Y);
  //
  if (this->MiddleButtonPressMethod) 
    {
    (*this->MiddleButtonPressMethod)(this->MiddleButtonPressMethodArg);
    }
  else 
    {
    }
}
//----------------------------------------------------------------------------
void vtkInteractorStyleExtent::OnMiddleButtonUp(int ctrl, int shift, 
                                                int X, int Y) 
{
  //
 this->UpdateInternalState(ctrl, shift, X, Y);
  //
  if (this->MiddleButtonReleaseMethod) 
    {
    (*this->MiddleButtonReleaseMethod)(this->MiddleButtonReleaseMethodArg);
    }
  else 
    {
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleExtent::OnRightButtonDown(int ctrl, int shift, 
                                                 int X, int Y) 
{
  //
 this->UpdateInternalState(ctrl, shift, X, Y);
  //
 this->FindPokedCamera(X, Y);
  if (this->RightButtonPressMethod) 
    {
    (*this->RightButtonPressMethod)(this->RightButtonPressMethodArg);
    }
  else 
    {
    }
}







//----------------------------------------------------------------------------
void vtkInteractorStyleExtent::OnRightButtonUp(int ctrl, int shift, int X, int Y) 
{
  //
 this->UpdateInternalState(ctrl, shift, X, Y);
  //
  if (this->RightButtonReleaseMethod) 
    {
    (*this->RightButtonReleaseMethod)(this->RightButtonReleaseMethodArg);
    }
  else 
    {
    }
}




//----------------------------------------------------------------------------
void vtkInteractorStyleExtent::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkInteractorStyle::PrintSelf(os,indent);

  os << indent << "CallbackType: " << this->CallbackType << endl;

  if ( this->CallbackMethod )
    {
    os << indent << "Callback Method defined\n";
    }
  else
    {
    os << indent <<"No Callback Method\n";
    }
}
