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
#ifndef __pqStructuredInputDecorator_h
#define __pqStructuredInputDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidgetDecorator.h"
#include "vtkWeakPointer.h"

class vtkObject;

/// pqStructuredInputDecorator is a pqPropertyWidgetDecorator subclass.
/// For certain properties, they should update the enable state based on
/// input data types. This decorator is used to enable a property widget
/// only when an input data type is a StructuredData. Please see
/// vtkPVDataInformation::IsDataStructured() for structured types.
/// For example, "Computer Gradients" in Contour filter.
class PQAPPLICATIONCOMPONENTS_EXPORT pqStructuredInputDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;
public:
  pqStructuredInputDecorator(vtkPVXMLElement* config, pqPropertyWidget* parent);
  virtual ~pqStructuredInputDecorator();

  /// Overridden to enable/disable the widget based on input data type.
  virtual bool enableWidget() const;

private:
  Q_DISABLE_COPY(pqStructuredInputDecorator)

  vtkWeakPointer<vtkObject> ObservedObject;
  unsigned long ObserverId;
};

#endif
