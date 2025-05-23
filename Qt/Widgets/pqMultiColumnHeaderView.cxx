// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMultiColumnHeaderView.h"

#include <QAbstractItemModel>
#include <QHeaderView>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqMultiColumnHeaderView::pqMultiColumnHeaderView(Qt::Orientation orientation, QWidget* parentObject)
  : Superclass(orientation, parentObject)
{
  // when a section is resized, for section spanning multiple sections, we
  // need to ensure that we repaint all sections in the span otherwise the text
  // doesn't render correctly on adjacent section not being resized.
  QObject::connect(this, &QHeaderView::sectionResized,
    [this](int logicalIndex, int, int)
    {
      const auto span = this->sectionSpan(this->visualIndex(logicalIndex));
      if (span.first != span.second)
      {
        const auto lspan =
          QPair<int, int>(this->logicalIndex(span.first), this->logicalIndex(span.second));
        int start = this->sectionViewportPosition(lspan.first);
        int end = this->sectionViewportPosition(lspan.second) + this->sectionSize(lspan.second);
        auto viewport = this->viewport();
        if (this->orientation() == Qt::Horizontal && viewport != nullptr)
        {
          viewport->update(start, 0, end, viewport->height());
        }
        else
        {
          viewport->update(0, start, viewport->width(), end);
        }
      }
    });
}

//-----------------------------------------------------------------------------
pqMultiColumnHeaderView::~pqMultiColumnHeaderView() = default;

//-----------------------------------------------------------------------------
QString pqMultiColumnHeaderView::sectionDisplayText(int logicalIndex) const
{
  auto amodel = this->model();
  if (amodel)
  {
    return amodel->headerData(logicalIndex, this->orientation(), Qt::DisplayRole).toString();
  }
  return QString();
}

//-----------------------------------------------------------------------------
QPair<int, int> pqMultiColumnHeaderView::sectionSpan(int visual) const
{
  QPair<int, int> vrange(visual, visual);
  const int logical = this->logicalIndex(visual);
  const auto vlabel = this->sectionDisplayText(logical);
  if (vlabel.isEmpty())
  {
    return vrange;
  }

  for (int cc = vrange.first - 1; cc >= 0; --cc)
  {
    const int clogical = this->logicalIndex(cc);
    if (this->sectionDisplayText(clogical) == vlabel)
    {
      vrange.first = cc;
    }
    else
    {
      break;
    }
  }

  for (int cc = vrange.second + 1, max = this->count(); cc < max; ++cc)
  {
    const int clogical = this->logicalIndex(cc);
    if (this->sectionDisplayText(clogical) == vlabel)
    {
      vrange.second = cc;
    }
    else
    {
      break;
    }
  }
  return vrange;
}

//-----------------------------------------------------------------------------
void pqMultiColumnHeaderView::paintSection(
  QPainter* painter, const QRect& rect, int logicalIndex) const
{
  if (!rect.isValid())
  {
    return;
  }

  const int visual = this->visualIndex(logicalIndex);
  const auto span = this->sectionSpan(visual);
  if (span.first == span.second)
  {
    this->Superclass::paintSection(painter, rect, logicalIndex);
  }
  else
  {
    if (this->isSortIndicatorShown())
    {
      // if sort indicator is shown and the indicator is being shown on one of
      // the sections we have grouped together, we have to handle it specially.
      // In such a case, we still want to paint the sort indicator for the
      // entire section (we're ignoring that it still won't be obvious which
      // component is being sorted by, but that's not a huge UX issue). Do do
      // that, we just paint the section with the sort indicator instead of the
      // one being requested.
      const int vSortIndicatorShown = this->visualIndex(this->sortIndicatorSection());
      if (span.first <= vSortIndicatorShown && span.second >= vSortIndicatorShown)
      {
        logicalIndex = this->sortIndicatorSection();
      }
    }

    QRect newrect(rect);
    for (int cc = span.first; cc < visual; ++cc)
    {
      const int cc_size = this->sectionSize(this->logicalIndex(cc));
      newrect.adjust(-cc_size, 0, 0, 0);
    }

    for (int cc = visual + 1; cc <= span.second; ++cc)
    {
      const int cc_size = this->sectionSize(this->logicalIndex(cc));
      newrect.adjust(0, 0, cc_size, 0);
    }
    this->Superclass::paintSection(painter, newrect, logicalIndex);
  }
}
