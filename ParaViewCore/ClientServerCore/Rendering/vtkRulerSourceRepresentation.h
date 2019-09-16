/*=========================================================================

  Program:   ParaView
  Module:    vtkRulerSourceRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRulerSourceRepresentation
 *
 * vtkRulerSourceRepresentation is a representation to show a ruler. The input
 * is expected to be vtkPolyData with two points.
*/

#ifndef vtkRulerSourceRepresentation_h
#define vtkRulerSourceRepresentation_h

#include "vtkNew.h"                               //needed for instances of vtkPolyData
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataRepresentation.h"

class vtkDistanceRepresentation2D;
class vtkPolyData;
class vtkProperty2D;
class vtkTextProperty;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkRulerSourceRepresentation
  : public vtkPVDataRepresentation
{
public:
  static vtkRulerSourceRepresentation* New();
  vtkTypeMacro(vtkRulerSourceRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the text widget.
   */
  void SetDistanceRepresentation(vtkDistanceRepresentation2D*);
  vtkGetObjectMacro(DistanceRepresentation, vtkDistanceRepresentation2D);
  //@}

  /**
   * Set the text property for the printed distance.
   */
  void SetTextProperty(vtkTextProperty* prop);

  /**
   * Set the line width for the displayed axis in screen units.
   */
  void SetAxisLineWidth(float width);

  /**
   * Set the color for the displayed axis.
   */
  void SetAxisColor(double red, double green, double blue);

  /**
   * Set the visibility.
   */
  void SetVisibility(bool) override;

  /**
   * Specify the format to use for labeling the distance. Note that an empty
   * string results in no label, or a format string without a "%" character
   * will not print the distance value.
   */
  void SetLabelFormat(char* labelFormat);

  //@{
  /**
   * Enable or disable ruler mode. When enabled, the ticks on the distance
   * widget are separated by the amount specified by
   * vtkDistanceRepresentation::RulerDistance. Otherwise, the value
   * vtkDistanceRepresentation::NumberOfRulerTicks is used to draw the tick
   * marks.
   */
  void SetRulerMode(int choice);
  int GetRulerMode();
  //@}

  //@{
  /**
   * Specify the RulerDistance which indicates the spacing of the major ticks
   * in the unit space obtained after the Scale is applied (see SetScale()).
   * This ivar only has effect when the RulerMode is on.
   */
  void SetRulerDistance(double distance);
  double GetRulerDistance();
  //@}

  //@{
  /**
   * Set scale factor to apply to the ruler graduation scale and the displayed
   * distance. Used to transform VTK world space units to a desired unit,
   * e.g., inches to centimeters.
   */
  void SetScale(double distance);
  double GetScale();
  //@}

  /**
   * Specify the number of major ruler ticks. Note: the number of ticks is the
   * number between the two handle endpoints. This ivar only has effect
   * when the RulerMode is off.
   */
  void SetNumberOfRulerTicks(int numberOfRulerTicks);

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

protected:
  vtkRulerSourceRepresentation();
  ~vtkRulerSourceRepresentation() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Fill input port information.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

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

  vtkDistanceRepresentation2D* DistanceRepresentation;
  vtkNew<vtkPolyData> Clone;

private:
  vtkRulerSourceRepresentation(const vtkRulerSourceRepresentation&) = delete;
  void operator=(const vtkRulerSourceRepresentation&) = delete;
};

#endif
