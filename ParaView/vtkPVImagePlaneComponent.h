/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImagePlaneComponent.h
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


