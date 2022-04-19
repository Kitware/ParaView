/*=========================================================================

  Program:   ParaView
  Module:    vtkWebPageRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWebPageRepresentation
 *
 * vtkWebPageRepresentation is a representation to show web pages. The input
 * is expected to be a vtkTable with a single row and column (at least on
 * the data server nodes). The content of this entry in the table is shown
 * as a web page on the rendering nodes.
 */

#ifndef vtkWebPageRepresentation_h
#define vtkWebPageRepresentation_h

#include "vtkPVDataRepresentation.h"
#include "vtkXRInterfaceRepresentationsModule.h" // for export macro

class vtk3DWidgetRepresentation;
class vtkQWidgetWidget;
class vtkPlaneSource;
class vtkXRInterfaceWebView;

class VTKXRINTERFACEREPRESENTATIONS_EXPORT vtkWebPageRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkWebPageRepresentation* New();
  vtkTypeMacro(vtkWebPageRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the text widget.
   */
  void SetTextWidgetRepresentation(vtk3DWidgetRepresentation* widget);
  vtkGetObjectMacro(TextWidgetRepresentation, vtk3DWidgetRepresentation);
  //@}

  ///@{
  /**
   * Set/Get the origin of the plane.
   */
  void SetOrigin(double x, double y, double z);
  void SetOrigin(double x[3]);
  double* GetOrigin() VTK_SIZEHINT(3);
  void GetOrigin(double xyz[3]);
  ///@}

  ///@{
  /**
   * Set/Get the position of the point defining the first axis of the plane.
   */
  void SetPoint1(double x, double y, double z);
  void SetPoint1(double x[3]);
  double* GetPoint1() VTK_SIZEHINT(3);
  void GetPoint1(double xyz[3]);
  ///@}

  ///@{
  /**
   * Set/Get the position of the point defining the second axis of the plane.
   */
  void SetPoint2(double x, double y, double z);
  void SetPoint2(double x[3]);
  double* GetPoint2() VTK_SIZEHINT(3);
  void GetPoint2(double xyz[3]);
  ///@}

  /**
   * Set the visibility.
   */
  void SetVisibility(bool) override;

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
    vtkInformation* outInfo) override;

  /**
   * Return a duplciate widget/rep/texture that can be added to a
   * different renderer
   */
  vtkQWidgetWidget* GetDuplicateWidget();

  void SetInputText(const char* input);

protected:
  vtkWebPageRepresentation();
  ~vtkWebPageRepresentation() override;

  vtkQWidgetWidget* QWidgetWidget;

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
  vtkPlaneSource* PlaneSource;
  vtkXRInterfaceWebView* WebView;

private:
  vtkWebPageRepresentation(const vtkWebPageRepresentation&) = delete;
  void operator=(const vtkWebPageRepresentation&) = delete;
};

#endif
