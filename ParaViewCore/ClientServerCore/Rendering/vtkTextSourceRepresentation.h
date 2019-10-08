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
class vtkBillboardTextActor3D;
class vtkFlagpoleLabel;
class vtkPolyData;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkTextSourceRepresentation
  : public vtkPVDataRepresentation
{
public:
  static vtkTextSourceRepresentation* New();
  vtkTypeMacro(vtkTextSourceRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the text widget.
   */
  void SetTextWidgetRepresentation(vtk3DWidgetRepresentation* widget);
  vtkGetObjectMacro(TextWidgetRepresentation, vtk3DWidgetRepresentation);
  //@}

  /**
   * Set the visibility.
   */
  void SetVisibility(bool) override;

  /**
   * Set the interactivity.
   */
  void SetInteractivity(bool);

  /**
   * Control how the text is rendered. Possible values include
   * 0: render as a 3DWidgetRepresentation
   * 1: Render as a FlagpoleLabel
   */
  void SetTextPropMode(int);

  //@{
  /**
   * Set the FlagpoleLabel
   */
  void SetFlagpoleLabel(vtkFlagpoleLabel* val);
  vtkGetObjectMacro(FlagpoleLabel, vtkFlagpoleLabel);
  //@}

  //@{
  /**
   * Set the BillboardTextActor
   */
  void SetBillboardTextActor(vtkBillboardTextActor3D* val);
  vtkGetObjectMacro(BillboardTextActor, vtkBillboardTextActor3D);
  //@}

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

protected:
  vtkTextSourceRepresentation();
  ~vtkTextSourceRepresentation() override;

  /**
   * Fill input port information.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Overridden to invoke vtkCommand::UpdateDataEvent.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Adds the representation to the view.  This is called from
   * vtkView::AddRepresentation().  Subclasses should override this method.
   * Returns true if the addition succeeds.
   */
  bool AddToView(vtkView* view) override;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  bool RemoveFromView(vtkView* view) override;

  vtkPolyData* DummyPolyData;
  vtk3DWidgetRepresentation* TextWidgetRepresentation;
  vtkFlagpoleLabel* FlagpoleLabel;
  vtkBillboardTextActor3D* BillboardTextActor;
  int TextPropMode;

private:
  vtkTextSourceRepresentation(const vtkTextSourceRepresentation&) = delete;
  void operator=(const vtkTextSourceRepresentation&) = delete;
};

#endif
