/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDoubleVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDoubleVectorProperty - property representing a vector of doubles
// .SECTION Description
// vtkSMDoubleVectorProperty is a concrete sub-class of vtkSMVectorProperty
// representing a vector of doubles.
// .SECTION See Also
// vtkSMVectorProperty vtkSMIntVectorProperty vtkSMStringVectorProperty

#ifndef __vtkSMDoubleVectorProperty_h
#define __vtkSMDoubleVectorProperty_h

#include "vtkSMVectorProperty.h"

//BTX
struct vtkSMDoubleVectorPropertyInternals;
//ETX

class VTK_EXPORT vtkSMDoubleVectorProperty : public vtkSMVectorProperty
{
public:
  static vtkSMDoubleVectorProperty* New();
  vtkTypeRevisionMacro(vtkSMDoubleVectorProperty, vtkSMVectorProperty);
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
  int SetElement(unsigned int idx, double value);

  // Description:
  // Set the values of all elements. The size of the values array
  // has to be equal or larger to the size of the vector.
  // Returns 0 if Set fails either because the property is read only
  // or one or more of the values is not in all domains.
  // Returns 1 otherwise.
  int SetElements(const double* values);
  double *GetElements();

  // Description:
  // Set the value of 1st element. The vector is resized as necessary.
  // Returns 0 if Set fails either because the property is read only
  // or one or more of the values is not in all domains.
  // Returns 1 otherwise.
  int SetElements1(double value0);

  // Description:
  // Set the values of the first 2 elements. The vector is resized as necessary.
  // Returns 0 if Set fails either because the property is read only
  // or one or more of the values is not in all domains.
  // Returns 1 otherwise.
  int SetElements2(double value0, double value1);

  // Description:
  // Set the values of the first 3 elements. The vector is resized as necessary.
  // Returns 0 if Set fails either because the property is read only
  // or one or more of the values is not in all domains.
  // Returns 1 otherwise.
  int SetElements3(double value0, double value1, double value2);

  // Description:
  // Set the values of the first 4 elements. The vector is resized as necessary.
  // Returns 0 if Set fails either because the property is read only
  // or one or more of the values is not in all domains.
  // Returns 1 otherwise.
  int SetElements4(double value0, double value1, double value2, double value3);

  // Description:
  // Returns the value of 1 element.
  double GetElement(unsigned int idx);

  // Description:
  // Returns the size of unchecked elements. Usually this is
  // the same as the number of elements but can be different
  // before a domain check is performed.
  virtual unsigned int GetNumberOfUncheckedElements();

  // Description:
  // Returns the value of 1 unchecked element. These are used by
  // domains. SetElement() first sets the value of 1 unchecked
  // element and then calls IsInDomain and updates the value of
  // the corresponding element only if IsInDomain passes.
  double GetUncheckedElement(unsigned int idx);

  // Description:
  // Set the value of 1 unchecked element. This can be used to
  // check if a value is in all domains of the property. Call
  // this and call IsInDomains().
  void SetUncheckedElement(unsigned int idx, double value);

  // Description:
  // If ArgumentIsArray is true, multiple elements are passed in as
  // array arguments. For example, For example, if
  // RepeatCommand is true, NumberOfElementsPerCommand is 2, the
  // command is SetFoo and the values are 1 2 3 4 5 6, the resulting
  // stream will have:
  // @verbatim
  // * Invoke obj SetFoo array(1, 2)
  // * Invoke obj SetFoo array(3, 4)
  // * Invoke obj SetFoo array(5, 6)
  // @endverbatim
  vtkGetMacro(ArgumentIsArray, int);
  vtkSetMacro(ArgumentIsArray, int);

protected:
  vtkSMDoubleVectorProperty();
  ~vtkSMDoubleVectorProperty();

  virtual int ReadXMLAttributes(vtkSMProxy* parent, 
                                vtkPVXMLElement* element);

//BTX  
  // Description:
  // Append a command to update the vtk object with the property values(s).
  // The proxy objects create a stream by calling this method on all the
  // modified properties.
  virtual void AppendCommandToStream(
    vtkSMProxy*, vtkClientServerStream* stream, vtkClientServerID objectId );
//ETX

  vtkSMDoubleVectorPropertyInternals* Internals;

  int ArgumentIsArray;

  virtual void SaveState(const char* name, ostream* file, vtkIndent indent);

  // Description:
  // Sets the size of unchecked elements. Usually this is
  // the same as the number of elements but can be different
  // before a domain check is performed.
  virtual void SetNumberOfUncheckedElements(unsigned int num);

  // Description:
  // If SetNumberCommand is set, it is called before Command
  // with the number of arguments as the parameter.
  vtkSetStringMacro(SetNumberCommand);
  vtkGetStringMacro(SetNumberCommand);

  char* SetNumberCommand;

private:
  vtkSMDoubleVectorProperty(const vtkSMDoubleVectorProperty&); // Not implemented
  void operator=(const vtkSMDoubleVectorProperty&); // Not implemented
};

#endif
