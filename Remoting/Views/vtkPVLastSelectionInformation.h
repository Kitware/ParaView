// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVLastSelectionInformation
 *
 * vtkPVLastSelectionInformation is used to obtain the LastSelection from
 * vtkPVRenderView.
 */

#ifndef vtkPVLastSelectionInformation_h
#define vtkPVLastSelectionInformation_h

#include "vtkPVSelectionInformation.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkPVLastSelectionInformation : public vtkPVSelectionInformation
{
public:
  static vtkPVLastSelectionInformation* New();
  vtkTypeMacro(vtkPVLastSelectionInformation, vtkPVSelectionInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void CopyFromObject(vtkObject*) override;

protected:
  vtkPVLastSelectionInformation();
  ~vtkPVLastSelectionInformation() override;

private:
  vtkPVLastSelectionInformation(const vtkPVLastSelectionInformation&) = delete;
  void operator=(const vtkPVLastSelectionInformation&) = delete;
};

#endif
