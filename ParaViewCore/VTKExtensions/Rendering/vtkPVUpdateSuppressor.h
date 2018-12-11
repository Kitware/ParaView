/*=========================================================================

  Program:   ParaView
  Module:    vtkPVUpdateSuppressor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVUpdateSuppressor
 * @brief   prevents propagation of update
 *
 * vtkPVUpdateSuppressor is simple filter that work with
 * vtkUpdateSuppressorPipeline to block pipeline updates. One should call
 * ForceUpdate() to update the input, if needed, explicitly.
 * Note that ForceUpdate() may not result in input updating at all if it has
 * been already updated by some other means.
*/

#ifndef vtkPVUpdateSuppressor_h
#define vtkPVUpdateSuppressor_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVUpdateSuppressor : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkPVUpdateSuppressor, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with user-specified implicit function.
   */
  static vtkPVUpdateSuppressor* New();

  /**
   * Force update on the input.
   */
  virtual void ForceUpdate();

  //@{
  /**
   * Get/Set if the update suppressor is enabled. If the update suppressor
   * is not enabled, it won't suppress any updates. Enabled by default.
   */
  void SetEnabled(bool);
  vtkGetMacro(Enabled, bool);
  //@}

  //@{
  /**
   * Provides access to the timestamp when the most recent ForceUpdate() was
   * called.
   */
  vtkGetMacro(ForcedUpdateTimeStamp, vtkTimeStamp);
  //@}

protected:
  vtkPVUpdateSuppressor();
  ~vtkPVUpdateSuppressor() override;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  vtkTimeStamp ForcedUpdateTimeStamp;

  bool Enabled;

  // Create a default executive.
  vtkExecutive* CreateDefaultExecutive() override;

private:
  vtkPVUpdateSuppressor(const vtkPVUpdateSuppressor&) = delete;
  void operator=(const vtkPVUpdateSuppressor&) = delete;
};

#endif
