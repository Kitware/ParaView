/*=========================================================================

  Module:    vtkKWMath.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWMath.h"

#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"

vtkCxxRevisionMacro(vtkKWMath, "1.11");
vtkStandardNewMacro(vtkKWMath);

//----------------------------------------------------------------------------
int vtkKWMath::GetScalarRange(vtkDataArray *array, int comp, double range[2])
{
  if (!array || comp < 0 || comp >= array->GetNumberOfComponents())
    {
    return 0;
    }
  
  array->GetRange(range, comp);
  return 1;
}


//----------------------------------------------------------------------------
int vtkKWMath::GetAdjustedScalarRange(
  vtkDataArray *array, int comp, double range[2])
{
  if (!vtkKWMath::GetScalarRange(array, comp, range))
    {
    return 0;
    }

  switch (array->GetDataType())
    {
    case VTK_UNSIGNED_CHAR:
      range[0] = (double)array->GetDataTypeMin();
      range[1] = (double)array->GetDataTypeMax();
      break;

    case VTK_UNSIGNED_SHORT:
      range[0] = (double)array->GetDataTypeMin();
      if (range[1] <= 4095.0)
        {
        if (range[1] > VTK_UNSIGNED_CHAR_MAX)
          {
          range[1] = 4095.0;
          }
        }
      else
        {
        range[1] = (double)array->GetDataTypeMax();
        }
      break;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWMath::GetScalarTypeFittingRange(
  double range_min, double range_max, double scale, double shift)
{
  class TypeRange
  {
  public:
    int Type;
    double Min;
    double Max;
  };

  TypeRange FloatTypes[] = 
    {
      { VTK_FLOAT,          VTK_FLOAT_MIN,          VTK_FLOAT_MAX },
      { VTK_DOUBLE,         VTK_DOUBLE_MIN,         VTK_DOUBLE_MAX }
    };

  TypeRange IntTypes[] = 
    {
      { VTK_BIT,            VTK_BIT_MIN,            VTK_BIT_MAX },
      { VTK_CHAR,           VTK_CHAR_MIN,           VTK_CHAR_MAX },
      { VTK_UNSIGNED_CHAR,  VTK_UNSIGNED_CHAR_MIN,  VTK_UNSIGNED_CHAR_MAX },
      { VTK_SHORT,          VTK_SHORT_MIN,          VTK_SHORT_MAX },
      { VTK_UNSIGNED_SHORT, VTK_UNSIGNED_SHORT_MIN, VTK_UNSIGNED_SHORT_MAX },
      { VTK_INT,            VTK_INT_MIN,            VTK_INT_MAX },
      { VTK_UNSIGNED_INT,   VTK_UNSIGNED_INT_MIN,   VTK_UNSIGNED_INT_MAX },
      { VTK_LONG,           VTK_LONG_MIN,           VTK_LONG_MAX },
      { VTK_UNSIGNED_LONG,  VTK_UNSIGNED_LONG_MIN,  VTK_UNSIGNED_LONG_MAX }
    };

  // If the range, scale or shift are decimal number, just browse
  // the decimal types

  double intpart;

  int range_min_is_int = (modf(range_min, &intpart) == 0.0);
  int range_max_is_int = (modf(range_max, &intpart) == 0.0);
  int scale_is_int = (modf(scale, &intpart) == 0.0);
  int shift_is_int = (modf(shift, &intpart) == 0.0);

  range_min = range_min * scale + shift;
  range_max = range_max * scale + shift;

  if (range_min_is_int && range_max_is_int && scale_is_int && shift_is_int)
    {
    for (unsigned int i = 0; i < sizeof(IntTypes) / sizeof(TypeRange); i++)
      {
      if (IntTypes[i].Min <= range_min && range_max <= IntTypes[i].Max)
        {
        return IntTypes[i].Type;
        }
      }
    }

  for (unsigned int i = 0; i < sizeof(FloatTypes) / sizeof(TypeRange); i++)
    {
    if (FloatTypes[i].Min <= range_min && range_max <= FloatTypes[i].Max)
      {
      return FloatTypes[i].Type;
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
int vtkKWMath::FixTransferFunctionPointsOutOfRange(
  vtkPiecewiseFunction *func, double range[2])
{
  if (!func || !range)
    {
    return 0;
    }

  double *function_range = func->GetRange();
  
  // Make sure we have points at each end of the range

  if (function_range[0] < range[0])
    {
    func->AddPoint(range[0], func->GetValue(range[0]));
    }
  else
    {
    func->AddPoint(range[0], func->GetValue(function_range[0]));
    }

  if (function_range[1] > range[1])
    {
    func->AddPoint(range[1], func->GetValue(range[1]));
    }
  else
    {
    func->AddPoint(range[1], func->GetValue(function_range[1]));
    }

  // Remove all points out-of-range

  int func_size = func->GetSize();
  double *func_ptr = func->GetDataPointer();
  
  int i;
  for (i = func_size - 1; i >= 0; i--)
    {
    double x = func_ptr[i * 2];
    if (x < range[0] || x > range[1])
      {
      func->RemovePoint(x);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWMath::FixTransferFunctionPointsOutOfRange(
  vtkColorTransferFunction *func, double range[2])
{
  if (!func || !range)
    {
    return 0;
    }

  double *function_range = func->GetRange();
  
  // Make sure we have points at each end of the range

  double rgb[3];
  if (function_range[0] < range[0])
    {
    func->GetColor(range[0], rgb);
    func->AddRGBPoint(range[0], rgb[0], rgb[1], rgb[2]);
    }
  else
    {
    func->GetColor(function_range[0], rgb);
    func->AddRGBPoint(range[0], rgb[0], rgb[1], rgb[2]);
    }

  if (function_range[1] > range[1])
    {
    func->GetColor(range[1], rgb);
    func->AddRGBPoint(range[1], rgb[0], rgb[1], rgb[2]);
    }
  else
    {
    func->GetColor(function_range[1], rgb);
    func->AddRGBPoint(range[1], rgb[0], rgb[1], rgb[2]);
    }

  // Remove all points out-of-range

  int func_size = func->GetSize();
  double *func_ptr = func->GetDataPointer();
  
  int i;
  for (i = func_size - 1; i >= 0; i--)
    {
    double x = func_ptr[i * 4];
    if (x < range[0] || x > range[1])
      {
      func->RemovePoint(x);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWMath::ExtentIsWithinOtherExtent(int extent1[6], int extent2[6])
{
  if (!extent1 || !extent2)
    {
    return 0;
    }
  
  int i;
  for (i = 0; i < 6; i += 2)
    {
    if (extent1[i]     < extent2[i] || extent1[i]     > extent2[i + 1] ||
        extent1[i + 1] < extent2[i] || extent1[i + 1] > extent2[i + 1])
      {
      return 0;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWMath::LabToXYZ(double lab[3], double xyz[3])
{
  //LAB to XYZ
  double var_Y = ( lab[0] + 16 ) / 116;
  double var_X = lab[1] / 500 + var_Y;
  double var_Z = var_Y - lab[2] / 200;
    
  if ( pow(var_Y,3) > 0.008856 ) var_Y = pow(var_Y,3);
  else var_Y = ( var_Y - 16 / 116 ) / 7.787;
                                                            
  if ( pow(var_X,3) > 0.008856 ) var_X = pow(var_X,3);
  else var_X = ( var_X - 16 / 116 ) / 7.787;

  if ( pow(var_Z,3) > 0.008856 ) var_Z = pow(var_Z,3);
  else var_Z = ( var_Z - 16 / 116 ) / 7.787;
  double ref_X =  95.047;
  double ref_Y = 100.000;
  double ref_Z = 108.883;
  xyz[0] = ref_X * var_X;     //ref_X =  95.047  Observer= 2 Illuminant= D65
  xyz[1] = ref_Y * var_Y;     //ref_Y = 100.000
  xyz[2] = ref_Z * var_Z;     //ref_Z = 108.883
}


//----------------------------------------------------------------------------
void vtkKWMath::XYZToRGB(double xyz[3], double rgb[3])
{
  
  //double ref_X =  95.047;        //Observer = 2° Illuminant = D65
  //double ref_Y = 100.000;
  //double ref_Z = 108.883;
 
  double var_X = xyz[0] / 100;        //X = From 0 to ref_X
  double var_Y = xyz[1] / 100;        //Y = From 0 to ref_Y
  double var_Z = xyz[2] / 100;        //Z = From 0 to ref_Y
 
  double var_R = var_X *  3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
  double var_G = var_X * -0.9689 + var_Y *  1.8758 + var_Z *  0.0415;
  double var_B = var_X *  0.0557 + var_Y * -0.2040 + var_Z *  1.0570;
 
  if ( var_R > 0.0031308 ) var_R = 1.055 * ( pow(var_R, ( 1 / 2.4 )) ) - 0.055;
  else var_R = 12.92 * var_R;
  if ( var_G > 0.0031308 ) var_G = 1.055 * ( pow(var_G ,( 1 / 2.4 )) ) - 0.055;
  else  var_G = 12.92 * var_G;
  if ( var_B > 0.0031308 ) var_B = 1.055 * ( pow(var_B, ( 1 / 2.4 )) ) - 0.055;
  else var_B = 12.92 * var_B;
                                                                                                 
  rgb[0] = var_R;
  rgb[1] = var_G;
  rgb[2] = var_B;
  
  //clip colors. ideally we would do something different for colors
  //out of gamut, but not really sure what to do atm.
  if (rgb[0]<0) rgb[0]=0;
  if (rgb[1]<0) rgb[1]=0;
  if (rgb[2]<0) rgb[2]=0;
  if (rgb[0]>1) rgb[0]=1;
  if (rgb[1]>1) rgb[1]=1;
  if (rgb[2]>1) rgb[2]=1;

}
