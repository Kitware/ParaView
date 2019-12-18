/*=========================================================================

  Program:   ParaView
  Module:    vtkTimeToTextConvertor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTimeToTextConvertor
 *
 * This filter can be attached to any filter/source/reader that supports time.
 * vtkTimeToTextConvertor will generate a 1x1 vtkTable with the string
 * for the data time using the format specified.
 * The input to this filter is optional. If no input is specified, it will show
 * produce request time in the output.
*/

#ifndef vtkTimeToTextConvertor_h
#define vtkTimeToTextConvertor_h

#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports
#include "vtkTableAlgorithm.h"

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkTimeToTextConvertor : public vtkTableAlgorithm
{
public:
  static vtkTimeToTextConvertor* New();
  vtkTypeMacro(vtkTimeToTextConvertor, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the format in which the to display the
   * input update time. Use printf formatting.
   * Default is "Time: %f".
   */
  vtkSetStringMacro(Format);
  vtkGetStringMacro(Format);
  //@}

  //@{
  /**
   * Apply a translation to the time
   */
  vtkSetMacro(Shift, double);
  vtkGetMacro(Shift, double);
  //@}

  //@{
  /**
   * Apply a scale to the time.
   */
  vtkSetMacro(Scale, double);
  vtkGetMacro(Scale, double);
  //@}

protected:
  vtkTimeToTextConvertor();
  ~vtkTimeToTextConvertor() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  char* Format;
  double Shift;
  double Scale;

private:
  vtkTimeToTextConvertor(const vtkTimeToTextConvertor&) = delete;
  void operator=(const vtkTimeToTextConvertor&) = delete;
};

#endif
