/*=========================================================================

   Program: ParaView
   Module:  pqDisplayColor2Widget.h

   Copyright (c) 2005-2022 Sandia Corporation, Kitware Inc.
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
#ifndef pqDisplayColor2Widget_h
#define pqDisplayColor2Widget_h

#include "pqComponentsModule.h"

#include <QWidget>

#include <memory>

class pqDataRepresentation;

/**
 * pqDisplayColor2Widget enables a user to select the array corresponding to the y-axis
 * of a 2D transfer function. This feature is available when the representation
 * is rendering volumes.
 *
 * This widget uses a `pqArraySelectorWidget` to present the available arrays in a combobox and
 * a pqIntVectorPropertyWidget enumerates the components of the selected array in another combobox.
 * Both the widgets are laid out horizontally.
 *
 * The first array in this widget is "Gradient Magnitude". This entry corresponds the
 * "UseGradientForTransfer2D" property on the volume representation.
 * As a result, this entry will always be present and it is the default.
 */
class PQCOMPONENTS_EXPORT pqDisplayColor2Widget : public QWidget
{
  Q_OBJECT
  using Superclass = QWidget;

public:
  pqDisplayColor2Widget(QWidget* parent = nullptr);
  ~pqDisplayColor2Widget() override;

  /**
   * Set the representation on which the scalar Color2 array will be set.
   */
  void setRepresentation(pqDataRepresentation* display);

private:
  Q_DISABLE_COPY(pqDisplayColor2Widget);

  void onArrayModified();

  class pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
