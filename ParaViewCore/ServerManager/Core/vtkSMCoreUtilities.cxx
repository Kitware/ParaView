/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCoreUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCoreUtilities.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <sstream>

vtkStandardNewMacro(vtkSMCoreUtilities);
//----------------------------------------------------------------------------
vtkSMCoreUtilities::vtkSMCoreUtilities()
{
}

//----------------------------------------------------------------------------
vtkSMCoreUtilities::~vtkSMCoreUtilities()
{
}

//----------------------------------------------------------------------------
const char* vtkSMCoreUtilities::GetFileNameProperty(vtkSMProxy* proxy)
{
  if (!proxy)
  {
    return NULL;
  }

  if (proxy->GetHints())
  {
    vtkPVXMLElement* filenameHint =
      proxy->GetHints()->FindNestedElementByName("DefaultFileNameProperty");
    if (filenameHint && filenameHint->GetAttribute("name") &&
      proxy->GetProperty(filenameHint->GetAttribute("name")))
    {
      return filenameHint->GetAttribute("name");
    }
  }

  // Find the first property that has a vtkSMFileListDomain. Assume that
  // it is the property used to set the filename.
  vtkSmartPointer<vtkSMPropertyIterator> piter;
  piter.TakeReference(proxy->NewPropertyIterator());
  piter->Begin();
  while (!piter->IsAtEnd())
  {
    vtkSMProperty* prop = piter->GetProperty();
    if (prop && prop->IsA("vtkSMStringVectorProperty"))
    {
      vtkSmartPointer<vtkSMDomainIterator> diter;
      diter.TakeReference(prop->NewDomainIterator());
      diter->Begin();
      while (!diter->IsAtEnd())
      {
        if (diter->GetDomain()->IsA("vtkSMFileListDomain"))
        {
          return piter->GetKey();
        }
        diter->Next();
      }
      if (!diter->IsAtEnd())
      {
        break;
      }
    }
    piter->Next();
  }
  return NULL;
}

//----------------------------------------------------------------------------
// This is reimplemented in python's paraview.make_name_valid(). Keep both
// implementations consistent.
vtkStdString vtkSMCoreUtilities::SanitizeName(const char* name)
{
  if (!name || name[0] == '\0')
  {
    return vtkStdString();
  }

  std::ostringstream cname;
  for (size_t cc = 0; name[cc]; cc++)
  {
    if (isalnum(name[cc]) || name[cc] == '_')
    {
      cname << name[cc];
    }
  }
  // if first character is not an alphabet, add an 'a' to it.
  if (cname.str().empty() || isalpha(cname.str()[0]))
  {
    return cname.str();
  }
  else
  {
    return "a" + cname.str();
  }
}

//----------------------------------------------------------------------------
bool vtkSMCoreUtilities::AdjustRangeForLog(double range[2])
{
  assert(range[0] <= range[1]);
  if (range[0] <= 0.0 || range[1] <= 0.0)
  {
    // ranges not valid for log-space. Cannot convert.
    if (range[1] <= 0.0)
    {
      range[0] = 1.0e-4;
      range[1] = 1.0;
    }
    else
    {
      range[0] = range[1] * 0.0001;
      range[0] = (range[0] < 1.0) ? range[0] : 1.0;
    }
    return true;
  }
  return false;
}

namespace
{

template <typename T>
struct MinDelta
{
};
// This value seems to work well for float ranges we have tested
template <>
struct MinDelta<float>
{
  static const int value = 65536;
};
// There are 29 more bits in a double's mantissa compared to a float.  Shift the
// offset so that the 65536 is in the same position relative to the beginning of
// the mantissa.  This causes the modified range to still be valid when it is
// downcast to float later on.
template <>
struct MinDelta<double>
{
  static const vtkTypeInt64 value = static_cast<vtkTypeInt64>(65536) << 29;
};

//----------------------------------------------------------------------------
template <typename T, typename EquivSizeIntT>
bool AdjustTRange(T range[2], EquivSizeIntT)
{
  if (range[1] < range[0])
  {
    // invalid range.
    return false;
  }

  const bool spans_zero_boundary = range[0] < 0 && range[1] > 0;
  if (spans_zero_boundary)
  { // nothing needs to be done, but this check is required.
    // if we convert into integer space the delta difference will overflow
    // an integer
    return false;
  }

  EquivSizeIntT irange[2];
  // needs to be a memcpy to avoid strict aliasing issues, doing a count
  // of 2*sizeof(T) to couple both values at the same time
  std::memcpy(irange, range, sizeof(T) * 2);
  const EquivSizeIntT minDelta = MinDelta<T>::value;

  // determine the absolute delta between these two numbers.
  EquivSizeIntT delta = std::abs(irange[1] - irange[0]);

  // if our delta is smaller than the min delta push out the max value
  // so that it is equal to minRange + minDelta. When our range is entirely
  // negative we should instead subtract from our max, to max a larger negative
  // value
  if (delta < minDelta && irange[1] < 0)
  {
    irange[1] = irange[0] - minDelta;
    // needs to be a memcpy to avoid strict aliasing issues
    std::memcpy(range + 1, irange + 1, sizeof(T));
    return true;
  }
  if (delta < minDelta)
  {
    irange[1] = irange[0] + minDelta;
    // needs to be a memcpy to avoid strict aliasing issues
    std::memcpy(range + 1, irange + 1, sizeof(T));
    return true;
  }
  return false;
}
}

//----------------------------------------------------------------------------
bool vtkSMCoreUtilities::AdjustRange(double range[2])
{
  // when the values are between [-2, 2] and you push the max out you need to
  // make sure you do it in float space, since if you do it in double space the
  // values will be truncated downstream and be made equivalent again. Once
  // you move out of this range the double space is generally not truncated
  if (range[0] > -2 && range[0] < 2)
  {
    float frange[2] = { static_cast<float>(range[0]), static_cast<float>(range[1]) };
    bool result = AdjustTRange(frange, int());
    if (result)
    { // range should be left untouched to avoid loss of precision when no
      // adjustment was needed
      range[0] = static_cast<double>(frange[0]);
      range[1] = static_cast<double>(frange[1]);
    }
    return result;
  }
  vtkTypeInt64 intType = 0;
  return AdjustTRange(range, intType);
}

//----------------------------------------------------------------------------
void vtkSMCoreUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
