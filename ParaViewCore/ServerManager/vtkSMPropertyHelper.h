/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
// .NAME vtkSMPropertyHelper - helper class to get/set property values.
// .SECTION Description
// vtkSMPropertyHelper is a helper class to get/set property values in a type
// independent fashion.
// eg.
// \code
//    vtkSMPropertyHelper(proxy, "Visibility").Set(0);
//    vtkSMPropertyHelper(proxy, "Input").Set(inputProxy, 0);
//
//    double center[3] = {...};
//    vtkSMPropertyHelper(proxy, "Center").Set(center, 3);
// \endcode
// .SECTION Caveat
// This class is not wrapped, hence not available in any of the wrapped
// languagues such as python.

#ifndef __vtkSMPropertyHelper_h
#define __vtkSMPropertyHelper_h

#include "vtkSMObject.h"
#include "vtkVariant.h"

#include <vector>

#ifdef INT
#undef INT
#endif
#ifdef DOUBLE
#undef DOUBLE
#endif
#ifdef NONE
#undef NONE
#endif

class vtkSMProperty;
class vtkSMProxy;
class vtkSMVectorProperty;
class vtkSMIntVectorProperty;
class vtkSMDoubleVectorProperty;
class vtkSMIdTypeVectorProperty;
class vtkSMStringVectorProperty;
class vtkSMProxyProperty;
class vtkSMInputProperty;

class VTK_EXPORT vtkSMPropertyHelper 
{
public:
  // Description:
  // If quiet is true, then no errors or warning are raised if the property is
  // missing or of incorrect type.
  vtkSMPropertyHelper(vtkSMProxy* proxy, const char* name, bool quiet=false);
  vtkSMPropertyHelper(vtkSMProperty *property, bool quiet = false);
  ~vtkSMPropertyHelper();

  // Description:
  // Updates the property value by fetching the value from the server. This only
  // works for properties with \c InformationOnly attribute set to 1. For all
  // other properties, this has no effect.
  void UpdateValueFromServer();

  // Description:
  // Set the number of elements in the property. For vtkSMProxyProperty, this is
  // equivalent to SetNumberOfProxies().
  void SetNumberOfElements(unsigned int elems);

  // Description:
  // Get the number of elements in the property.
  // For vtkSMProxyProperty, this is equivalent to GetNumberOfProxies().
  unsigned int GetNumberOfElements() const;

  // Description:
  // Equivalent to SetNumberOfElements(0).
  void RemoveAllValues() { this->SetNumberOfElements(0); }

  // Description:
  // Get value as a variant.
  vtkVariant GetAsVariant(unsigned int index);

  // Description:
  // Set/Get methods with \c int API. Calling these method on
  // vtkSMStringVectorProperty or vtkSMProxyProperty will raise errors.
  void Set(int value)
    { this->Set(0, value); }
  void Set(unsigned int index, int value);
  void Set(const int* values, unsigned int count);
  int GetAsInt(unsigned int index = 0);
  unsigned int Get(int* values, unsigned int count = 1);
  std::vector<int> GetIntArray();

  // Description:
  // Set/Get methods with \c double API. Calling these method on
  // vtkSMStringVectorProperty or vtkSMProxyProperty will raise errors.
  void Set(double value)
    { this->Set(0, value); }
  void Set(unsigned int index, double value);
  void Set(const double* values, unsigned int count);
  double GetAsDouble(unsigned int index = 0);
  unsigned int Get(double* values, unsigned int count = 1);
  std::vector<double> GetDoubleArray();

#if VTK_SIZEOF_ID_TYPE != VTK_SIZEOF_INT
  // Description:
  // Set/Get methods with \c vtkIdType API. Calling these method on
  // vtkSMStringVectorProperty or vtkSMProxyProperty will raise errors.
  void Set(vtkIdType value)
    { this->Set(0, value); }
  void Set(unsigned int index, vtkIdType value);
  void Set(const vtkIdType* values, unsigned int count);
  unsigned int Get(vtkIdType* values, unsigned int count = 1);
#endif
  vtkIdType GetAsIdType(unsigned int index = 0);
  std::vector<vtkIdType> GetIdTypeArray();

  // Description:
  // Set/Get methods for vtkSMStringVectorProperty. Calling these methods on any
  // other type of property will raise errors.
  // These overloads can be used for vtkSMIntVectorProperty with an enumeration
  // domain as well, in which case the string-to-int (and vice-versa)
  // translations are done internally.
  void Set(const char* value)
    { this->Set(0, value); }
  void Set(unsigned int index, const char* value);
  const char* GetAsString(unsigned int index = 0);

  // Description:
  // Set/Get methods for vtkSMProxyProperty or vtkSMInputProperty. 
  // Calling these methods on any other type of property will raise errors.
  // The option \c outputport(s) argument is used only for vtkSMInputProperty.
  void Set(vtkSMProxy* value, unsigned int outputport=0)
    { this->Set(0, value, outputport); }
  void Set(unsigned int index, vtkSMProxy* value, unsigned int outputport=0);
  void Set(vtkSMProxy** value, unsigned int count, unsigned int *outputports=NULL);
  void Add(vtkSMProxy* value, unsigned int outputport=0);
  void Remove(vtkSMProxy* value);
  vtkSMProxy* GetAsProxy(unsigned int index=0);
  unsigned int GetOutputPort(unsigned int index=0);
 
  // Description:
  // This API is useful for setting values on vtkSMStringVectorProperty that is
  // used for status where the first value is the name of the array (for
  // example) and the second value is it's status.
  void SetStatus(const char* key, int value);
  int GetStatus(const char* key, int default_value=0);

  // Description:
  // This API is useful for setting values on vtkSMStringVectorProperty that is
  // used for status where the first value is the name of the array (for
  // example) and the second value is it's status.
  void SetStatus(const char* key, double *values, int num_values);
  bool GetStatus(const char* key, double *values, int num_values);

  // Description:
  // This API is useful for setting values on vtkSMStringVectorProperty that is
  // used for status where the first value is the name of the array (for
  // example) and the second value is it's status (as a string)
  void SetStatus(const char* key, const char* value);
  const char* GetStatus(const char* key, const char* default_value);

protected:
  void setUseUnchecked(bool useUnchecked) { this->UseUnchecked = useUnchecked; }

//BTX
private:
  vtkSMPropertyHelper(const vtkSMPropertyHelper&); // Not implemented
  void operator=(const vtkSMPropertyHelper&); // Not implemented
  void Initialize(vtkSMProperty *property);

  template<typename T> T GetProperty(unsigned int index) const;
  template<typename T> std::vector<T> GetPropertyArray() const;
  template<typename T> unsigned int GetPropertyArray(T *values, unsigned int count = 1);
  template<typename T> void SetProperty(unsigned int index, T value);
  template<typename T> void SetPropertyArray(const T *values, unsigned int count);
 
  enum PType {
    INT,
    DOUBLE,
    IDTYPE,
    STRING,
    PROXY,
    INPUT,
    NONE
  };

  bool Quiet;
  bool UseUnchecked;
  vtkSMProxy* Proxy;
  PType Type;

  union
    {
    vtkSMProperty *Property;
    vtkSMVectorProperty *VectorProperty;
    vtkSMIntVectorProperty *IntVectorProperty;
    vtkSMDoubleVectorProperty *DoubleVectorProperty;
    vtkSMIdTypeVectorProperty *IdTypeVectorProperty;
    vtkSMStringVectorProperty *StringVectorProperty;
    vtkSMProxyProperty *ProxyProperty;
    vtkSMInputProperty *InputProperty;
    };
//ETX
};

#endif

