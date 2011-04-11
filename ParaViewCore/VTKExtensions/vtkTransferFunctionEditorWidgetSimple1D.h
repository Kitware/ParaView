/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorWidgetSimple1D.h,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransferFunctionEditorWidgetSimple1D - a 3D widget for manipulating a transfer function
// .SECTION Description
// vtkTransferFunctionEditorWidgetSimple1D is a 3D widget used for manipulating
// 1D transfer functions using nodes.

#ifndef __vtkTransferFunctionEditorWidgetSimple1D_h
#define __vtkTransferFunctionEditorWidgetSimple1D_h

#include "vtkTransferFunctionEditorWidget1D.h"

class vtkAbstractWidget;
class vtkHandleWidget;
class vtkNodeList;
class vtkTransferFunctionEditorRepresentationSimple1D;

class VTK_EXPORT vtkTransferFunctionEditorWidgetSimple1D : public vtkTransferFunctionEditorWidget1D
{
public:
  static vtkTransferFunctionEditorWidgetSimple1D* New();
  vtkTypeMacro(vtkTransferFunctionEditorWidgetSimple1D, vtkTransferFunctionEditorWidget1D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a default representation for this widget,
  // vtkTransferFunctionEditorRepresentationSimple1D in this case.
  virtual void CreateDefaultRepresentation();

  // Description:
  // Method for activating this widget. Note that the widget representation
  // must be specified or the widget will not appear.
  virtual void SetEnabled(int enabling);

  // Description:
  // Set the scalar range of the underlying data to display with this widget.
  virtual void SetVisibleScalarRange(double min, double max);
  virtual void SetVisibleScalarRange(double range[2]) 
    { this->SetVisibleScalarRange(range[0], range[1]); }

  // Description:
  // Update the size of the rendering window containing this widget, and
  // recompute the position of the transfer function nodes based on the new
  // window size.
  virtual void Configure(int size[2]);

  // Description:
  // Respond to keypress events.
  virtual void OnChar();

  // Description:
  // Set/get the opacity of a particular node in the transfer function.
  virtual void SetElementOpacity(unsigned int idx, double opacity);
  virtual double GetElementOpacity(unsigned int idx);

  // Description:
  // Set/get the color of a particular node in the transfer function using the
  // RGB or HSV color space.
  virtual void SetElementRGBColor(unsigned int idx,
                                  double r, double g, double b);
  virtual int GetElementRGBColor(unsigned int idx, double color[3]);
  virtual void SetElementHSVColor(unsigned int idx,
                                  double h, double s, double v);
  virtual int GetElementHSVColor(unsigned int idx, double color[3]);

  // Description:
  // Set/get the scalar value associated with a particular element in the
  // transfer function editor.
  virtual void SetElementScalar(unsigned int idx, double value);
  virtual double GetElementScalar(unsigned int idx);

  // Description:
  // Set the color space.
  virtual void SetColorSpace(int space);

  // Description:
  // Update this widget based on changes to the transfer functions.
  virtual void UpdateFromTransferFunctions();

  // Description:
  // Set the type of function to modify.
  virtual void SetModificationType(int type);

  // Descripition:
  // Set whether the endpoint nodes may or may not change scalar values
  // and/or be deleted. By default moving and deleting are allowed.
  vtkSetMacro(LockEndPoints, int);

  // Description:
  // Set the width (in pixels) of the border around the transfer function
  // editor.
  virtual void SetBorderWidth(int width);

protected:
  vtkTransferFunctionEditorWidgetSimple1D();
  ~vtkTransferFunctionEditorWidgetSimple1D();

  void RemoveAllNodes();

  // the positioning handle widgets
  vtkNodeList *Nodes;
  int WidgetState;

  double InitialMinimumColor[3];
  double InitialMaximumColor[3];

  int LockEndPoints;

  int LeftClickEventPosition[2];
  int LeftClickCount;

//BTX
  // the state of the widget
  enum
  {
    Start = 0,
    PlacingNode,
    MovingNode
  };
//ETX

  // Callback interface to capture events.
  static void AddNodeAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveNodeAction(vtkAbstractWidget*);

  void AddNewNode(int x, int y);
  void AddNewNode(double scalar);
  void AddOpacityPoint(double x, double y);
  void RemoveOpacityPoint(unsigned int id);
  void AddColorPoint(double x);
  void RepositionColorPoint(unsigned int idx, double scalar);
  void RemoveColorPoint(unsigned int id);
  void ClampToWholeRange(double pos[2], int size[2], double &scalar);
  int NodeExists(double scalar);
  
  // Helper method for creating widgets
  static vtkHandleWidget* CreateHandleWidget(
    vtkTransferFunctionEditorWidgetSimple1D *self,
    vtkTransferFunctionEditorRepresentationSimple1D *rep,
    unsigned int currentHandleNumber);

  void RecomputeNodePositions(double oldRange[2], double newRange[2]);
  void RecomputeNodePositions(int oldSize[2], int newSize[2],
                              int changeBorder = 0,
                              int oldWidth = 0, int newWidth = 0);

  void RemoveNode(unsigned int id);

private:
  vtkTransferFunctionEditorWidgetSimple1D(const vtkTransferFunctionEditorWidgetSimple1D&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorWidgetSimple1D&); // Not implemented.
};

#endif
