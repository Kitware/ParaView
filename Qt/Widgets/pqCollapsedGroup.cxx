// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqCollapsedGroup.h"

#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionGroupBox>
#include <QStylePainter>

QStyleOptionGroupBox pqCollapsedGroup::pqCollapseGroupGetStyleOption(const pqCollapsedGroup* p)
{
  QStyleOptionGroupBox option;
  p->initStyleOption(&option);
  option.text = p->title();
  option.lineWidth = 1;
  option.midLineWidth = 0;
  option.textAlignment = Qt::AlignLeft;
  option.subControls = QStyle::SC_None;
  if (!p->collapsed())
  {
    option.subControls = QStyle::SC_GroupBoxFrame;
  }
  if (!p->title().isEmpty())
  {
    option.subControls |= QStyle::SC_GroupBoxLabel;
  }
  return option;
}

pqCollapsedGroup::pqCollapsedGroup(QWidget* p)
  : QGroupBox(p)
  , Collapsed(false)
  , Pressed(false)
{
}

QRect pqCollapsedGroup::textRect()
{
  QStyleOptionGroupBox option = pqCollapseGroupGetStyleOption(this);
  option.subControls |= QStyle::SC_GroupBoxCheckBox;
  return this->style()->subControlRect(
    QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxLabel, this);
}

QRect pqCollapsedGroup::collapseRect()
{
  QStyleOptionGroupBox option = pqCollapseGroupGetStyleOption(this);
  option.subControls |= QStyle::SC_GroupBoxCheckBox;
  return this->style()->subControlRect(
    QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxCheckBox, this);
  /*
    QRect r = this->style()->subControlRect(QStyle::CC_GroupBox, &option,
                                            QStyle::SC_GroupBoxCheckBox, this);
    r.moveRight(this->width() - r.left());
    return r;
  */
}

void pqCollapsedGroup::paintEvent(QPaintEvent*)
{
  QStylePainter painter(this);
  QStyle* mystyle = this->style();
  QStyleOptionGroupBox option = pqCollapseGroupGetStyleOption(this);

  QRect tRect = this->textRect();
  QRect cRect = this->collapseRect();

  // Draw frame
  if (option.subControls & QStyle::SC_GroupBoxFrame)
  {
    QStyleOptionFrame frame;
    frame.QStyleOption::operator=(option);
    frame.features = option.features;
    frame.lineWidth = option.lineWidth;
    frame.midLineWidth = option.midLineWidth;
    frame.rect =
      mystyle->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxFrame, this);
    painter.save();
    QRegion region(option.rect);
    if (!option.text.isEmpty())
    {
      region -= tRect;
    }
    region -= cRect;
    painter.setClipRegion(region);
    mystyle->drawPrimitive(QStyle::PE_FrameGroupBox, &frame, &painter, this);
    painter.restore();
  }

  // Draw title
  if ((option.subControls & QStyle::SC_GroupBoxLabel) && !option.text.isEmpty())
  {
    if (!option.text.isEmpty())
    {
      QColor textColor = option.textColor;
      if (textColor.isValid())
        painter.setPen(textColor);
      int align = int(option.textAlignment);
      if (!mystyle->styleHint(QStyle::SH_UnderlineShortcut, &option, this))
        align |= Qt::TextHideMnemonic;

      mystyle->drawItemText(&painter, tRect, Qt::TextShowMnemonic | Qt::AlignHCenter | align,
        option.palette, option.state & QStyle::State_Enabled, option.text,
        textColor.isValid() ? QPalette::NoRole : QPalette::WindowText);

      if (option.state & QStyle::State_HasFocus)
      {
        QStyleOptionFocusRect fropt;
        fropt.QStyleOption::operator=(option);
        fropt.rect = tRect;
        mystyle->drawPrimitive(QStyle::PE_FrameFocusRect, &fropt, &painter, this);
      }
    }
  }

  // Draw indicator
  QStyleOption indicatorOption;
  indicatorOption.rect = cRect;
  indicatorOption.state = QStyle::State_Children;
  if (!this->collapsed())
  {
    indicatorOption.state |= QStyle::State_Open;
  }
  painter.drawPrimitive(QStyle::PE_IndicatorBranch, indicatorOption);
}

QSize pqCollapsedGroup::minimumSizeHint() const
{
  QStyleOptionGroupBox option = pqCollapseGroupGetStyleOption(this);

  int baseWidth = fontMetrics().horizontalAdvance(this->title() + QLatin1Char(' '));
  int baseHeight = fontMetrics().height();

  baseWidth += this->style()->pixelMetric(QStyle::PM_IndicatorWidth);
  baseHeight = qMax(baseHeight, this->style()->pixelMetric(QStyle::PM_IndicatorHeight));

  QSize sz(baseWidth, baseHeight);

  if (this->Collapsed)
  {
    return sz;
  }
  sz = QWidget::minimumSizeHint().expandedTo(sz);
  return this->style()->sizeFromContents(QStyle::CT_GroupBox, &option, sz, this);
}

void pqCollapsedGroup::mousePressEvent(QMouseEvent* e)
{
  QRect cRect = this->collapseRect();
  this->Pressed = cRect.contains(e->pos());
}

void pqCollapsedGroup::mouseMoveEvent(QMouseEvent* e)
{
  if (this->Pressed == true)
  {
    QRect cRect = this->collapseRect();
    this->Pressed = cRect.contains(e->pos());
  }
}

void pqCollapsedGroup::mouseReleaseEvent(QMouseEvent* e)
{
  if (this->Pressed)
  {
    QRect cRect = this->collapseRect();
    this->Pressed = cRect.contains(e->pos());
  }

  if (this->Pressed)
  {
    this->setCollapsed(!this->collapsed());
  }
}

void pqCollapsedGroup::childEvent(QChildEvent* c)
{
  if ((c->type() == QEvent::ChildAdded) && c->child()->isWidgetType())
  {
    QWidget* w = (QWidget*)c->child();
    if (!this->Collapsed)
    {
      if (!w->testAttribute(Qt::WA_ForceDisabled))
      {
        w->setEnabled(true);
      }
    }
    else
    {
      if (w->isEnabled())
      {
        w->setEnabled(false);
        w->setAttribute(Qt::WA_ForceDisabled, false);
      }
    }
  }
  this->QGroupBox::childEvent(c);
}

bool pqCollapsedGroup::collapsed() const
{
  return this->Collapsed;
}

void pqCollapsedGroup::setCollapsed(bool v)
{
  if (v == this->Collapsed)
  {
    return;
  }

  this->Collapsed = v;
  QSize sz = this->minimumSizeHint();

  if (this->Collapsed)
  {
    this->setChildrenEnabled(false);
    this->setMinimumHeight(sz.height());
    this->setMaximumHeight(sz.height());
  }
  else
  {
    this->setChildrenEnabled(true);
    this->setMinimumHeight(sz.height());
    this->setMaximumHeight(QWIDGETSIZE_MAX);
  }
  this->updateGeometry();
  this->update();
}

void pqCollapsedGroup::setChildrenEnabled(bool enable)
{
  QObjectList childList = this->children();
  for (int i = 0; i < childList.size(); i++)
  {
    QObject* o = childList.at(i);
    if (o->isWidgetType())
    {
      QWidget* w = static_cast<QWidget*>(o);
      if (enable)
      {
        if (!w->testAttribute(Qt::WA_ForceDisabled))
        {
          w->setEnabled(true);
        }
      }
      else
      {
        if (w->isEnabled())
        {
          w->setEnabled(false);
          w->setAttribute(Qt::WA_ForceDisabled, false);
        }
      }
    }
  }
}
