/*=========================================================================

   Program: ParaView
   Module:  pqIntMaskPropertyWidget.h

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
#ifndef pqIntMaskPropertyWidget_h
#define pqIntMaskPropertyWidget_h

#include "pqComponentsModule.h"
#include "pqPropertyWidget.h"
#include <QScopedPointer>

/**
* pqIntMaskPropertyWidget is designed to be used for an IntVectorProperty
* that can have a value that is set by or-ing together flags. The user is
* presented with a button with menu from which they can select multiple items.
* The resulting property value is determined by or-ing together the flag
* values for each of the checked item.
*
* The flag/mask labels and their values are specified as hints on the XML
* property. The following XML snippet demonstrates how to use this property
* widget.
* @code
*   <IntVectorProperty name="FacesToRender"
*                      command="SetFaceMask"
*                      number_of_elements="1"
*                      default_values="63"
*                      panel_widget="int_mask">
*     <IntRangeDomain name="range" min="0" />
*     <Hints>
*       <Mask>
*         <Item name="Min-YZ" value="1" />
*         <Item name="Min-ZX" value="2" />
*         <Item name="Min-XY" value="4" />
*         <Item name="Max-YZ" value="8" />
*         <Item name="Max-ZX" value="16" />
*         <Item name="Max-XY" value="32" />
*       </Mask>
*     </Hints>
*   </IntVectorProperty>
* @endcode
*/
class PQCOMPONENTS_EXPORT pqIntMaskPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(int mask READ mask WRITE setMask NOTIFY maskChanged);

public:
  pqIntMaskPropertyWidget(vtkSMProxy* proxy, vtkSMProperty* smproperty, QWidget* parent = 0);
  ~pqIntMaskPropertyWidget() override;

  /**
  * Returns the current mask value.
  */
  int mask() const;

public Q_SLOTS:
  /**
  * Set the mask.
  */
  void setMask(int mask);

Q_SIGNALS:
  /**
  * Fired whenever the user changes the selection.
  */
  void maskChanged();

private:
  Q_DISABLE_COPY(pqIntMaskPropertyWidget)

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
