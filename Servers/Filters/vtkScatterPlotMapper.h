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
// .NAME vtkScatterPlotMapper - vtkGlyph3D on the GPU.
// .SECTION Description
// Do the same job than vtkGlyph3D but on the GPU. For this reason, it is
// a mapper not a vtkPolyDataAlgorithm. Also, some methods of vtkGlyph3D
// don't make sense in vtkScatterPlotMapper: GeneratePointIds, old-style
// SetSource, PointIdsName, IsPointVisible.
// .SECTION Implementation
//
// .SECTION See Also
// vtkGlyph3D

#ifndef __vtkScatterPlotMapper_h
#define __vtkScatterPlotMapper_h

#include "vtkMapper.h"
#include "vtkGlyph3D.h" // for the constants (VTK_SCALE_BY_SCALAR, ...).
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.
#include <iostream>
class vtkScatterPlotMapperArray; // pimp
class vtkPainterPolyDataMapper;
class vtkScalarsToColorsPainter;

class VTK_EXPORT vtkScatterPlotMapper : public vtkMapper
{
public:
  static vtkScatterPlotMapper* New();
  vtkTypeRevisionMacro(vtkScatterPlotMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent);
  virtual unsigned long GetMTime();
  //BTX
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
    GLYPH_Z_ORIENTATION
    };
  //ETX

  //BTX
  enum GlyphDrawingMode
    {
    NoGlyph       = 0,
    UseGlyph      = 1,
    ScaledGlyph   = 2,
    UseMultiGlyph = 4,
    OrientedGlyph = 8,
    };
  //ETX


  virtual void SetInputConnection(int port, vtkAlgorithmOutput* input);
  // Description:
  // Set the name of the point array to use as a mask for generating the glyphs.
  // This is a convenience method. The same effect can be achieved by using
  // SetInputArrayToProcess(idx, 0, connection,
  //    vtkDataObject::FIELD_ASSOCIATION_POINTS, arrayName)
  //void SetMaskArray();
  void SetArrayByFieldName(ArrayIndex idx, const char* arrayName, 
                           int component=0, int connection = 0 );
  void SetArrayByFieldIndex(ArrayIndex idx, int fiedIndex, 
                            int component=0, int connection = 0 );
  // Description:
  // Set the point attribute to use as a mask for generating the glyphs.
  // \c idx is one of the following:
  // \li vtkScatterPlotMapper::X_COORDS
  // \li vtkScatterPlotMapper::Y_COORDS
  // \li vtkScatterPlotMapper::Z_COORDS
  // \li vtkScatterPlotMapper::COLOR
  // \li vtkScatterPlotMapper::GLYPH_SCALING
  // \li vtkScatterPlotMapper::GLYPH_ORIENTATION
  // \li vtkScatterPlotMapper::GLYPH_SHAPE
  // \c fieldAttributeType is one of the following:
  // \li vtkDataSetAttributes::SCALARS
  // \li vtkDataSetAttributes::VECTORS
  // \li vtkDataSetAttributes::NORMALS
  // \li vtkDataSetAttributes::TCOORDS
  // \li vtkDataSetAttributes::TENSORS
  // 
  // This is a convenience method. This internally  same effect can be achieved by using
  // SetInputArrayToProcess(idx, 0, connection,
  //    vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType)
  void SetArrayByFieldType(ArrayIndex idx, int fieldAttributeType, 
                         int component=0, int connection=0);
  
  void SetArrayByPointCoord(ArrayIndex idx, 
                            int component=0, int connection=0);
  void SetArrayByName(ArrayIndex idx, const char* arrayName);
  void SetXCoordsArray(const char* arrayName)
  {this->SetArrayByName(X_COORDS,arrayName);}
  void SetYCoordsArray(const char* arrayName)
  {this->SetArrayByName(Y_COORDS,arrayName);}
  void SetZCoordsArray(const char* arrayName)
  {this->SetArrayByName(Z_COORDS,arrayName);}
  void SetColorArray(const char* arrayName)
  {this->SetArrayByName(COLOR,arrayName);}
  void SetGlyphXScalingArray(const char* arrayName)
  {this->SetArrayByName(GLYPH_X_SCALE,arrayName);}
  void SetGlyphYScalingArray(const char* arrayName)
  {this->SetArrayByName(GLYPH_Y_SCALE,arrayName);}
  void SetGlyphZScalingArray(const char* arrayName)
  {this->SetArrayByName(GLYPH_Z_SCALE,arrayName);}
  void SetGlyphSourceArray(const char* arrayName)
  {this->SetArrayByName(GLYPH_SOURCE,arrayName);}
  void SetGlyphXOrientationArray(const char* arrayName)
  {this->SetArrayByName(GLYPH_X_ORIENTATION,arrayName);}
  void SetGlyphYOrientationArray(const char* arrayName)
  {this->SetArrayByName(GLYPH_Y_ORIENTATION,arrayName);}
  void SetGlyphZOrientationArray(const char* arrayName)
  {this->SetArrayByName(GLYPH_Z_ORIENTATION,arrayName);}


  vtkDataArray* GetArray(ArrayIndex idx);
  vtkDataArray* GetArray(ArrayIndex idx, vtkDataSet* input);
  int GetArrayComponent(ArrayIndex idx);

  // Description:
  // Enable or not the third (z) coordinate for 3D rendering (instead of 2D).
  // Note:
  // To work, the Z_Coords index array must be given.
  vtkSetMacro(ThreeDMode,bool);
  vtkGetMacro(ThreeDMode,bool);
  vtkBooleanMacro(ThreeDMode,bool);

  // Description:
  // Enable or not the color painting at each point.
  // Note:
  // To work, the Color index array must be given.
  vtkSetMacro(Colorize,bool);
  vtkGetMacro(Colorize,bool);
  vtkBooleanMacro(Colorize,bool);

  // Description:
  // Enable or not the Glyph representation at each point.
  // Note:
  // To work, at least 1 Glyph polydata input must be set and the Glyph index
  // array must be given.
  vtkSetMacro(GlyphMode,int);
  vtkGetMacro(GlyphMode,int);
  //vtkBooleanMacro(GlyphMode,int);

  //BTX
  enum ScalingArrayModes
  {
    // Where Sx,y,z are the scaling coefs,
    // X,Y and Z are the GLYPH_[X,Y,Z]_SCALE arrays,
    // Xc,Yc and Zc are the active component of the respective X,Y and Z arrays
    // Sx = X[Xc], Sy = Y[Yc], Sz = Z[Zc]
    Xc_Yc_Zc    = 0, // Use one component of each GLYPH_[X,Y,Z]_SCALE arrays (default)
    // Sx = X[Xc+0], Sy = X[Xc+1], Sz = X[Xc+2]
    Xc0_Xc1_Xc2 = 1, // Use three components of the GLYPH_X_SCALE array
    // Sx = X[Xc], Sy = X[Xc], Sz = X[Xc]
    Xc_Xc_Xc    = 2 // Duplicate the component of the GLYPH_X_SCALE array
  };
  //ETX

  // Description:
  // If the GlyphMode has ScaledGlyph turned on, ScalingArrayMode describes 
  // how to data in the different GLYPH_[X,Y,Z]_SCALE arrays
  vtkSetMacro(ScalingArrayMode,int);
  vtkGetMacro(ScalingArrayMode,int);

  //BTX
  enum ScaleModes
  {
    SCALE_BY_MAGNITUDE  = 0, // XScale = YScale = ZScale = Norm(Sx,Sy,Sz,...)
    SCALE_BY_COMPONENTS = 1, // XScale = Sx, YScale = Sy, ZScale = Sz
  };
  //ETX

  // Description:
  // If the GlyphMode has ScaledGlyph turned on, decide how to scale the 
  // glyph. By Magnitude or components.
  vtkSetMacro(ScaleMode,int);
  vtkGetMacro(ScaleMode,int);

  // Description:
  // Specify scale factor to scale object by. This is used only when Scaling is
  // On.
  vtkSetMacro(ScaleFactor,double);
  vtkGetMacro(ScaleFactor,double);

  //BTX
  enum OrientationModes
  {
    DIRECTION = 0,
    ROTATION  = 1,
  };
  //ETX
  
  vtkSetMacro(OrientationMode, int);
  vtkGetMacro(OrientationMode, int);

  // Description:
  // Specify a source object at a specified table location. New style.
  // Source connection is stored in port 1. This method is equivalent
  // to SetInputConnection(1, id, outputPort).
  void SetGlyphSourceConnection(int id, vtkAlgorithmOutput* algOutput);
  void SetGlyphSourceConnection(vtkAlgorithmOutput* algOutput)
    {
      this->SetGlyphSourceConnection(0, algOutput);
    }
  // Description:
  // Specify a source object at a specified table location. New style.
  // Source connection is stored in port 1. This method is equivalent
  // to SetInputConnection(1, id, outputPort).
  void AddGlyphSourceConnection(vtkAlgorithmOutput* algOutput);
  
  // Description:
  // Get a pointer to a source object at a specified table location.
  vtkPolyData *GetGlyphSource(int id = 0);

  // Description:
  // Enable/disable indexing into table of the glyph sources. When disabled,
  // only the 1st source input will be used to generate the glyph. Otherwise the
  // source index array will be used to select the glyph source. The source
  // index array can be specified using SetSourceIndexArray().
  //vtkSetMacro(SourceIndexing, bool);
  //vtkGetMacro(SourceIndexing, bool);
  //vtkBooleanMacro(SourceIndexing, bool);
  
  
  // Description:
  // Convenience method to set the array to use as index within the sources. 
  // This is same as calling
  // SetInputArrayToProcess(vtkScatterPlotMapper::SOURCE_INDEX, 0, 0,
  //    vtkDataObject::FIELD_ASSOCIATION_POINTS, arrayname).
  //void SetSourceIndexArray(const char* arrayname);

  // Description:
  // Convenience method to set the array to use as index within the sources. 
  // This is same as calling
  // SetInputArrayToProcess(vtkScatterPlotMapper::SOURCE_INDEX, 0, 0,
  //    vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType).
  //void SetSourceIndexArray(int fieldAttributeType);


  // Description:
  // Turn on/off scaling of source geometry. When turned on, ScaleFactor
  // controls the scale applied. To scale with some data array, ScaleMode should
  // be set accordingly.
  //vtkSetMacro(Scaling, bool);
  //vtkBooleanMacro(Scaling, bool);
  //vtkGetMacro(Scaling, bool);

  // Description:
  // Either scale by individual components (SCALE_BY_COMPONENTS) or magnitude
  // (SCALE_BY_MAGNITUDE) of the chosen array to SCALE with or disable scaling
  // using data array all together (NO_DATA_SCALING). Default is
  // NO_DATA_SCALING.
  //vtkSetMacro(ScaleMode, int);
  //vtkGetMacro(ScaleMode, int);

  // Description:
  // Specify scale factor to scale object by. This is used only when Scaling is
  // On.
  //vtkSetMacro(ScaleFactor,double);
  //vtkGetMacro(ScaleFactor,double);

  //BTX
  //enum ScaleModes
  //  {
  //  NO_DATA_SCALING = 0,
  //  SCALE_BY_MAGNITUDE = 1,
  //  SCALE_BY_COMPONENTS = 2
  //  };
  //ETX
  //void SetScaleModeToScaleByMagnitude()
  //  { this->SetScaleMode(SCALE_BY_MAGNITUDE); }
  //void SetScaleModeToScaleByVectorComponents()
  //  { this->SetScaleMode(SCALE_BY_COMPONENTS); }
  //void SetScaleModeToNoDataScaling()
  //  { this->SetScaleMode(NO_DATA_SCALING); }
  //const char *GetScaleModeAsString();

  // Description:
  // Specify range to map scalar values into.
  //vtkSetVector2Macro(Range,double);
  //vtkGetVectorMacro(Range,double,2);

  // Description:
  // Turn on/off orienting of input geometry. 
  // When turned on, the orientation array specified
  // using SetOrientationArray() will be used. 
  //vtkSetMacro(Orient, bool);
  //vtkGetMacro(Orient, bool);
  //vtkBooleanMacro(Orient, bool);

  // Description:
  // Orientation mode indicates if the OrientationArray provides the direction
  // vector for the orientation or the rotations around each axes. Default is
  // DIRECTION
  //vtkSetClampMacro(OrientationMode, int, DIRECTION, ROTATION);
  //vtkGetMacro(OrientationMode, int);
  //void SetOrientationModeToDirection()
  //  { this->SetOrientationMode(vtkScatterPlotMapper::DIRECTION); }
  //void SetOrientationModeToRotation()
  //  { this->SetOrientationMode(vtkScatterPlotMapper::ROTATION); }
  //const char* GetOrientationModeAsString();
  //BTX
  //enum OrientationModes
  //  {
  //  DIRECTION=0,
  //  ROTATION=1
  //  };
  //ETX

  // Description:
  // Turn on/off clamping of data values to scale with to the specified range.
  //vtkSetMacro(Clamping, bool);
  //vtkGetMacro(Clamping, bool);
  //vtkBooleanMacro(Clamping, bool);

  // Description:
  // Method initiates the mapping process. Generally sent by the actor 
  // as each frame is rendered.
  // Its behavior depends on the value of SelectMode.
  virtual void Render(vtkRenderer *ren, vtkActor *a);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *window);

  // Description:
  // Redefined to take into account the bounds of the scaled glyphs.
  virtual double *GetBounds();

  // Description:
  // Same as superclass. Appear again to stop warnings about hidden method.
  virtual void GetBounds(double bounds[6]);

  // Description:
  // Send mapper ivars to sub-mapper.
  // \pre mapper_exists: mapper!=0
  void CopyInformationToSubMapper(vtkPainterPolyDataMapper *mapper);

  // Description:
  // If immediate mode is off, if Glyphs are in use and if NestedDisplayLists 
  // is false, only the mappers of each glyph use display lists. If true,
  // in addition, matrices transforms and color per glyph are also
  // in a parent display list.
  // Not relevant if immediate mode is on.
  // For debugging/profiling purpose. Initial value is true.
  vtkSetMacro(NestedDisplayLists, bool);
  vtkGetMacro(NestedDisplayLists, bool);
  vtkBooleanMacro(NestedDisplayLists, bool);

  // Description:
  // Tells the mapper to skip glyphing input points that haves false values
  // in the mask array. If there is no mask array (id access mode is set
  // and there is no such id, or array name access mode is set and
  // the there is no such name), masking is silently ignored.
  // A mask array is a vtkBitArray with only one component.
  // Initial value is false.
  //vtkSetMacro(Masking, bool);
  //vtkGetMacro(Masking, bool);
  //vtkBooleanMacro(Masking, bool);

  // Description:
  // Set the name of the point array to use as a mask for generating the glyphs.
  // This is a convenience method. The same effect can be achieved by using
  // SetInputArrayToProcess(vtkScatterPlotMapper::MASK, 0, 0,
  //    vtkDataObject::FIELD_ASSOCIATION_POINTS, maskarrayname)
  //void SetMaskArray(const char* maskarrayname);

  // Description:
  // Set the point attribute to use as a mask for generating the glyphs.
  // \c fieldAttributeType is one of the following:
  // \li vtkDataSetAttributes::SCALARS
  // \li vtkDataSetAttributes::VECTORS
  // \li vtkDataSetAttributes::NORMALS
  // \li vtkDataSetAttributes::TCOORDS
  // \li vtkDataSetAttributes::TENSORS
  // This is a convenience method. The same effect can be achieved by using
  // SetInputArrayToProcess(vtkScatterPlotMapper::MASK, 0, 0,
  //    vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType)
  //void SetMaskArray(int fieldAttributeType);

  // Description:
  // Tells the mapper to use an orientation array if Orient is true. 
  // An orientation array is a vtkDataArray with 3 components. The first
  // component is the angle of rotation along the X axis. The second
  // component is the angle of rotation along the Y axis. The third
  // component is the angle of rotation along the Z axis. Orientation is
  // specified in X,Y,Z order but the rotations are performed in Z,X an Y.
  // This definition is compliant with SetOrientation method on vtkProp3D.
  // By using vector or normal there is a degree of freedom or rotation
  // left (underconstrained). With the orientation array, there is no degree of
  // freedom left.
  // This is convenience method. The same effect can be achieved by using
  // SetInputArrayToProcess(vtkScatterPlotMapper::ORIENTATION, 0, 0, 
  //    vtkDataObject::FIELD_ASSOCIATION_POINTS, orientationarrayname);
  //void SetOrientationArray(const char* orientationarrayname);

  // Description:
  // Tells the mapper to use an orientation array if Orient is true. 
  // An orientation array is a vtkDataArray with 3 components. The first
  // component is the angle of rotation along the X axis. The second
  // component is the angle of rotation along the Y axis. The third
  // component is the angle of rotation along the Z axis. Orientation is
  // specified in X,Y,Z order but the rotations are performed in Z,X an Y.
  // This definition is compliant with SetOrientation method on vtkProp3D.
  // By using vector or normal there is a degree of freedom or rotation
  // left (underconstrained). With the orientation array, there is no degree of
  // freedom left.
  // \c fieldAttributeType is one of the following:
  // \li vtkDataSetAttributes::SCALARS
  // \li vtkDataSetAttributes::VECTORS
  // \li vtkDataSetAttributes::NORMALS
  // \li vtkDataSetAttributes::TCOORDS
  // \li vtkDataSetAttributes::TENSORS
  // This is convenience method. The same effect can be achieved by using
  // SetInputArrayToProcess(vtkScatterPlotMapper::ORIENTATION, 0, 0, 
  //    vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType);
  //void SetOrientationArray(int fieldAttributeType);

  // Description:
  // Convenience method to set the array to scale with. This is same as calling
  // SetInputArrayToProcess(vtkScatterPlotMapper::SCALE, 0, 0,
  //    vtkDataObject::FIELD_ASSOCIATION_POINTS, scalarsarrayname).
  //void SetScaleArray(const char* scalarsarrayname);

  // Description:
  // Convenience method to set the array to scale with. This is same as calling
  // SetInputArrayToProcess(vtkScatterPlotMapper::SCALE, 0, 0,
  //    vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType).
  //void SetScaleArray(int fieldAttributeType);

  // Description:
  // For selection by color id mode (not for end-user, called by
  // vtkKWEGlyphSelectionRenderMode). 0 is reserved for miss. it has to
  // start at 1. Initial value is 1.
  vtkSetMacro(SelectionColorId,unsigned int);
  vtkGetMacro(SelectionColorId,unsigned int);

  // Description:
  // Called by vtkKWEGlyphSelectionRenderMode.
  vtkSetMacro(SelectMode, int);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Used by vtkHardwareSelector to determine if the prop supports hardware
  // selection.
  virtual bool GetSupportsSelection()
    { return true; }
  //BTX
