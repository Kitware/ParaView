
#include "pqAnimationWidget.h"

#include <QResizeEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QHeaderView>

#include "pqAnimationModel.h"

pqAnimationWidget::pqAnimationWidget(QWidget* p) 
  : QWidget(p) 
{
  QHBoxLayout* wlayout = new QHBoxLayout(this);
  QFrame* frame = new QFrame(this);
  frame->setFrameShape(QFrame::StyledPanel);
  frame->setFrameShadow(QFrame::Sunken);
  wlayout->addWidget(frame);
  QHBoxLayout* l = new QHBoxLayout(frame);
  this->View = new QGraphicsView(frame);
  this->View->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  this->View->setFrameShape(QFrame::NoFrame);
  this->Model = new pqAnimationModel(this->View);
  this->View->setScene(this->Model);
  QHeaderView* header = new QHeaderView(Qt::Vertical, frame);
  header->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
  header->setResizeMode(QHeaderView::Fixed);
  header->setDefaultSectionSize(this->Model->rowHeight());
  header->setModel(this->Model->header());
  l->addWidget(header);
  l->addWidget(this->View);
  l->setMargin(0);
  l->setSpacing(0);
}

pqAnimationWidget::~pqAnimationWidget()
{
}

pqAnimationModel* pqAnimationWidget::animationModel() const
{
  return this->Model;
}

