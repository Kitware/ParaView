/*=========================================================================

  Module:    vtkKWHistogram.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWHistogram.h"

#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkKWMath.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkDoubleArray.h"

#define VTK_KW_HIST_TESTING 0

#define VTK_KW_ASSUME_LOWER_BOUND_IS_IN_TYPE_RANGE 1

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWHistogram);
vtkCxxRevisionMacro(vtkKWHistogram, "1.1");

//----------------------------------------------------------------------------
vtkKWHistogram::vtkKWHistogram()
{
  this->Range[0]                 = 0;
  this->Range[1]                 = 1.0;

  this->Bins                     = vtkDoubleArray::New();
  this->EmptyHistogram();

  this->Image                    = NULL;
  this->LastImageDescriptor      = NULL;
  this->LastImageBuildTime       = 0;
  this->LastTransferFunctionTime = 0;
  this->LogMode                  = 1;

  this->LastStatisticsBuildTime  = 0;
  this->MaximumNumberOfBins      = 10000;
}

//----------------------------------------------------------------------------
vtkKWHistogram::~vtkKWHistogram()
{
  if (this->Bins)
    {
    this->Bins->Delete();
    this->Bins = NULL;
    }

  if (this->Image)
    {
    this->Image->Delete();
    this->Image = NULL;
    }

  if (this->LastImageDescriptor)
    {
    delete this->LastImageDescriptor;
    this->LastImageDescriptor = NULL;
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkKWHistogram::GetNumberOfBins()
{
  if (this->Bins)
    {
    return this->Bins->GetNumberOfTuples();
    }
  
  return 0;
}

//----------------------------------------------------------------------------

// Special version for unsigned long: since we can not promote to any
// signed bigger type, use double for safety (I have no data to verify)

void vtkKWHistogramBuildInt(
  unsigned long *data, vtkIdType nb_tuples, int nb_of_components, vtkKWHistogram *self)
{
  if (!data || !nb_tuples || nb_of_components < 1 || !self)
    {
    return;
    }

  typedef double cast_type;

  double *bins_ptr = self->GetBins()->GetPointer(0);
  cast_type shift = (cast_type)self->GetRange()[0];

  unsigned long *data_end = data + nb_tuples * nb_of_components;
  while (data < data_end)
    {
    bins_ptr[(int)(cast_type(*data) - shift)]++;
    data += nb_of_components;
    }
}

// Normal version for any integer type

#if VTK_KW_ASSUME_LOWER_BOUND_IS_IN_TYPE_RANGE

template <class T>
void vtkKWHistogramBuildInt(
  T *data, vtkIdType nb_tuples, int nb_of_components, vtkKWHistogram *self)
{
  if (!data || !nb_tuples || nb_of_components < 1 || !self)
    {
    return;
    }

  double *bins_ptr = self->GetBins()->GetPointer(0);
  T shift = (T)self->GetRange()[0];

  T *data_end = data + nb_tuples * nb_of_components;
  while (data < data_end)
    {
    bins_ptr[*data - shift]++;
    data += nb_of_components;
    }
}

#else

template <class T>
void vtkKWHistogramBuildInt(
  T *data, vtkIdType nb_tuples, int nb_of_components, vtkKWHistogram *self)
{
  if (!data || !nb_tuples || nb_of_components < 1 || !self)
    {
    return;
    }
  
  typedef long cast_type;

  double *bins_ptr = self->GetBins()->GetPointer(0);
  cast_type shift = (cast_type)self->GetRange()[0];

  T *data_end = data + nb_tuples * nb_of_components;
  while (data < data_end)
    {
    bins_ptr[cast_type(*data) - shift]++;
    data += nb_of_components;
    }
}

#endif

template <class T>
void vtkKWHistogramBuildFloat(
  T *data, vtkIdType nb_tuples, int nb_of_components, vtkKWHistogram *self)
{
  if (!data || !nb_tuples || nb_of_components < 1 || !self)
    {
    return;
    }

  double range[2];
  self->GetRange(range);
  double bin_width = (double)self->GetNumberOfBins() / (range[1] - range[0]);

  double *bins_ptr = self->GetBins()->GetPointer(0);

  T *data_end = data + nb_tuples * nb_of_components;
  while (data < data_end)
    {
    bins_ptr[vtkMath::Floor(((double)*data - range[0]) * bin_width)]++;
    data += nb_of_components;
    }
}

template <class T>
void vtkKWHistogramBuildIntOrFloat(
  T *data, vtkIdType nb_tuples, int nb_of_components, vtkKWHistogram *self)
{
  if (!data || !nb_tuples || nb_of_components < 1 || !self)
    {
    return;
    }

  double range[2];
  self->GetRange(range);
  if (self->GetNumberOfBins() == (vtkIdType)(range[1] - range[0]))
    {
    vtkKWHistogramBuildInt(data, nb_tuples, nb_of_components, self);
    }
  else
    {
    vtkKWHistogramBuildFloat(data, nb_tuples, nb_of_components, self);
    }
}

//----------------------------------------------------------------------------
void vtkKWHistogram::EmptyHistogram()
{
  if (this->Bins)
    {
    this->Bins->SetNumberOfComponents(1);
    this->Bins->SetNumberOfTuples(0);
    }
}

//----------------------------------------------------------------------------
void vtkKWHistogram::EstimateHistogramRangeAndNumberOfBins(
  vtkDataArray *scalars, 
  int comp,
  double range[2], 
  vtkIdType *nb_of_bins)
{
  if (!scalars ||
      comp < 0 || comp >= scalars->GetNumberOfComponents() ||
      !range || !nb_of_bins)
    {
    return;
    }

  // Get the histogram range required for those scalars.
  // For some big data types (int, long, double), let's use the more
  // accurate GetScalarRange(..., double) function, which does
  // not cache the range (it iterates over data each time), but
  // get the proper result and avoid accessing memory out of the array.
  // If 'reset_range' is true, the Range ivar is reset to the range
  // that was just computed. If not (accumulate mode), make sure the
  // current range fits inside the Range ivar.

  // Given the range, compute the number of bins required.

  double dscalars_range[2];
  double try_nb_of_bins, delta;

  switch (scalars->GetDataType())
    {
    case VTK_CHAR:
    case VTK_UNSIGNED_CHAR:
      range[0] = scalars->GetDataTypeMin();
      range[1] = scalars->GetDataTypeMax() + 1.0;
      try_nb_of_bins = (range[1] - range[0]);
      break;

    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
      vtkKWMath::GetScalarRange(scalars, comp, dscalars_range);
      range[0] = dscalars_range[0];
      range[1] = dscalars_range[1] + 1.0;
      try_nb_of_bins = (range[1] - range[0]);
      break;

    case VTK_INT:
    case VTK_UNSIGNED_INT:
    case VTK_LONG:
    case VTK_UNSIGNED_LONG:
      vtkKWMath::GetScalarRange(scalars, comp, dscalars_range);
      range[0] = dscalars_range[0];
      range[1] = dscalars_range[1] + 1.0;
      try_nb_of_bins = (range[1] - range[0]);
      break;

    case VTK_FLOAT:
      vtkKWMath::GetScalarRange(scalars, comp, dscalars_range);
      delta = (dscalars_range[1] - dscalars_range[0]) * 0.01;
      range[0] = dscalars_range[0];
      range[1] = dscalars_range[1] + delta;
      try_nb_of_bins = this->MaximumNumberOfBins;
      break;

    case VTK_DOUBLE:
      vtkKWMath::GetScalarRange(scalars, comp, dscalars_range);
      delta = (dscalars_range[1] - dscalars_range[0]) * 0.01;
      range[0] = dscalars_range[0];
      range[1] = dscalars_range[1] + delta;
      try_nb_of_bins = this->MaximumNumberOfBins;
      break;

    default:
      return;
    }

  // Check if too large number of bins, or exceeded capacity

  if (try_nb_of_bins > this->MaximumNumberOfBins || try_nb_of_bins < 1)
    {
    *nb_of_bins = this->MaximumNumberOfBins;
    }
  else
    {
    *nb_of_bins = (vtkIdType)ceil(try_nb_of_bins);
    }
}

//----------------------------------------------------------------------------
void vtkKWHistogram::EstimateHistogramRange(
  vtkDataArray *scalars, 
  int comp,
  double range[2])
{
  vtkIdType nb_bins;
  vtkKWHistogram::EstimateHistogramRangeAndNumberOfBins(
    scalars, comp, range, &nb_bins);
}

//----------------------------------------------------------------------------
void vtkKWHistogram::UpdateHistogram(vtkDataArray *scalars, 
                                     int comp,
                                     int reset_range)
{
  if (!scalars)
    {
    vtkErrorMacro(<< "Can not build histogram from NULL scalars!");
    return;
    }

  if (comp < 0 || comp >= scalars->GetNumberOfComponents())
    {
    vtkErrorMacro(<< "Can not build histogram from invalid component!");
    return;
    }

  // Get the histogram range required for those scalars.
  // Also, given the range, compute the number of bins needed
  // if 'reset_range' is true, reset the Range ivar, otherwise check
  // that the current range is within the ivar (accumulate mode)

  double range[2];
  vtkIdType nb_of_bins;

  this->EstimateHistogramRangeAndNumberOfBins(
    scalars, comp, range, &nb_of_bins);

  if (reset_range)
    {
    this->Range[0] = range[0];
    this->Range[1] = range[1];
    }
  else
    {
    if (range[0] < this->Range[0] || range[1] > this->Range[1])
      {
      vtkErrorMacro(<< "Scalars range [" 
                    << range[0] << ".." << range[1] << "] "
                    << "does not fit in the current Range ["
                    << this->Range[0] << ".." << this->Range[1] << "]!");
      return;
      }
    }

  this->InvokeEvent(vtkCommand::StartEvent, NULL);

  // Allocate the bins 
  // if 'reset_range' is true, reset the number of bins, otherwise reset
  // it only if it was 0 before (i.e., EmptyHistogram was called).

  this->Bins->SetNumberOfComponents(1);
  vtkIdType old_nb_of_bins = this->Bins->GetNumberOfTuples();
  if (reset_range)
    {
    this->Bins->SetNumberOfTuples(nb_of_bins);
    }
  else
    {
    if (old_nb_of_bins == 0)
      {
      this->Bins->SetNumberOfTuples(nb_of_bins);
      }
    }

  // Reset the bins to 0

  if (nb_of_bins != old_nb_of_bins)
    {
    double *bins_ptr = this->Bins->GetPointer(0);
    double *bins_ptr_end = bins_ptr + nb_of_bins;
    while (bins_ptr < bins_ptr_end)
      {
      *bins_ptr++ = 0.0;
      }
    }

  float progress = 0.2;
  this->InvokeEvent(vtkCommand::ProgressEvent, &progress);

  // Loop over the data and fill in the bins

  vtkIdType nb_of_components = scalars->GetNumberOfComponents();
  vtkIdType nb_of_tuples = scalars->GetNumberOfTuples();

  vtkIdType inc_tuple = (vtkIdType)ceil((double)nb_of_tuples / 5.0);
  vtkIdType start_tuple = 0;

  while (start_tuple < nb_of_tuples)
    {
    // Make sure we are really one after...

    if (start_tuple + inc_tuple >= nb_of_tuples)
      {
      inc_tuple = nb_of_tuples - start_tuple;
      }

    switch (scalars->GetDataType())
      {
      case VTK_CHAR:
      {
      typedef char type;
      type *data = static_cast<type *>
        (scalars->GetVoidPointer(start_tuple * nb_of_components)) + comp;
      vtkKWHistogramBuildIntOrFloat(
        data, inc_tuple, nb_of_components, this);
      }
      break;

      case VTK_UNSIGNED_CHAR:
      {
      typedef unsigned char type;
      type *data = static_cast<type *>
        (scalars->GetVoidPointer(start_tuple * nb_of_components)) + comp;
      vtkKWHistogramBuildIntOrFloat(
        data, inc_tuple, nb_of_components, this);
      }
      break;

      case VTK_SHORT:
      {
      typedef short type;
      type *data = static_cast<type *>
        (scalars->GetVoidPointer(start_tuple * nb_of_components)) + comp;
      vtkKWHistogramBuildIntOrFloat(
        data, inc_tuple, nb_of_components, this);
      }
      break;

      case VTK_UNSIGNED_SHORT:
      {
      typedef unsigned short type;
      type *data = static_cast<type *>
        (scalars->GetVoidPointer(start_tuple * nb_of_components)) + comp;
      vtkKWHistogramBuildIntOrFloat(
        data, inc_tuple, nb_of_components, this);
      }
      break;

      case VTK_INT:
      {
      typedef int type;
      type *data = static_cast<type *>
        (scalars->GetVoidPointer(start_tuple * nb_of_components)) + comp;
      vtkKWHistogramBuildIntOrFloat(
        data, inc_tuple, nb_of_components, this);
      }
      break;

      case VTK_UNSIGNED_INT:
      {
      typedef unsigned int type;
      type *data = static_cast<type *>
        (scalars->GetVoidPointer(start_tuple * nb_of_components)) + comp;
      vtkKWHistogramBuildIntOrFloat(
        data, inc_tuple, nb_of_components, this);
      }
      break;

      case VTK_LONG:
      {
      typedef long type;
      type *data = static_cast<type *>
        (scalars->GetVoidPointer(start_tuple * nb_of_components)) + comp;
      vtkKWHistogramBuildIntOrFloat(
        data, inc_tuple, nb_of_components, this);
      }
      break;

      case VTK_UNSIGNED_LONG:
      {
      typedef unsigned long type;
      type *data = static_cast<type *>
        (scalars->GetVoidPointer(start_tuple * nb_of_components)) + comp;
      vtkKWHistogramBuildIntOrFloat(
        data, inc_tuple, nb_of_components, this);
      }
      break;

      case VTK_FLOAT:
      {
      typedef float type;
      type *data = static_cast<type *>
        (scalars->GetVoidPointer(start_tuple * nb_of_components)) + comp;
      vtkKWHistogramBuildFloat(
        data, inc_tuple, nb_of_components, this);
      }
      break;

      case VTK_DOUBLE:
      {
      typedef double type;
      type *data = static_cast<type *>
        (scalars->GetVoidPointer(start_tuple * nb_of_components)) + comp;
      vtkKWHistogramBuildFloat(
        data, inc_tuple, nb_of_components, this);
      }
      break;

      default:
        vtkErrorMacro(
          << "Can not build histogram from unsupported data type!");
        return;
      }

    progress = 0.2 + ((float)start_tuple / (float)nb_of_tuples) * 0.8;
    this->InvokeEvent(vtkCommand::ProgressEvent, &progress);

    start_tuple += inc_tuple;
    }

  progress = 1.0;
  this->InvokeEvent(vtkCommand::ProgressEvent, &progress);

  this->Bins->Modified();

  this->InvokeEvent(vtkCommand::EndEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkKWHistogram::BuildHistogram(vtkDataArray *scalars, int comp)
{
  this->UpdateHistogram(scalars, comp, 1);
}

//----------------------------------------------------------------------------
void vtkKWHistogram::AccumulateHistogram(vtkDataArray *scalars, int comp)
{
  this->UpdateHistogram(scalars, comp, 0);
}

//----------------------------------------------------------------------------
vtkKWHistogram::ImageDescriptor::ImageDescriptor()
{
  this->Style                   = vtkKWHistogram::ImageDescriptor::STYLE_BARS;
  this->Width                   = 0;
  this->Height                  = 0;
  this->DrawBackground          = 1;
  this->DrawGrid                = 0;
  this->GridSize                = 15;
  this->ColorTransferFunction   = NULL;
  this->DefaultMaximumOccurence = 0.0;
  this->LastMaximumOccurence    = 0.0;

  this->Range[0]                = 0.0;
  this->Range[1]                = 0.0;

  this->Color[0]                = 0.63;
  this->Color[1]                = 0.63;
  this->Color[2]                = 0.63;

  this->BackgroundColor[0]      = 0.83;
  this->BackgroundColor[1]      = 0.83;
  this->BackgroundColor[2]      = 0.83;

  this->OutOfRangeColor[0]      = 0.83;
  this->OutOfRangeColor[1]      = 0.83;
  this->OutOfRangeColor[2]      = 0.83;

  this->GridColor[0]            = 0.93;
  this->GridColor[1]            = 0.93;
  this->GridColor[2]            = 0.93;
}

//----------------------------------------------------------------------------
int vtkKWHistogram::ImageDescriptor::IsEqualTo(const ImageDescriptor *desc)
{
  return (
    desc &&
    this->Range[0]              == desc->Range[0] &&
    this->Range[1]              == desc->Range[1] &&
    this->Width                 == desc->Width &&
    this->Height                == desc->Height &&
    this->Color[0]              == desc->Color[0] &&
    this->Color[1]              == desc->Color[1] &&
    this->Color[2]              == desc->Color[2] &&
    this->DrawBackground        == desc->DrawBackground &&
    this->BackgroundColor[0]    == desc->BackgroundColor[0] &&
    this->BackgroundColor[1]    == desc->BackgroundColor[1] &&
    this->BackgroundColor[2]    == desc->BackgroundColor[2] &&
    this->OutOfRangeColor[0]    == desc->OutOfRangeColor[0] &&
    this->OutOfRangeColor[1]    == desc->OutOfRangeColor[1] &&
    this->OutOfRangeColor[2]    == desc->OutOfRangeColor[2] &&
    this->DrawGrid              == desc->DrawGrid &&
    this->GridSize              == desc->GridSize &&
    this->GridColor[0]          == desc->GridColor[0] &&
    this->GridColor[1]          == desc->GridColor[1] &&
    this->GridColor[2]          == desc->GridColor[2] &&
    this->ColorTransferFunction == desc->ColorTransferFunction
    );
}

//----------------------------------------------------------------------------
void vtkKWHistogram::ImageDescriptor::Copy(const ImageDescriptor *desc)
{
  if (!desc)
    {
    return;
    }

  this->Range[0]              = desc->Range[0];
  this->Range[1]              = desc->Range[1];
  this->Width                 = desc->Width;
  this->Height                = desc->Height;
  this->Color[0]              = desc->Color[0];
  this->Color[1]              = desc->Color[1];
  this->Color[2]              = desc->Color[2];
  this->DrawBackground        = desc->DrawBackground;
  this->BackgroundColor[0]    = desc->BackgroundColor[0];
  this->BackgroundColor[1]    = desc->BackgroundColor[1];
  this->BackgroundColor[2]    = desc->BackgroundColor[2];
  this->OutOfRangeColor[0]    = desc->OutOfRangeColor[0];
  this->OutOfRangeColor[1]    = desc->OutOfRangeColor[1];
  this->OutOfRangeColor[2]    = desc->OutOfRangeColor[2];
  this->DrawGrid              = desc->DrawGrid;
  this->GridSize              = desc->GridSize;
  this->GridColor[0]          = desc->GridColor[0];
  this->GridColor[1]          = desc->GridColor[1];
  this->GridColor[2]          = desc->GridColor[2];
  this->ColorTransferFunction = desc->ColorTransferFunction;
}

//----------------------------------------------------------------------------
int vtkKWHistogram::ImageDescriptor::IsValid() const
{
  return (
    this->Range[0] != this->Range[1] &&
    this->Width &&
    this->Height &&
    (this->Color[0] >= 0.0 && this->Color[0] <= 1.0) &&
    (this->Color[1] >= 0.0 && this->Color[1] <= 1.0) &&
    (this->Color[2] >= 0.0 && this->Color[2] <= 1.0) &&
    (this->BackgroundColor[0] >= 0.0 && this->BackgroundColor[0] <= 1.0) &&
    (this->BackgroundColor[1] >= 0.0 && this->BackgroundColor[1] <= 1.0) &&
    (this->BackgroundColor[2] >= 0.0 && this->BackgroundColor[2] <= 1.0) &&
    (this->OutOfRangeColor[0] >= 0.0 && this->OutOfRangeColor[0] <= 1.0) &&
    (this->OutOfRangeColor[1] >= 0.0 && this->OutOfRangeColor[1] <= 1.0) &&
    (this->OutOfRangeColor[2] >= 0.0 && this->OutOfRangeColor[2] <= 1.0) &&
    (this->GridColor[0] >= 0.0 && this->GridColor[0] <= 1.0) &&
    (this->GridColor[1] >= 0.0 && this->GridColor[1] <= 1.0) &&
    (this->GridColor[2] >= 0.0 && this->GridColor[2] <= 1.0) &&
    this->GridSize > 0
    );
}

//----------------------------------------------------------------------------
void vtkKWHistogram::ImageDescriptor::SetRange(double range0, double range1)
{
  this->Range[0] = range0;
  this->Range[1] = range1;
}

//----------------------------------------------------------------------------
void vtkKWHistogram::ImageDescriptor::SetRange(double range[2])
{
  this->SetRange(range[0], range[1]); 
}

//----------------------------------------------------------------------------
void vtkKWHistogram::ImageDescriptor::SetDimensions(unsigned int width, 
                                                    unsigned int height)
{
  this->Width = width;
  this->Height = height;
}

//----------------------------------------------------------------------------
void vtkKWHistogram::ImageDescriptor::SetColor(double color[3])
{
  this->Color[0] = color[0];
  this->Color[1] = color[1];
  this->Color[2] = color[2];
}

//----------------------------------------------------------------------------
void vtkKWHistogram::ImageDescriptor::SetBackgroundColor(double color[3])
{
  this->BackgroundColor[0] = color[0];
  this->BackgroundColor[1] = color[1];
  this->BackgroundColor[2] = color[2];
}

//----------------------------------------------------------------------------
void vtkKWHistogram::ImageDescriptor::SetOutOfRangeColor(double color[3])
{
  this->OutOfRangeColor[0] = color[0];
  this->OutOfRangeColor[1] = color[1];
  this->OutOfRangeColor[2] = color[2];
}

//----------------------------------------------------------------------------
void vtkKWHistogram::ImageDescriptor::SetGridColor(double color[3])
{
  this->GridColor[0] = color[0];
  this->GridColor[1] = color[1];
  this->GridColor[2] = color[2];
}

//----------------------------------------------------------------------------
int vtkKWHistogram::IsImageUpToDate(
  const ImageDescriptor *desc)
{
  // Create new image if it does not exist (same with descriptor)

  if (!this->Image)
    {
    this->Image = vtkImageData::New();
    }

  if (!this->LastImageDescriptor)
    {
    this->LastImageDescriptor = new vtkKWHistogram::ImageDescriptor;
    }

  return (this->LastImageBuildTime >= this->Bins->GetMTime() &&
          (!desc ||
           (this->LastImageDescriptor->IsEqualTo(desc) &&
            (!desc->ColorTransferFunction || 
             this->LastTransferFunctionTime >= 
             desc->ColorTransferFunction->GetMTime()))));
}

//----------------------------------------------------------------------------
vtkImageData* vtkKWHistogram::GetImage(
  ImageDescriptor *desc)
{
  // Check histogram

  if (!this->GetNumberOfBins())
    {
    vtkErrorMacro(<< "Can not compute histogram image from empty histogram!");
    return NULL;
    }

  // Check descriptor
  
  if (desc && !desc->IsValid())
    {
    vtkErrorMacro(
      << "Can not compute histogram image with invalid descriptor!");
    return NULL;
    }

  // Do we really need to recreate ?

  if (this->IsImageUpToDate(desc))
    {
    return this->Image;
    }

  // We need width x height RGBA (unsigned char)

  int nb_of_components = 3 + (desc->DrawBackground ? 0 : 1);
  this->Image->SetDimensions(desc->Width, desc->Height, 1);
  this->Image->SetWholeExtent(this->Image->GetExtent());
  this->Image->SetUpdateExtent(this->Image->GetExtent());
  this->Image->SetScalarTypeToUnsignedChar();
  this->Image->SetNumberOfScalarComponents(nb_of_components);
  this->Image->AllocateScalars();

  unsigned char *image_ptr = 
    static_cast<unsigned char*>(this->Image->GetScalarPointer());

  unsigned char *column_ptr;

  // Colors

  unsigned char color[3], bgcolor[3], gridcolor[3], oorcolor[3];

  color[0] = (unsigned char)(255.0 * desc->Color[0]);
  color[1] = (unsigned char)(255.0 * desc->Color[1]);
  color[2] = (unsigned char)(255.0 * desc->Color[2]);

  bgcolor[0] = (unsigned char)(255.0 * desc->BackgroundColor[0]);
  bgcolor[1] = (unsigned char)(255.0 * desc->BackgroundColor[1]);
  bgcolor[2] = (unsigned char)(255.0 * desc->BackgroundColor[2]);

  oorcolor[0] = (unsigned char)(255.0 * desc->OutOfRangeColor[0]);
  oorcolor[1] = (unsigned char)(255.0 * desc->OutOfRangeColor[1]);
  oorcolor[2] = (unsigned char)(255.0 * desc->OutOfRangeColor[2]);

  gridcolor[0] = (unsigned char)(255.0 * desc->GridColor[0]);
  gridcolor[1] = (unsigned char)(255.0 * desc->GridColor[1]);
  gridcolor[2] = (unsigned char)(255.0 * desc->GridColor[2]);

  double *tfunccolors = NULL;
  if (desc->ColorTransferFunction)
    {
    tfunccolors = new double[3 * desc->Width];
    desc->ColorTransferFunction->GetTable(
      desc->Range[0], desc->Range[1], desc->Width, tfunccolors);
    }
    
  // Quickly set the whole image to be transparent if no background color
  // is going to be used

  if (!desc->DrawBackground)
    {
    memset(image_ptr, 0, desc->Width * desc->Height * nb_of_components);
    }
  
  // Fill the image
  
  double *bins_ptr = this->Bins->GetPointer(0);
  
  double value, next_value;
  double occurrence;

  // width of bins in the original histogram.

  double bin_width = 
    (this->Range[1] - this->Range[0]) / (double)this->GetNumberOfBins();
  
  // Range of the original histogram that is visible in the interface.

  double hist_subrange = desc->Range[1] - desc->Range[0]; 

  // Scale between pixel units in the image of the histogram on the interface
  // and value units on the scale of the original histogram. Multiplying by
  // x_scale converts pixel units into histogram domain units.

  double x_scale = hist_subrange / (double)(desc->Width - 1);

  vtkIdType bin, next_bin, iter_bin;

  int row_length = desc->Width * nb_of_components;

  double bin_real;
  double next_bin_real;
  double max_occurrence = 0.0;

  double * resampled_histogram = new double[desc->Width];

  unsigned int x, y, ylimit;
  for (x = 0; x < desc->Width; x++)
    {
    // Get the value and occurrence for this x (column)

    value = desc->Range[0] + x_scale * (double)x;

    // "occurrence" cumulates area under the histogram inside a particular
    // range.

    occurrence = 0;

    // Has to be in the range (upper range is exclusive)
    // Collect all the bins up to the next value (for the next pixel)

    if (value >= this->Range[0] && value < this->Range[1])
      {
      bin_real = (value - this->Range[0]) / bin_width;
      bin      = vtkMath::Floor(bin_real);
     
      next_value = desc->Range[0] + x_scale * (double)(x+1);
      next_bin_real = (next_value - this->Range[0]) / bin_width;
      next_bin       = vtkMath::Floor(next_bin_real);
      if (next_bin > this->GetNumberOfBins())
        {
        next_bin = this->GetNumberOfBins();
        }
      iter_bin = bin;
      while (++iter_bin < next_bin)
        {
        // We should cumulate values from the bins
        occurrence += bins_ptr[iter_bin];
        }

      // Compute the partial contribution from the first bin and last bin
      // This is done differently depending on whether bin and next_bin are
      // the same or not.
      
      if( next_bin != bin )
        {
        // Compute the partial contribution from the first bin plus the
        // contribution of the last one.  This is a fraction of the counts on
        // each bin.
        occurrence += bins_ptr[bin]       * (1.0 - (bin_real - bin));
        occurrence += bins_ptr[next_bin]  * (next_bin_real - next_bin);
        }
      else
        {
        // Otherwise, if they are the same, the number of counts must
        // be scaled by the ratio between bin widths of the original
        // and the resampled histogram.
        occurrence = bins_ptr[bin] * x_scale / bin_width ;
        }
      
      // Height of histogram bars should be such that the area is conserved.
      // we should divide by the range of values that are now mapped in a 
      // single bin on the histogram visible on the interface.

      resampled_histogram[x] = occurrence / x_scale;
      if( resampled_histogram[x] > max_occurrence )
        {
        max_occurrence = resampled_histogram[x];
        }
      }

    // Out of range, set the occurence to -1, so that it can be
    // used as a flag in the next loop, which draws pixels

    else
      {
      resampled_histogram[x] = -1.0;
      }
    }

  if (desc->DefaultMaximumOccurence > 0.0 &&
      desc->DefaultMaximumOccurence > max_occurrence)
    {
    max_occurrence = desc->DefaultMaximumOccurence;
    }
  desc->LastMaximumOccurence = max_occurrence;

#if VTK_KW_HIST_TESTING
  cout << this << " : vtkKWHistogram::GetImage: " 
       << "(" << desc->Range[0] << ".." << desc->Range[1] << ") from "
       << "(" << this->Range[0] << ".." << this->Range[1] << ") "
       << desc->Width << "x" << desc->Height 
       << " (max occ: " << max_occurrence << ")"
       << endl;
#endif

  for (x = 0; x < desc->Width; x++)
    {
    // If the occurence is < 0, it is a flag that in fact we were
    // out of range for that pixel column

    int out_of_range = (resampled_histogram[x] < 0);
    occurrence = resampled_histogram[x] < 0 ? 0 : resampled_histogram[x];

    // No occurrence and no background to next column since we
    // already assigned all pixels to be fully transparent
    
    if (occurrence < 0.00001 && !desc->DrawBackground)
      {
      continue;
      }
    
    if (this->LogMode && occurrence)
      {
      if( occurrence < 1.0 ) // check for logarithm(occurrence) going negative
        {
        ylimit = 0;
        }
      else
        {
        ylimit = (int)(
          log((double)occurrence) * desc->Height/log((double)max_occurrence));
        }
      }
    else
      {
      ylimit = (int)(
        (double)occurrence * desc->Height / (double)max_occurrence);
      }

    // This should not happen, but check anyway, otherwise we will poke
    // all over memory and crash

    if (ylimit > desc->Height)
      {
      ylimit = desc->Height;
      }

    column_ptr = image_ptr + x * nb_of_components;

    // Draw the background if needed (eventually the grid)
    // This is used only in DOTS mode

    if (desc->DrawBackground &&
        desc->Style == vtkKWHistogram::ImageDescriptor::STYLE_DOTS &&
        ylimit > 1)
      {
      unsigned char *oor_or_bg_color = out_of_range ? oorcolor : bgcolor;
      if (desc->DrawGrid)
        {
        int xval = x / desc->GridSize;
        for (y = 0; y < ylimit - 1; y++)
          {
          if ((xval + (desc->Height - 1 - y) / desc->GridSize) & 1)
            {
            column_ptr[0] = oor_or_bg_color[0];
            column_ptr[1] = oor_or_bg_color[1];
            column_ptr[2] = oor_or_bg_color[2];
            }
          else
            {
            column_ptr[0] = gridcolor[0];
            column_ptr[1] = gridcolor[1];
            column_ptr[2] = gridcolor[2];
            }
          column_ptr += row_length;
          }
        }
      else
        {
        for (y = 0; y < ylimit - 1; y++)
          {
          column_ptr[0] = oor_or_bg_color[0];
          column_ptr[1] = oor_or_bg_color[1];
          column_ptr[2] = oor_or_bg_color[2];
          column_ptr += row_length;
          }
        }
      }

    // Draw the bin

    if (desc->ColorTransferFunction)
      {
      double *value_color = tfunccolors + 3 * x;
      color[0] = (unsigned char)(255.0 * value_color[0]);
      color[1] = (unsigned char)(255.0 * value_color[1]);
      color[2] = (unsigned char)(255.0 * value_color[2]);
      }
      
    if (desc->DrawBackground)
      {
      switch (desc->Style)
        {
        case vtkKWHistogram::ImageDescriptor::STYLE_BARS:
          for (y = 0; y < ylimit; y++)
            {
            column_ptr[0] = color[0];
            column_ptr[1] = color[1];
            column_ptr[2] = color[2];
            column_ptr += row_length;
            }
          break;
        case vtkKWHistogram::ImageDescriptor::STYLE_DOTS:
          if (ylimit > 0)
            {
            column_ptr[0] = color[0];
            column_ptr[1] = color[1];
            column_ptr[2] = color[2];
            column_ptr += row_length;
            }
          break;
        }
      }
    else
      {
      switch (desc->Style)
        {
        case vtkKWHistogram::ImageDescriptor::STYLE_BARS:
          for (y = 0; y < ylimit; y++)
            {
            column_ptr[0] = color[0];
            column_ptr[1] = color[1];
            column_ptr[2] = color[2];
            column_ptr[3] = 255;
            column_ptr += row_length;
            }
          break;
        case vtkKWHistogram::ImageDescriptor::STYLE_DOTS:
          if (ylimit > 0)
            {
            column_ptr += (ylimit - 1) * row_length;
            column_ptr[0] = color[0];
            column_ptr[1] = color[1];
            column_ptr[2] = color[2];
            column_ptr[3] = 255;
            column_ptr += row_length;
            }
          break;
        }
      }

    // Draw the background if needed (eventually the grid)

    if (desc->DrawBackground)
      {
      unsigned char *oor_or_bg_color = out_of_range ? oorcolor : bgcolor;
      if (desc->DrawGrid)
        {
        int xval = x / desc->GridSize;
        for (y = ylimit; y < desc->Height; y++)
          {
          if ((xval + (desc->Height - 1 - y) / desc->GridSize) & 1)
            {
            column_ptr[0] = oor_or_bg_color[0];
            column_ptr[1] = oor_or_bg_color[1];
            column_ptr[2] = oor_or_bg_color[2];
            }
          else
            {
            column_ptr[0] = gridcolor[0];
            column_ptr[1] = gridcolor[1];
            column_ptr[2] = gridcolor[2];
            }
          column_ptr += row_length;
          }
        }
      else
        {
        for (y = ylimit; y < desc->Height; y++)
          {
          column_ptr[0] = oor_or_bg_color[0];
          column_ptr[1] = oor_or_bg_color[1];
          column_ptr[2] = oor_or_bg_color[2];
          column_ptr += row_length;
          }
        }
      }

    }

  delete [] resampled_histogram;
  
  if (tfunccolors)
    {
    delete [] tfunccolors;
    }

  this->LastImageDescriptor->Copy(desc);
  this->LastImageBuildTime = this->Bins->GetMTime();
  this->LastTransferFunctionTime = 
    (desc->ColorTransferFunction ? desc->ColorTransferFunction->GetMTime() :0);

  this->Image->Modified();

  return this->Image;
}

//----------------------------------------------------------------------------
void vtkKWHistogram::SetLogMode(int arg)
{
  if (this->LogMode == arg)
    {
    return;
    }

  this->LogMode = arg;
  
  // The next time the image is queried, it will be refreshed

  this->LastImageBuildTime = 0;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWHistogram::ComputeStatistics()
{
  if (this->LastStatisticsBuildTime >= this->Bins->GetMTime())
    {
    return;
    }

  double min = VTK_DOUBLE_MAX;
  double max = VTK_DOUBLE_MIN;
  double total = 0.0;

  double *bins_ptr = this->Bins->GetPointer(0);
  double *bins_ptr_end = bins_ptr + this->GetNumberOfBins();

  while (bins_ptr < bins_ptr_end)
    {
    if (*bins_ptr < min)
      {
      min = *bins_ptr;
      }
    if (*bins_ptr > max)
      {
      max = *bins_ptr;
      }
    total += *bins_ptr;
    bins_ptr++;
    }

  this->MinimumOccurence = min;
  this->MaximumOccurence = max;
  this->TotalOccurence   = total;

  this->LastStatisticsBuildTime = this->Bins->GetMTime();
}

//----------------------------------------------------------------------------
double vtkKWHistogram::GetMinimumOccurence()
{
  this->ComputeStatistics();
  return this->MinimumOccurence;
}

//----------------------------------------------------------------------------
double vtkKWHistogram::GetMaximumOccurence()
{
  this->ComputeStatistics();
  return this->MaximumOccurence;
}

//----------------------------------------------------------------------------
double vtkKWHistogram::GetTotalOccurence()
{
  this->ComputeStatistics();
  return this->TotalOccurence;
}

//----------------------------------------------------------------------------
double vtkKWHistogram::GetOccurenceAtValue(double value)
{
  vtkIdType nb_of_bins = this->GetNumberOfBins();
  if (value < this->Range[0] || value >= this->Range[1] || !nb_of_bins)
    {
    return 0;
    }

  double bin_width = (this->Range[1] - this->Range[0]) / (double)nb_of_bins;

  double *bins_ptr = this->Bins->GetPointer(0) 
    + (vtkIdType)((value - this->Range[0]) / bin_width);

  return *bins_ptr;
}

//----------------------------------------------------------------------------
double vtkKWHistogram::GetValueAtAccumulatedOccurence(
  double acc, double *exclude_value)
{
  double total = 0.0;
  double bin_width = 
    (this->Range[1] - this->Range[0]) / (double)this->GetNumberOfBins();

  double *bins_ptr = this->Bins->GetPointer(0);
  double *bins_ptr_end = bins_ptr + this->GetNumberOfBins();

  if (exclude_value)
    {
    double low = this->Range[0];
    while (bins_ptr < bins_ptr_end)
      {
      if (*exclude_value < low || *exclude_value >= low + bin_width)
        {
        total += *bins_ptr;
        if (total > acc)
          {
          break;
          }
        }
      bins_ptr++;
      low += bin_width;
      }
    }
  else
    {
    while (bins_ptr < bins_ptr_end)
      {
      total += *bins_ptr;
      if (total > acc)
        {
        break;
        }
      bins_ptr++;
      }
    }
  
  // If we never reached acc, return the upper bound

  if (bins_ptr == bins_ptr_end)
    {
    return this->Range[1];
    }

  // Otherwise interpolate

  vtkIdType index = bins_ptr - this->Bins->GetPointer(0);
  double previous_total = total - *bins_ptr;
  return this->Range[0] + bin_width * 
    ((double)index + (acc - previous_total) / (total - previous_total));
}

//----------------------------------------------------------------------------
void vtkKWHistogram::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Range: " 
     << this->Range[0] << ", " << this->Range[1] << endl;
  os << indent << "LogMode: " << (this->LogMode ? "On" : "Off") << endl;
  os << indent << "MaximumNumberOfBins: " << this->MaximumNumberOfBins << endl;
  os << indent << "DataSet: ";
  if (this->Bins)
    {
    os << endl;
    this->Bins->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}

