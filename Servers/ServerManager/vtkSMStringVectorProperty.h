/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStringVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStringVectorProperty - property representing a vector of strings
// .SECTION Description
// vtkSMStringVectorProperty is a concrete sub-class of vtkSMVectorProperty
// representing a vector of strings. vtkSMStringVectorProperty can also
// be used to store double and int values as strings. The strings
// are converted to the appropriate type when they are being passed
// to the stream. This is generally used for calling methods that have mixed 
// type arguments.
// .SECTION See Also
// vtkSMVectorProperty vtkSMDoubleVectorProperty vtkSMIntVectorProperty

#ifndef __vtkSMStringVectorProperty_h
#define __vtkSMStringVectorProperty_h

#include "vtkSMVectorProperty.h"

//BTX
struct vtkSMStringVectorPropertyInternals;
//ETX

class vtkStringList;

class VTK_EXPORT vtkSMStringVectorProperty : public vtkSMVectorProperty
{
public:
  static vtkSMStringVectorProperty* New();
  vtkTypeMacro(vtkSMStringVectorProperty, vtkSMVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the size of the vector.
  virtual unsigned int GetNumberOfElements();

  // Description:
  // Sets the size of the vector. If num is larger than the current
  // number of elements, this may cause reallocation and copying.
  virtual void SetNumberOfElements(unsigned int num);

  // Description:
  // Set the value of 1 element. The vector is resized as necessary.
  // Returns 0 if Set fails either because the property is read only
  // or the value is not in all domains. Returns 1 otherwise.
  int SetElement(unsigned int idx, const char* value);

  // Description:
  // Sets multiple elements. The size of the property is changed to match count.
  int SetElements(unsigned int count, const char* values[]);

  // Description:
  // Sets multiple elements. The size of the property is changed to match count.
  int SetElements(vtkStringList* newvalue);

  // Description:
  // Fills up the vtkStringList instance with the current value.
  void GetElements(vtkStringList* list);

  // Description:
  // Returns the value of 1 element.
  const char* GetElement(unsigned int idx);

  // Description:
  // Returns the index of an element with a particular value.
  // exists is set to false if element does not exist.
  unsigned int GetElementIndex(const char *value, int& exists);
  
  // Description:
  // Set the cast type used when passing a value to the stream.
  // For example, if the type is INT, the string is converted
  // to an int (with atoi()) before being passed to stream.
  // Note that representing scalar values as strings can result
  // in loss of accuracy.
  // Possible values are: INT, DOUBLE, STRING.
  void SetElementType(unsigned int idx, int type);
  int GetElementType(unsigned int idx);

  // Description:
  // Returns the value of 1 unchecked element. These are used by
  // domains. SetElement() first sets the value of 1 unchecked
  // element and then calls IsInDomain and updates the value of
  // the corresponding element only if IsInDomain passes.
  const char* GetUncheckedElement(unsigned int idx);

  // Description:
  // Set the value of 1 unchecked element. This can be used to
  // check if a value is in all domains of the property. Call
  // this and call IsInDomains().
  void SetUncheckedElement(unsigned int idx, const char* value);

  // Description:
  // Returns the size of unchecked elements. Usually this is
  // the same as the number of elements but can be different
  // before a domain check is performed.
  virtual unsigned int GetNumberOfUncheckedElements();

  //BTX
  enum ElementTypes{ INT, DOUBLE, STRING };
  //ETX

  // Description: 
  // Copy all property values.
  virtual void Copy(vtkSMProperty* src);

  // Description:
  // Returns the default value, if any, specified in the XML.
  const char* GetDefaultValue(int idx);

protected:
  vtkSMStringVectorProperty();
  ~vtkSMStringVectorProperty();

  vtkSMStringVectorPropertyInternals* Internals;

  // Description:
  // Sets the size of unchecked elements. Usually this is
  // the same as the number of elements but can be different
  // before a domain check is performed.
  virtual void SetNumberOfUncheckedElements(unsigned int num);

  //BTX  
  // Description:
  // Update the vtk object (with the given id and on the given
  // nodes) with the property values(s).
  virtual void AppendCommandToStream(
    vtkSMProxy*, vtkClientServerStream* stream, vtkClientServerID objectId );
  //ETX

  virtual int ReadXMLAttributes(vtkSMProxy* parent, 
                                vtkPVXMLElement* element);

  // Description:
  // Updates state from an XML element. Returns 0 on failure.
  virtual int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader,
    int loadLastPushedValues=0);

  virtual void ChildSaveState(vtkPVXMLElement* parent, int saveLastPushedValues);
  bool Initialized;

private:
  vtkSMStringVectorProperty(const vtkSMStringVectorProperty&); // Not implemented
  void operator=(const vtkSMStringVectorProperty&); // Not implemented
};

#endif
