/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkKWMath.h"

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkKWMath, "1.4.4.1");
vtkStandardNewMacro(vtkKWMath);

//----------------------------------------------------------------------------
int vtkKWMath::GetScalarRange(vtkDataArray *array, int comp, float range[2])
{
  if (!array || comp < 0 || comp >= array->GetNumberOfComponents())
    {
    return 0;
    }
  
  array->GetRange(range, comp);
  return 1;
}

//----------------------------------------------------------------------------
template <class T>
void vtkKWMathGetScalarRange(
  vtkDataArray *array, int comp, double range[2], T *)
{
  if (!array || comp < 0 || comp >= array->GetNumberOfComponents())
    {
    return;
    }

  vtkIdType nb_of_scalars = array->GetNumberOfTuples();
  int nb_of_components = array->GetNumberOfComponents();

  T *data = (T*)array->GetVoidPointer(0) + comp;
  T *data_end = data + nb_of_scalars * nb_of_components;

  double min = VTK_DOUBLE_MAX;
  double max = VTK_DOUBLE_MIN;

  if (nb_of_components > 1)
    {
    while (data < data_end)
      {
      if (*data < min)
        {
        min = *data;
        }
      if (*data > max)
        {
        max = *data;
        }
      data += nb_of_components;
      }
    }
  else
    {
    while (data < data_end)
      {
      if (*data < min)
        {
        min = *data;
        }
      if (*data > max)
        {
        max = *data;
        }
      data++;
      }
    }

  range[0] = min;
  range[1] = max;
}

//----------------------------------------------------------------------------
int vtkKWMath::GetScalarRange(vtkDataArray *array, int comp, double range[2])
{
  if (!array || comp < 0 || comp >= array->GetNumberOfComponents())
    {
    return 0;
    }

  switch (array->GetDataType())
    {
    vtkTemplateMacro4(vtkKWMathGetScalarRange,
                      array, comp, range, static_cast<VTK_TT *>(0));
    }
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWMath::GetAdjustedScalarRange(
  vtkDataArray *array, int comp, float range[2])
{
  if (!vtkKWMath::GetScalarRange(array, comp, range))
    {
    return 0;
    }

  switch (array->GetDataType())
    {
    case VTK_UNSIGNED_CHAR:
      range[0] = (float)array->GetDataTypeMin();
      range[1] = (float)array->GetDataTypeMax();
      break;

    case VTK_UNSIGNED_SHORT:
      range[0] = (float)array->GetDataTypeMin();
      if (range[1] <= 4095.0)
        {
        if (range[1] > VTK_UNSIGNED_CHAR_MAX)
          {
          range[1] = 4095.0;
          }
        }
      else
        {
        range[1] = (float)array->GetDataTypeMax();
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

