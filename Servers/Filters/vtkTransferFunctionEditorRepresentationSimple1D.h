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

class vtkActor;
class vtkColorTransferFunction;
class vtkHandleList;
class vtkHandleRepresentation;
class vtkPointHandleRepresentationSphere;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkTransformPolyDataFilter;

class VTK_EXPORT vtkTransferFunctionEditorRepresentationSimple1D : public vtkTransferFunctionEditorRepresentation1D
{
public:
  static vtkTransferFunctionEditorRepresentationSimple1D* New();
  vtkTypeMacro(vtkTransferFunctionEditorRepresentationSimple1D, vtkTransferFunctionEditorRepresentation1D);
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
  unsigned int CreateHandle(double displayPos[3], double scalar);

  // Description:
  // Get the scalar value associated with the handle at the given index.
  double GetHandleScalar(unsigned int idx, int &valid);

  // Description:
  // Return the number of existing handles.
  virtual unsigned int GetNumberOfHandles();

  // Description:
  // Return the vtkHandleRepresentation (for a transfer function node) with a
  // particular id. Returns NULL if the id is out of range.
  vtkHandleRepresentation* GetHandleRepresentation(unsigned int idx);

  // Description:
  // Set/get the display position of a vtkHandleRepresentation (for a transfer
  // function node) with a particular id.
  // When setting the position, a return value of 1 indicates the node
  // was set to the position indicated. A return value of 0 indicates the
  // nodes was not repositioned.
  virtual int SetHandleDisplayPosition(unsigned int nodeNum, double pos[3],
                                       double scalar);
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
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);
  virtual int RenderOverlay(vtkViewport *viewport);
  virtual int HasTranslucentPolygonalGeometry();

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Release any graphics resources that are being consumed by this widget
  // representation. The parameter window could be used to determine which
  // graphic resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *window);

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

  // Description:
  // Pass the color transfer function to the representation.
  // Used for coloring the lines by the color transfer function.
  virtual void SetColorFunction(vtkColorTransferFunction *color);

  // Description:
  // Tell the representation whether the lines should be a solid color or
  // whether they should display the color transfer function.
  virtual void SetColorLinesByScalar(int color);

  // Description:
  // Specify the color to use for the lines if they are not displaying
  // the color transfer function.
  virtual void SetLinesColor(double r, double g, double b);

  // Description:
  // Specify whether the node color should be determined by the color
  // transfer function.
  virtual void SetColorElementsByColorFunction(int color);

  // Description:
  // Set the color to use for nodes if not coloring them by the color
  // transfer function.
  virtual void SetElementsColor(double r, double g, double b);
  virtual void SetElementsColor(double color[3])
    { this->SetElementsColor(color[0], color[1], color[2]); }

  // Description:
  // Set the lighting parameters for the transfer function editor elements.
  virtual void SetElementLighting(double ambient, double diffuse,
                                  double specular, double specularPower);

  // Description:
  // Get the tolerance value used for determining whether a handle is selected.
  vtkGetMacro(Tolerance, int);

protected:
  vtkTransferFunctionEditorRepresentationSimple1D();
  ~vtkTransferFunctionEditorRepresentationSimple1D();

  void UpdateHandleProperty(vtkPointHandleRepresentationSphere *handleRep);
  void HighlightActiveHandle();
  void ColorAllElements();

  vtkHandleList *Handles;
  vtkPointHandleRepresentationSphere *HandleRepresentation;
  vtkTransformPolyDataFilter *ActiveHandleFilter;
  unsigned int ActiveHandle;
  int Tolerance;  // Selection tolerance for the handles

  vtkPolyData *Lines;
  vtkPolyDataMapper *LinesMapper;
  vtkActor *LinesActor;

private:
  vtkTransferFunctionEditorRepresentationSimple1D(const vtkTransferFunctionEditorRepresentationSimple1D&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorRepresentationSimple1D&); // Not implemented.
};

#endif
