/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScatterPlotPainter.h

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
 * vtkGlyph3D
*/

#ifndef vtkScatterPlotPainter_h
#define vtkScatterPlotPainter_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkPainter.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkCollection;
class vtkDataArray;
class vtkInformationDoubleKey;
class vtkPolyData;
class vtkScalarsToColors;
class vtkScalarsToColorsPainter;
class vtkUnsignedCharArray;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkScatterPlotPainter : public vtkPainter
{
public:
  static vtkScatterPlotPainter* New();
  vtkTypeMacro(vtkScatterPlotPainter, vtkPainter);
  void PrintSelf(ostream& os, vtkIndent indent);
  virtual vtkMTimeType GetMTime();

  //@{
  /**
   * See vtkScatterPlotMapper::ArrayIndex
   */
  vtkDataArray* GetArray(int idx);
  vtkDataArray* GetArray(int idx, vtkDataSet* input);
  int GetArrayComponent(int idx);
  //@}

  static vtkInformationIntegerKey* THREED_MODE();
  static vtkInformationIntegerKey* COLORIZE();
  static vtkInformationIntegerKey* GLYPH_MODE();
  static vtkInformationIntegerKey* SCALING_ARRAY_MODE();
  static vtkInformationIntegerKey* SCALE_MODE();
  static vtkInformationDoubleKey* SCALE_FACTOR();
  static vtkInformationIntegerKey* ORIENTATION_MODE();
  static vtkInformationIntegerKey* NESTED_DISPLAY_LISTS();
  static vtkInformationIntegerKey* PARALLEL_TO_CAMERA();

  virtual void SetSourceGlyphMappers(vtkCollection*);
  vtkGetObjectMacro(SourceGlyphMappers, vtkCollection);

protected:
  //@{
  /**
   * Enable or not the third (z) coordinate for 3D rendering (instead of 2D).
   * Note:
   * To work, the Z_Coords index array must be given.
   */
  vtkSetMacro(ThreeDMode, int);
  vtkGetMacro(ThreeDMode, int);
  vtkBooleanMacro(ThreeDMode, int);
  //@}

  //@{
  /**
   * Enable or not the color painting at each point.
   * Note:
   * To work, the Color index array must be given.
   */
  vtkSetMacro(Colorize, int);
  vtkGetMacro(Colorize, int);
  vtkBooleanMacro(Colorize, int);
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
  vtkSetMacro(NestedDisplayLists, int);
  vtkGetMacro(NestedDisplayLists, int);
  vtkBooleanMacro(NestedDisplayLists, int);
  //@}

  vtkSetMacro(ParallelToCamera, int);
  vtkGetMacro(ParallelToCamera, int);
  vtkBooleanMacro(ParallelToCamera, int);

  virtual void SetLookupTable(vtkScalarsToColors*);
  vtkGetObjectMacro(LookupTable, vtkScalarsToColors);

  /**
   * Get a pointer to a source object at a specified table location.
   */
  vtkPolyData* GetGlyphSource(int id = 0);

public:
  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow* window);

  virtual void UpdateBounds(double bounds[6]);
  vtkInformation* GetInputArrayInformation(int idx);

protected:
  /**
   * Method initiates the mapping process. Generally sent by the actor
   * as each frame is rendered.
   * Its behavior depends on the value of SelectMode.
   * virtual void Render(vtkRenderer *ren, vtkActor *a);
   */
  virtual void RenderInternal(
    vtkRenderer* renderer, vtkActor* actor, unsigned long typeflags, bool forceCompileOnly);

protected:
  vtkScatterPlotPainter();
  virtual ~vtkScatterPlotPainter();

  //@{
  /**
   * Take part in garbage collection.
   */
  virtual void ReportReferences(vtkGarbageCollector* collector);
  virtual void ProcessInformation(vtkInformation* info);
  //@}
  /**
   * Called when the PainterInformation becomes obsolete.
   * It is called before the Render is initiated on the Painter.
   */
  virtual void UpdatePainterInformation();

  /**
   * Convenience methods to get each of the arrays.
   */
  vtkUnsignedCharArray* GetColors();

  virtual void PrepareForRendering(vtkRenderer* renderer, vtkActor* actor);
  void RenderPoints(
    vtkRenderer* renderer, vtkActor* actor, unsigned long typeflags, bool forceCompileOnly);
  void RenderGlyphs(
    vtkRenderer* renderer, vtkActor* actor, unsigned long typeflags, bool forceCompileOnly);

  /**
   * Release display list used for matrices and color.
   */
  void ReleaseDisplayList();

  int ThreeDMode;
  int Colorize;
  int GlyphMode;

  double ScaleFactor; // Scale factor to use to scale geometry
  int ScaleMode;      // Scale by scalar value or vector magnitude
  int ScalingArrayMode;
  int OrientationMode;
  int NestedDisplayLists;     // boolean
  unsigned int DisplayListId; // GLuint
  int ParallelToCamera;

  // vtkScatterPlotPainterArray* SourceGlyphMappers; // array of mappers
  vtkCollection* SourceGlyphMappers; // array of mappers

  vtkScalarsToColorsPainter* ScalarsToColorsPainter;
  vtkTimeStamp ColorPainterUpdateTime;
  vtkTimeStamp BuildTime;

  unsigned int SelectionColorId;
  int SelectMode;

  vtkScalarsToColors* LookupTable;

private:
  vtkScatterPlotPainter(const vtkScatterPlotPainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkScatterPlotPainter&) VTK_DELETE_FUNCTION;
};

#endif
