/*=========================================================================

   Program: ParaView
   Module: pqDoubleVectorPropertyWidget.h

   Copyright (c) 2005-2012 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#ifndef _pqDoubleVectorPropertyWidget_h
#define _pqDoubleVectorPropertyWidget_h

#include "pqPropertyWidget.h"

class PQCOMPONENTS_EXPORT pqDoubleVectorPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqDoubleVectorPropertyWidget(vtkSMProperty* property, vtkSMProxy* proxy, QWidget* parent = 0);

  ~pqDoubleVectorPropertyWidget() override;

  // Overridden to clear highlights from the pqHighlightablePushButton.
  void apply() override;
  void reset() override;

Q_SIGNALS:
  /**
  * internal signal used to clear highlights from pqHighlightablePushButton.
  */
  void clearHighlight();
  void highlightResetButton();

protected Q_SLOTS:
  /**
  * called when the user clicks the "reset" button for a specific property.
  */
  virtual void resetButtonClicked();

  void scaleHalf();
  void scaleTwice();
  void scale(double);

  /**
   * sets the value using active source's data bounds.
   */
  void resetToActiveDataBounds();

  /**
   * sets the value to the specified bounds.
   */
  void resetToBounds(const double bds[6]);

private:
  Q_DISABLE_COPY(pqDoubleVectorPropertyWidget)
};

#endif // _pqDoubleVectorPropertyWidget_h
