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

#include <utility> // for std::pair
#include <vector>  // for std::vector

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
  Q_PROPERTY(bool showCustomIndicator READ isCustomIndicatorShown WRITE setCustomIndicatorShown);

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

  //@{
  /**
   * This property holds if a custom indicator is shown for sections. The custom
   * indicator can be used to popup a custom menu or filtering options, for
   * example. If enabled, all visible sections with non-empty header data get a
   * custom indicator rendered.
   *
   * By default, this property is `false`.
   */
  bool isCustomIndicatorShown() const { return this->CustomIndicatorShown; }
  void setCustomIndicatorShown(bool val);
  //@}

  //@{
  /**
   * When `showCustomIndicator` property is true, these are the icons
   * rendered. Icons are rendered in order from right to left i.e. first added
   * icon on the rightmost side and so on. The `role` is useful in
   * distinguishing user clicks when the `customIndicatorClicked` signal is
   * fired.
   */
  void addCustomIndicatorIcon(const QIcon& icon, const QString& role);
  void removeCustomIndicatorIcon(const QString& role);
  QIcon customIndicatorIcon(const QString& role) const;
  //@}

  /**
   * For testing purposes only. Returns the position of the painted checkbox in
   * the most recent paint call. Note, currently we only support at most one
   * checkable section. We will need to change this API if we start
   * supporting multiple checkable sections.
   */
  QRect lastCheckRect() const { return this->CheckRect; }

Q_SIGNALS:
  void customIndicatorClicked(int section, const QPoint& pt, const QString& role);

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

  bool CustomIndicatorShown;

  // we keep as a vector since order of addition of icons is important.
  std::vector<std::pair<QIcon, QString> > CustomIndicatorIcons;
  mutable std::vector<std::pair<QRect, QString> > CustomIndicatorRects;
};

#endif
