/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScatterPlotMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkScatterPlotMapper
 * @brief   Display a vtkDataSet with flexibility
 *
 * The mappers gives flexibility in the display of the input. By setting
 * the arrays to process, every element of the display can be controlled.
 * i.e. the coordinates of the points can be controlled by any field array
 * or the color of the points can controlled by the x-axes.
 *
 * @sa
 * vtkGlyph3D, vtkCompositePolyDataMapper2
*/

#ifndef vtkScatterPlotMapper_h
#define vtkScatterPlotMapper_h

#include "vtkCompositePolyDataMapper2.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkWeakPointer.h"                  // needed for vtkWeakPointer.

class vtkPainterPolyDataMapper;
class vtkPolyData;
class vtkScalarsToColorsPainter;
class vtkScatterPlotPainter;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkScatterPlotMapper : public vtkCompositePolyDataMapper2
{
public:
  static vtkScatterPlotMapper* New();
  vtkTypeMacro(vtkScatterPlotMapper, vtkCompositePolyDataMapper2);
  void PrintSelf(ostream& os, vtkIndent indent);

  enum ArrayIndex
  {
    X_COORDS = 0,
    Y_COORDS,
    Z_COORDS,
    COLOR,
    GLYPH_X_SCALE,
    GLYPH_Y_SCALE,
    GLYPH_Z_SCALE,
    GLYPH_SOURCE,
    GLYPH_X_ORIENTATION,
    GLYPH_Y_ORIENTATION,
    GLYPH_Z_ORIENTATION,
    NUMBER_OF_ARRAYS
  };

  /**
   * Flags to control how the glyphs are displayed.
   * To use the default glyphs, set the GlyphMode to UseGlyph.
   * The other flags must have their corresponding array in order to be valid.
   * GLYPH_[XYZ]_SCALE for ScaledGlyph; GLYPH_SOURCE for UseMultiGlyph and
   * GLYPH_[XYZ]_ORIENTATION for OrientedGlyph.
   */
  enum GlyphDrawingMode
  {
    NoGlyph = 0,
    UseGlyph = 1,
    ScaledGlyph = 2,
    UseMultiGlyph = 4,
    OrientedGlyph = 8,
  };

  enum ScalingArrayModes
  {
    // Where Sx,y,z are the scaling coefs,
    // X,Y and Z are the GLYPH_[X,Y,Z]_SCALE arrays,
    // Xc,Yc and Zc are the active component of the respective X,Y and Z arrays
    // Sx = X[Xc], Sy = Y[Yc], Sz = Z[Zc]
    Xc_Yc_Zc = 0, // Use one component of each GLYPH_[X,Y,Z]_SCALE arrays (default)
    // Sx = X[Xc+0], Sy = X[Xc+1], Sz = X[Xc+2]
    Xc0_Xc1_Xc2 = 1, // Use three components of the GLYPH_X_SCALE array
    // Sx = X[Xc], Sy = X[Xc], Sz = X[Xc]
    Xc_Xc_Xc = 2 // Duplicate the component of the GLYPH_X_SCALE array
  };

  enum ScaleModes
  {
    SCALE_BY_MAGNITUDE = 0,  // XScale = YScale = ZScale = Norm(Sx,Sy,Sz,...)
    SCALE_BY_COMPONENTS = 1, // XScale = Sx, YScale = Sy, ZScale = Sz
  };

  enum OrientationModes
  {
    DIRECTION = 0,
    ROTATION = 1,
  };

  static vtkInformationIntegerKey* FIELD_ACTIVE_COMPONENT();

