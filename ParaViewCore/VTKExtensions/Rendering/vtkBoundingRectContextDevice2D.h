/*=========================================================================

  Program:   ParaView
  Module:    vtkBoundingRectContextDevice2D.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * @class   vtkBoundingRectContextDevice2D
 * @brief   Utility class for computing the bounds of items drawn by a
 * vtkContext2D.

 * vtkBoundingRectContextDevice2D is a utility class that can be used to
 * determine the bounding box of items drawn in a vtkContext2D. It overrides all
 * the member functions for drawing primitives, and instead of drawing anything,
 * it expands a running bounding box so that the bounding box contains the
 * rendered primitive. The running bounding box can be reset using Reset(), and
 * it can be queried with GetBoundingRect().
 *
 * This class delegates the task of computing the bounds of some primitives,
 * such as text strings, to a delegate device using SetDelegateDevice().
 *
 * Example usage:
 * \code{.cpp}
 * vtkContextScene* scene = ...;
 * vtkViewport* viewport = ...;
 * vtkNew<vtkContextDevice2D> contextDevice;
 * vtkNew<vtkBoundingRectContextDevice2D> bbDevice;
 * bbDevice->SetDelegateDevice(contextDevice.Get());
 * bbDevice->Begin(viewport);
 * vtkNew<vtkContextDevice2D> context;
 * context->Begin(bbDevice.Get());
 * scene->Paint(context.Get());
 * context->End();
 * bbDevice->End();
 *
 * \warning Currently ignores transformation matrices.
 * \endcode
 */

#ifndef vtkBoundingRectContextDevice2D_h
#define vtkBoundingRectContextDevice2D_h

#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

#include "vtkContextDevice2D.h"

