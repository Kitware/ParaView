
#include "pqMultiViewManager.h"

#include <QSignalMapper>
#include <QBuffer>
#include <QDataStream>

#include "pqMultiView.h"
#include "pqMultiViewFrame.h"

static int gNameNum = 0;

pqMultiViewManager::pqMultiViewManager(QWidget* parent)
  : pqMultiView(parent)
{
  pqMultiViewFrame* frame = new pqMultiViewFrame(this);
  QString name;
  name.setNum(gNameNum);
  gNameNum++;
  frame->setObjectName(name);
  QWidget* old = this->replaceView(pqMultiView::Index(), frame);
  delete old;
  this->installEventFilter(this);
  this->setup(frame);
}

pqMultiViewManager::~pqMultiViewManager()
{
  // emit signals for removing frames
  QList<pqMultiViewFrame*> frames = this->findChildren<pqMultiViewFrame*>();
  foreach(pqMultiViewFrame* v, frames)
    {
    this->removeWidget(v);
    }
}

bool pqMultiViewManager::eventFilter(QObject*, QEvent* e)
{
  if(e->type() == QEvent::Polish)
    {
    // delay emit of first signal
    emit this->frameAdded(qobject_cast<pqMultiViewFrame*>(
        this->widgetOfIndex(pqMultiView::Index())));
    this->removeEventFilter(this);
    }
  return false;
}

void pqMultiViewManager::removeWidget(QWidget* widget)
{
  this->removeView(widget);
  emit this->frameRemoved(qobject_cast<pqMultiViewFrame*>(widget));
  delete widget;
}

void pqMultiViewManager::splitWidget(QWidget* widget, Qt::Orientation o)
{
  pqMultiView::Index index = this->indexOf(widget);
  pqMultiView::Index newindex = this->splitView(index, o);
  pqMultiViewFrame* frame = new pqMultiViewFrame;
  this->replaceView(newindex, frame);
  QString name;
  name.setNum(gNameNum);
  gNameNum++;
  frame->setObjectName(name);
  this->setup(frame);
  emit this->frameAdded(frame);
}

void pqMultiViewManager::splitWidgetHorizontal(QWidget* widget)
{
  this->splitWidget(widget, Qt::Horizontal);
}

void pqMultiViewManager::splitWidgetVertical(QWidget* widget)
{
  this->splitWidget(widget, Qt::Vertical);
}

void pqMultiViewManager::maximizeWidget(QWidget* widget)
{
  /*
  pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(widget);
  Q_ASSERT(frame != NULL);
  
  QWidget* holder = new QWidget;
  holder->setAttribute(Qt::WA_ShowModal);
  QWidget* w = frame->mainWidget();
  w->setParent(holder);
  QHBoxLayout* l = new QHBoxLayout(holder);
  l->addWidget(w);
  holder->setWindowState(Qt::WindowMaximized);
  holder->showFullScreen();
  */
}

void pqMultiViewManager::restoreWidget(QWidget* widget)
{

}


void pqMultiViewManager::setup(pqMultiViewFrame* frame)
{
  Q_ASSERT(frame != NULL);
  if(!frame)
    return;

  QSignalMapper* CloseSignalMapper = new QSignalMapper(frame);
  QSignalMapper* HorizontalSignalMapper = new QSignalMapper(frame);
  QSignalMapper* VerticalSignalMapper = new QSignalMapper(frame);
  QSignalMapper* MaximizeSignalMapper = new QSignalMapper(frame);

  CloseSignalMapper->setMapping(frame, frame);
  HorizontalSignalMapper->setMapping(frame, frame);
  VerticalSignalMapper->setMapping(frame, frame);
  MaximizeSignalMapper->setMapping(frame, frame);

  // connect close button
  QObject::connect(frame, SIGNAL(closePressed()), 
                   CloseSignalMapper, SLOT(map()));
  QObject::connect(CloseSignalMapper, SIGNAL(mapped(QWidget*)), 
                   this, SLOT(removeWidget(QWidget*)));

  // connect split buttons
  QObject::connect(frame, SIGNAL(splitHorizontalPressed()), 
                   HorizontalSignalMapper, SLOT(map()));
  QObject::connect(HorizontalSignalMapper, SIGNAL(mapped(QWidget*)), 
                   this, SLOT(splitWidgetHorizontal(QWidget*)));
  
  QObject::connect(frame, SIGNAL(splitVerticalPressed()), 
                   VerticalSignalMapper, SLOT(map()));
  QObject::connect(VerticalSignalMapper, SIGNAL(mapped(QWidget*)), 
                   this, SLOT(splitWidgetVertical(QWidget*)));
  
  QObject::connect(frame, SIGNAL(maximizePressed()), 
                   MaximizeSignalMapper, SLOT(map()));
  QObject::connect(MaximizeSignalMapper, SIGNAL(mapped(QWidget*)), 
                   this, SLOT(maximizeWidget(QWidget*)));
}

