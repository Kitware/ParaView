/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUpdateSuppressorPipeline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUpdateSuppressorPipeline
 * @brief   pipeline for vtkPVUpdateSuppressor
 *
 * vtkUpdateSuppressorPipeline is designed to for with vtkPVUpdateSuppressor.
 * It stops all update extent and data requests.
*/

#ifndef vtkUpdateSuppressorPipeline_h
#define vtkUpdateSuppressorPipeline_h

#include "vtkCompositeDataPipeline.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkUpdateSuppressorPipeline
  : public vtkCompositeDataPipeline
{
public:
  static vtkUpdateSuppressorPipeline* New();
  vtkTypeMacro(vtkUpdateSuppressorPipeline, vtkCompositeDataPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Generalized interface for asking the executive to fulfill update
   * requests.
   */
  int ProcessRequest(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  //@{
  /**
   * Get/Set if the update suppressions are enabled.
   * Enabled by default.
   */
  void SetEnabled(bool e) { this->Enabled = e; }
  vtkGetMacro(Enabled, bool);

protected:
  vtkUpdateSuppressorPipeline();
  ~vtkUpdateSuppressorPipeline() override;
  //@}

  bool Enabled;

private:
  vtkUpdateSuppressorPipeline(const vtkUpdateSuppressorPipeline&) = delete;
  void operator=(const vtkUpdateSuppressorPipeline&) = delete;
};

#endif
