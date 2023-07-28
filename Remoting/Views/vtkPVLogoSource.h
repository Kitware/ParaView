// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVLogoSource
 * @brief   source that generates a 1x1 vtkTable with a single
 * string data.
 *
 * vtkPVLogoSource is used to generate an image from a provided texture
 * Usually used with the logo representation.
 */

#ifndef vtkPVLogoSource_h
#define vtkPVLogoSource_h

#include "vtkImageAlgorithm.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class vtkTexture;
class VTKREMOTINGVIEWS_EXPORT vtkPVLogoSource : public vtkImageAlgorithm
{
public:
  static vtkPVLogoSource* New();
  vtkTypeMacro(vtkPVLogoSource, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the texture to generate an image from
   */
  void SetTexture(vtkTexture* texture);

protected:
  vtkPVLogoSource();
  ~vtkPVLogoSource() override;

  void ExecuteDataWithInformation(vtkDataObject* data, vtkInformation* outInfo) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkPVLogoSource(const vtkPVLogoSource&) = delete;
  void operator=(const vtkPVLogoSource&) = delete;

  vtkTexture* Texture = nullptr;
};

#endif
