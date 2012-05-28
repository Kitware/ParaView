/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageSliceMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVImageSliceMapper - Mapper for vtkImageData that renders the image
// using a texture applied to a quad.
// .SECTION Description
// vtkPVImageSliceMapper is a mapper for vtkImageData that renders the image by
// loading the image as a texture and then applying it to a quad. For 3D images,
// this mapper only shows a single Z slice which can be choosen using SetZSlice.
// By default, the image data scalars are rendering, however, this mapper
// provides API to select another point or cell data array. Internally, this
// mapper uses painters similar to those employed by vtkPainterPolyDataMapper.
// .SECTION See Also
// vtkPainterPolyDataMapper


#ifndef __vtkPVImageSliceMapper_h
#define __vtkPVImageSliceMapper_h

#include "vtkMapper.h"
#include "vtkStructuredData.h" // needed for VTK_*_PLANE

class vtkImageData;
class vtkRenderer;
class vtkPainter;

class VTK_EXPORT vtkPVImageSliceMapper : public vtkMapper
{
public:
  static vtkPVImageSliceMapper* New();
  vtkTypeMacro(vtkPVImageSliceMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This calls RenderPiece (in a for loop is streaming is necessary).
  virtual void Render(vtkRenderer *ren, vtkActor *act);

  virtual void ReleaseGraphicsResources (vtkWindow *);

  // Description:
  // Get/Set the painter that does the actual rendering.
  void SetPainter(vtkPainter*);
  vtkGetObjectMacro(Painter, vtkPainter);

  // Description:
  // Specify the input data to map.
  void SetInputData(vtkImageData *in);
  virtual vtkImageData *GetInput();

  // Description:
  // Set/Get the current X/Y or Z slice number. 
  vtkSetMacro(Slice,int);
  vtkGetMacro(Slice,int);

  //BTX
  enum 
    {
    XY_PLANE = VTK_XY_PLANE,
    YZ_PLANE = VTK_YZ_PLANE,
    XZ_PLANE = VTK_XZ_PLANE,
    };
  //ETX

  vtkSetClampMacro(SliceMode, int, XY_PLANE, XZ_PLANE);
  vtkGetMacro(SliceMode, int);
  void SetSliceModeToYZPlane()
    { this->SetSliceMode(YZ_PLANE); }
  void SetSliceModeToXZPlane()
    { this->SetSliceMode(XZ_PLANE); }
  void SetSliceModeToXYPlane()
    { this->SetSliceMode(XY_PLANE); }

  // Description:
  // When set, the image slice is always rendered in the XY plane (Z==0)
  // irrespective of the image bounds. Default if Off.
  vtkSetClampMacro(UseXYPlane, int, 0, 1);
  vtkBooleanMacro(UseXYPlane, int);
  vtkGetMacro(UseXYPlane, int);
  
  // Description:
  // Update that sets the update piece first.
  virtual void Update(int port);
  virtual void Update()
    { this->Superclass::Update(); }

  // Description:
  // If you want only a part of the data, specify by setting the piece.
  vtkSetMacro(Piece, int);
  vtkGetMacro(Piece, int);
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  vtkSetMacro(NumberOfSubPieces, int);
  vtkGetMacro(NumberOfSubPieces, int);

  // Description:
  // Set the number of ghost cells to return.
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);

  // Description:
  // Return bounding box (array of six doubles) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  virtual double *GetBounds();
  virtual void GetBounds(double bounds[6]) 
    {this->Superclass::GetBounds(bounds);};
  
  // Description:
  // Make a shallow copy of this mapper.
  virtual void ShallowCopy(vtkAbstractMapper *m);


//BTX
protected:
  vtkPVImageSliceMapper();
  ~vtkPVImageSliceMapper();

  // Tell the executive that we accept vtkImageData.
  virtual int FillInputPortInformation(int, vtkInformation*);

  // Description:
  // Perform the actual rendering.
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Called when the PainterInformation becomes obsolete. It is called before
  // Render request is propogated to the painter.
  void UpdatePainterInformation();

  vtkInformation* PainterInformation;
  vtkTimeStamp PainterInformationUpdateTime;
  vtkPainter* Painter;
 
  int Piece;
  int NumberOfSubPieces;
  int NumberOfPieces;
  int GhostLevel;

  int SliceMode;
  int Slice;
  int UseXYPlane;
private:
  vtkPVImageSliceMapper(const vtkPVImageSliceMapper&); // Not implemented
  void operator=(const vtkPVImageSliceMapper&); // Not implemented

  class vtkObserver;
  vtkObserver* Observer;
//ETX
};

#endif
