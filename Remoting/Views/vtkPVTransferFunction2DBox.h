// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVTransferFunction2DBox
 * @brief Shape that represents an individual control node in a 2D transfer function
 *
 */

#ifndef vtkPVTransferFunction2DBox_h
#define vtkPVTransferFunction2DBox_h

// VTK includes
#include <vtkObject.h>

#include "vtkRemotingViewsModule.h" // needed for export macro

#include <vtkRect.h> // needed for ivar

// Forward declarations
class vtkImageData;

class VTKREMOTINGVIEWS_EXPORT vtkPVTransferFunction2DBox : public vtkObject
{
public:
  /**
   * Instantiate the class.
   */
  static vtkPVTransferFunction2DBox* New();

  ///@{
  /**
   * Standard methods for the VTK class.
   */
  vtkTypeMacro(vtkPVTransferFunction2DBox, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Returns the current box as [x0, y0, width, height].
   */
  virtual const vtkRectd& GetBox();

  ///@{
  /**
   * Set position and width with respect to origin i.e. bottom left corner.
   */
  virtual void SetBox(double x, double y, double width, double height);
  virtual void SetBox(const vtkRectd& b);
  ///@}

  ///@{
  /**
   * Set/Get the color (r,g,b,a) to be used for this box.
   * Defaults to opaque white (1, 1, 1, 1).
   */
  vtkSetVector4Macro(Color, double);
  vtkGetVector4Macro(Color, double);
  ///@}

  /**
   * Get the texture for this box item.
   * The texture will be computed, if needed.
   */
  virtual vtkImageData* GetTexture();

  ///@{
  /**
   * Set/Get the texture size of the box.
   * Defaults to (128, 128).
   */
  vtkSetVector2Macro(TextureSize, int);
  vtkGetVector2Macro(TextureSize, int);
  ///@}

  ///@{
  /**
   * Set/Get the standard deviation for the gaussian function.
   * Defaults to 30.
   */
  vtkSetMacro(GaussianSigmaFactor, double);
  vtkGetMacro(GaussianSigmaFactor, double);
  ///@}

protected:
  vtkPVTransferFunction2DBox();
  ~vtkPVTransferFunction2DBox() override;

  // Helper members
  vtkRectd Box;
  double Color[4] = { 1, 1, 1, 1 };
  int TextureSize[2] = { 128, 128 };
  double GaussianSigmaFactor = 30.0;

  vtkImageData* Texture = nullptr;

  /**
   * Internal method to compute texture.
   */
  virtual void ComputeTexture();

private:
  vtkPVTransferFunction2DBox(const vtkPVTransferFunction2DBox&) = delete;
  void operator=(const vtkPVTransferFunction2DBox) = delete;
};

#endif // vtkPVTransferFunction2DBox_h