class vtkAbstractContextItem;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkBoundingRectContextDevice2D : public vtkContextDevice2D
{
public:
  vtkTypeMacro(vtkBoundingRectContextDevice2D, vtkContextDevice2D) virtual void PrintSelf(
    ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkBoundingRectContextDevice2D* New();

  /**
   * Set/get delegate device used to compute bounding boxes around strings.
   */
  vtkSetObjectMacro(DelegateDevice, vtkContextDevice2D);
  vtkGetObjectMacro(DelegateDevice, vtkContextDevice2D);

  /**
   * Reset the bounding box.
   */
  void Reset();

  /**
   * Get the bounding rect that contains the given vtkAbstractContextItem.
   */
  static vtkRectf GetBoundingRect(vtkAbstractContextItem* item, vtkViewport* viewport);

  /**
   * Get the bounding rect that contains all the primitives rendered by
   * the device so far.
   */
  vtkRectf GetBoundingRect();

  /**
   * Expand bounding box to contain the string's bounding box.
   */
  void DrawString(float* point, const vtkStdString& string) VTK_OVERRIDE;

  /**
   * Expand bounding box to contain the string's bounding box.
   */
  void DrawString(float* point, const vtkUnicodeString& string) VTK_OVERRIDE;

  /**
   * Expand bounding box to contain the string's bounding box.
   */
  void DrawMathTextString(float* point, const vtkStdString& string) VTK_OVERRIDE;

  /**
   * Expand bounding box to contain the image's bounding box.
   */
  void DrawImage(float p[2], float scale, vtkImageData* image) VTK_OVERRIDE;

  /**
   * Expand bounding box to contain the image's bounding box.
   */
  void DrawImage(const vtkRectf& pos, vtkImageData* image) VTK_OVERRIDE;

  /**
   * Draw the supplied PolyData at the given x, y (p[0], p[1]) (bottom corner),
   * scaled by scale (1.0 would match the actual dataset).
   * @warning Not currently implemented.
   */
  virtual void DrawPolyData(float vtkNotUsed(p)[2], float vtkNotUsed(scale),
    vtkPolyData* vtkNotUsed(polyData), vtkUnsignedCharArray* vtkNotUsed(colors),
    int vtkNotUsed(scalarMode)) VTK_OVERRIDE
  {
  }

  /**
   * Implement pure virtual member function. Does not affect bounding rect.
   */
  void SetColor4(unsigned char color[4]) VTK_OVERRIDE;

  /**
   * Implement pure virtual member function. Does not affect bounding rect.
   */
  void SetTexture(vtkImageData* image, int properties) VTK_OVERRIDE;

  /**
   * Implement pure virtual member function. Does not affect bounding rect.
   */
  void SetPointSize(float size) VTK_OVERRIDE;

  /**
   * Implement pure virtual member function. Forward line width to
   * delegate device.
   */
  void SetLineWidth(float width) VTK_OVERRIDE;

  /**
   * Implement pure virtual member function. Forward line type to
   * delegate device.
   */
  void SetLineType(int type) VTK_OVERRIDE;

  /**
   * Forward current matrix to delegate device.
   */
  void SetMatrix(vtkMatrix3x3* m) VTK_OVERRIDE;

  /**
   * Get current matrix from delegate device.
   */
  void GetMatrix(vtkMatrix3x3* m) VTK_OVERRIDE;

  /**
   * Multiply the current matrix in the delegate device by this one.
   */
  void MultiplyMatrix(vtkMatrix3x3* m) VTK_OVERRIDE;

  /**
   * Push matrix in the delegate device.
   */
  void PushMatrix() VTK_OVERRIDE;

  /**
   * Pope matrix from the delegate device.
   */
  void PopMatrix() VTK_OVERRIDE;

  /**
   * Implement pure virtual member function. Does nothing.
   */
  void EnableClipping(bool enable) VTK_OVERRIDE;

  /**
   * Implement pure virtual member function. Does nothing.
   */
  void SetClipping(int* x) VTK_OVERRIDE;

  /**
   * Forward the pen to the delegate device.
   */
  void ApplyPen(vtkPen* pen) VTK_OVERRIDE;

  /**
   * Get the pen from the delegate device.
   */
  vtkPen* GetPen() VTK_OVERRIDE;

  /**
   * Forward the brush to the delegate device.
   */
  void ApplyBrush(vtkBrush* brush) VTK_OVERRIDE;

  /**
   * Get the brush from the delegate device.
   */
  vtkBrush* GetBrush() VTK_OVERRIDE;

  /**
   * Forward the text property to the delegate device.
   */
  void ApplyTextProp(vtkTextProperty* prop) VTK_OVERRIDE;

  /**
   * Get the text property from the delegate device.
   */
  vtkTextProperty* GetTextProp() VTK_OVERRIDE;

  /**
   * Expand bounding box to contain the given polygon.
   */
  void DrawPoly(float* points, int n, unsigned char* colors = 0, int nc_comps = 0) VTK_OVERRIDE;

  /**
   * Expand bounding rect to contain the given lines.
   */
  void DrawLines(float* f, int n, unsigned char* colors = 0, int nc_comps = 0) VTK_OVERRIDE;

  /**
   * Expand bounding rect to contain the given points.
   */
  void DrawPoints(float* points, int n, unsigned char* colors = 0, int nc_comps = 0) VTK_OVERRIDE;

  /**
   * Expand bounding rect to contain the point sprites.
   */
  void DrawPointSprites(vtkImageData* sprite, float* points, int n, unsigned char* colors = 0,
    int nc_comps = 0) VTK_OVERRIDE;

  /**
   * Expand bounding rect to contain the markers.
   */
  void DrawMarkers(int shape, bool highlight, float* points, int n, unsigned char* colors = 0,
    int nc_comps = 0) VTK_OVERRIDE;

  /**
   * Expand bounding rect to contain the ellipse.
   */
  void DrawEllipseWedge(float x, float y, float outRx, float outRy, float inRx, float inRy,
    float startAngle, float stopAngle) VTK_OVERRIDE;

  /**
   * Expand bounding rect to contain the elliptic arc.
   */
  void DrawEllipticArc(
    float x, float y, float rX, float rY, float startAngle, float stopAngle) VTK_OVERRIDE;

  /**
   * Forward string bounds calculation to the delegate device.
   */
  void ComputeStringBounds(const vtkStdString& string, float bounds[4]) VTK_OVERRIDE;

  /**
   * Forward string bounds calculation to the delegate device.
   */
  void ComputeStringBounds(const vtkUnicodeString& string, float bounds[4]) VTK_OVERRIDE;

  /**
   * Forward string bounds calculation to the delegate device.
   */
  void ComputeJustifiedStringBounds(const char* string, float bounds[4]) VTK_OVERRIDE;

  /**
   * Call before drawing to this device.
   */
  void Begin(vtkViewport*) VTK_OVERRIDE;

  /**
   * Call after drawing to this device.
   */
  void End() VTK_OVERRIDE;

  /**
   * Get value from delegate device.
   */
  bool GetBufferIdMode() const VTK_OVERRIDE;

  /**
   * Begin ID buffering mode.
   */
  void BufferIdModeBegin(vtkAbstractContextBufferId* bufferId) VTK_OVERRIDE;

  /**
   * End ID buffering mode.
   */
  void BufferIdModeEnd() VTK_OVERRIDE;

protected:
  vtkBoundingRectContextDevice2D();
  virtual ~vtkBoundingRectContextDevice2D();

  /**
   * Is the bounding rect initialized?
   */
  bool Initialized;

  /**
   * Cumulative rect holding the bounds of the primitives rendered by the device.
   */
  vtkRectf BoundingRect;

  /**
   * Delegate ContextDevice2D to handle certain computations
   */
  vtkContextDevice2D* DelegateDevice;

  /**
   * Add a point to the cumulative bounding rect.
   */
  void AddPoint(float x, float y);
  void AddPoint(float point[2]);

  /**
   * Add a rect to the cumulative bounding rect.
   */
  void AddRect(const vtkRectf& rect);

private:
  vtkBoundingRectContextDevice2D(const vtkBoundingRectContextDevice2D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBoundingRectContextDevice2D&) VTK_DELETE_FUNCTION;
};

#endif // vtkBoundingRectContextDevice2D