protected:
  vtkScatterPlotMapper();
  virtual ~vtkScatterPlotMapper();

  static vtkInformationIntegerKey *FIELD_ACTIVE_COMPONENT();

  virtual int RequestUpdateExtent(vtkInformation *request,
    vtkInformationVector **inInfo,
    vtkInformationVector *outInfo);

  virtual int FillInputPortInformation(int port,
    vtkInformation *info);

  // Description:
  // Take part in garbage collection.
  virtual void ReportReferences(vtkGarbageCollector *collector);

  //vtkPolyData *GetGlyphSource(int idx, vtkInformationVector *sourceInfo);
  
  // Description:
  // Convenience methods to get each of the arrays.
  //vtkDataArray* GetMaskArray(vtkDataSet* input);
  vtkUnsignedCharArray* GetColors();

  void PrepareForRendering(vtkRenderer* renderer, vtkActor* actor);
  void RenderPoints(vtkRenderer* renderer, vtkActor* actor);
  void RenderGlyphs(vtkRenderer* renderer, vtkActor* actor);
  void InitGlyphMappers(vtkRenderer* renderer, vtkActor* actor, bool createDisplayList = true);
  void GenerateDefaultGlyphs();
  // Description:
  // Release display list used for matrices and color.
  void ReleaseDisplayList();

  // Description:
  // Called when the PainterInformation becomes obsolete. 
  // It is called before the Render is initiated on the Painter.
  virtual void UpdatePainterInformation();

  int ThreeDMode;
  bool Colorize;
  int GlyphMode;

  double ScaleFactor; // Scale factor to use to scale geometry
  int ScaleMode; // Scale by scalar value or vector magnitude
  int ScalingArrayMode;
  
  //double Range[2]; // Range to use to perform scalar scaling
  //bool Orient; // boolean controls whether to "orient" data
  //bool Clamping; // whether to clamp scale factor
  //bool SourceIndexing; // Enable/disable indexing into the glyph table
  //bool Masking; // Enable/disable masking.
  int OrientationMode;

  vtkScatterPlotMapperArray *SourceMappers; // array of mappers

  vtkWeakPointer<vtkWindow> LastWindow; // Window used for previous render.

  unsigned int DisplayListId; // GLuint

  bool NestedDisplayLists; // boolean

  vtkScalarsToColorsPainter* ScalarsToColorsPainter;
  vtkInformation* PainterInformation;
  vtkTimeStamp PainterUpdateTime;

  unsigned int SelectionColorId;
  int SelectMode;

private:
  vtkScatterPlotMapper(const vtkScatterPlotMapper&); // Not implemented.
  void operator=(const vtkScatterPlotMapper&); // Not implemented.
  //ETX
};

#endif
