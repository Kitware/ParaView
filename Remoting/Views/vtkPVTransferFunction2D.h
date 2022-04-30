/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTransferFunction2D.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVTransferFunction2D
 * @brief Defines a 2D transfer function for mapping to RGBA values for volume rendering.
 *
 * vtkPVTransferFunction2D is a color mapping in RGBA space that uses 2D functions to allow
 * interpolation along both the X and Y axes of the function.
 */

#ifndef vtkPVTransferFunction2D_h
#define vtkPVTransferFunction2D_h

// VTK includes
#include <vtkObject.h>

#include "vtkPVTransferFunction2DBox.h" // needed for ivar
#include "vtkRemotingViewsModule.h"     // needed for export macro

#include <memory> // for unique_ptr
#include <vector> // needed for ivar

// Forward declarations
class vtkImageData;
class vtkPVTransferFunction2DInternals;

class VTKREMOTINGVIEWS_EXPORT vtkPVTransferFunction2D : public vtkObject
{
public:
  /**
   * Instantiate the class.
   */
  static vtkPVTransferFunction2D* New();

  ///@{
  /**
   * Standard methods for the VTK class.
   */
  vtkTypeMacro(vtkPVTransferFunction2D, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Generate the transfer function image data
   */
  virtual void Build();

  ///@{
  /**
   * Set/Get the output dimensions of the image data that is the transfer function.
   * By default, this is (10, 10).
   */
  vtkSetVector2Macro(OutputDimensions, int);
  vtkGetVector2Macro(OutputDimensions, int);
  ///@}

  ///@{
  /**
   * Add/Remove a control box to/from the 2D transfer function.
   * Returns the index of the added box or -1 on error.
   *
   * \note This function does not check to see if the box already exists in the function before
   * adding it. Adding an existing box would add a duplicate control to the transfer function.
   */
  int AddControlBox(
    double x, double y, double width, double height, double r, double g, double b, double a);
  int AddControlBox(double x, double y, double width, double height, double* color);
  int AddControlBox(vtkRectd& box, double* color);
  int AddControlBox(vtkSmartPointer<vtkPVTransferFunction2DBox> box);
  int SetControlBox(int id, vtkSmartPointer<vtkPVTransferFunction2DBox> box);
  int RemoveControlBox(int id);
  int RemoveControlBox(vtkSmartPointer<vtkPVTransferFunction2DBox> box);
  ///@}

  /**
   * Get the control boxes defined on the function.
   */
  std::vector<vtkSmartPointer<vtkPVTransferFunction2DBox>> GetBoxes();

  /**
   * Remove all control boxes from the transfer function.
   */
  void RemoveAllBoxes();

  ///@{
  /**
   * Set/Get the range of the function.
   * Set/Get the (minX, maxX, minY, maxY) for the 2D transfer function.
   */
  vtkSetVector4Macro(Range, double);
  vtkGetVector4Macro(Range, double);
  ///@}

  /**
   * Get the 2D transfer function.
   */
  vtkImageData* GetFunction();

protected:
  vtkPVTransferFunction2D();
  ~vtkPVTransferFunction2D() override;

  // Helper members
  /**
   * The range for the X and Y axes of the function.
   * This will determine the bounds of the transfer function.
   */
  double Range[4] = { 0, 0, 0, 0 };

  /**
   * The output dimensions for the transfer function.
   */
  int OutputDimensions[2] = { 10, 10 };

  /**
   * Update the range of the function. Returns true if the range changes. If the range has changed,
   * calls Modified().
   * If UseCustomRange is set, this is a no-op.
   */
  bool UpdateRange();

private:
  // Helper members
  std::unique_ptr<vtkPVTransferFunction2DInternals> Internals;

  vtkPVTransferFunction2D(const vtkPVTransferFunction2D&) = delete;
  void operator=(const vtkPVTransferFunction2D) = delete;
};

#endif // vtkPVTransferFunction2D_h
