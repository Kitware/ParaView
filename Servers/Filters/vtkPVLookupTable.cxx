/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVLookupTable.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLookupTable.h"

#include "vtkObjectFactory.h"
#include "vtkLookupTable.h"
#include "vtkColorTransferFunction.h"

vtkStandardNewMacro(vtkPVLookupTable);
vtkCxxRevisionMacro(vtkPVLookupTable, "1.1");
//-----------------------------------------------------------------------------
vtkPVLookupTable::vtkPVLookupTable()
{
  this->LookupTable = vtkLookupTable::New();
  this->ColorTransferFunction = vtkColorTransferFunction::New();
  this->Discretize = 0;
  this->TableRange[0] = 0.0;
  this->TableRange[1] = 1.0;
  this->NumberOfValues = 256;
}

//-----------------------------------------------------------------------------
vtkPVLookupTable::~vtkPVLookupTable()
{
  this->LookupTable->Delete();
  this->ColorTransferFunction->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVLookupTable::SetRange(double min, double max)
{
  this->SetTableRange(min, max);
  this->LookupTable->SetTableRange(min, max);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVLookupTable::SetNumberOfValues(vtkIdType number)
{
  this->NumberOfValues = number;
  this->LookupTable->SetNumberOfTableValues(number);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVLookupTable::Build()
{
  this->ColorTransferFunction->Build();
  if (this->Discretize && (this->GetMTime() > this->BuildTime ||
    this->ColorTransferFunction->GetMTime() > this->BuildTime))
    {
    unsigned char* lut_ptr = this->LookupTable->WritePointer(0,
      this->NumberOfValues);
    double* table = new double[this->NumberOfValues*3];
    this->ColorTransferFunction->GetTable(0.0, 1.0,
      this->NumberOfValues, table);
    // Now, convert double to unsigned chars and fill the LUT.
    for (int cc=0; cc < this->NumberOfValues; cc++)
      {
      lut_ptr[4*cc]   = (unsigned char)(255.0*table[3*cc] + 0.5);
      lut_ptr[4*cc+1] = (unsigned char)(255.0*table[3*cc+1] + 0.5);
      lut_ptr[4*cc+2] = (unsigned char)(255.0*table[3*cc+2] + 0.5);
      lut_ptr[4*cc+3] = 255;
      }
    delete table;

    this->BuildTime.Modified();
    }
}

//-----------------------------------------------------------------------------
unsigned char* vtkPVLookupTable::MapValue(double v)
{
  if (this->Discretize)
    {
    return this->LookupTable->MapValue(v);
    }

  // vtkColorTransferFunction does not take TableRanges, so
  // we normalize the value v.
  v = (v - this->TableRange[0])/ (this->TableRange[1] - this->TableRange[0]);
  return this->ColorTransferFunction->MapValue(v);
}


//-----------------------------------------------------------------------------
void vtkPVLookupTable::GetColor(double v, double rgb[3])
{
  unsigned char *rgb8 = this->MapValue(v);

  rgb[0] = rgb8[0]/255.0;
  rgb[1] = rgb8[1]/255.0;
  rgb[2] = rgb8[2]/255.0;
}
//----------------------------------------------------------------------------
// Accelerate the mapping by copying the data in 32-bit chunks instead
// of 8-bit chunks.  The extra "long" argument is to help broken
// compilers select the non-templates below for unsigned char
// and unsigned short.
template <class T>
void vtkPVLookupTableMapData(vtkColorTransferFunction* self,
                                     T* input,
                                     unsigned char* output,
                                     int length, int inIncr,
                                     int outFormat, long,
                                     vtkPVLookupTable* pvlut)
{
  double          x;
  int            i = length;
  double          rgb[3];
  unsigned char  *optr = output;
  T              *iptr = input;
  unsigned char   alpha = (unsigned char)(self->GetAlpha()*255.0);
  
  if(self->GetSize() == 0)
    {
    vtkGenericWarningMacro("Transfer Function Has No Points!");
    return;
    }

  double *range = pvlut->GetRange();
  while (--i >= 0) 
    {
    x = (double) *iptr;
    x = (x-range[0])/ (range[1]-range[0]);
    self->GetColor(x, rgb);
    
    if (outFormat == VTK_RGB || outFormat == VTK_RGBA)
      {
      *(optr++) = (unsigned char)(rgb[0]*255.0 + 0.5);
      *(optr++) = (unsigned char)(rgb[1]*255.0 + 0.5);
      *(optr++) = (unsigned char)(rgb[2]*255.0 + 0.5);
      }
    else // LUMINANCE  use coeffs of (0.30  0.59  0.11)*255.0
      {
      *(optr++) = (unsigned char)(rgb[0]*76.5 + rgb[1]*150.45 + rgb[2]*28.05 + 0.5); 
      }
    
    if (outFormat == VTK_RGBA || outFormat == VTK_LUMINANCE_ALPHA)
      {
      *(optr++) = alpha;
      }
    iptr += inIncr;
    }
}



//----------------------------------------------------------------------------
// Special implementation for unsigned char input.
void vtkPVLookupTableMapData(vtkColorTransferFunction* self,
                                     unsigned char* input,
                                     unsigned char* output,
                                     int length, int inIncr,
                                     int outFormat, int,
                                     vtkPVLookupTable* )
{
  int            x;
  int            i = length;
  unsigned char  *optr = output;
  unsigned char  *iptr = input;

  if(self->GetSize() == 0)
    {
    vtkGenericWarningMacro("Transfer Function Has No Points!");
    return;
    }
  
  const unsigned char *table = self->GetTable(0,255,256);
  switch (outFormat)
    {
    case VTK_RGB:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        *(optr++) = table[x+1];
        *(optr++) = table[x+2];
        iptr += inIncr;
        }
      break;
    case VTK_RGBA:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        *(optr++) = table[x+1];
        *(optr++) = table[x+2];
        *(optr++) = 255;
        iptr += inIncr;
        }
      break;
    case VTK_LUMINANCE_ALPHA:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        *(optr++) = 255;
        iptr += inIncr;
        }
      break;
    case VTK_LUMINANCE:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        iptr += inIncr;
        }
      break;
    }  
}

