/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDiscretizableColorTransferFunction.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVDiscretizableColorTransferFunction
 * @brief   custom version of
 * vtkDiscretizableColorTransferFunction that adds some functionality.
 *
 * This class is the same as vtkDiscretizableColorTransferFunction, but
 * it adds the concept of "active" annotations. These annotations are a
 * subset of the full list of annotations available and are used in place
 * of the full annotation list.
 */

#ifndef vtkPVDiscretizableColorTransferFunction_h
#define vtkPVDiscretizableColorTransferFunction_h

#include "vtkDiscretizableColorTransferFunction.h"

#include "vtkRemotingViewsModule.h"     // needed for export macro
#include "vtkTransferFunctionBoxItem.h" // needed for ivar

#include <vector> // needed for ivar

class vtkAbstractArray;
class vtkDoubleArray;
class vtkImageData;
class vtkVariantArray;

class VTKREMOTINGVIEWS_EXPORT vtkPVDiscretizableColorTransferFunction
  : public vtkDiscretizableColorTransferFunction
{
public:
  static vtkPVDiscretizableColorTransferFunction* New();
  vtkTypeMacro(vtkPVDiscretizableColorTransferFunction, vtkDiscretizableColorTransferFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Parallel API to API for annotated values to set/get the full list of annotations.
   * A subset of the full list will be used.
   */
  virtual void SetAnnotationsInFullSet(vtkAbstractArray* values, vtkStringArray* annotations);
  vtkGetObjectMacro(AnnotatedValuesInFullSet, vtkAbstractArray);
  vtkGetObjectMacro(AnnotationsInFullSet, vtkStringArray);
  virtual vtkIdType SetAnnotationInFullSet(vtkVariant value, std::string annotation);
  virtual vtkIdType SetAnnotationInFullSet(std::string value, std::string annotation);
  virtual void ResetAnnotationsInFullSet();
  //@}

  void ResetActiveAnnotatedValues();
  void SetActiveAnnotatedValue(std::string value);

  void SetNumberOfIndexedColorsInFullSet(int n);
  int GetNumberOfIndexedColorsInFullSet();
  void SetIndexedColorInFullSet(unsigned int index, double r, double g, double b);
  void GetIndexedColorInFullSet(unsigned int index, double rgb[3]);

  void SetNumberOfIndexedOpacitiesInFullSet(int n);
  int GetNumberOfIndexedOpacitiesInFullSet();
  void SetIndexedOpacityInFullSet(unsigned int index, double alpha);
  void GetIndexedOpacityInFullSet(unsigned int index, double* alpha);

  //@{
  /**
   * Set whether to use restrict annotations to only the values
   * designated as active. Off by default.
   */
  vtkSetMacro(UseActiveValues, bool);
  vtkGetMacro(UseActiveValues, bool);
  vtkBooleanMacro(UseActiveValues, bool);
  //@}

  /**
   * Override to set only the active annotations
   */
  void Build() override;

  ///@{
  /**
   * Set/get the 2D transfer function.
   */
  virtual void SetTransferFunction2D(vtkImageData* f);
  virtual vtkImageData* GetTransferFunction2D() const;
  ///@}

  ///@{
  /**
   * Add/Remove a transfer function box control to the 2D transfer function.
   * Returns the index for the added box or -1 on error.
   */
  virtual int AddTransfer2DBox(double x, double y, double width, double height, double r, double g,
    double b, double a, double rangexmin = 0, double rangexmax = 1, double rangeymin = 0,
    double rangeymax = 1);
  virtual int AddTransfer2DBox(
    double x, double y, double width, double height, double* color, double alpha, double* range);
  virtual int AddTransfer2DBox(vtkSmartPointer<vtkTransferFunctionBoxItem> box);
  virtual int RemoveTransfer2DBox(int n);
  virtual int RemoveTransfer2DBox(vtkSmartPointer<vtkTransferFunctionBoxItem> box);
  virtual void RemoveAllTransfer2DBoxes();
  ///@}

  /**
   * Get access to the 2D transfer function boxes.
   */
  std::vector<vtkSmartPointer<vtkTransferFunctionBoxItem>> GetTransferFunction2DBoxes() const;

protected:
  vtkPVDiscretizableColorTransferFunction();
  ~vtkPVDiscretizableColorTransferFunction() override;

private:
  vtkPVDiscretizableColorTransferFunction(const vtkPVDiscretizableColorTransferFunction&) = delete;
  void operator=(const vtkPVDiscretizableColorTransferFunction&) = delete;

  //@{
  /**
   * All annotations.
   */
  vtkAbstractArray* AnnotatedValuesInFullSet;
  vtkStringArray* AnnotationsInFullSet;
  //@}

  vtkDoubleArray* IndexedColorsInFullSet;
  vtkDoubleArray* IndexedOpacitiesInFullSet;

  /**
   * Set of active annotations.
   */
  vtkVariantArray* ActiveAnnotatedValues;

  /**
   * Set whether only "active" annotations should be display. If off, show all
   * annotations.
   */
  bool UseActiveValues;

  /**
   * Build time for this subclass.
   */
  vtkTimeStamp BuildTime;

  /**
   * 2D transfer function
   */
  vtkSmartPointer<vtkImageData> TransferFunction2D;
  std::vector<vtkSmartPointer<vtkTransferFunctionBoxItem>> TransferFunction2DBoxes;
};

#endif
