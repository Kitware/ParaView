/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImagePlaneComponent.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPVImagePlaneComponent - Extract an axis alligned plane.
// .SECTION Description
// This component extracts a plane from a volume and uses a textured
// plane to diplay the results.  It handels distributed data
// appropriately.

#ifndef __vtkPVImagePlaneComponent_h
#define __vtkPVImagePlaneComponent_h

#include "vtkImageData.h"
#include "vtkActor.h"
class vtkImageClip;
class vtkExtractVOI;
class vtkPlaneSource;
class vtkPolyDataMapper;

class VTK_EXPORT vtkPVImagePlaneComponent : public vtkObject
{
public:
  static vtkPVImagePlaneComponent* New();
  vtkTypeMacro(vtkPVImagePlaneComponent,vtkObject);

  // Description:
  // Set/Get the image/volume input.
  void SetInput(vtkImageData *input);
  vtkGetObjectMacro(Input, vtkImageData);

  // Description:
  // This is the extent of the plane that will be extracted from the volume.
  void SetPlaneExtent(int xMin, int xMax, int yMin, int yMax, int zMin, int zMax);
  vtkGetVector6Macro(PlaneExtent, int);
  
  // Description:
  // The lookup table for the texture.
  void SetLookupTable(vtkLookupTable *lut);
  vtkLookupTable *GetLookupTable();
  
  // Description:
  // Set the piece that this component will be processing.
  void SetPiece(int piece, int numPieces);
  
  // Description:
  // The texture mapped actor to put into the renderer.
  vtkGetObjectMacro(Actor, vtkActor);
  
  // Description:
  // Called when the scale widget is manipulated.
  void SetPosition(int val);
  
protected:
  vtkPVImagePlaneComponent();
  ~vtkPVImagePlaneComponent();
  vtkPVImagePlaneComponent(const vtkPVImagePlaneComponent&) {};
  void operator=(const vtkPVImagePlaneComponent&) {};

  // Description:
  // This recomputes the extent to display when something changes.
  void Recompute();
  void ComputePieceExtent(int piece, int numPieces, 
			  int *wholeExt, int *pieceExt);
  int SplitExtent(int piece, int numPieces, int *ext);
  
  vtkImageData *Input;
  int PlaneExtent[6];
  int PlaneAxis;
  int Piece;
  int NumberOfPieces;
  
  vtkImageClip *Clip;
  vtkExtractVOI *Extract;
  vtkTexture *Texture;
  vtkPlaneSource *PlaneSource;
  vtkPolyDataMapper *Mapper;
  vtkActor *Actor;
  
};


#endif


