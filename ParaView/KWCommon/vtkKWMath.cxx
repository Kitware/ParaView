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

vtkCxxRevisionMacro(vtkKWMath, "1.2");
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
template <class T>
void vtkKWMathGetScalarMinDelta(
  vtkDataArray *array, int comp, double *delta, T *)
{
  if (!array || comp < 0 || comp >= array->GetNumberOfComponents() || !delta)
    {
    return;
    }

  vtkIdType nb_of_scalars = array->GetNumberOfTuples();
  int nb_of_components = array->GetNumberOfComponents();

  T *data = (T*)array->GetVoidPointer(0) + comp;
  T *data_end = data + nb_of_scalars * nb_of_components;

  T min1 = (T)array->GetRange(comp)[0];
  T min2 = (T)array->GetRange(comp)[1];

  if (nb_of_components > 1)
    {
    while (data < data_end)
      {
      if (*data < min2 && *data > min1)
        {
        min2 = *data;
        }
      data += nb_of_components;
      }
    }
  else
    {
    while (data < data_end)
      {
      if (*data < min2 && *data > min1)
        {
        min2 = *data;
        }
      data++;
      }
    }

  *delta = (double)min2 - (double)min1;
}

//----------------------------------------------------------------------------
int vtkKWMath::GetScalarMinDelta(vtkDataArray *array, int comp, double *delta)
{
  if (!array || comp < 0 || comp >= array->GetNumberOfComponents() || !delta)
    {
    return 0;
    }

  switch (array->GetDataType())
    {
    vtkTemplateMacro4(vtkKWMathGetScalarMinDelta,
                      array, comp, delta, static_cast<VTK_TT *>(0));
    }
  
  return 1;
}
