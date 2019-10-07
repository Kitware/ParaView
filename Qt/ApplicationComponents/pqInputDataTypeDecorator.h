/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef pqInputDataTypeDecorator_h
#define pqInputDataTypeDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidgetDecorator.h"
#include "vtkWeakPointer.h"

class vtkObject;

/**
* pqInputDataTypeDecorator is a pqPropertyWidgetDecorator subclass.
* For certain properties, they should update the enable state
* based on input data types.
* For example, "Computer Gradients" in Contour filter should only
* be enabled when an input data type is a StructuredData. Please see
* vtkPVDataInformation::IsDataStructured() for structured types.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqInputDataTypeDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqInputDataTypeDecorator(vtkPVXMLElement* config, pqPropertyWidget* parent);
  ~pqInputDataTypeDecorator() override;

  /**
  * Overridden to enable/disable the widget based on input data type.
  */
  bool enableWidget() const override;

  /**
   * Overriden to show or not the widget based on input data type.
   */
  bool canShowWidget(bool show_advanced) const override;

protected:
  virtual bool processState() const;

private:
  Q_DISABLE_COPY(pqInputDataTypeDecorator)

  vtkWeakPointer<vtkObject> ObservedObject;
  unsigned long ObserverId;
};

#endif
