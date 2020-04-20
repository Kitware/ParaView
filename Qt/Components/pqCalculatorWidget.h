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
#ifndef pqCalculatorWidget_h
#define pqCalculatorWidget_h

#include "pqPropertyWidget.h"

/**
* pqCalculatorWidget is a property-widget that can shows a calculator-like
* UI for the property. It is designed to be used with vtkPVArrayCalculator (or
* similar) filters. It allows users to enter expressions to compute derived
* quantities. To determine the list of input arrays available.
*
* CAVEATS: Currently, this widget expects two additional properties on the
* proxy: "Input", that provides the input and "AttributeMode", which
* corresponds to the chosen attribute type. This code can be revised to use
* RequiredProperties on the domain to make it reuseable, if needed.
*/
class PQCOMPONENTS_EXPORT pqCalculatorWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqCalculatorWidget(vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = 0);
  ~pqCalculatorWidget() override;

protected Q_SLOTS:
  /**
  * called when the user selects a variable from the scalars/vectors menus.
  */
  void variableChosen(QAction* action);

  /**
  * called when user clicks one of the function buttons
  */
  void buttonPressed(const QString&);

  /**
  * updates the variables in the menus.
  */
  void updateVariableNames();
  void updateVariables(const QString& mode);

private:
  Q_DISABLE_COPY(pqCalculatorWidget)

  class pqInternals;
  pqInternals* Internals;
};

#endif
