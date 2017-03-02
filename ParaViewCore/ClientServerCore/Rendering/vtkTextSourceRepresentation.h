/*=========================================================================

  Program:   ParaView
  Module:    vtkTextSourceRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTextSourceRepresentation
 *
 * vtkTextSourceRepresentation is a representation to show text. The input is
 * expected to a vtkTable with a single row and column (atleast on the data
 * server nodes). The content of this entry in the table is shown as text on the
 * rendering nodes.
*/

#ifndef vtkTextSourceRepresentation_h
#define vtkTextSourceRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataRepresentation.h"

class vtk3DWidgetRepresentation;
class vtkPolyData;
class vtkPVCacheKeeper;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkTextSourceRepresentation
  : public vtkPVDataRepresentation
{
public:
  static vtkTextSourceRepresentation* New();
  vtkTypeMacro(vtkTextSourceRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set the text widget.
   */
  void SetTextWidgetRepresentation(vtk3DWidgetRepresentation* widget);
  vtkGetObjectMacro(TextWidgetRepresentation, vtk3DWidgetRepresentation);
  //@}

  virtual void MarkModified() VTK_OVERRIDE;

  /**
   * Set the visibility.
   */
  virtual void SetVisibility(bool) VTK_OVERRIDE;

  /**
   * Set the interactivity.
   */
  void SetInteractivity(bool);

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) VTK_OVERRIDE;

protected:
  vtkTextSourceRepresentation();
  ~vtkTextSourceRepresentation();

  /**
   * Fill input port information.
   */
  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  /**
   * Overridden to invoke vtkCommand::UpdateDataEvent.
   */
  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  /**
   * Adds the representation to the view.  This is called from
   * vtkView::AddRepresentation().  Subclasses should override this method.
   * Returns true if the addition succeeds.
   */
  virtual bool AddToView(vtkView* view) VTK_OVERRIDE;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  virtual bool RemoveFromView(vtkView* view) VTK_OVERRIDE;

  /**
   * Overridden to check with the vtkPVCacheKeeper to see if the key is cached.
   */
  virtual bool IsCached(double cache_key) VTK_OVERRIDE;

  vtkPVCacheKeeper* CacheKeeper;
  vtkPolyData* DummyPolyData;
  vtk3DWidgetRepresentation* TextWidgetRepresentation;

private:
  vtkTextSourceRepresentation(const vtkTextSourceRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTextSourceRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
