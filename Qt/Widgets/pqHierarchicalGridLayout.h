/*=========================================================================

   Program: ParaView
   Module:  pqHierarchicalGridLayout.h

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
#ifndef pqHierarchicalGridLayout_h
#define pqHierarchicalGridLayout_h

#include <QLayout>
#include <QScopedPointer> // for ivar

#include "pqWidgetsModule.h" // for export macros

/**
 * @class pqHierarchicalGridLayout
 * @brief QLayout specialization that supports a hierarchical layout.
 *
 * pqHierarchicalGridLayout supports a layout where the widgets added to the
 * layout can be treated as laid out in a binary tree with each non-leaf node
 * splitting the viewport horizontally or vertically.
 *
 * If one adds widgets to the layout using QLayout API e.g. `addWidget` then the
 * layout will automatically build the binary tree by adding splits in the
 * layout as needed.
 *
 * Another way to explicitly provide an arrangement of widgets is to use
 * `rearrange` to add/remove widgets from the layout and place them in
 * particular fashion.
 *
 * If pqHierarchicalGridLayout is added to a pqHierarchicalGridWidget then it is
 * possible to allow the user to interactively resize individual widgets.
 *
 * @note Caveat
 *
 * pqHierarchicalGridLayout currently does not support adding non QWidget items
 * to the layout e.g. another QLayout or QSpacerItem etc. Support for these can
 * be added in future when needed.
 *
 */
class PQWIDGETS_EXPORT pqHierarchicalGridLayout : public QLayout
{
  Q_OBJECT
  typedef QLayout Superclass;

public:
  pqHierarchicalGridLayout(QWidget* parent = 0);
  ~pqHierarchicalGridLayout() override;

  //@{
  /**
   * pure virtual methods from QLayout
   */
  void addItem(QLayoutItem* item) override;
  QLayoutItem* itemAt(int index) const override;
  QLayoutItem* takeAt(int index) override;
  int count() const override;
  //@}

  //@{
  /**
   * recommended overrides from QLayout
   */
  QSize minimumSize() const override;
  void setGeometry(const QRect& rect) override;
  QSize sizeHint() const override;
  //@}

  //@{
  /**
   * Returns true if the location points to a valid reachable location.
   */
  bool isLocationValid(int location) const;
  //@}

  //@{
  /**
   * Adds a splitter item. \c location specified must be valid and reachable.
   */
  void split(int location, Qt::Orientation direction, double splitFraction);
  void splitAny(int location, double splitFraction);
  void splitVertical(int location, double splitFraction)
  {
    this->split(location, Qt::Vertical, splitFraction);
  }
  void splitHorizontal(int location, double splitFraction)
  {
    this->split(location, Qt::Horizontal, splitFraction);
  }
  //@}

  /**
   * Specify the split fraction for a location. \c location must be valid and
   * reachable.
   */
  void setSplitFraction(int location, double splitFraction);
  //@}

  //@{
  /**
   * Maximize any cell location. Set to 0 to return to default un-maximized state.
   * Invalid location i.e. negative or referring to a non-existent or
   * unreachable cell will be ignored with a warning message.
   */
  void maximize(int location);
  //@}

  class Item
  {
  public:
    Item(Qt::Orientation direction, double fraction)
      : Direction(direction)
      , Fraction(fraction)
    {
    }
    Item(QWidget* widget = nullptr)
      : Widget(widget)
    {
    }

  private:
    QWidget* Widget = nullptr;
    Qt::Orientation Direction = Qt::Horizontal;
    double Fraction = -1.0;
    friend class pqHierarchicalGridLayout;
  };

  /**
   * API to rearrange the widgets in this layout. The argument is simply a sequential representation
   * for a binary tree were for a split node at index `i`, the first child node is at index `2*i +
   * 1` and the second child node is at index `2*i + 2`.
   *
   * Any widgets previously added to the layout no longer present in the arrangement are removed
   * from the layout and returned.
   *
   * Any widgets not already present in the layout are added to the layout using `addWidget`.
   *
   */
  QVector<QWidget*> rearrange(const QVector<Item>& layout);

private:
  Q_DISABLE_COPY(pqHierarchicalGridLayout);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
