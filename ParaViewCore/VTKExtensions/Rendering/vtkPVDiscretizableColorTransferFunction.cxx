/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDiscretizableColorTransferFunction.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDiscretizableColorTransferFunction.h"

#include "vtkDoubleArray.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkVariantArray.h"

vtkStandardNewMacro(vtkPVDiscretizableColorTransferFunction);

//-------------------------------------------------------------------------
vtkPVDiscretizableColorTransferFunction::vtkPVDiscretizableColorTransferFunction()
{
  this->AnnotatedValuesInFullSet = NULL;
  this->AnnotationsInFullSet = NULL;
  this->IndexedColorsInFullSet = vtkDoubleArray::New();
  this->IndexedColorsInFullSet->SetNumberOfComponents(3);

  this->ActiveAnnotatedValues = vtkVariantArray::New();

  this->UseActiveValues = 1;
}

//-------------------------------------------------------------------------
vtkPVDiscretizableColorTransferFunction::~vtkPVDiscretizableColorTransferFunction()
{
  if (this->AnnotatedValuesInFullSet)
  {
    this->AnnotatedValuesInFullSet->Delete();
  }

  if (this->AnnotationsInFullSet)
  {
    this->AnnotationsInFullSet->Delete();
  }

  if (this->IndexedColorsInFullSet)
  {
    this->IndexedColorsInFullSet->Delete();
  }

  if (this->ActiveAnnotatedValues)
  {
    this->ActiveAnnotatedValues->Delete();
  }
}

//-------------------------------------------------------------------------
void vtkPVDiscretizableColorTransferFunction::SetAnnotationsInFullSet(
  vtkAbstractArray* values, vtkStringArray* annotations)
{
  if ((values && !annotations) || (!values && annotations))
    return;

  if (values && annotations && values->GetNumberOfTuples() != annotations->GetNumberOfTuples())
  {
    vtkErrorMacro(<< "Values and annotations do not have the same number of tuples ("
                  << values->GetNumberOfTuples() << " and " << annotations->GetNumberOfTuples()
                  << ", respectively. Ignoring.");
    return;
  }

  if (this->AnnotatedValuesInFullSet && !values)
  {
    this->AnnotatedValuesInFullSet->Delete();
    this->AnnotatedValuesInFullSet = 0;
  }
  else if (values)
  { // Ensure arrays are of the same type before copying.
    if (this->AnnotatedValuesInFullSet)
    {
      if (this->AnnotatedValuesInFullSet->GetDataType() != values->GetDataType())
      {
        this->AnnotatedValuesInFullSet->Delete();
        this->AnnotatedValuesInFullSet = 0;
      }
    }
    if (!this->AnnotatedValuesInFullSet)
    {
      this->AnnotatedValuesInFullSet = vtkAbstractArray::CreateArray(values->GetDataType());
    }
  }
  bool sameVals = (values == this->AnnotatedValuesInFullSet);
  if (!sameVals && values)
  {
    this->AnnotatedValuesInFullSet->DeepCopy(values);
  }

  if (this->AnnotationsInFullSet && !annotations)
  {
    this->AnnotationsInFullSet->Delete();
    this->AnnotationsInFullSet = 0;
  }
  else if (!this->AnnotationsInFullSet && annotations)
  {
    this->AnnotationsInFullSet = vtkStringArray::New();
  }
  bool sameText = (annotations == this->AnnotationsInFullSet);
  if (!sameText)
  {
    this->AnnotationsInFullSet->DeepCopy(annotations);
  }
  //  this->UpdateAnnotatedValueMap();
  this->Modified();
}

//----------------------------------------------------------------------------
vtkIdType vtkPVDiscretizableColorTransferFunction::SetAnnotationInFullSet(
  vtkVariant value, vtkStdString annotation)
{
  vtkIdType idx = -1;
  bool modified = false;
  if (this->AnnotatedValuesInFullSet)
  {
    idx = this->AnnotatedValuesInFullSet->LookupValue(value);
    if (idx >= 0)
    {
      if (this->AnnotationsInFullSet->GetValue(idx) != annotation)
      {
        this->AnnotationsInFullSet->SetValue(idx, annotation);
        modified = true;
      }
    }
    else
    {
      idx = this->AnnotationsInFullSet->InsertNextValue(annotation);
      this->AnnotatedValuesInFullSet->InsertVariantValue(idx, value);
      modified = true;
    }
  }
  else
  {
    vtkErrorMacro(<< "AnnotatedValuesInFullSet is NULL");
  }

  if (modified)
  {
    this->Modified();
  }

  return idx;
}

//-------------------------------------------------------------------------
vtkIdType vtkPVDiscretizableColorTransferFunction::SetAnnotationInFullSet(
  vtkStdString value, vtkStdString annotation)
{
  bool valid;
  vtkVariant val(value);
  double x;
  x = val.ToDouble(&valid);
  if (valid)
  {
    return this->SetAnnotationInFullSet(x, annotation);
  }
  else if (value == "")
  {
    // NOTE: This prevents the value "" in vtkStringArrays from being annotated.
    // Hopefully, that isn't a desired use case.
    return -1;
  }
  return this->SetAnnotationInFullSet(val, annotation);
}

