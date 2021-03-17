/*=========================================================================

  Program:   ParaView
  Module:    vtkPVArrayInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVArrayInformation
 * @brief provides meta data about arrays.
 *
 * vtkPVArrayInformation provides meta-data about individual arrays. It is
 * accessed through `vtkPVDataInformation` and provides information like ranges,
 * component names, etc. for array present in a dataset.
 */

#ifndef vtkPVArrayInformation_h
#define vtkPVArrayInformation_h

#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" //needed for exports
#include "vtkTuple.h"              // for vtkTuple

#include <set>    // for std::set
#include <string> // for std::string
#include <vector> // for std::vector

class vtkAbstractArray;
class vtkClientServerStream;
class vtkGenericAttribute;

class VTKREMOTINGCORE_EXPORT vtkPVArrayInformation : public vtkObject
{
public:
  static vtkPVArrayInformation* New();
  vtkTypeMacro(vtkPVArrayInformation, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Resets to default state.
   */
  void Initialize();

  /**
   * Get the type for values in the array e.g. VTK_FLOAT, VTK_DOUBLE etc.
   */
  vtkGetMacro(DataType, int);

  /**
   * Returns a printable string for the array value type.
   */
  const char* GetDataTypeAsString() const { return vtkImageScalarTypeNameMacro(this->DataType); }

  /**
   * Returns range as a string. For string arrays, this lists string values
   * instead.
   */
  std::string GetRangesAsString() const;

  /**
   * Get array's name
   */
  const char* GetName() const { return this->Name.empty() ? nullptr : this->Name.c_str(); }

  /**
   * Get number of components.
   */
  int GetNumberOfComponents() const;

  /**
   * Returns the name for a component. If none present, a default component
   * name may be returned.
   */
  const char* GetComponentName(int component) const;

  /**
   * Returns the number of tuples in this array.
   */
  vtkGetMacro(NumberOfTuples, vtkTypeInt64);

  //@{
  /**
   * Returns component range. If component is `-1`, then the range of the
   * magnitude (L2 norm) over all components will be provided.
   *
   * Returns invalid range for non-numeric arrays.
   */
  const double* GetComponentRange(int comp) const VTK_SIZEHINT(2);
  void GetComponentRange(int comp, double range[2]) const;
  //@}

  //@{
  /**
   * Returns the finite range for each component.
   * If component is `-1`, then the range of the
   * magnitude (L2 norm) over all components will be provided.
   *
   * Returns invalid range for non-numeric arrays.
   */
  const double* GetComponentFiniteRange(int component) const VTK_SIZEHINT(2);
  void GetComponentFiniteRange(int comp, double range[2]) const;
  //@}

  /**
   * This method return the Min and Max possible range of the native
   * data type. For example if a vtkScalars consists of unsigned char
   * data these will return (0,255).
   * Nothing particular for 12bits data is done
   */
  void GetDataTypeRange(double range[2]) const;

  //@{
  /**
   * If IsPartial is true, this array is in only some of the
   * parts of a multi-block dataset. By default, IsPartial is
   * set to 0.
   */
  vtkGetMacro(IsPartial, bool);
  //@}

  //@{
  /**
   * Get information on the InformationKeys of this array
   */
  int GetNumberOfInformationKeys() const;
  const char* GetInformationKeyLocation(int) const;
  const char* GetInformationKeyName(int) const;
  bool HasInformationKey(const char* location, const char* name) const;
  //@}

  //@{
  /**
   * For string arrays, this returns first few non-empty values.
   */
  int GetNumberOfStringValues();
  const char* GetStringValue(int);
  //@}

  void CopyFromArray(vtkAbstractArray* array);
  void CopyFromGenericAttribute(vtkGenericAttribute* array);
  void CopyToStream(vtkClientServerStream*) const;
  bool CopyFromStream(const vtkClientServerStream*);

protected:
  vtkPVArrayInformation();
  ~vtkPVArrayInformation() override;

  friend class vtkPVDataSetAttributesInformation;
  friend class vtkPVDataInformation;

  //@{
  /**
   * API for vtkPVDataSetAttributesInformation.
   */
  void DeepCopy(vtkPVArrayInformation* info);
  void AddInformation(vtkPVArrayInformation*, int fieldAssociation);
  vtkSetMacro(IsPartial, bool);
  //@}

  vtkSetMacro(Name, std::string);

private:
  std::string Name;
  int DataType = -1;
  vtkTypeInt64 NumberOfTuples = 0;
  bool IsPartial = false;

  struct ComponentInfo
  {
    vtkTuple<double, 2> Range = vtkTuple<double, 2>({ VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX });
    vtkTuple<double, 2> FiniteRange = vtkTuple<double, 2>({ VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX });
    std::string Name;
    mutable std::string DefaultName;
  };

  std::vector<ComponentInfo> Components;
  std::vector<std::string> StringValues;

  // this array is used to store existing information keys (location/name pairs)
  std::set<std::pair<std::string, std::string> > InformationKeys;

  vtkPVArrayInformation(const vtkPVArrayInformation&) = delete;
  void operator=(const vtkPVArrayInformation&) = delete;
};

#endif
