/*=========================================================================

   Program: ParaView
   Module:  pqGenericPropertyWidgetDecorator.h

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
#ifndef pqGenericPropertyWidgetDecorator_h
#define pqGenericPropertyWidgetDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidgetDecorator.h"

#include <QScopedPointer>

/**
* pqGenericPropertyWidgetDecorator is a pqPropertyWidgetDecorator that
* supports multiple common use cases from a pqPropertyWidgetDecorator.
* The use cases supported are as follows:
* \li 1. enabling the pqPropertyWidget when the value of another
*   property element matches a specific value (disabling otherwise).
* \li 2. similar to 1, except instead of enabling/disabling the widget is made
*   "default" when the values match and "advanced" otherwise.
* \li 3. enabling the pqPropertyWidget when the array named in the property
*   has a specified number of components.
* \li 4. as well as "inverse" of all the above i.e. when the value doesn't
*   match the specified value.
* Example usages:
* \li VectorScaleMode, Stride, Seed, MaximumNumberOfSamplePoints properties on the Glyph proxy.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqGenericPropertyWidgetDecorator
  : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqGenericPropertyWidgetDecorator(vtkPVXMLElement* config, pqPropertyWidget* parent);
  ~pqGenericPropertyWidgetDecorator() override;

  /**
  * Methods overridden from pqPropertyWidget.
  */
  bool canShowWidget(bool show_advanced) const override;
  bool enableWidget() const override;

private slots:
  void updateState();

private:
  Q_DISABLE_COPY(pqGenericPropertyWidgetDecorator)

  class pqInternals;
  const QScopedPointer<pqInternals> Internals;
};

#endif
