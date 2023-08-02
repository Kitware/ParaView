// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  pqIntMaskPropertyWidget(vtkSMProxy* proxy, vtkSMProperty* smproperty, QWidget* parent = nullptr);
  ~pqIntMaskPropertyWidget() override;

  /**
   * Returns the current mask value.
   */
  int mask() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
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
