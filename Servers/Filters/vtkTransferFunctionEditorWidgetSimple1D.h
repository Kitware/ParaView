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
  vtkTypeRevisionMacro(vtkTransferFunctionEditorWidgetSimple1D, vtkTransferFunctionEditorWidget1D);
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

protected:
  vtkTransferFunctionEditorWidgetSimple1D();
  ~vtkTransferFunctionEditorWidgetSimple1D();

  // the positioning handle widgets
  vtkNodeList *Nodes;
  int WidgetState;

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

  // Helper method for creating widgets
  static vtkHandleWidget* CreateHandleWidget(
    vtkTransferFunctionEditorWidgetSimple1D *self,
    vtkTransferFunctionEditorRepresentationSimple1D *rep,
    unsigned int currentHandleNumber);

  void RecomputeNodePositions(double oldRange[2], double newRange[2]);
  void RecomputeNodePositions(int oldSize[2], int newSize[2]);

  void RemoveNode(unsigned int id);

private:
  vtkTransferFunctionEditorWidgetSimple1D(const vtkTransferFunctionEditorWidgetSimple1D&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorWidgetSimple1D&); // Not implemented.
};

#endif
