/*=========================================================================

  Program:   ParaView
  Module:    vtkAnnotateGlobalDataFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkAnnotateGlobalDataFilter
 * @brief filter for annotating with global / field data
 *
 * vtkAnnotateGlobalDataFilter is a filter that can be used to generate a
 * vtkTable with a string that contains the contents of a global / field data
 * array. It also supports extracting temporal field data arrays that
 * vtkExodusIIReader produces for "global-data". These are simply arrays that
 * have as many tuples are number of timesteps. The filter then extracts only
 * the tuple associated with the current time-step.
 */

#ifndef vtkAnnotateGlobalDataFilter_h
#define vtkAnnotateGlobalDataFilter_h

#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports
#include "vtkTableAlgorithm.h"

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkAnnotateGlobalDataFilter : public vtkTableAlgorithm
{
public:
  static vtkAnnotateGlobalDataFilter* New();
  vtkTypeMacro(vtkAnnotateGlobalDataFilter, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Name of the field to display
   */
  vtkSetStringMacro(FieldArrayName);
  vtkGetStringMacro(FieldArrayName);
  //@}

  //@{
  /**
   * Set the text prefix to display in front of the Field value
   */
  vtkSetStringMacro(Prefix);
  vtkGetStringMacro(Prefix);
  //@}

  //@{
  /**
   * Set the text prefix to display in front of the Field value
   */
  vtkSetStringMacro(Postfix);
  vtkGetStringMacro(Postfix);
  //@}

  //@{
  /**
   * Set the format to use when displaying the field value
   */
  vtkSetStringMacro(Format);
  vtkGetStringMacro(Format);
  //@}

  //@{
  /**
   * Get/Set the controller to use.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}
protected:
  vtkAnnotateGlobalDataFilter();
  ~vtkAnnotateGlobalDataFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  char* Prefix;
  char* Postfix;
  char* FieldArrayName;
  char* Format;
  vtkMultiProcessController* Controller;

private:
  vtkAnnotateGlobalDataFilter(const vtkAnnotateGlobalDataFilter&) = delete;
  void operator=(const vtkAnnotateGlobalDataFilter&) = delete;
};

#endif
