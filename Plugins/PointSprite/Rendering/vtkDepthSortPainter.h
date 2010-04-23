/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDepthSortPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkDepthSortPainter - encapsulates a vtkPolyDataDepthSort filter in a painter
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>
// .SECTION Description
// vtkDepthSortPainter encapsulates a vtkPolyDataDepthSort filter in a painter.
// If there is no transparency or if depth peeling is enabled, then this
// painter does nothing.
// This painter is useful with the point sprite painter
// to sort points when depth peeling is disabled.

#ifndef __vtkDepthSortPainter_h
#define __vtkDepthSortPainter_h


#include "vtkPainter.h"
#include "vtkWeakPointer.h"

class vtkFloatArray;
class vtkIdTypeArray;
class vtkMatrix4x4;
class vtkCamera;
class vtkPoints;
class vtkDataObject;
class vtkTexture;
class vtkDepthSortPolyData;
class vtkUnsignedCharArray;

class VTK_EXPORT vtkDepthSortPainter : public vtkPainter
{
public:
  vtkTypeMacro(vtkDepthSortPainter, vtkPainter);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkDepthSortPainter *New();

  //BTX
  // Description:
  // Enable or Disable depth sort.
  // 0 : ENABLE_SORT_ALWAYS forces the depth sort.
  // 1 : ENABLE_SORT_IF_NO_DEPTH_PEELING does the depth sort only if the renderer is not using depth peeling
  // 2 : ENABLE_SORT_NEVER only shallow copy  the input to the output, no sorting.
  enum { ENABLE_SORT_ALWAYS=0, ENABLE_SORT_IF_NO_DEPTH_PEELING=1, ENABLE_SORT_NEVER=2 };
  //ETX

  // Description:
  // Enable or Disable depth sort.
  // 0 : ENABLE_SORT_ALWAYS forces the depth sort.
  // 1 : ENABLE_SORT_IF_NO_DEPTH_PEELING does the depth sort only if the renderer is not using depth peeling
  // 2 : ENABLE_SORT_NEVER only shallow copy  the input to the output, no sorting.
  vtkSetMacro(DepthSortEnableMode, int);
  vtkGetMacro(DepthSortEnableMode, int);
  void  SetDepthSortEnableModeToAlways(){this->SetDepthSortEnableMode(ENABLE_SORT_ALWAYS);}
  void  SetDepthSortEnableModeToIfNoDepthPeeling(){this->SetDepthSortEnableMode(ENABLE_SORT_IF_NO_DEPTH_PEELING);}
  void  DepthSortEnableModeToNever(){this->SetDepthSortEnableMode(ENABLE_SORT_NEVER);}

  // Description:
  // Get the output data object from this painter.
  // If Enabled, the output, the points will be ordered by
  // their depth in the camera direction.
  // If disabled, this painter only shallow copies the input.
  virtual vtkDataObject* GetOutput();

  // Description:
  // this methods returns if this painter needs to
  // sort the dataset or not.
  // returns :
  // 1. false if the DepthSortEnableMode is ENABLE_SORT_NEVER or ENABLE_SORT_IF_NO_DEPTH_PEELING and the renderer uses depth peeling.
  // 2. true if the color array has an alpha component (this result id cached)
  // 3. false if the texture is either fully opaque or fully transparent (alpha = 0 or 255) (this result is cached)
  // 4. the result of actor->HasTranslucentPolygonalGeometry.
  virtual int   NeedSorting(vtkRenderer* renderer, vtkActor* actor);

  // Description:
  // Set/Get the internal vtkDepthSortPolyData algorithm
  // Rem : this painter will set the camera, prop3D and direction
  // before sorting.
  virtual void  SetDepthSortPolyData(vtkDepthSortPolyData*);
  vtkGetObjectMacro(DepthSortPolyData, vtkDepthSortPolyData);

protected:
  vtkDepthSortPainter();
  virtual ~vtkDepthSortPainter();

  // Description:
  // do the sorting for a given dataset
  virtual void Sort(vtkDataSet* output, vtkDataSet* input, vtkRenderer* renderer, vtkActor* actor);

  // Description:
  // Called just before RenderInternal(). We sort the points here if the
  // renderer's camera has been modified.
  virtual void PrepareForRendering(vtkRenderer* renderer, vtkActor* actor);

  // Description:
  // Set the output data.
  // Thisis called during the PrepareForRendering method to update the
  // OutputData ivar. This data is either the output of the DepthSortPolyData filter
  // if not NULL and NeedsSorting returns true, or a shallow copy of the input.
  virtual void  SetOutputData(vtkDataObject*);

  // Description:
  // This is a helper method that detects if the used texture is semi-translucent :
  // returns true only if there is a texture and this texture has alpha values
  // not equal to 0 or 255
  // This class caches the results so that this test is not done at each rendering.
  virtual int IsTextureSemiTranslucent(vtkTexture*);

  // Description:
  // This is a helper method that tells if the color has some opacity (alpha values != 0 and != 255)
  virtual int IsColorSemiTranslucent(vtkUnsignedCharArray* color);

  vtkDataObject*        OutputData;
  int                   DepthSortEnableMode;
  vtkTimeStamp          SortTime;
  int                   CachedIsTextureSemiTranslucent;
  vtkTimeStamp          CachedIsTextureSemiTranslucentTime;
  vtkTimeStamp          CachedIsColorSemiTranslucentTime;
  int                   CachedIsColorSemiTranslucent;
  vtkDepthSortPolyData* DepthSortPolyData;

  //BTX
  vtkWeakPointer<vtkDataObject> PrevInput;
  vtkWeakPointer<vtkTexture> CachedTexture;
  vtkWeakPointer<vtkUnsignedCharArray> CachedColors;
  //ETX

private:
  vtkDepthSortPainter(const vtkDepthSortPainter &);  // Not implemented.
  void operator=(const vtkDepthSortPainter &);  // Not implemented.
};

#endif //__vtkDepthSortPainter_h

