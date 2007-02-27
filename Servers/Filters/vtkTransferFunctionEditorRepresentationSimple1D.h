/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorRepresentationSimple1D.h,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransferFunctionEditorRepresentationSimple1D - a representation of a 3D widget for manipulating a transfer function
// .SECTION Description
// vtkTransferFunctionEditorRepresentationSimple1D is the representation
// associated with vtkTransferFunctionEditorWidgetSimple1D. It is used for
// displaying / manipulating a 1D transfer function using nodes.

#ifndef __vtkTransferFunctionEditorRepresentationSimple1D_h
#define __vtkTransferFunctionEditorRepresentationSimple1D_h

#include "vtkTransferFunctionEditorRepresentation1D.h"

class vtkActor2D;
class vtkGlyphSource2D;
class vtkHandleList;
class vtkHandleRepresentation;
class vtkPointHandleRepresentation2D;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkTransformPolyDataFilter;

class VTK_EXPORT vtkTransferFunctionEditorRepresentationSimple1D : public vtkTransferFunctionEditorRepresentation1D
{
public:
  static vtkTransferFunctionEditorRepresentationSimple1D* New();
  vtkTypeRevisionMacro(vtkTransferFunctionEditorRepresentationSimple1D, vtkTransferFunctionEditorRepresentation1D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Put together the parts necessary for displaying this 3D widget.
  virtual void BuildRepresentation();

  // Description:
  // Given (x, y) display coordinates in a renderer, with a possible flag that
  // modifies the computation, determine the current state of the widget.
  virtual int ComputeInteractionState(int x, int y, int modify=0);

  // Description:
  // Create a new handle representation for a transfer function node at a
  // particular display position. Returns the id of the node.
  unsigned int CreateHandle(double displayPos[3]);

  // Description:
  // Return the vtkHandleRepresentation (for a transfer function node) with a
  // particular id. Returns NULL if the id is out of range.
  vtkHandleRepresentation* GetHandleRepresentation(unsigned int idx);

  // Description:
  // Set/get the display position of a vtkHandleRepresentation (for a transfer
  // function node) with a particular id.
  virtual void SetHandleDisplayPosition(unsigned int nodeNum, double pos[3]);
  void GetHandleDisplayPosition(unsigned int nodeNum, double pos[3]);
  double* GetHandleDisplayPosition(unsigned int nodeNum);

  // Description:
  // Remove a vtkHandleRepresentation (for a transfer function node) with a
  // particular id.
  virtual void RemoveHandle(unsigned int id);

  // Description:
  // Remove all the vtkHandleRepresentations currently used by this class.
  void RemoveAllHandles();

  // Description:
  // Set/get the index of the currently active handle (node).
  void SetActiveHandle(unsigned int handle);
  vtkGetMacro(ActiveHandle, unsigned int);

  // Description:
  // Rendering methods
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentGeometry(vtkViewport *viewport);
  virtual int RenderOverlay(vtkViewport *viewport);

//BTX
  enum
  {
    Outside = 0,
    NearNode
  };
//ETX

  // Description:
  // Set the color for the specified handle.
  void SetHandleColor(unsigned int idx, double r, double g, double b);

protected:
  vtkTransferFunctionEditorRepresentationSimple1D();
  ~vtkTransferFunctionEditorRepresentationSimple1D();

  void UpdateHandleProperty(vtkPointHandleRepresentation2D *handleRep);

  vtkHandleList *Handles;
  vtkPointHandleRepresentation2D *HandleRepresentation;
  vtkGlyphSource2D *HandlePolyDataSource;
  vtkTransformPolyDataFilter *ActiveHandleFilter;
  unsigned int ActiveHandle;
  int Tolerance;  // Selection tolerance for the handles

  vtkPolyData *Lines;
  vtkPolyDataMapper2D *LinesMapper;
  vtkActor2D *LinesActor;

  void HighlightActiveHandle();

private:
  vtkTransferFunctionEditorRepresentationSimple1D(const vtkTransferFunctionEditorRepresentationSimple1D&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorRepresentationSimple1D&); // Not implemented.
};

#endif