  //@{
  /**
   * Set the name of the point array to use as a mask for generating the glyphs.
   * This is a convenience method. The same effect can be achieved by using
   * SetInputArrayToProcess(idx, 0, connection,
   * vtkDataObject::FIELD_ASSOCIATION_POINTS, arrayName)
   * void SetMaskArray();
   */
  void SetArrayByFieldName(ArrayIndex idx, const char* arrayName, int fieldAssociation,
    int component = 0, int connection = 0);
  void SetArrayByFieldIndex(
    ArrayIndex idx, int fiedIndex, int fieldAssociation, int component = 0, int connection = 0);
  //@}
  /**
   * Set the point attribute to use for generating the glyphs.
   * \c idx is one of the following:
   * \li vtkScatterPlotMapper::X_COORDS
   * \li vtkScatterPlotMapper::Y_COORDS
   * \li vtkScatterPlotMapper::Z_COORDS
   * \li vtkScatterPlotMapper::COLOR
   * \li vtkScatterPlotMapper::GLYPH_SCALING
   * \li vtkScatterPlotMapper::GLYPH_ORIENTATION
   * \li vtkScatterPlotMapper::GLYPH_SHAPE
   * \c fieldAttributeType is one of the following:
   * \li vtkDataSetAttributes::SCALARS
   * \li vtkDataSetAttributes::VECTORS
   * \li vtkDataSetAttributes::NORMALS
   * \li vtkDataSetAttributes::TCOORDS
   * \li vtkDataSetAttributes::TENSORS

   * This is a convenience method. This internally  same effect can be achieved by using
   * SetInputArrayToProcess(idx, 0, connection,
   * vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType)
   */
  void SetArrayByFieldType(ArrayIndex idx, int fieldAttributeType, int fieldAssociation,
    int component = 0, int connection = 0);

  void SetArrayByPointCoord(ArrayIndex idx, int component = 0, int connection = 0);
  void SetArrayByName(ArrayIndex idx, const char* arrayName);
  void SetXCoordsArray(const char* arrayName) { this->SetArrayByName(X_COORDS, arrayName); }
  void SetYCoordsArray(const char* arrayName) { this->SetArrayByName(Y_COORDS, arrayName); }
  void SetZCoordsArray(const char* arrayName) { this->SetArrayByName(Z_COORDS, arrayName); }
  void SetColorArray(const char* arrayName) { this->SetArrayByName(COLOR, arrayName); }
  void SetGlyphXScalingArray(const char* arrayName)
  {
    this->SetArrayByName(GLYPH_X_SCALE, arrayName);
  }
  void SetGlyphYScalingArray(const char* arrayName)
  {
    this->SetArrayByName(GLYPH_Y_SCALE, arrayName);
  }
  void SetGlyphZScalingArray(const char* arrayName)
  {
    this->SetArrayByName(GLYPH_Z_SCALE, arrayName);
  }
  void SetGlyphSourceArray(const char* arrayName) { this->SetArrayByName(GLYPH_SOURCE, arrayName); }
  void SetGlyphXOrientationArray(const char* arrayName)
  {
    this->SetArrayByName(GLYPH_X_ORIENTATION, arrayName);
  }
  void SetGlyphYOrientationArray(const char* arrayName)
  {
    this->SetArrayByName(GLYPH_Y_ORIENTATION, arrayName);
  }
  void SetGlyphZOrientationArray(const char* arrayName)
  {
    this->SetArrayByName(GLYPH_Z_ORIENTATION, arrayName);
  }

  vtkDataArray* GetArray(ArrayIndex idx);
  vtkDataArray* GetArray(ArrayIndex idx, vtkDataSet* input);

  /*
    void SetThreeDMode(bool);
    void SetColorize(bool);
    void SetGlyphMode(int);
    void SetScalingArrayMode(int);
    void SetScaleMode(int);
    void SetScaleFactor(double);
    void SetOrientationMode(int);
    void SetNestedDisplayLists(bool);
  */
  //@{
  /**
   * Enable or not the third (z) coordinate for 3D rendering (instead of 2D).
   * Note:
   * To work, the Z_Coords index array must be given.
   */
  vtkSetMacro(ThreeDMode, bool);
  vtkGetMacro(ThreeDMode, bool);
  vtkBooleanMacro(ThreeDMode, bool);
  //@}

  //@{
  /**
   * Enable or not the color painting at each point.
   * Note:
   * To work, the Color index array must be given.
   */
  vtkSetMacro(Colorize, bool);
  vtkGetMacro(Colorize, bool);
  vtkBooleanMacro(Colorize, bool);
  //@}

  //@{
  /**
   * Enable or not the Glyph representation at each point.
   * Note:
   * To work, at least 1 Glyph polydata input must be set and the Glyph index
   * array must be given.
   */
  vtkSetMacro(GlyphMode, int);
  vtkGetMacro(GlyphMode, int);
  // vtkBooleanMacro(GlyphMode,int);
  //@}

  //@{
  /**
   * If the GlyphMode has ScaledGlyph turned on, ScalingArrayMode describes
   * how to data in the different GLYPH_[X,Y,Z]_SCALE arrays
   */
  vtkSetMacro(ScalingArrayMode, int);
  vtkGetMacro(ScalingArrayMode, int);
  //@}