//----------------------------------------------------------------------------
// Special implementation for unsigned short input.
void vtkPVLookupTableMapData(vtkColorTransferFunction* self,
                                     unsigned short* input,
                                     unsigned char* output,
                                     int length, int inIncr,
                                     int outFormat, int,
                                     vtkPVLookupTable* )
{
  int            x;
  int            i = length;
  unsigned char  *optr = output;
  unsigned short *iptr = input;

  if(self->GetSize() == 0)
    {
    vtkGenericWarningMacro("Transfer Function Has No Points!");
    return;
    }
  

  const unsigned char *table = self->GetTable(0,65535,65536);
  switch (outFormat)
    {
    case VTK_RGB:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        *(optr++) = table[x+1];
        *(optr++) = table[x+2];
        iptr += inIncr;
        }
      break;
    case VTK_RGBA:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        *(optr++) = table[x+1];
        *(optr++) = table[x+2];
        *(optr++) = 255;
        iptr += inIncr;
        }
      break;
    case VTK_LUMINANCE_ALPHA:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        *(optr++) = 255;
        iptr += inIncr;
        }
      break;
    case VTK_LUMINANCE:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        iptr += inIncr;
        }
      break;
    }  
}

//-----------------------------------------------------------------------------
void vtkPVLookupTable::MapScalarsThroughTable2(void *input, 
  unsigned char *output, int inputDataType, int numberOfValues,
  int inputIncrement, int outputFormat)
{
  if (this->Discretize)
    {
    this->LookupTable->MapScalarsThroughTable2(
      input, output, inputDataType, numberOfValues, inputIncrement, 
      outputFormat);
    }
  else
    {
    // We cannot directly call MapScalarsThroughTable2 for the
    // ColorTransferFunction, CTF does not support the notion
    // of ScalarRange.
    switch (inputDataType)
      {
      vtkTemplateMacro(
        vtkPVLookupTableMapData(this->ColorTransferFunction, 
          static_cast<VTK_TT*>(input),
          output, numberOfValues, inputIncrement,
          outputFormat, 1,
          this)
      );
    default:
      vtkErrorMacro(<< "MapImageThroughTable: Unknown input ScalarType");
      return;
      }
    }
}

//-----------------------------------------------------------------------------
int vtkPVLookupTable::AddRGBPoint( double x, double r, double g, double b )
{
  if (x > 1.0 || x < 0.0)
    {
    vtkErrorMacro("Point value must be normalized[0,1] : " << x);
    }
  this->Modified();
  return this->ColorTransferFunction->AddRGBPoint(x, r, g, b);
}

//-----------------------------------------------------------------------------
int vtkPVLookupTable::AddRGBPoint( double x, double r, double g, double b, 
  double midpoint, double sharpness )
{
  if (x > 1.0 || x < 0.0)
    {
    vtkErrorMacro("Point value must be normalized[0,1] : " << x);
    }
  this->Modified();
  return this->ColorTransferFunction->AddRGBPoint(x, r, g, b, midpoint, sharpness);
}

//-----------------------------------------------------------------------------
int vtkPVLookupTable::AddHSVPoint( double x, double r, double g, double b )
{
  if (x > 1.0 || x < 0.0)
    {
    vtkErrorMacro("Point value must be normalized[0,1] : " << x);
    }
  this->Modified();
  return this->ColorTransferFunction->AddHSVPoint(x, r, g, b);
}

//-----------------------------------------------------------------------------
int vtkPVLookupTable::AddHSVPoint( double x, double r, double g, double b, 
  double midpoint, double sharpness )
{
  if (x > 1.0 || x < 0.0)
    {
    vtkErrorMacro("Point value must be normalized[0,1] : " << x);
    }
  this->Modified();
  return this->ColorTransferFunction->AddHSVPoint(x, r, g, b, midpoint, sharpness);
}

//-----------------------------------------------------------------------------
void vtkPVLookupTable::RemoveAllPoints()
{
  this->Modified();
  this->ColorTransferFunction->RemoveAllPoints();
}

//-----------------------------------------------------------------------------
void vtkPVLookupTable::SetColorSpace(int cs)
{
  this->Modified();
  this->ColorTransferFunction->SetColorSpace(cs);
}

//-----------------------------------------------------------------------------
void vtkPVLookupTable::SetHSVWrap(int f)
{
  this->Modified();
  this->ColorTransferFunction->SetHSVWrap(f);
}

//-----------------------------------------------------------------------------
void vtkPVLookupTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Discretize: " << this->Discretize << endl;
  os << indent << "TableRange: " << this->TableRange[0] << ", "
    << this->TableRange[1] << endl;
  os << indent << "NumberOfValues: " << this->NumberOfValues << endl;
}
