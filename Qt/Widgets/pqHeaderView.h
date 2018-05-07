/*=========================================================================

   Program: ParaView
   Module:  pqHeaderView.h

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
#ifndef pqHeaderView_h
#define pqHeaderView_h

#include "pqWidgetsModule.h" // for exports
#include <QHeaderView>
#include <QPoint> // for QPoint
#include <QRect>  // for QRect

/**
 * @class pqHeaderView
 * @brief pqHeaderView extends QHeaderView to add support for
 *        Qt::CheckStateRole.
 *
 * pqHeaderView extends QHeaderView to add support for getting and setting
 * Qt::CheckStateRole. The QAbstractItemModel connected to the view must support
 * getting/setting check state by providing appropriate implementations
 * for `QAbstractItemModel::setHeaderData` and `QAbstractItemModel::headerData`.
 *
 * This class is a reimplementation of pqCheckableHeaderView with improved
 * rendering as well as letting the model handle getting and setting of header
 * check state via the header-data API.
 *
 * Currently, pqHeaderView is only intended to be used for horizontal headers.
 * However, it should be fairly straight forward to support vertical headers, if
 * needed.
 */
class PQWIDGETS_EXPORT pqHeaderView : public QHeaderView
{
  Q_OBJECT
  typedef QHeaderView Superclass;

public:
  pqHeaderView(Qt::Orientation orientation, QWidget* parent = nullptr);
  ~pqHeaderView() override;

  //@{
  /**
   * For checkable section, this property controls if clicks on the entire
   * section should be treated as a request to toggle the check state or only
   * limit the click to the checkbox. Default is to limit to the checkbox, set
   * to true if you want the entire section respond to clicks as toggling the
   * check state.
   */
  void setToggleCheckStateOnSectionClick(bool val) { this->ToggleCheckStateOnSectionClick = val; }
  bool toggleCheckStateOnSectionClick() const { return this->ToggleCheckStateOnSectionClick; }

  //@}
protected:
  void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

private:
  Q_DISABLE_COPY(pqHeaderView);

  void mouseClickEvent(QMouseEvent* event);

  bool ToggleCheckStateOnSectionClick;
  QPoint PressPosition;
  mutable QRect CheckRect;
};

#endif