  //@{
  /**
   * If the GlyphMode has ScaledGlyph turned on, decide how to scale the
   * glyph. By Magnitude or components.
   */
  vtkSetMacro(ScaleMode, int);
  vtkGetMacro(ScaleMode, int);
  //@}

  //@{
  /**
   * Specify scale factor to scale object by. This is used only when Scaling is
   * On.
   */
  vtkSetMacro(ScaleFactor, double);
  vtkGetMacro(ScaleFactor, double);
  //@}

  vtkSetMacro(OrientationMode, int);
  vtkGetMacro(OrientationMode, int);

  //@{
  /**
   * If immediate mode is off, if Glyphs are in use and if NestedDisplayLists
   * is false, only the mappers of each glyph use display lists. If true,
   * in addition, matrices transforms and color per glyph are also
   * in a parent display list.
   * Not relevant if immediate mode is on.
   * For debugging/profiling purpose. Initial value is true.
   */
  vtkSetMacro(NestedDisplayLists, bool);
  vtkGetMacro(NestedDisplayLists, bool);
  vtkBooleanMacro(NestedDisplayLists, bool);
  //@}

  //@{
  /**
   * When the glyphs are in 2D, it might be useful to force them to
   * be shown parallel to the camera.
   */
  vtkSetMacro(ParallelToCamera, bool);
  vtkGetMacro(ParallelToCamera, bool);
  vtkBooleanMacro(ParallelToCamera, bool);
  //@}

  //@{
  /**
   * Specify a source object at a specified table location. New style.
   * Source connection is stored in port 1. This method is equivalent
   * to SetInputConnection(1, id, outputPort).
   */
  void SetGlyphSourceConnection(int id, vtkAlgorithmOutput* algOutput);
  void SetGlyphSourceConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetGlyphSourceConnection(0, algOutput);
  }
  //@}
  /**
   * Specify a source object at a specified table location. New style.
   * Source connection is stored in port 1. This method is equivalent
   * to SetInputConnection(1, id, outputPort).
   */
  void AddGlyphSourceConnection(vtkAlgorithmOutput* algOutput);
  vtkPolyData* GetGlyphSource(int id = 0);

  /**
   * Reimplemented to allow "real" pre rendering by vtkScatterPlotPainter
   * Indeed the vtkPainter::PrepareForRendering is not exactly
   * a prepare for rendering (the rendering already started).
   */
  virtual void Render(vtkRenderer* renderer, vtkActor* actor);

protected:
  vtkScatterPlotMapper();
  virtual ~vtkScatterPlotMapper();

  /**
   * Take part in garbage collection.
   */
  virtual void ReportReferences(vtkGarbageCollector* collector);

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  /*
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);
  virtual int RequestData(vtkInformation* request,
                           vtkInformationVector** inputVector,
                           vtkInformationVector* outputVector);
  virtual int RequestUpdateExtent(vtkInformation* request,
                           vtkInformationVector** inputVector,
                           vtkInformationVector* outputVector);
  virtual int RequestInformation(vtkInformation* request,
                           vtkInformationVector** inputVector,
                           vtkInformationVector* outputVector);
  */
  /**
   * Send mapper ivars to sub-mapper.
   * \pre mapper_exists: mapper!=0
   */
  void CopyInformationToSubMapper(vtkPainterPolyDataMapper* mapper);

  virtual void PrepareForRendering(vtkRenderer* renderer, vtkActor* actor);
  void InitGlyphMappers(vtkRenderer* renderer, vtkActor* actor, bool createDisplayList = true);
  void GenerateDefaultGlyphs();
  virtual void ComputeBounds();
  virtual void UpdatePainterInformation();

  vtkScatterPlotPainter* GetScatterPlotPainter();

  bool ThreeDMode;
  bool Colorize;
  int GlyphMode;

  double ScaleFactor; // Scale factor to use to scale geometry
  int ScaleMode;      // Scale by scalar value or vector magnitude
  int ScalingArrayMode;
  int OrientationMode;
  bool NestedDisplayLists; // boolean
  bool ParallelToCamera;

private:
  vtkScatterPlotMapper(const vtkScatterPlotMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkScatterPlotMapper&) VTK_DELETE_FUNCTION;
};

#endif
