/*=========================================================================

  Module:    vtkKWParameterValueHermiteFunctionEditor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWParameterValueHermiteFunctionEditor - a hermite function editor
// .SECTION Description
// This widget implements and defines the interface needed to edit
// an hermitian curve. On top of the superclass API that already
// describes each control point of the curve, several pure virtual functions
// are provided for subclasses to specify the midpoint and sharpness at each
// control point.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWParameterValueHermiteFunctionEditor_h
#define __vtkKWParameterValueHermiteFunctionEditor_h

#include "vtkKWParameterValueFunctionEditor.h"

class vtkKWScaleWithEntry;

class KWWIDGETS_EXPORT vtkKWParameterValueHermiteFunctionEditor : public vtkKWParameterValueFunctionEditor
{
public:
  vtkTypeRevisionMacro(vtkKWParameterValueHermiteFunctionEditor,vtkKWParameterValueFunctionEditor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Pack the widget
  virtual void Pack();

  // Description:
  // Set/Get the mid-point entry UI visibility.
  // Not shown if superclass PointEntriesVisibility is set to Off
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(MidPointEntryVisibility, int);
  virtual void SetMidPointEntryVisibility(int);
  vtkGetMacro(MidPointEntryVisibility, int);

  // Description:
  // Access the parameter entry.
  virtual vtkKWScaleWithEntry* GetMidPointEntry();

  // Description:
  // Set/Get the mid-sharpness entry UI visibility.
  // Not shown if superclass PointEntriesVisibility is set to Off
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(SharpnessEntryVisibility, int);
  virtual void SetSharpnessEntryVisibility(int);
  vtkGetMacro(SharpnessEntryVisibility, int);

  // Description:
  // Access the parameter entry.
  virtual vtkKWScaleWithEntry* GetSharpnessEntry();

  // Description:
  // Set/Get the midpoint visibility in the canvas.
  // The style of the midpoint is a rectangle around the midpoint location.
  // Its color is controlled using MidPointColor.
  vtkBooleanMacro(MidPointVisibility, int);
  virtual void SetMidPointVisibility(int);
  vtkGetMacro(MidPointVisibility, int);

  // Description:
  // Set/Get the midpoints color. 
  vtkGetVector3Macro(MidPointColor, double);
  virtual void SetMidPointColor(double r, double g, double b);
  virtual void SetMidPointColor(double rgb[3])
    { this->SetMidPointColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the midpoint guideline visibility in the canvas 
  // (for ex: a vertical line at each midpoint).
  // The style of the midpoint guidelines is the same as the style of the
  // superclass point guideline (PointGuidelineStyle) for consistency.
  vtkBooleanMacro(MidPointGuidelineVisibility, int);
  virtual void SetMidPointGuidelineVisibility(int);
  vtkGetMacro(MidPointGuidelineVisibility, int);

  // Description:
  // Set/Get the midpoint guideline value visibility in the canvas 
  // (i.e., a value on top of the guideline).
  // Note that the value is not displayed if MidPointGuidelineVisibility is
  // set to Off (i.e. if we do not display the guideline itself, why displaying
  // the value ?).
  vtkBooleanMacro(MidPointGuidelineValueVisibility, int);
  virtual void SetMidPointGuidelineValueVisibility(int);
  vtkGetMacro(MidPointGuidelineValueVisibility, int);

  // Description:
  // Set/Get the midpoint guideline value printf format.
  virtual void SetMidPointGuidelineValueFormat(const char *);
  vtkGetStringMacro(MidPointGuidelineValueFormat);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Callbacks
  virtual void MidPointEntryChangedCallback();
  virtual void MidPointEntryChangingCallback();
  virtual void SharpnessEntryChangedCallback();
  virtual void SharpnessEntryChangingCallback();

  // Description:
  // Some constants
  //BTX
  static const char *MidPointTag;
  static const char *MidPointGuidelineTag;
  //ETX

protected:
  vtkKWParameterValueHermiteFunctionEditor();
  ~vtkKWParameterValueHermiteFunctionEditor();

  // Description:
  // Retrieve the mid-point between two adjacent points 'id' and 'id + 1'.
  // The midpoint is the normalized distance between the two points at which
  // the interpolated value reaches the median value in the value space.
  // Return 1 on success (there is a midpoint at normalized position 'pos'),
  // 0 otherwise.
  // The default implementation here does not provide any mid-point.
  virtual int GetFunctionMidPoint(int id, double *pos) = 0;

  // Description:
  // Set the mid-point between two adjacent points 'id' and 'id + 1'.
  // Return 1 on success (the midpoint was successfully set at normalized
  // position 'pos'), 0 otherwise.
  // The default implementation here does not provide any mid-point.
  virtual int SetFunctionMidPoint(int id, double pos) = 0;

  // Description:
  // Retrieve the sharpness of the transition between two adjacent points
  // 'id' and 'id + 1'.
  // Return 1 on success (there is a sharpness defined for this point),
  // 0 otherwise.
  virtual int GetFunctionSharpness(int id, double *sharpness) = 0;

  // Description:
  // Set the sharpness of the transition between two adjacent points
  // 'id' and 'id + 1'.
  // Return 1 on success (the sharpness was successfully set), 0 otherwise.
  virtual int SetFunctionSharpness(int id, double sharpness) = 0;

  // Description:
  // Update point entries
  virtual void UpdatePointEntries(int id);

  int    MidPointEntryVisibility;
  int    SharpnessEntryVisibility;
  int    MidPointVisibility;
  int    MidPointGuidelineVisibility;
  int    MidPointGuidelineValueVisibility;
  double MidPointColor[3];

  char* MidPointGuidelineValueFormat;

  // GUI

  vtkKWScaleWithEntry *MidPointEntry;
  vtkKWScaleWithEntry *SharpnessEntry;

  // Description:
  // Create some objects on the fly (lazy creation, to allow for a smaller
  // footprint)
  virtual void CreateMidPointEntry(vtkKWApplication *app);
  virtual void CreateSharpnessEntry(vtkKWApplication *app);
  virtual int IsPointEntriesFrameUsed();
  virtual int IsGuidelineValueCanvasUsed();

  // Description:
  // Update the mid-point entry according to the mid-point of a point
  virtual void UpdateMidPointEntry(int id);

  // Description:
  // Update the sharpness entry according to the sharpness of a point
  virtual void UpdateSharpnessEntry(int id);

  // Description:
  // Merge the point 'editor_id' from another function editor 'editor' into
  // our instance. Override the super to pass the midpoint and sharpness too
  virtual int MergePointFromEditor(
    vtkKWParameterValueFunctionEditor *editor, int editor_id, int &new_id);

  // Description:
  // Copy the point 'id' parameter and values from another function editor
  // 'editor' into the point 'id' in the instance.
  // Override the super to pass the midpoint and sharpness too
  virtual int CopyPointFromEditor(
    vtkKWParameterValueFunctionEditor *editor, int id);

  // Description:
  // Redraw the whole function or a specific point, or 
  // the line between two points. Overriden to take midpoints into account
  //BTX
  virtual void RedrawLine(int id1, int id2, ostrstream *tk_cmd = 0);
  //ETX

private:
  vtkKWParameterValueHermiteFunctionEditor(const vtkKWParameterValueHermiteFunctionEditor&); // Not implemented
  void operator=(const vtkKWParameterValueHermiteFunctionEditor&); // Not implemented
};

#endif

