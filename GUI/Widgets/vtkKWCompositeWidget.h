/*=========================================================================

  Module:    vtkKWCompositeWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWCompositeWidget - a composite widget.
// .SECTION Description
// A superclass for all composite widgets, i.e. widgets made of
// an assembly of sub-widgets.
// This superclass provides the container for the sub-widgets.
// Right now, it can be safely assumed to be a frame (similar to a
// vtkKWFrame).
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWCompositeWidget_h
#define __vtkKWCompositeWidget_h

#include "vtkKWFrame.h"

class KWWidgets_EXPORT vtkKWCompositeWidget : public vtkKWFrame
{
public:
  static vtkKWCompositeWidget* New();
  vtkTypeRevisionMacro(vtkKWCompositeWidget, vtkKWFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkKWCompositeWidget() {};
  ~vtkKWCompositeWidget() {};

  // Description:
  // Create the widget.
  virtual void CreateWidget();

private:
  vtkKWCompositeWidget(const vtkKWCompositeWidget&); // Not implemented
  void operator=(const vtkKWCompositeWidget&); // Not implemented
};


#endif



