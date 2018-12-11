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

#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class vtkAbstractArray;
class vtkDoubleArray;
class vtkVariantArray;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVDiscretizableColorTransferFunction
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
  virtual vtkIdType SetAnnotationInFullSet(vtkVariant value, vtkStdString annotation);
  virtual vtkIdType SetAnnotationInFullSet(vtkStdString value, vtkStdString annotation);
  virtual void ResetAnnotationsInFullSet();
  //@}

  void ResetActiveAnnotatedValues();
  void SetActiveAnnotatedValue(vtkStdString value);

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
};

#endif
