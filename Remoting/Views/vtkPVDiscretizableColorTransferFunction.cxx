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
  this->AnnotatedValuesInFullSet = nullptr;
  this->AnnotationsInFullSet = nullptr;
  this->IndexedColorsInFullSet = vtkDoubleArray::New();
  this->IndexedColorsInFullSet->SetNumberOfComponents(3);
  this->IndexedOpacitiesInFullSet = vtkDoubleArray::New();
  this->IndexedOpacitiesInFullSet->SetNumberOfComponents(1);

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

  if (this->IndexedOpacitiesInFullSet)
  {
    this->IndexedOpacitiesInFullSet->Delete();
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
    this->AnnotatedValuesInFullSet = nullptr;
  }
  else if (values)
  { // Ensure arrays are of the same type before copying.
    if (this->AnnotatedValuesInFullSet)
    {
      if (this->AnnotatedValuesInFullSet->GetDataType() != values->GetDataType())
      {
        this->AnnotatedValuesInFullSet->Delete();
        this->AnnotatedValuesInFullSet = nullptr;
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
    this->AnnotationsInFullSet = nullptr;
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
  vtkVariant value, std::string annotation)
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
  std::string value, std::string annotation)
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
void vtkPVDiscretizableColorTransferFunction::SetActiveAnnotatedValue(std::string value)
{
  this->ActiveAnnotatedValues->InsertNextValue(value.c_str());
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVDiscretizableColorTransferFunction::SetNumberOfIndexedColorsInFullSet(int n)
{
  if (n != static_cast<int>(this->IndexedColorsInFullSet->GetNumberOfTuples()))
  {
    vtkIdType old = this->IndexedColorsInFullSet->GetNumberOfTuples();
    this->IndexedColorsInFullSet->SetNumberOfTuples(n);
    if (old < n)
    {
      for (int i = 0; i < 3; i++)
      {
        this->IndexedColorsInFullSet->FillComponent(i, 0.0);
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
    this->SetNumberOfIndexedColorsInFullSet(static_cast<int>(index + 1));
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
  unsigned int index, double* rgb)
{
  if (index >= static_cast<unsigned int>(this->IndexedColorsInFullSet->GetNumberOfTuples()))
  {
    vtkErrorMacro(<< "Index out of range. Color not set.");
    return;
  }

  this->IndexedColorsInFullSet->GetTypedTuple(index, rgb);
}

//-----------------------------------------------------------------------------
void vtkPVDiscretizableColorTransferFunction::SetNumberOfIndexedOpacitiesInFullSet(int n)
{
  if (n != static_cast<int>(this->IndexedOpacitiesInFullSet->GetNumberOfTuples()))
  {
    vtkIdType old = this->IndexedOpacitiesInFullSet->GetNumberOfTuples();
    this->IndexedOpacitiesInFullSet->SetNumberOfTuples(n);
    if (old < n)
    {
      this->IndexedOpacitiesInFullSet->FillComponent(0, 1.0);
    }
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkPVDiscretizableColorTransferFunction::SetIndexedOpacityInFullSet(
  unsigned int index, double alpha)
{
  if (index >= static_cast<unsigned int>(this->IndexedOpacitiesInFullSet->GetNumberOfTuples()))
  {
    this->SetNumberOfIndexedOpacitiesInFullSet(static_cast<int>(index + 1));
    this->Modified();
  }

  double currentAlpha;
  this->IndexedOpacitiesInFullSet->GetTypedTuple(index, &currentAlpha);
  if (currentAlpha != alpha)
  {
    this->IndexedOpacitiesInFullSet->SetTypedTuple(index, &alpha);
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
int vtkPVDiscretizableColorTransferFunction::GetNumberOfIndexedOpacitiesInFullSet()
{
  return static_cast<int>(this->IndexedOpacitiesInFullSet->GetNumberOfTuples());
}

//-----------------------------------------------------------------------------
void vtkPVDiscretizableColorTransferFunction::GetIndexedOpacityInFullSet(
  unsigned int index, double* alpha)
{
  if (index >= static_cast<unsigned int>(this->IndexedOpacitiesInFullSet->GetNumberOfTuples()))
  {
    vtkErrorMacro(<< "Index out of range. Opacity not set.");
    return;
  }

  this->IndexedOpacitiesInFullSet->GetTypedTuple(index, alpha);
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
      std::string annotation = this->AnnotationsInFullSet->GetValue(i);
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
          double color[4];
          this->GetIndexedColorInFullSet(i, color);
          if (this->EnableOpacityMapping &&
            i < this->IndexedOpacitiesInFullSet->GetNumberOfTuples())
          {
            this->GetIndexedOpacityInFullSet(i, &color[3]);
          }
          else
          {
            color[3] = 1.0;
          }
          this->SetIndexedColorRGBA(annotationCount, color);
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
