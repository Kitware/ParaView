/*=========================================================================

   Program: ParaView
   Module:    pqMultiView.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#include "pqMultiView.h"

#include "vtkPVXMLElement.h"

#include <QBuffer>
#include <QDataStream>
#include <QLayout>
#include <QShortcut>
#include <QSignalMapper>
#include <QSplitter>
#include <QString>
#include <QStringList>
#include <QtDebug>

#include "pqMultiViewFrame.h"
#include "pqXMLUtil.h"


/// Special QSplitterHandle which can give minimal size hint when requested.
/// This makes it possible to almost hide the splitter handles in the view.
class pqSplitterHandle :public QSplitterHandle
{
  bool HideDecorations;
public:
  pqSplitterHandle(Qt::Orientation _orientation, QSplitter* _parent)
    :QSplitterHandle (_orientation, _parent),
    HideDecorations(false)
    {
    }

  // overrides default behavior when this->HideDecorations is true.
  virtual QSize sizeHint () const
    {
    return this->HideDecorations? QSize(): QSplitterHandle::sizeHint();
    }

  // When set to true this widget returns empty size hint indicating that this
  // widget needs no size.
  void hideDecorations(bool m)
    { this->HideDecorations = m; }
};

/// Special QSplitter which creates pqSplitterHandle.
class pqSplitter: public QSplitter
{
public:
  pqSplitter(Qt::Orientation _orientation, QWidget* _parent=0):
    QSplitter(_orientation, _parent)
  {}
  pqSplitter(QWidget* _parent=0):QSplitter(_parent){}
    

protected:
  QSplitterHandle* createHandle()
    {
    return new pqSplitterHandle(this->orientation(), this);
    }

};

//-----------------------------------------------------------------------------
QString pqMultiView::Index::getString() const
{
/*  QByteArray index_string;
  QDataStream stream(&index_string, QIODevice::WriteOnly);
  stream << *this;
  return QString(index_string.toBase64());
  */
  QString string;
  foreach(int index, *this)
    {
    if (string != QString::null)
      {
      string += ".";
      }
    string += QString::number(index);
    }
  return string;
}

