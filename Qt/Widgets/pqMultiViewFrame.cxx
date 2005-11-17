
#include "pqMultiViewFrame.h"

#include <QStyle>
#include <QPainter>
#include <QPen>
#include <QVBoxLayout>

static int gPenWidth = 2;

pqMultiViewFrame::pqMultiViewFrame(QWidget* parent)
  : QWidget(parent), MainWidget(0), AutoHide(false), Active(false), Color(QColor("red"))
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(gPenWidth);
  layout->setSpacing(gPenWidth);

  this->Menu = new QWidget(this);
  this->MenuUi.setupUi(Menu);
  layout->addWidget(this->Menu);

  QVBoxLayout* sublayout = new QVBoxLayout();
  layout->addLayout(sublayout);
  sublayout->addStretch();

  this->MenuUi.Close->setIcon(QIcon(this->style()->standardPixmap(QStyle::SP_TitleBarCloseButton)));
  this->MenuUi.Maximize->setIcon(QIcon(this->style()->standardPixmap(QStyle::SP_TitleBarMaxButton)));

  // queued connections because these signals can potentially modify/delete this object
  connect(this->MenuUi.Active, SIGNAL(toggled(bool)), SLOT(setActive(bool)), Qt::QueuedConnection);
  connect(this->MenuUi.Close, SIGNAL(clicked(bool)), SLOT(close()), Qt::QueuedConnection);
  connect(this->MenuUi.Maximize, SIGNAL(clicked(bool)), SLOT(maximize()), Qt::QueuedConnection);
  connect(this->MenuUi.SplitVertical, SIGNAL(clicked(bool)), SLOT(splitVertical()), Qt::QueuedConnection);
  connect(this->MenuUi.SplitHorizontal, SIGNAL(clicked(bool)), SLOT(splitHorizontal()), Qt::QueuedConnection);
}

pqMultiViewFrame::~pqMultiViewFrame()
{
}

bool pqMultiViewFrame::autoHide() const
{
  return this->AutoHide;
}

void pqMultiViewFrame::setAutoHide(bool autohide)
{
  this->AutoHide = autohide;
}

bool pqMultiViewFrame::active() const
{
  return this->Active;
}

void pqMultiViewFrame::setActive(bool a)
{
  if(this->MenuUi.Active->isChecked() != a)
    {
    this->MenuUi.Active->blockSignals(true);
    this->MenuUi.Active->setChecked(a);
    this->MenuUi.Active->blockSignals(false);
    }

  if(this->Active != a)
    {
    this->Active = a;
    emit this->activeChanged(a);
    this->update();
    }
}

QColor pqMultiViewFrame::color() const
{
  return this->Color;
}

void pqMultiViewFrame::setColor(QColor c)
{
  this->Color = c;
}

void pqMultiViewFrame::setMainWidget(QWidget* w)
{
  QLayout* l = this->layout()->itemAt(1)->layout();
  l->removeItem(l->itemAt(0));
  if(w)
    l->addWidget(w);
  else
    static_cast<QBoxLayout*>(l)->addStretch();
}

QWidget* pqMultiViewFrame::mainWidget()
{
  return this->layout()->itemAt(1)->layout()->itemAt(0)->widget();
}

void pqMultiViewFrame::paintEvent(QPaintEvent* e)
{
  QWidget::paintEvent(e);
  if(this->Active)
    {
    QPainter painter(this);
    QPen pen;
    pen.setColor(this->Color);
    pen.setWidth(gPenWidth);
    painter.setPen(pen);
    QLayoutItem* i = this->layout()->itemAt(0);
    QRect r = contentsRect();
    r.adjust(-gPenWidth/2+2, i->geometry().height()+4-gPenWidth/2, gPenWidth/2-2, gPenWidth/2-2);
    painter.drawRect(r);
    }
}

void pqMultiViewFrame::close()
{
  emit this->closePressed();
}

void pqMultiViewFrame::maximize()
{
  emit this->maximizePressed();
}

void pqMultiViewFrame::splitVertical()
{
  emit this->splitVerticalPressed();
}

void pqMultiViewFrame::splitHorizontal()
{
  emit this->splitHorizontalPressed();
}

