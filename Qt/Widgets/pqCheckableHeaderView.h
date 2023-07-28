// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCheckableHeaderView_h
#define pqCheckableHeaderView_h

#include "pqWidgetsModule.h"
#include <QHeaderView>

/**
 * Pimpl class that handles drawing of all the section header checkboxes and
 * stores states of the checkboxes
 */
class pqCheckableHeaderViewInternal;

/**
 * @class pqCheckableHeaderView
 * @brief A convenience QHeaderView painted with a QCheckBox.
 *
 * This allows for providing a global checkbox when the model items are
 * user checkable.  The checkbox is painted per section with one of the
 * three states (checked, partially checked, unchecked) depending on
 * the check state of individual items. Currently used in pqTreeView.
 *
 * pqCheckableHeaderView is flagged to be investigated for deprecation.
 * pqHeaderView provides a much simpler and more model-view friendly
 * implementation that relies on the QAbstractItemModel simply respecting
 * Qt::CheckStateRole for the header. It is recommended that you update your code
 * to use pqHeaderView instead.
 */
class PQWIDGETS_EXPORT pqCheckableHeaderView : public QHeaderView
{
  Q_OBJECT

public:
  /**
   * Constructs the pqCheckableHeaderView class.
   *
   * @param[in] orientation Orientation for the header.
   * @param[in] parent The parent object
   */
  pqCheckableHeaderView(Qt::Orientation orientation, QWidget* parent = nullptr);
  ~pqCheckableHeaderView() override;

  /**
   * \brief
   *   Get the checkstate of the header checkbox for the \em section.
   * \param section The section to get the checkstate of
   */
  QVariant getCheckState(int section);

Q_SIGNALS:
  /**
   * \brief
   *   This signal is emitted whenever the state of the \em section header
   *   checkbox changes. The new state can be obtained using the \em
   *   getCheckState method.
   * \param section The section whose checkstate changed
   * \sa getCheckState
   */
  void checkStateChanged(int section) const;

protected:
  /**
   * \brief
   *   Paint the header section.
   *   Depending on whether the top-level items in the model are checkable
   *   a checkbox is painted left-aligned on the header section.
   *   The checkbox is tristate and the state is decided based on the
   *   initial checkstates of model items.
   *   Reimplemented form QHeaderView::paintSection()
   * \sa QHeaderView::paintSection
   */
  void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override;

  /**
   * \brief
   *   Handle mouse press event on the header.
   *   Checks whether the mouse press was in the checkbox.
   *   Clicking on the checkbox triggers the \em checkStateChanged signal.
   *   Reimplemented from QWidget::mousePressEvent()
   * \sa QHeaderView::mousePressEvent
   */
  void mousePressEvent(QMouseEvent* event) override;

  /**
   * \brief
   *   Update the checkstate of all checkable items in the model based
   *   on the checkstate of the header checkbox.
   *   This will undo any individual item checkstate modifications.
   * \param section The section whose model checkstate is updated.
   */
  void updateModelCheckState(int section);

private:
  pqCheckableHeaderViewInternal* Internal;
};

#endif // pqCheckableHeaderView_h
