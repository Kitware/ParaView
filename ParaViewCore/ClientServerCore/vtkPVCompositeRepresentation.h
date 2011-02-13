/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCompositeRepresentation - a data-representation used by ParaView.
// .SECTION Description
// vtkPVCompositeRepresentation is a data-representation used by ParaView for showing
// a type of data-set in the render view. It is a composite-representation with
// some fixed representations for showing things like selection and cube-axes.
// This representation has two input ports:
// \li 0: the dataset to show
// \li 1: the extracted selection to show

#ifndef __vtkPVCompositeRepresentation_h
#define __vtkPVCompositeRepresentation_h

#include "vtkCompositeRepresentation.h"

class vtkSelectionRepresentation;
class vtkCubeAxesRepresentation;

class VTK_EXPORT vtkPVCompositeRepresentation : public vtkCompositeRepresentation
{
public:
  static vtkPVCompositeRepresentation* New();
  vtkTypeMacro(vtkPVCompositeRepresentation, vtkCompositeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These must only be set during initialization before adding the
  // representation to any views or calling Update().
  void SetCubeAxesRepresentation(vtkCubeAxesRepresentation*);
  void SetSelectionRepresentation(vtkSelectionRepresentation*);

  // Description:
  // Overridden to simply pass the input to the internal representations. We
  // won't need this if vtkDataRepresentation correctly respected in the
  // arguments passed to it during ProcessRequest() etc.
  virtual void SetInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void SetInputConnection(vtkAlgorithmOutput* input);
  virtual void AddInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void AddInputConnection(vtkAlgorithmOutput* input);
  virtual void RemoveInputConnection(int port, vtkAlgorithmOutput* input);

  // Description:
  // Propagate the modification to all internal representations.
  virtual void MarkModified();

  // Description:
  // Set visibility of the representation.
  // Overridden to update the cube-axes and selection visibilities.
  virtual void SetVisibility(bool visible);

  void SetCubeAxesVisibility(bool visible);
  void SetSelectionVisibility(bool visible);

  // Description:
  // Passed on to internal representations as well.
  virtual void SetUpdateTime(double time);
  virtual void SetUseCache(bool val);
  virtual void SetCacheKey(double val);
  virtual void SetForceUseCache(bool val);
  virtual void SetForcedCacheKey(double val);

  // Description:
  // Forwarded to vtkSelectionRepresentation.
  virtual void SetPointFieldDataArrayName(const char*);
  virtual void SetCellFieldDataArrayName(const char*);

//BTX
protected:
  vtkPVCompositeRepresentation();
  ~vtkPVCompositeRepresentation();

  // Description:
  // Fill input port information.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().  Subclasses should override this method.
  // Returns true if the addition succeeds.
  virtual bool AddToView(vtkView* view);

  // Description:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().  Subclasses should override this method.
  // Returns true if the removal succeeds.
  virtual bool RemoveFromView(vtkView* view);

  vtkCubeAxesRepresentation* CubeAxesRepresentation;
  vtkSelectionRepresentation* SelectionRepresentation;

  bool CubeAxesVisibility;
  bool SelectionVisibility;

private:
  vtkPVCompositeRepresentation(const vtkPVCompositeRepresentation&); // Not implemented
  void operator=(const vtkPVCompositeRepresentation&); // Not implemented
//ETX
};

#endif
