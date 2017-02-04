/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProminentValuesInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVProminentValuesInformation
 * @brief   Prominent values a data array takes on.
 *
 * This vtkPVInformation subclass provides a way for clients to discover
 * whether a specific remote vtkAbstractArray instance behaves like a
 * discrete set or a continuum (for each component of its tuples as well
 * as for its tuples as a whole).
 *
 * If the array behaves discretely (which we define to be: takes on fewer
 * than 33 distinct values over more than 99.9% of its entries to within a
 * given confidence that dictates the number of samples required), then
 * the prominent values are also made available.
 *
 * This class uses vtkAbstractArray::GetProminentComponentValues().
*/

#ifndef vtkPVProminentValuesInformation_h
#define vtkPVProminentValuesInformation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVInformation.h"

class vtkAbstractArray;
class vtkClientServerStream;
class vtkCompositeDataSet;
class vtkDataObject;
class vtkStdString;
class vtkStringArray;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVProminentValuesInformation
  : public vtkPVInformation
{
public:
  static vtkPVProminentValuesInformation* New();
  vtkTypeMacro(vtkPVProminentValuesInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/get the output port whose dataset should be queried.
   */
  vtkSetMacro(PortNumber, int);
  vtkGetMacro(PortNumber, int);
  //@}

  //@{
  /**
   * Set/get array's association
   */
  vtkSetStringMacro(FieldAssociation);
  vtkGetStringMacro(FieldAssociation);
  //@}

  //@{
  /**
   * Set/get array's name
   */
  vtkSetStringMacro(FieldName);
  vtkGetStringMacro(FieldName);
  //@}

  //@{
  /**
   * Changing the number of components clears the ranges back to the default.
   */
  void SetNumberOfComponents(int numComps);
  vtkGetMacro(NumberOfComponents, int);
  //@}

  //@{
  /**
   * Set/get the minimum fraction of the array that should be composed of
   * a value (between 0 and 1) in order for it to be considered prominent.

   * Setting this to one indicates that an array must have every value be
   * identical in order to have any considered prominent.
   */
  vtkSetClampMacro(Fraction, double, 0., 1.);
  vtkGetMacro(Fraction, double);
  //@}

  //@{
  /**
   * Set/get the maximum uncertainty allowed in the detection of prominent values.
   * The uncertainty is the probability of prominent values going undetected.
   * Setting this to zero forces the entire array to be inspected.
   */
  vtkSetClampMacro(Uncertainty, double, 0., 1.);
  vtkGetMacro(Uncertainty, double);
  //@}

  /**
   * Returns 1 if the array can be combined.
   * It must have the same name and number of components.
   */
  int Compare(vtkPVProminentValuesInformation* info);

  /**
   * Copy information from an \a other object.
   */
  void DeepCopy(vtkPVProminentValuesInformation* other);

  /**
   * Transfer information about a single object into this object.
   */
  virtual void CopyFromObject(vtkObject*) VTK_OVERRIDE;

  /**
   * Transfer information about a single vtkAbstractArray's prominent values into this object.

   * This is called *after* CopyFromObject has determined the number of components available;
   * this method relies on this->NumberOfComponents being valid.
   */
  virtual void CopyDistinctValuesFromObject(vtkAbstractArray*);

  /**
   * Merge another information object.
   */
  virtual void AddInformation(vtkPVInformation* other) VTK_OVERRIDE;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  virtual void CopyToStream(vtkClientServerStream*) VTK_OVERRIDE;
  virtual void CopyFromStream(const vtkClientServerStream*) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Push/pop parameters controlling which array to sample onto/off of the stream.
   */
  virtual void CopyParametersToStream(vtkMultiProcessStream&) VTK_OVERRIDE;
  virtual void CopyParametersFromStream(vtkMultiProcessStream&) VTK_OVERRIDE;
  //@}

  /**
   * Remove all parameter information.

   * You must copy/set parameter values before adding data or copying data from an object.
   */
  void InitializeParameters();

  /**
   * Remove all gathered information (but not parameters). Next add will behave like a copy.
   */
  void Initialize();

  /**
   * Merge another list of prominent values.
   */
  void AddDistinctValues(vtkPVProminentValuesInformation*);

  /**
   * Returns either NULL (array component appears to be continuous) or
   * a pointer to a vtkAbstractArray (array component appears to be discrete)
   * containing a sorted list of all distinct prominent values encountered in
   * the array component.

   * Passing -1 as the component will return information about distinct tuple values
   * as opposed to distinct component values.
   */
  vtkAbstractArray* GetProminentComponentValues(int component);

protected:
  vtkPVProminentValuesInformation();
  ~vtkPVProminentValuesInformation();

  void DeepCopyParameters(vtkPVProminentValuesInformation* other);
  void CopyFromCompositeDataSet(vtkCompositeDataSet*);
  void CopyFromLeafDataObject(vtkDataObject*);

  /// Information parameters
  //@{
  int PortNumber;
  int NumberOfComponents;
  char* FieldName;
  char* FieldAssociation;
  double Fraction;
  double Uncertainty;
  //@}

  /// Information results
  //@{

  class vtkInternalDistinctValues;
  vtkInternalDistinctValues* DistinctValues;

  //@}

  vtkPVProminentValuesInformation(const vtkPVProminentValuesInformation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVProminentValuesInformation&) VTK_DELETE_FUNCTION;
};

#endif