void pqMultiView::Index::setFromString(const QString& str) 
{
  /*
  QByteArray data = QByteArray::fromBase64(str.toAscii());
  QDataStream stream(data);
  stream >> *this;
  */
  this->clear();
  QStringList indexes = str.split(".", QString::SkipEmptyParts);
  foreach (QString index, indexes)
    {
    QVariant v(index);
    if (v.canConvert(QVariant::Int))
      {
      this->push_back(v.toInt());
      }
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
pqMultiView::pqMultiView(QWidget* p)
  : Superclass(p)
{
  this->FullScreenParent = 0;
  this->FullScreenWidget = 0;

  QObject::connect(this, SIGNAL(frameAdded(pqMultiViewFrame*)),
    this, SLOT(updateFrameNames()));
  QObject::connect(this, SIGNAL(frameRemoved(pqMultiViewFrame*)),
    this, SLOT(updateFrameNames()));
  QObject::connect(this, SIGNAL(afterSplitView(const Index&, 
    Qt::Orientation, float, const Index&)),
    this, SLOT(updateFrameNames()));

  this->SplitterFrame= new QFrame(this);
  this->SplitterFrame->setObjectName("SplitterFrame");
  this->addWidget(this->SplitterFrame);

  this->MaximizeFrame= new QFrame(this);
  this->MaximizeFrame->setObjectName("MaximizeFrame");
  this->addWidget(this->MaximizeFrame);
  
  QHBoxLayout* l = new QHBoxLayout(this->SplitterFrame);
  l->setSpacing(0);
  l->setMargin(0);
  this->SplitterFrame->setLayout(l);

  QSplitter* splitter = new pqSplitter(this->SplitterFrame);
  splitter->setObjectName("MultiViewSplitter");
  l->addWidget(splitter);

  QHBoxLayout* ml = new QHBoxLayout(this->MaximizeFrame);
  ml->setSpacing(0);
  ml->setMargin(0);
  this->MaximizeFrame->setLayout(ml);

  this->FillerFrame= new pqMultiViewFrame(this->MaximizeFrame);
  ml->addWidget(this->FillerFrame);
  
  this->setCurrentWidget(this->SplitterFrame);

  this->CurrentMaximizedFrame=0;
}

//-----------------------------------------------------------------------------
void pqMultiView::init()
{
  QWidget *w = this->SplitterFrame->layout()->itemAt(0)->widget();
  QSplitter *splitter = qobject_cast<QSplitter *>(w);
  if(splitter)
    {
    pqMultiViewFrame* frame = new pqMultiViewFrame;
    splitter->addWidget(frame);
    this->setup(frame);
    emit this->frameAdded(frame);
    }
}

//-----------------------------------------------------------------------------
pqMultiView::~pqMultiView()
{
  QList<pqMultiViewFrame*> frames = this->findChildren<pqMultiViewFrame*>();
  foreach(pqMultiViewFrame* v, frames)
    {
    emit this->preFrameRemoved(qobject_cast<pqMultiViewFrame*>(v));
    emit this->frameRemoved(qobject_cast<pqMultiViewFrame*>(v));
    delete v;
    }
}

//-----------------------------------------------------------------------------
void pqMultiView::reset(QList<QWidget*> &removed)
{
  pqMultiViewFrame* frame = new pqMultiViewFrame();
  // Remove all the widgets. Put them in the list. Then clean
  // up all the extra splitters.
  QWidget *w = this->SplitterFrame->layout()->itemAt(0)->widget();
  QSplitter *splitter = qobject_cast<QSplitter *>(w);
  if(splitter)
    {
    this->cleanSplitter(splitter, removed);

    QSplitter *subsplitter = 0;
    for(int i = splitter->count() - 1; i >= 0; i--)
      {
      subsplitter = qobject_cast<QSplitter *>(splitter->widget(i));
      if(subsplitter)
        {
        delete subsplitter;
        }
      }

    splitter->refresh();
    splitter->addWidget(frame);
    }
  this->setup(frame);
  emit this->frameAdded(frame);
}

//-----------------------------------------------------------------------------
QWidget* pqMultiView::replaceView(pqMultiView::Index index, 
                                  QWidget* replaceWidget)
{
  if(!replaceWidget)
    return NULL;

  QWidget* w = this->widgetOfIndex(index);
  QSplitter* splitter = qobject_cast<QSplitter*>(w->parentWidget());
  
  if(splitter)
    {
    // get location of widget in splitter
    int location = splitter->indexOf(w);
    // save splitter sizes
    QList<int> sizes;
    if(splitter->count() > 1)
      {
      sizes = splitter->sizes();
      }

    splitter->hide();

    // remove widget
    w->setParent(NULL);
    // add replacement at same location
    splitter->insertWidget(location, replaceWidget);
    
    splitter->show();

    // ensure same splitter sizes
    if(splitter->count() > 1)
      {
      splitter->setSizes(sizes);
      }
    return w;
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void pqMultiView::removeView(QWidget* w)
{
  QSplitter* splitter = qobject_cast<QSplitter*>(w->parentWidget());
  if(splitter)
    {

    // remove widget
    w->setParent(NULL);
    
    // if splitter is empty, add place holder
    if(splitter->count() == 0 && splitter->parentWidget() == this->SplitterFrame)
      {
      pqMultiViewFrame* frame = new pqMultiViewFrame;
      splitter->addWidget(frame);
      }
    // if splitter can be merged with parent splitter
    else if(splitter->count() < 2 && splitter->parentWidget() != this->SplitterFrame)
      {
      QWidget* otherWidget = splitter->widget(0);
      QSplitter* parentSplitter = qobject_cast<QSplitter*>(splitter->parentWidget());
      Q_ASSERT(parentSplitter != NULL);
      int location = parentSplitter->indexOf(splitter);
      // get sizes
      QList<int> sizes = parentSplitter->sizes();
      // in Qt 4.0.1, setParent has a side effect of calling XFlush, so we 
      // see some intermediate stuff
      splitter->setParent(NULL);
      parentSplitter->insertWidget(location, otherWidget);
      // restore sizes
      parentSplitter->setSizes(sizes);
      delete splitter;
      }
    else if (splitter->count() == 1 && splitter->parentWidget() == this->SplitterFrame)
      {
      // If the "other" widget is a splitter, then that subtree
      // must become the root.
      QWidget* otherWidget = splitter->widget(0);
      QSplitter* otherSplitter = qobject_cast<QSplitter*>(otherWidget);
      if (otherSplitter)
        {
        otherSplitter->setParent(this->SplitterFrame);
        delete splitter;
        otherSplitter->setObjectName("MultiViewSplitter");
        this->SplitterFrame->layout()->addWidget(otherSplitter);
        }
      }
    else
      {
      // will Qt 4.1 fix this so we don't have to call refresh?
      // fix stray splitter handles because children were removed
      splitter->refresh();
      }
    }
}

//-----------------------------------------------------------------------------
pqMultiView::Index pqMultiView::splitView(pqMultiView::Index index, 
  Qt::Orientation orientation)
{
  return this->splitView(index, orientation, 0.5);
}

//-----------------------------------------------------------------------------
pqMultiView::Index pqMultiView::splitView(pqMultiView::Index index, 
  Qt::Orientation orientation, float percent)
{
  if(percent <0.0)
    percent=0.0;
  else if(percent >1.0)
    percent=1.0;

  pqMultiViewFrame* newFrame = NULL;

  QWidget* w = this->widgetOfIndex(index);
  Q_ASSERT(w != NULL);
  QSplitter* splitter = qobject_cast<QSplitter*>(w->parentWidget());
  if(!splitter)
    return Index();

  // if there is only one item in splitter
  if(splitter->count() < 2)
    {
    QList<int> old_sizes = splitter->sizes();
    // change orientation
    splitter->setOrientation(orientation);
    // add new place holder
    newFrame = new pqMultiViewFrame;
    splitter->addWidget(newFrame);

    QList<int> sizes = splitter->sizes();

    sizes[0]=static_cast<int>(old_sizes[0]*(1.0-percent));
    sizes[1]=static_cast<int>(old_sizes[0]*percent);

    splitter->setSizes(sizes);
    }
  // else if the orientation isn't the same, we need to make a new child splitter
  // with the desired orientation
  else if(splitter->orientation() != orientation)
    {
    // get parent sizes
    QList<int> parentsizes = splitter->sizes();
    splitter->hide();
    
    int location = splitter->indexOf(w);
    QSplitter* newSplitter = new pqSplitter(orientation);
    // add splitter to splitter
    splitter->insertWidget(location, newSplitter);
    // remove from old splitter, and add to new splitter
    w->setParent(newSplitter);
    newSplitter->addWidget(w);
    // add new place holder
    newFrame = new pqMultiViewFrame;

    newSplitter->addWidget(newFrame);
    
    splitter->show();
    // ensure same sizes for parent splitter
    splitter->setSizes(parentsizes);

    // make equal spacing for new splitter
    QList<int> sizes_old = newSplitter->sizes();
    QList<int> sizes = newSplitter->sizes();

    sizes[0]=static_cast<int>(sizes_old[0]*(1.0-percent));
    sizes[1]=static_cast<int>(sizes_old[0]*percent);

    newSplitter->setSizes(sizes);
    
    QByteArray n = "MultiViewSplitter:";
    Index idxName = this->indexOf(newSplitter);
    int i;
    for(i=0; i<idxName.size(); i++)
      {
      QString tmp;
      tmp.setNum(idxName[i]);
      if(i != 0)
        n += ",";
      n += tmp;
      }
    newSplitter->setObjectName(n);
    }
  else
    {
    // insert new below or on right of existing one
    newFrame = new pqMultiViewFrame;
    QList<int> sizes_old = splitter->sizes();
    splitter->insertWidget(splitter->indexOf(w)+1, newFrame);
    QList<int> sizes = splitter->sizes();
    sizes[splitter->indexOf(w)] = static_cast<int>(
      sizes_old[splitter->indexOf(w)]*(1.0-percent));
    sizes[splitter->indexOf(w)+1] = static_cast<int>(
      sizes_old[splitter->indexOf(w)]*percent);
    splitter->setSizes(sizes);
    }
    
  Index childIndex = this->indexOf(newFrame);
  emit this->afterSplitView(index, orientation, percent, childIndex);
  return childIndex;
}



//-----------------------------------------------------------------------------
pqMultiView::Index pqMultiView::indexOf(QWidget* w) const
{
  Index index;
  
  if(!w)
    return index;

  QWidget* p = w->parentWidget();
  while(p && p != this->SplitterFrame)
    {
    QSplitter* splitter = qobject_cast<QSplitter*>(p);
    if(splitter)
      {
      index.push_front(splitter->indexOf(w));
      }
    else
      {
      QLayout* l = p->layout();
      Q_ASSERT(l != NULL);
      index.push_front(l->indexOf(w));
      }
    w = p;
    p = p->parentWidget();
    }
  return index;
}

//-----------------------------------------------------------------------------
QWidget* pqMultiView::widgetOfIndex(Index index)
{
  if(index.empty() && static_cast<QSplitter*>(
      this->SplitterFrame->layout()->itemAt(0)->widget())->count() == 1)
    {
    return static_cast<QSplitter*>(this->SplitterFrame->layout()->itemAt(0)->widget())->widget(0);
    }
  else if(index.empty())
    {
    return NULL;
    }

  Index::iterator iter = index.begin();
  Index::iterator end = index.end();
  QWidget* w = this->SplitterFrame->layout()->itemAt(0)->widget();
  for(; iter != end && w; ++iter)
    {
    QSplitter* splitter = qobject_cast<QSplitter*>(w);
    if(splitter)
      {
      w = splitter->widget(*iter);
      }
    else
      {
      return NULL;
      }
    }
  
  // Bad index was passed in, return NULL
  if(iter != index.end())
    {
    w = NULL;
    }

  return w;
}

//-----------------------------------------------------------------------------
// Returns the top level splitter widget
QWidget* pqMultiView::getMultiViewWidget() const
{
  return this->SplitterFrame->layout()->itemAt(0)->widget();
}

//-----------------------------------------------------------------------------
Qt::Orientation pqMultiView::widgetOrientation(QWidget* _widget) const
{
  QSplitter *splitter = qobject_cast<QSplitter *>(_widget->parentWidget());
  if (!splitter)
    {
    qCritical() << "widgetOrientation called with incorrect widget.";
    return Qt::Vertical;
    }

  return splitter->orientation();
}

//-----------------------------------------------------------------------------
float pqMultiView::widgetSplitRatio(QWidget* _widget) const
{
  QSplitter *splitter = qobject_cast<QSplitter *>(_widget->parentWidget());
  if (!splitter)
    {
    qCritical() << "widgetSplitRatio called with incorrect widget.";
    return 0.5;
    }

  QList<int> sizes = splitter->sizes();
  float sum = 0;
  foreach (int _size, sizes)
    {
    sum += _size;
    }

  float ratio = sum>0? (1.0 - sizes[0]/sum) : 0.5;
  return ratio;
}

//-----------------------------------------------------------------------------
pqMultiView::Index pqMultiView::parentIndex(const Index& index) const
{
  if (index.size() <= 1)
    {
    return Index();
    }
  Index parent_index = index;
  parent_index.pop_back();
  return parent_index;
}

//-----------------------------------------------------------------------------
void pqMultiView::saveState(vtkPVXMLElement *root)
{
  if(!root)
    {
    return;
    }

  //TODO This restores the maximized view before the state is saved.
  // Need to fix things so that the maximized view is saved.
  this->restoreWidget(NULL);


  // Create an element to hold the multi-view state.
  vtkPVXMLElement *multiView = vtkPVXMLElement::New();
  multiView->SetName("MultiView");

  QSplitter *splitter = qobject_cast<QSplitter *>(
      this->SplitterFrame->layout()->itemAt(0)->widget());
  if(splitter)
    {
    // Save the splitter. This will recursively save the children.
    this->saveSplitter(multiView, splitter, 0);
    }

 // vtkPVXMLElement *MaximizedElement = vtkPVXMLElement::New();
 // splitterElement->SetName("MaximizedWidget");




  root->AddNestedElement(multiView);
  multiView->Delete();
}

//-----------------------------------------------------------------------------
void pqMultiView::loadState(vtkPVXMLElement *root)
{
  if(!root)
    {
    return;
    }

 //TODO This restores the maximized view before the state is restored.
  // Need to fix things so that the maximized view is restored.
  this->restoreWidget(NULL);



  // Look for the multi-view element in the xml.
  vtkPVXMLElement *multiView = pqXMLUtil::FindNestedElementByName(root,
      "MultiView");
  if(multiView)
    {
    QSplitter *splitter = qobject_cast<QSplitter *>(
        this->SplitterFrame->layout()->itemAt(0)->widget());
    if(splitter)
      {
      QWidget *w = splitter->widget(0);
      vtkPVXMLElement *element = pqXMLUtil::FindNestedElementByName(multiView,
          "Splitter");
      if(element && w)
        {
        // This will be called recursively to restore the multi-view.
        this->restoreSplitter(w, element);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqMultiView::removeWidget(QWidget* w)
{
  emit this->preFrameRemoved(qobject_cast<pqMultiViewFrame*>(w));

  // If this is the only widget in the multi-view, replace it
  // with a new one so there is always something in the space.
  QSplitter *splitter = qobject_cast<QSplitter *>(w->parentWidget());
  if(splitter && splitter->parentWidget() == this->SplitterFrame && splitter->count() < 2)
    {
    pqMultiViewFrame* frame = new pqMultiViewFrame();
    this->replaceView(this->indexOf(w), frame);
    this->setup(frame);
    emit this->frameAdded(frame);
    }
  else
    {
    this->removeView(w);
    }

  emit this->frameRemoved(qobject_cast<pqMultiViewFrame*>(w));
  delete w;
}

//-----------------------------------------------------------------------------
pqMultiViewFrame* pqMultiView::splitWidget(QWidget* w, Qt::Orientation o)
{
  pqMultiView::Index index = this->indexOf(w);
  pqMultiView::Index newindex = this->splitView(index, o);
  QWidget *newWidget=this->widgetOfIndex(newindex);
  pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(newWidget); 
  this->setup(frame);
  emit this->frameAdded(frame);
  return frame;
}

//-----------------------------------------------------------------------------
pqMultiViewFrame* pqMultiView::splitWidget(QWidget* w, Qt::Orientation o,float percent)
{
  pqMultiView::Index index = this->indexOf(w);
  pqMultiView::Index newindex = this->splitView(index, o,percent);
  QWidget *newWidget=this->widgetOfIndex(newindex);
  pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(newWidget); 
  this->setup(frame);
  emit this->frameAdded(frame);
  return frame;
}

//-----------------------------------------------------------------------------
pqMultiViewFrame* pqMultiView::splitWidgetHorizontal(QWidget* w)
{
  return this->splitWidget(w, Qt::Horizontal);

}

//-----------------------------------------------------------------------------
pqMultiViewFrame* pqMultiView::splitWidgetVertical(QWidget* w)
{
  return this->splitWidget(w, Qt::Vertical);
}

//-----------------------------------------------------------------------------
void pqMultiView::maximizeWidget(QWidget* maxWidget)
{
  pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(maxWidget);
  if(!frame)
    {
    return;
    }
  if (this->CurrentMaximizedFrame == frame)
    {
    // Already maximized, nothing to do.
    return;
    }
  if (this->CurrentMaximizedFrame)
    {
    this->CurrentMaximizedFrame->restore();
    this->CurrentMaximizedFrame = 0;
    }


  QWidget *w = this->SplitterFrame->layout()->itemAt(0)->widget();
  QSplitter *splitter = qobject_cast<QSplitter *>(w);
  if(splitter)
    {
      this->hide();  

      Index currentIdx=this->indexOf(frame);
      QLayout *l=this->MaximizeFrame->layout();
      l->removeWidget(this->FillerFrame);
      this->replaceView(currentIdx,this->FillerFrame);
      frame->setParent(this->MaximizeFrame);
      this->MaximizeFrame->layout()->addWidget(frame);

      this->CurrentMaximizedFrame=frame;

      frame->MaximizeButton->hide();
      frame->CloseButton->hide();
      frame->SplitHorizontalButton->hide();
      frame->SplitVerticalButton->hide();
      frame->RestoreButton->show();
        
      this->setCurrentWidget(this->MaximizeFrame);
      this->show();
    }

}

//-----------------------------------------------------------------------------
void pqMultiView::restoreWidget(QWidget*)
{

  if(this->CurrentMaximizedFrame)
    {
  QWidget *w = this->SplitterFrame->layout()->itemAt(0)->widget();
  QSplitter *splitter = qobject_cast<QSplitter *>(w);
  if(splitter)
    {
    this->hide();  

    QLayout *l=this->MaximizeFrame->layout();
    l->removeWidget(this->CurrentMaximizedFrame);

    Index currentIdx=this->indexOf(this->FillerFrame);
    this->replaceView(currentIdx,this->CurrentMaximizedFrame);

    this->FillerFrame->setParent(this->MaximizeFrame);
    this->MaximizeFrame->layout()->addWidget(this->FillerFrame);

    this->CurrentMaximizedFrame->MaximizeButton->show();
    this->CurrentMaximizedFrame->CloseButton->show();
    this->CurrentMaximizedFrame->SplitHorizontalButton->show();
    this->CurrentMaximizedFrame->SplitVerticalButton->show();
    this->CurrentMaximizedFrame->RestoreButton->hide();

    this->CurrentMaximizedFrame=NULL;

    this->setCurrentWidget(this->SplitterFrame);
    this->show();
    }
  }

}

//-----------------------------------------------------------------------------
/*
bool pqMultiView::eventFilter(QObject*, QEvent* e)
{
  if(e->type() == QEvent::Polish)
    {
    // delay emit of first signal
    emit this->frameAdded(qobject_cast<pqMultiViewFrame*>(
        this->widgetOfIndex(pqMultiView::Index())));
  //  this->removeEventFilter(this);
    }
  return false;
}
*/

//-----------------------------------------------------------------------------
void pqMultiView::cleanSplitter(QSplitter *splitter, QList<QWidget*> &removed)
{
  QWidget *w = 0;
  QSplitter *subsplitter = 0;
  for(int i = splitter->count() - 1; i >= 0; i--)
    {
    w = splitter->widget(i);
    subsplitter = qobject_cast<QSplitter *>(w);
    if(subsplitter)
      {
      this->cleanSplitter(subsplitter, removed);
      }
    else if(w)
      {
      w->setParent(0);
      removed.append(w);
      }
    }
}

//-----------------------------------------------------------------------------
void pqMultiView::setup(pqMultiViewFrame* frame)
{
  Q_ASSERT(frame != NULL);
  if (!frame)
    {
    return;
    }

  QSignalMapper* CloseSignalMapper = new QSignalMapper(frame);
  QSignalMapper* HorizontalSignalMapper = new QSignalMapper(frame);
  QSignalMapper* VerticalSignalMapper = new QSignalMapper(frame);
  QSignalMapper* MaximizeSignalMapper = new QSignalMapper(frame);
  QSignalMapper* RestoreSignalMapper = new QSignalMapper(frame);

  CloseSignalMapper->setMapping(frame, frame);
  HorizontalSignalMapper->setMapping(frame, frame);
  VerticalSignalMapper->setMapping(frame, frame);
  MaximizeSignalMapper->setMapping(frame, frame);
  RestoreSignalMapper->setMapping(frame, frame);

  // connect close button
  QObject::connect(frame, SIGNAL(closePressed()), 
                   CloseSignalMapper, SLOT(map()));
  QObject::connect(CloseSignalMapper, SIGNAL(mapped(QWidget*)), 
                   this, SLOT(removeWidget(QWidget*)), Qt::QueuedConnection);

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
 
  QObject::connect(frame, SIGNAL(restorePressed()), 
                   RestoreSignalMapper, SLOT(map()));
  QObject::connect(RestoreSignalMapper, SIGNAL(mapped(QWidget*)), 
                   this, SLOT(restoreWidget(QWidget*)));

  // Connect decorations signals.
  QObject::connect(this, SIGNAL(hideFrameDecorations()),
    frame, SLOT(hideDecorations()));
  QObject::connect(this, SIGNAL(showFrameDecorations()),
    frame, SLOT(showDecorations()));
}

//-----------------------------------------------------------------------------
void pqMultiView::saveSplitter(vtkPVXMLElement *element,
    QSplitter *splitter, int index)
{
  // Make a new element for the splitter.
  vtkPVXMLElement *splitterElement = vtkPVXMLElement::New();
  splitterElement->SetName("Splitter");

  // Save the splitter's index in the parent splitter.
  QString number;
  if(index >= 0)
    {
    number.setNum(index);
    splitterElement->AddAttribute("index", number.toAscii().data());
    }

  // Save the splitter orientation and sizes.
  if(splitter->orientation() == Qt::Horizontal)
    {
    splitterElement->AddAttribute("orientation", "Horizontal");
    }
  else
    {
    splitterElement->AddAttribute("orientation", "Vertical");
    }

  number.setNum(splitter->count());
  splitterElement->AddAttribute("count", number.toAscii().data());
  splitterElement->AddAttribute("sizes",
      pqXMLUtil::GetStringFromIntList(splitter->sizes()).toAscii().data());

  // Save each of the child widgets.
  QSplitter *subsplitter = 0;
  for(int i = 0; i < splitter->count(); ++i)
    {
    subsplitter = qobject_cast<QSplitter *>(splitter->widget(i));
    if(subsplitter)
      {
      // Save the splitter. This will recursively save the children.
      this->saveSplitter(splitterElement, subsplitter, i);
      }
    }

  // Add the element to the xml.
  element->AddNestedElement(splitterElement);
  splitterElement->Delete();
}

void pqMultiView::restoreSplitter(QWidget *w,
    vtkPVXMLElement *element)
{
  // Set the orientation.
  Qt::Orientation orientation = Qt::Horizontal;
  QString value = element->GetAttribute("orientation");
  if(value == "Vertical")
    {
    orientation = Qt::Vertical;
    }

  // Get the number of child widgets. Split the view to hold
  // enough child widgets.
  int num = 0;
  if(element->GetScalarAttribute("count", &num))
    {
    for(int i = 1; i < num; i++)
      {
      this->splitWidget(w, orientation);
      }

    // Get the view sizes. Convert them to a list of ints to
    // restore the splitter sizes.
    QSplitter *splitter = qobject_cast<QSplitter *>(w->parentWidget());
    if(splitter)
      {
      QList<int> sizes = pqXMLUtil::GetIntListFromString(
          element->GetAttribute("sizes"));
      if(sizes.size() >= splitter->count())
        {
        splitter->setSizes(sizes);
        }

      // Search the element for sub-splitters.
      int index = 0;
      value = "Splitter";
      vtkPVXMLElement *splitterElement = 0;
      unsigned int total = element->GetNumberOfNestedElements();
      for(unsigned int j = 0; j < total; j++)
        {
        splitterElement = element->GetNestedElement(j);
        if(value == splitterElement->GetName())
          {
          if(splitterElement->GetScalarAttribute("index", &index) &&
              index >= 0 && index < splitter->count())
            {
            this->restoreSplitter(splitter->widget(index), splitterElement);
            }
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqMultiView::hideDecorations()
{
  QList<pqSplitterHandle*> handles = this->findChildren<pqSplitterHandle*>();
  foreach (pqSplitterHandle* shandle, handles)
    {
    shandle->hideDecorations(true);
    }

  emit this->hideFrameDecorations();
}

//-----------------------------------------------------------------------------
void pqMultiView::showDecorations()
{
  QList<pqSplitterHandle*> handles = this->findChildren<pqSplitterHandle*>();
  foreach (pqSplitterHandle* shandle, handles)
    {
    shandle->hideDecorations(false);
    }
  emit this->showFrameDecorations();
}

//-----------------------------------------------------------------------------
QSize pqMultiView::clientSize() const
{
  QRect bounds;
  QList<pqMultiViewFrame*> frames = this->findChildren<pqMultiViewFrame*>();
  foreach (pqMultiViewFrame* frame, frames)
    {
    if (frame == this->FillerFrame || !frame->isVisible())
      {
      continue;
      }
    QWidget* w = frame->mainWidget();
    w = w ? w : frame->emptyMainWidget();
    w = w ? w : frame;
    QRect curBounds = w->rect();
    curBounds.moveTo(w->mapToGlobal(QPoint(0, 0)));
    bounds |= curBounds;
    }
  return bounds.size();
}

//-----------------------------------------------------------------------------
QSize pqMultiView::computeSize(QSize client_size) const
{
  QSize innerSize = this->clientSize();
  QSize outerSize = this->size();
  QSize padding = outerSize-innerSize;
  return client_size + padding;
}

//-----------------------------------------------------------------------------
void pqMultiView::updateFrameNames()
{
  static int name_uniquer=0;
  QList<pqMultiViewFrame*> frames = this->findChildren<pqMultiViewFrame*>();
  foreach (pqMultiViewFrame* frame, frames)
    {
    QSplitter* parentWdg = qobject_cast<QSplitter*>(frame->parentWidget());
    if (parentWdg)
      {
      frame->setObjectName(QString::number(parentWdg->indexOf(frame)));
      }
    else
      {
      frame->setObjectName(QString("orphan%1").arg(name_uniquer++));
      }
    }
}

//-----------------------------------------------------------------------------
void pqMultiView::setCurrentWidget(QWidget* cur_widget)
{
  if (this->FullScreenParent && cur_widget)
    {
    this->Superclass::addWidget(this->FullScreenWidget);
    this->FullScreenWidget = cur_widget;
    
    delete this->FullScreenParent->layout();
    cur_widget->setParent(this->FullScreenParent);
    QVBoxLayout* vbl = new QVBoxLayout(this->FullScreenParent);
    vbl->setMargin(0);
    vbl->setSpacing(0);
    vbl->addWidget(cur_widget);
    cur_widget->show();
    }
  else
    {
    this->Superclass::setCurrentWidget(cur_widget);
    }
}

//-----------------------------------------------------------------------------
void pqMultiView::toggleFullScreen()
{
  if (!this->FullScreenParent)
    {
    QWidget* cur_widget = this->Superclass::currentWidget();
    this->Superclass::removeWidget(cur_widget);
    this->FullScreenWidget = cur_widget;
    this->FullScreenParent = new QWidget(this, Qt::Window);
    cur_widget->setParent(this->FullScreenParent);
    QVBoxLayout* vbl = new QVBoxLayout(this->FullScreenParent);
    vbl->setMargin(0);
    vbl->setSpacing(0);
    vbl->addWidget(cur_widget);
    cur_widget->show();

    QShortcut *esc= new QShortcut(Qt::Key_Escape, this->FullScreenParent);
    QObject::connect(esc, SIGNAL(activated()), this, SLOT(toggleFullScreen()));
    QShortcut *f11= new QShortcut(Qt::Key_F11, this->FullScreenParent);
    QObject::connect(f11, SIGNAL(activated()), this, SLOT(toggleFullScreen()));

    this->FullScreenParent->showFullScreen();
    this->FullScreenParent->show();
    }
  else
    {
    this->FullScreenParent->hide();
    this->FullScreenWidget->hide();
    this->Superclass::addWidget(this->FullScreenWidget);
    this->Superclass::setCurrentWidget(this->FullScreenWidget);
    delete this->FullScreenParent;
    this->FullScreenParent = 0;
    this->FullScreenWidget = 0;
    }
}

