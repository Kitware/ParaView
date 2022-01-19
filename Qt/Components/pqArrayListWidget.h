/*=========================================================================

   Program: ParaView
   Module:  pqArrayListWidget.cxx

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
#ifndef pqArrayListWidget_h
#define pqArrayListWidget_h

#include "pqComponentsModule.h"

#include <QWidget>

class pqArrayListModel;
class pqExpandableTableView;

/**
 * @class pqArrayListWidget
 * @brief A widget for array labeling.
 *
 * pqArrayListWidget is intended to be used with 2-elements string properties.
 * It creates a two-columns string table.
 * First column is readonly, the second is editable.
 *
 * It is useful to associate an editable label to each array,
 * for array renaming for instance.
 */
class PQCOMPONENTS_EXPORT pqArrayListWidget : public QWidget
{
  Q_OBJECT;
  typedef QWidget Superclass;

public:
  pqArrayListWidget(QWidget* parent = nullptr);
  ~pqArrayListWidget() override = default;

  /**
   * overridden to handle QDynamicPropertyChangeEvent events.
   */
  bool event(QEvent* e) override;

  /**
   * Set table header label. It should match the Property name.
   */
  void setHeaderLabel(const QString& label);

  /**
   * Set the maximum number of visible rows. Beyond this threshold,
   * a scrollbar is added.
   * Forwarded to the inner pqTableView.
   */
  void setMaximumRowCountBeforeScrolling(int size);

  /**
   * Set the icon type corresponding to the arrays.
   */
  void setIconType(const QString& icon_type);

Q_SIGNALS:
  /**
   * fired whenever the state has been modified.
   */
  void widgetModified();

protected:
  /**
   * called in `event()` to handle change in a dynamic property with the given name.
   */
  void propertyChanged(const QString& pname);

protected Q_SLOTS:
  void updateProperty();

private:
  Q_DISABLE_COPY(pqArrayListWidget);

  pqArrayListModel* Model = nullptr;
  pqExpandableTableView* TableView = nullptr;

  bool UpdatingProperty = false;
};

#endif
