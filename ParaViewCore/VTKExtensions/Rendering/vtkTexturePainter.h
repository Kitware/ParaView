/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexturePainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTexturePainter
 * @brief   renders a slice of vtkImageData by loading the
 * slice as a texture and then applying it to a quad.
 *
 * vtkTexturePainter is a painter for vtkImageData. It can render a slice of
 * image data by loading it as an texture and then displaying it on a quad. It
 * uses the bounds of the slice to position the quad. Unlike other image data
 * algorithms, this painter provides API to choose the scalars to upload. If
 * cell data is used, then cell centers are used to position the slice.
*/

#ifndef vtkTexturePainter_h
#define vtkTexturePainter_h

#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro
#include "vtkPainter.h"

class vtkImageData;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkScalarsToColors;
class vtkTexture;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkTexturePainter : public vtkPainter
{
public:
  static vtkTexturePainter* New();
  vtkTypeMacro(vtkTexturePainter, vtkPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Specify the X, Y or Z slice to use. The slice mode dictates how the data is
   * slicde.
   */
  static vtkInformationIntegerKey* SLICE();

  /**
   * Specify how the slices are obtained.
   */
  static vtkInformationIntegerKey* SLICE_MODE();

  /**
   * Turn on/off the mapping of color scalars through the lookup table.
   * The default is Off. If Off, unsigned char scalars will be used
   * directly as texture. If On, scalars will be mapped through the
   * lookup table to generate 4-component unsigned char scalars.
   * This ivar does not affect other scalars like unsigned short, float,
   * etc. These scalars are always mapped through lookup tables.
   * Look at vtkTexture::MapColorScalarsThroughLookupTable for more details.
   */
  static vtkInformationIntegerKey* MAP_SCALARS();

  /**
   * Set the lookuptable to use for scalar mapping. If none is specified and the
   * scalars are not unsigned char scalars, then a default lookup table will be
   * created and used.
   */
  static vtkInformationObjectBaseKey* LOOKUP_TABLE();

  //@{
  /**
   * Determines the whether the scalars are to be obtained from point data or
   * cell data.
   * Look at the documentation for ScalarMode in vtkMapper for the different
   * possible values and their effect.
   */
  static vtkInformationIntegerKey* SCALAR_MODE();
  vtkSetMacro(ScalarMode, int);
  vtkGetMacro(ScalarMode, int);
  //@}

  //@{
  /**
   * These three keys help identify the scalar array. If SCALAR_ARRAY_NAME is
   * absent or NULL, SCALAR_ARRAY_INDEX is used.
   * NOTE: We are deliberately not adding support to select a component to color
   * with. That is now a property of the lookup table and ideally must be set on
   * the lookup table.
   */
  static vtkInformationStringKey* SCALAR_ARRAY_NAME();
  vtkSetStringMacro(ScalarArrayName);
  vtkGetStringMacro(ScalarArrayName);
  //@}

  //@{
  /**
   * Sepecify the index of the array to color with when scalar array name is
   * absent or null.
   */
  static vtkInformationIntegerKey* SCALAR_ARRAY_INDEX();
  vtkSetMacro(ScalarArrayIndex, int);
  vtkGetMacro(ScalarArrayIndex, int);
  //@}

  //@{
  /**
   * Get/Set the Slice that needs to be rendering. This is applicable for 3D
   * images. If the Slice number is not valid, then the 0th slice is
   * rendered.
   */
  vtkSetMacro(Slice, int);
  vtkGetMacro(Slice, int);
  //@}

  //@{
  /**
   * Indicates the direction in which the slices are made into 3D data.
   * If the input image is 2D, the the entire data is shown.
   */
  vtkSetClampMacro(SliceMode, int, YZ_PLANE, XY_PLANE);
  vtkGetMacro(SliceMode, int);
  //@}

  /**
   * Set the lookuptable to use.
   */
  void SetLookupTable(vtkScalarsToColors*);

  //@{
  /**
   * Set if LUT must be used when scalars in the image can be directly used as
   * colors. Look at vtkTexture::MapColorScalarsThroughLookupTable for more
   * details.
   */
  vtkSetMacro(MapScalars, int);
  vtkGetMacro(MapScalars, int);
  //@}

  //@{
  /**
   * When set, the image slice is always rendered in the XY plane (Z==0)
   * irrespective of the image bounds. Default if Off.
   */
  static vtkInformationIntegerKey* USE_XY_PLANE();
  vtkSetClampMacro(UseXYPlane, int, 0, 1);
  vtkBooleanMacro(UseXYPlane, int);
  vtkGetMacro(UseXYPlane, int);
  //@}

  enum
  {
    YZ_PLANE = 0,
    XZ_PLANE = 1,
    XY_PLANE = 2,
  };

  virtual void ReleaseGraphicsResources(vtkWindow*);

  vtkSetVector6Macro(WholeExtent, int);

protected:
  vtkTexturePainter();
  ~vtkTexturePainter();

  /**
   * Called before RenderInternal() if the Information has been changed
   * since the last time this method was called.
   */
  virtual void ProcessInformation(vtkInformation*);

  /**
   * Performs the actual rendering. Subclasses may override this method.
   * default implementation merely call a Render on the DelegatePainter,
   * if any. When RenderInternal() is called, it is assured that the
   * DelegatePainter is in sync with this painter i.e. UpdateDelegatePainter()
   * has been called.
   */
  virtual void RenderInternal(
    vtkRenderer* renderer, vtkActor* actor, unsigned long typeflags, bool forceCompileOnly);

  /**
   * Internal method passes correct scalars to the Texture and returns 1 if cell
   * scalars are used else 0.
   */
  int SetupScalars(vtkImageData* input);

  int Slice;
  int SliceMode;
  int MapScalars;
  int ScalarMode;
  int ScalarArrayIndex;
  int UseXYPlane;
  int WholeExtent[6];
  char* ScalarArrayName;
  vtkScalarsToColors* LookupTable;

  // We compute the coordinates for the quad used to draw the texture. This uses
  // the input data bounds and the slice properties.
  float QuadPoints[4][3];

  // This is used to load the image data to the texture.
  vtkTexture* Texture;

  vtkTimeStamp UpdateTime;

private:
  vtkTexturePainter(const vtkTexturePainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTexturePainter&) VTK_DELETE_FUNCTION;
};

#endif
