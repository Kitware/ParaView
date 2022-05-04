/*=========================================================================

  Program:   ParaView
  Module:    vtkTransferFunctionChartHistogram2D.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkTransferFunctionChartHistogram2D_h
#define vtkTransferFunctionChartHistogram2D_h

#include "vtkChartHistogram2D.h"

#include "vtkCommand.h"             // needed for vtkCommand::UserEvent
#include "vtkRemotingViewsModule.h" // needed for export macro
#include "vtkWeakPointer.h"         // needed for vtkWeakPointer

#include <vector> // needed for ivar

// Forward declarations
class vtkContext2D;
class vtkContextMouseEvent;
class vtkPVTransferFunction2D;
class vtkTransferFunctionBoxItem;

class VTKREMOTINGVIEWS_EXPORT vtkTransferFunctionChartHistogram2D : public vtkChartHistogram2D
{
public:
  static vtkTransferFunctionChartHistogram2D* New();
  vtkTypeMacro(vtkTransferFunctionChartHistogram2D, vtkChartHistogram2D);

  // Events fires by this class (and subclasses).
  // \li TransferFunctionModified is fired when the 2D transfer function is modified.
  enum
  {
    TransferFunctionModified = vtkCommand::UserEvent + 1000,
  };

  /**
   * Get whether the chart has been initialized with a histogram.
   */
  bool IsInitialized();

  ///@{
  /**
   * Add a new box item to the chart.
   * If addToTF2D is set to true (default), the box is also added to the client transfer function.
   * This flag is set to false when adding box items corresponding to the boxes already existing in
   * the transfer function.
   */
  vtkSmartPointer<vtkTransferFunctionBoxItem> AddNewBox();
  vtkSmartPointer<vtkTransferFunctionBoxItem> AddNewBox(
    const vtkRectd& r, double* color, double alpha, bool addToTF2D = true);
  void AddBox(vtkSmartPointer<vtkTransferFunctionBoxItem> box, bool addToTF2D = true);
  ///@}

  /**
   * Remove box item from the chart
   */
  virtual void RemoveBox(vtkSmartPointer<vtkTransferFunctionBoxItem> box);

  /**
   * Override to add new box item to the chart when double clicked.
   */
  bool MouseDoubleClickEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Override to delete the active box item
   */
  bool KeyPressEvent(const vtkContextKeyEvent& key) override;

  /**
   * Set the input image data for the 2D histogram
   */
  void SetInputData(vtkImageData*, vtkIdType z = 0) override;

  ///@{
  /**
   * Set/Get the 2D transfer function
   */
  virtual void SetTransferFunction2D(vtkPVTransferFunction2D* transfer2D);
  virtual vtkPVTransferFunction2D* GetTransferFunction2D();
  ///@}

  ///@{
  /**
   * Set/Get the actively selected box.
   */
  vtkSmartPointer<vtkTransferFunctionBoxItem> GetActiveBox() const;
  void SetActiveBox(vtkSmartPointer<vtkTransferFunctionBoxItem> box);
  ///@}

  ///@{
  /**
   * Set active box color and alpha.
   */
  void SetActiveBoxColorAlpha(double r, double g, double b, double a);
  void SetActiveBoxColorAlpha(double color[3], double alpha);
  ///@}

  /**
   * Paint event for the chart, called whenever the chart needs to be drawn
   */
  bool Paint(vtkContext2D* painter) override;

protected:
  vtkTransferFunctionChartHistogram2D() = default;
  ~vtkTransferFunctionChartHistogram2D() override = default;

  /**
   * Update individual item bounds based on the chart range.
   */
  void UpdateItemsBounds(
    const double xMin, const double xMax, const double yMin, const double yMax);

  /**
   * Generate the 2D transfer function from the box items.
   */
  virtual void GenerateTransfer2D();

  /**
   * Callback listening to the SelectionChangedEvent of box items to indicate that the 2D transfer
   * function was modified.
   */
  void OnTransferFunctionBoxItemModified(vtkObject* caller, unsigned long eid, void* callData);

  // Member variables;
  vtkWeakPointer<vtkPVTransferFunction2D> TransferFunction2D;
  vtkSmartPointer<vtkTransferFunctionBoxItem> ActiveBox;
  std::vector<int> BoxesToRemove;

private:
  vtkTransferFunctionChartHistogram2D(const vtkTransferFunctionChartHistogram2D&);
  void operator=(const vtkTransferFunctionChartHistogram2D&);
};

#endif // vtkTransferFunctionChartHistogram2D_h
