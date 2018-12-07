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
 * @class   vtkPVArrayInformation
 * @brief   Data array information like type.
 *
 * This objects is for eliminating direct access to vtkDataObjects
 * by the "client".  Only vtkPVPart and vtkPVProcessModule should access
 * the data directly.  At the moment, this object is only a container
 * and has no useful methods for operating on data.
 * Note:  I could just use vtkDataArray objects and store the range
 * as values in the array.  This would eliminate this object.
*/

#ifndef vtkPVArrayInformation_h
#define vtkPVArrayInformation_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkPVInformation.h"
class vtkAbstractArray;
class vtkClientServerStream;
class vtkStdString;
class vtkStringArray;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVArrayInformation : public vtkPVInformation
{
public:
  static vtkPVArrayInformation* New();
  vtkTypeMacro(vtkPVArrayInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * DataType is the string name of the data type: VTK_FLOAT ...
   * the value "VTK_VOID" means that different processes have different types.
   */
  vtkSetMacro(DataType, int);
  vtkGetMacro(DataType, int);
  //@}

  //@{
  /**
   * Set/get array's name
   */
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);
  //@}

  //@{
  /**
   * Changing the number of components clears the ranges back to the default.
   */
  void SetNumberOfComponents(int numComps);
  vtkGetMacro(NumberOfComponents, int);
  //@}

  /**
   * Set the name for a component. Must be >= 1.
   */
  void SetComponentName(vtkIdType component, const char* name);

  /**
   * Get the component name for a given component.
   * Note: the const char* that is returned is only valid
   * intill the next call to this method!
   */
  const char* GetComponentName(vtkIdType component);

  //@{
  /**
   * Set/get the array's length
   */
  vtkSetMacro(NumberOfTuples, vtkTypeInt64);
  vtkGetMacro(NumberOfTuples, vtkTypeInt64);
  //@}

  //@{
  /**
   * There is a range for each component.
   * Range for component -1 is the range of the vector magnitude.
   * The number of components should be set before these ranges.
   */
  void SetComponentRange(int comp, double min, double max);
  void SetComponentRange(int comp, double* range)
  {
    this->SetComponentRange(comp, range[0], range[1]);
  }
  double* GetComponentRange(int component) VTK_SIZEHINT(2);
  void GetComponentRange(int comp, double range[2]);
  //@}

  //@{
  /**
   * There is a range for each component.
   * Range for component -1 is the range of the vector magnitude.
   * The number of components should be set before these ranges.
   */
  void SetComponentFiniteRange(int comp, double min, double max);
  void SetComponentFiniteRange(int comp, double* range)
  {
    this->SetComponentFiniteRange(comp, range[0], range[1]);
  }
  double* GetComponentFiniteRange(int component);
  void GetComponentFiniteRange(int comp, double range[2]);
  //@}

  /**
   * This method return the Min and Max possible range of the native
   * data type. For example if a vtkScalars consists of unsigned char
   * data these will return (0,255).
   * Nothing particular for 12bits data is done
   */
  void GetDataTypeRange(double range[2]);

  /**
   * Returns 1 if the array can be combined.
   * It must have the same name and number of components.
   */
  int Compare(vtkPVArrayInformation* info);

  /**
   * Merge (union) ranges into this object.
   */
  void AddRanges(vtkPVArrayInformation* info);
  void AddFiniteRanges(vtkPVArrayInformation* info);

  void DeepCopy(vtkPVArrayInformation* info);

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) override;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  //@}

  //@{
  /**
   * If IsPartial is true, this array is in only some of the
   * parts of a multi-block dataset. By default, IsPartial is
   * set to 0.
   */
  vtkSetMacro(IsPartial, int);
  vtkGetMacro(IsPartial, int);
  //@}

  /**
   * Remove all infommation. Next add will be like a copy.
   */
  void Initialize();

  //@{
  /**
   * Merge (union) keys into this object.
   */
  void AddInformationKeys(vtkPVArrayInformation* info);
  void AddInformationKey(const char* location, const char* name);
  void AddUniqueInformationKey(const char* location, const char* name);
  //@}

  //@{
  /**
   * Get information on the InformationKeys of this array
   */
  int GetNumberOfInformationKeys();
  const char* GetInformationKeyLocation(int);
  const char* GetInformationKeyName(int);
  int HasInformationKey(const char* location, const char* name);
  //@}

protected:
  vtkPVArrayInformation();
  ~vtkPVArrayInformation() override;

  int IsPartial;
  int DataType;
  int NumberOfComponents;
  vtkTypeInt64 NumberOfTuples;
  char* Name;
  double* Ranges;
  double* FiniteRanges;

  // this array is used to store existing information keys (location/name pairs)

  class vtkInternalInformationKeys;
  vtkInternalInformationKeys* InformationKeys;

  // this is used by GetComponentName, so that it always return a valid component name

  vtkStdString* DefaultComponentName;

  /// assigns to a string to DefaultComponentName for this component
  void DetermineDefaultComponentName(const int& component_no, const int& numComps);

  class vtkInternalComponentNames;
  vtkInternalComponentNames* ComponentNames;

  vtkPVArrayInformation(const vtkPVArrayInformation&) = delete;
  void operator=(const vtkPVArrayInformation&) = delete;
};

#endif
