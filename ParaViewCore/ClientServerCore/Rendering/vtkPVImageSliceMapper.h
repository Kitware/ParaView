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
/**
 * @class   vtkPVImageSliceMapper
 * @brief   Mapper for vtkImageData that renders the image
 * using a texture applied to a quad.
 *
 * vtkPVImageSliceMapper is a mapper for vtkImageData that renders the image by
 * loading the image as a texture and then applying it to a quad. For 3D images,
 * this mapper only shows a single Z slice which can be choosen using SetZSlice.
 * By default, the image data scalars are rendering, however, this mapper
 * provides API to select another point or cell data array. Internally, this
 * mapper uses painters similar to those employed by vtkPainterPolyDataMapper.
 * @sa
 * vtkPainterPolyDataMapper
*/

#ifndef vtkPVImageSliceMapper_h
#define vtkPVImageSliceMapper_h

#include "vtkMapper.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkStructuredData.h"                    // needed for VTK_*_PLANE

class vtkImageData;
class vtkRenderer;

#ifdef VTKGL2
class vtkOpenGLTexture;
class vtkActor;
#endif
class vtkPainter;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVImageSliceMapper : public vtkMapper
{
public:
  static vtkPVImageSliceMapper* New();
  vtkTypeMacro(vtkPVImageSliceMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * This calls RenderPiece (in a for loop is streaming is necessary).
   */
  virtual void Render(vtkRenderer* ren, vtkActor* act) VTK_OVERRIDE;

  virtual void ReleaseGraphicsResources(vtkWindow*) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the painter that does the actual rendering.
   */
  void SetPainter(vtkPainter*);
  vtkGetObjectMacro(Painter, vtkPainter);
  //@}

  //@{
  /**
   * Specify the input data to map.
   */
  void SetInputData(vtkImageData* in);
  virtual vtkImageData* GetInput();
  //@}

  //@{
  /**
   * Set/Get the current X/Y or Z slice number.
   */
  vtkSetMacro(Slice, int);
  vtkGetMacro(Slice, int);
  //@}

  enum
  {
    XY_PLANE = VTK_XY_PLANE,
    YZ_PLANE = VTK_YZ_PLANE,
    XZ_PLANE = VTK_XZ_PLANE,
  };

  vtkSetClampMacro(SliceMode, int, XY_PLANE, XZ_PLANE);
  vtkGetMacro(SliceMode, int);
  void SetSliceModeToYZPlane() { this->SetSliceMode(YZ_PLANE); }
  void SetSliceModeToXZPlane() { this->SetSliceMode(XZ_PLANE); }
  void SetSliceModeToXYPlane() { this->SetSliceMode(XY_PLANE); }

  //@{
  /**
   * When set, the image slice is always rendered in the XY plane (Z==0)
   * irrespective of the image bounds. Default is Off.
   */
  vtkSetClampMacro(UseXYPlane, int, 0, 1);
  vtkBooleanMacro(UseXYPlane, int);
  vtkGetMacro(UseXYPlane, int);
  //@}

  /**
   * Update that sets the update piece first.
   */
  virtual void Update(int port) VTK_OVERRIDE;
  virtual void Update() VTK_OVERRIDE { this->Superclass::Update(); }
  virtual int Update(int port, vtkInformationVector* requests) VTK_OVERRIDE
  {
    return this->Superclass::Update(port, requests);
  }
  virtual int Update(vtkInformation* requests) VTK_OVERRIDE
  {
    return this->Superclass::Update(requests);
  }

  //@{
  /**
   * If you want only a part of the data, specify by setting the piece.
   */
  vtkSetMacro(Piece, int);
  vtkGetMacro(Piece, int);
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  vtkSetMacro(NumberOfSubPieces, int);
  vtkGetMacro(NumberOfSubPieces, int);
  //@}

  //@{
  /**
   * Set the number of ghost cells to return.
   */
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);
  //@}

  //@{
  /**
   * Return bounding box (array of six doubles) of data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  virtual double* GetBounds() VTK_OVERRIDE;
  virtual void GetBounds(double bounds[6]) VTK_OVERRIDE { this->Superclass::GetBounds(bounds); };
  //@}

  /**
   * Make a shallow copy of this mapper.
   */
  virtual void ShallowCopy(vtkAbstractMapper* m);

protected:
  vtkPVImageSliceMapper();
  ~vtkPVImageSliceMapper();

  // Tell the executive that we accept vtkImageData.
  virtual int FillInputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

  /**
   * Perform the actual rendering.
   */
  virtual void RenderPiece(vtkRenderer* ren, vtkActor* act);

#ifdef VTKGL2
  vtkOpenGLTexture* Texture;
  int SetupScalars(vtkImageData*);
  void RenderInternal(vtkRenderer* ren, vtkActor* act);
  vtkTimeStamp UpdateTime;
  vtkActor* PolyDataActor;
#else
  /**
   * Called when the PainterInformation becomes obsolete. It is called before
   * Render request is propogated to the painter.
   */
  void UpdatePainterInformation();

  vtkInformation* PainterInformation;
  vtkTimeStamp PainterInformationUpdateTime;

  class vtkObserver;
  vtkObserver* Observer;
#endif

  vtkPainter* Painter;

  int Piece;
  int NumberOfSubPieces;
  int NumberOfPieces;
  int GhostLevel;

  int SliceMode;
  int Slice;
  int UseXYPlane;

private:
  vtkPVImageSliceMapper(const vtkPVImageSliceMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVImageSliceMapper&) VTK_DELETE_FUNCTION;
};

#endif