//-------------------------------------------------------------------------
void vtkPVDiscretizableColorTransferFunction::ResetAnnotationsInFullSet()
{
  if (!this->AnnotationsInFullSet)
  {
    vtkVariantArray* va = vtkVariantArray::New();
    vtkStringArray* sa = vtkStringArray::New();
    this->SetAnnotationsInFullSet(va, sa);
    va->FastDelete();
    sa->FastDelete();
  }
  this->AnnotatedValuesInFullSet->Initialize();
  this->AnnotationsInFullSet->Initialize();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVDiscretizableColorTransferFunction::ResetActiveAnnotatedValues()
{
  if (this->ActiveAnnotatedValues->GetNumberOfTuples() > 0)
  {
    this->ActiveAnnotatedValues->Initialize();
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkPVDiscretizableColorTransferFunction::SetActiveAnnotatedValue(vtkStdString value)
{
  this->ActiveAnnotatedValues->InsertNextValue(value);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVDiscretizableColorTransferFunction::SetNumberOfIndexedColorsInFullSet(int n)
{
  if (n != this->IndexedColorsInFullSet->GetNumberOfTuples())
  {
    vtkIdType old = this->IndexedColorsInFullSet->GetNumberOfTuples();
    this->IndexedColorsInFullSet->SetNumberOfTuples(n);
    if (old < n)
    {
      double rgb[3] = { 0, 0, 0 };
      for (int i = old; i < n; ++i)
      {
        this->IndexedColorsInFullSet->SetTypedTuple(i, rgb);
      }
    }
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkPVDiscretizableColorTransferFunction::SetIndexedColorInFullSet(
  unsigned int index, double r, double g, double b)
{
  if (index >= static_cast<unsigned int>(this->IndexedColorsInFullSet->GetNumberOfTuples()))
  {
    this->SetNumberOfIndexedColorsInFullSet(index + 1);
    this->Modified();
  }

  // double *currentRGB = static_cast<double*>(this->IndexedColorsInFullSet->GetVoidPointer(index));
  double currentRGB[3];
  this->IndexedColorsInFullSet->GetTypedTuple(index, currentRGB);
  if (currentRGB[0] != r || currentRGB[1] != g || currentRGB[2] != b)
  {
    double rgb[3] = { r, g, b };
    this->IndexedColorsInFullSet->SetTypedTuple(index, rgb);
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
int vtkPVDiscretizableColorTransferFunction::GetNumberOfIndexedColorsInFullSet()
{
  return static_cast<int>(this->IndexedColorsInFullSet->GetNumberOfTuples());
}

//-----------------------------------------------------------------------------
void vtkPVDiscretizableColorTransferFunction::GetIndexedColorInFullSet(
  unsigned int index, double rgb[3])
{
  if (index >= static_cast<unsigned int>(this->IndexedColorsInFullSet->GetNumberOfTuples()))
  {
    vtkErrorMacro(<< "Index out of range. Color not set.");
    return;
  }

  this->IndexedColorsInFullSet->GetTypedTuple(index, rgb);
}

//-----------------------------------------------------------------------------
void vtkPVDiscretizableColorTransferFunction::Build()
{
  if (this->BuildTime > this->GetMTime())
  {
    // no need to rebuild anything.
    return;
  }

  this->ResetAnnotations();

  int annotationCount = 0;

  if (this->AnnotatedValuesInFullSet)
  {
    vtkNew<vtkVariantArray> builtValues;
    vtkNew<vtkStringArray> builtAnnotations;

    for (vtkIdType i = 0; i < this->AnnotatedValuesInFullSet->GetNumberOfTuples(); ++i)
    {
      vtkStdString annotation = this->AnnotationsInFullSet->GetValue(i);
      vtkVariant value = this->AnnotatedValuesInFullSet->GetVariantValue(i);

      bool useAnnotation = true;
      if (this->IndexedLookup && this->UseActiveValues)
      {
        vtkIdType id = this->ActiveAnnotatedValues->LookupValue(value);
        if (id < 0)
        {
          useAnnotation = false;
        }
      }

      if (useAnnotation)
      {
        builtValues->InsertNextValue(value);
        builtAnnotations->InsertNextValue(annotation);

        if (i < this->IndexedColorsInFullSet->GetNumberOfTuples())
        {
          double color[3];
          this->GetIndexedColorInFullSet(i, color);
          this->SetIndexedColor(annotationCount, color);
          annotationCount++;
        }
      }
    }
    this->SetAnnotations(builtValues.GetPointer(), builtAnnotations.GetPointer());
  }

  this->Superclass::Build();

  this->BuildTime.Modified();
}

//-------------------------------------------------------------------------
void vtkPVDiscretizableColorTransferFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
