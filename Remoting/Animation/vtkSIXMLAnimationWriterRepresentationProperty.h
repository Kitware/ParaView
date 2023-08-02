// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSIXMLAnimationWriterRepresentationProperty
 *
 * vtkSIXMLAnimationWriterRepresentationProperty extends vtkSIInputProperty to
 * add push-API specific to vtkXMLPVAnimationWriter to add representations while
 * assigning them unique names consistently across all processes.
 */

#ifndef vtkSIXMLAnimationWriterRepresentationProperty_h
#define vtkSIXMLAnimationWriterRepresentationProperty_h

#include "vtkRemotingAnimationModule.h" //needed for exports
#include "vtkSIInputProperty.h"

class VTKREMOTINGANIMATION_EXPORT vtkSIXMLAnimationWriterRepresentationProperty
  : public vtkSIInputProperty
{
public:
  static vtkSIXMLAnimationWriterRepresentationProperty* New();
  vtkTypeMacro(vtkSIXMLAnimationWriterRepresentationProperty, vtkSIInputProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSIXMLAnimationWriterRepresentationProperty();
  ~vtkSIXMLAnimationWriterRepresentationProperty() override;

  /**
   * Overridden to call AddRepresentation on the vtkXMLPVAnimationWriter
   * instance with correct API.
   */
  bool Push(vtkSMMessage*, int) override;

private:
  vtkSIXMLAnimationWriterRepresentationProperty(
    const vtkSIXMLAnimationWriterRepresentationProperty&) = delete;
  void operator=(const vtkSIXMLAnimationWriterRepresentationProperty&) = delete;
};

#endif
