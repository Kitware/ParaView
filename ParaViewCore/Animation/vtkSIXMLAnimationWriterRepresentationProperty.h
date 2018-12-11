/*=========================================================================

  Program:   ParaView
  Module:    vtkSIXMLAnimationWriterRepresentationProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSIXMLAnimationWriterRepresentationProperty
 *
 * vtkSIXMLAnimationWriterRepresentationProperty extends vtkSIInputProperty to
 * add push-API specific to vtkXMLPVAnimationWriter to add representations while
 * assigning them unique names consistently across all processes.
*/

#ifndef vtkSIXMLAnimationWriterRepresentationProperty_h
#define vtkSIXMLAnimationWriterRepresentationProperty_h

#include "vtkPVAnimationModule.h" //needed for exports
#include "vtkSIInputProperty.h"

class VTKPVANIMATION_EXPORT vtkSIXMLAnimationWriterRepresentationProperty
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
