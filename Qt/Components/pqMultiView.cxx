/*=========================================================================

   Program: ParaView
   Module:    pqMultiView.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include <QSignalMapper>
#include <QSplitter>
#include <QString>
#include <QBuffer>
#include <QDataStream>
#include <QLayout>
#include "pqMultiViewFrame.h"
#include "pqXMLUtil.h"

#include "vtkPVXMLElement.h"


//-----------------------------------------------------------------------------
QString pqMultiView::Index::getString() const
{
  QByteArray index_string;
  QDataStream stream(&index_string, QIODevice::WriteOnly);
  stream << *this;
  return QString(index_string.toBase64());
}

void pqMultiView::Index::setFromString(const QString& str) 
{
  QByteArray data = QByteArray::fromBase64(str.toAscii());
  QDataStream stream(data);
  stream >> *this;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
pqMultiView::pqMultiView(QWidget* p)
  : QFrame(p)
{
  QHBoxLayout* l = new QHBoxLayout(this);
  l->setSpacing(0);
  l->setMargin(0);
  this->setLayout(l);
  QSplitter* splitter = new QSplitter(this);
  splitter->setObjectName("MultiViewSplitter");
  l->addWidget(splitter);
  pqMultiViewFrame* frame = new pqMultiViewFrame;
  splitter->addWidget(frame);

  this->installEventFilter(this);
  this->setup(frame);
}

//-----------------------------------------------------------------------------
pqMultiView::~pqMultiView()
{
  // emit signals for removing frames
  QList<pqMultiViewFrame*> frames = this->findChildren<pqMultiViewFrame*>();
  foreach(pqMultiViewFrame* v, frames)
    {
    this->removeWidget(v);
    }
}

//-----------------------------------------------------------------------------
void pqMultiView::reset(QList<QWidget*> &removed)
{
  pqMultiViewFrame* frame = new pqMultiViewFrame();
  // Remove all the widgets. Put them in the list. Then clean
  // up all the extra splitters.
  QWidget *widget = this->layout()->itemAt(0)->widget();
  QSplitter *splitter = qobject_cast<QSplitter *>(widget);
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
}

//-----------------------------------------------------------------------------
QWidget* pqMultiView::replaceView(pqMultiView::Index index, QWidget* widget)
{
  if(!widget)
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
    splitter->insertWidget(location, widget);
    
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
void pqMultiView::removeView(QWidget* widget)
{
  QSplitter* splitter = qobject_cast<QSplitter*>(widget->parentWidget());
  if(splitter)
    {

    // remove widget
    widget->setParent(NULL);
    
    // if splitter is empty, add place holder
    if(splitter->count() == 0 && splitter->parentWidget() == this)
      {
      pqMultiViewFrame* frame = new pqMultiViewFrame;
      splitter->addWidget(frame);
      }
    // if splitter can be merged with parent splitter
    else if(splitter->count() < 2 && splitter->parentWidget() != this)
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

  pqMultiViewFrame* newFrame = NULL;

  QWidget* w = this->widgetOfIndex(index);
  Q_ASSERT(w != NULL);
  QSplitter* splitter = qobject_cast<QSplitter*>(w->parentWidget());
  if(!splitter)
    return Index();

  // if there is only one item in splitter
  if(splitter->count() < 2)
    {
    // change orientation
    splitter->setOrientation(orientation);
    // add new place holder
    newFrame = new pqMultiViewFrame;
    splitter->addWidget(newFrame);
    // make equal spacing
    QList<int> sizes = splitter->sizes();
    int sum=0, i;
    for(i=0; i<sizes.size(); i++)
      sum += sizes[i];
    for(i=0; i<sizes.size(); i++)
      sizes[i] = sum / sizes.size();
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
    QSplitter* newSplitter = new QSplitter(orientation);
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
    QList<int> sizes = newSplitter->sizes();
    int sum=0, i;
    for(i=0; i<sizes.size(); i++)
      sum += sizes[i];
    for(i=0; i<sizes.size(); i++)
      sizes[i] = sum / sizes.size();
    newSplitter->setSizes(sizes);
    
    QByteArray n = "MultiViewSplitter:";
    Index idxName = this->indexOf(newSplitter);
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

    splitter->insertWidget(splitter->indexOf(w)+1, newFrame);
    
    // make equal spacing
    QList<int> sizes = splitter->sizes();
    int sum=0, i;
    for(i=0; i<sizes.size(); i++)
      sum += sizes[i];
    for(i=0; i<sizes.size(); i++)
      sizes[i] = sum / sizes.size();
    splitter->setSizes(sizes);
    }
    
  return this->indexOf(newFrame);
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
    QSplitter* newSplitter = new QSplitter(orientation);
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


    // make equal spacing
 /*   QList<int> sizes = splitter->sizes();
    int sum=0, i;
    for(i=0; i<sizes.size(); i++)
      sum += sizes[i];
    for(i=0; i<sizes.size(); i++)
      sizes[i] = sum / sizes.size();
      */
    splitter->setSizes(sizes);
    }
    
  return this->indexOf(newFrame);
}



//-----------------------------------------------------------------------------
pqMultiView::Index pqMultiView::indexOf(QWidget* widget) const
{
  Index index;
  
  if(!widget)
    return index;

  QWidget* p = widget->parentWidget();
  while(p && p != this)
    {
    QSplitter* splitter = qobject_cast<QSplitter*>(p);
    if(splitter)
      {
      index.push_front(splitter->indexOf(widget));
      }
    else
      {
      QLayout* l = p->layout();
      Q_ASSERT(l != NULL);
      index.push_front(l->indexOf(widget));
      }
    widget = p;
    p = p->parentWidget();
    }
  return index;
}

//-----------------------------------------------------------------------------
QWidget* pqMultiView::widgetOfIndex(Index index)
{
  if(index.empty() && static_cast<QSplitter*>(
      this->layout()->itemAt(0)->widget())->count() == 1)
    {
    return static_cast<QSplitter*>(this->layout()->itemAt(0)->widget())->widget(0);
    }
  else if(index.empty())
    {
    return NULL;
    }

  Index::iterator iter = index.begin();
  Index::iterator end = index.end();
  QWidget* w = this->layout()->itemAt(0)->widget();
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
void pqMultiView::saveState(vtkPVXMLElement *root)
{
  if(!root)
    {
    return;
    }

  // Create an element to hold the multi-view state.
  vtkPVXMLElement *multiView = vtkPVXMLElement::New();
  multiView->SetName("MultiView");

  QSplitter *splitter = qobject_cast<QSplitter *>(
      this->layout()->itemAt(0)->widget());
  if(splitter)
    {
    // Save the splitter. This will recursively save the children.
    this->saveSplitter(multiView, splitter, 0);
    }

  root->AddNestedElement(multiView);
  multiView->Delete();
}

void pqMultiView::loadState(vtkPVXMLElement *root)
{
  if(!root)
    {
    return;
    }

  // Look for the multi-view element in the xml.
  vtkPVXMLElement *multiView = pqXMLUtil::FindNestedElementByName(root,
      "MultiView");
  if(multiView)
    {
    QSplitter *splitter = qobject_cast<QSplitter *>(
        this->layout()->itemAt(0)->widget());
    if(splitter)
      {
      QWidget *widget = splitter->widget(0);
      vtkPVXMLElement *element = pqXMLUtil::FindNestedElementByName(multiView,
          "Splitter");
      if(element && widget)
        {
        // This will be called recursively to restore the multi-view.
        this->restoreSplitter(widget, element);
        }
      }
    }
}

void pqMultiView::removeWidget(QWidget* widget)
{
  // If this is the only widget in the multi-view, replace it
  // with a new one so there is always something in the space.
  QSplitter *splitter = qobject_cast<QSplitter *>(widget->parentWidget());
  if(splitter && splitter->parentWidget() == this && splitter->count() < 2)
    {
    pqMultiViewFrame* frame = new pqMultiViewFrame();
    this->replaceView(this->indexOf(widget), frame);
    this->setup(frame);
    }
  else
    {
    this->removeView(widget);
    }

  emit this->frameRemoved(qobject_cast<pqMultiViewFrame*>(widget));
  delete widget;
}

pqMultiViewFrame* pqMultiView::splitWidget(QWidget* widget, Qt::Orientation o)
{
  pqMultiView::Index index = this->indexOf(widget);
  pqMultiView::Index newindex = this->splitView(index, o);
  QWidget *newWidget=this->widgetOfIndex(newindex);
  pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(newWidget); 
  this->setup(frame);
  emit this->frameAdded(frame);
  return frame;
}
pqMultiViewFrame* pqMultiView::splitWidget(QWidget* widget, Qt::Orientation o,float percent)
{
  pqMultiView::Index index = this->indexOf(widget);
  pqMultiView::Index newindex = this->splitView(index, o,percent);
  QWidget *newWidget=this->widgetOfIndex(newindex);
  pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(newWidget); 
  this->setup(frame);
  emit this->frameAdded(frame);
  return frame;
}
pqMultiViewFrame* pqMultiView::splitWidgetHorizontal(QWidget* widget)
{
  return this->splitWidget(widget, Qt::Horizontal);

}

pqMultiViewFrame* pqMultiView::splitWidgetVertical(QWidget* widget)
{
  return this->splitWidget(widget, Qt::Vertical);
}

void pqMultiView::maximizeWidget(QWidget* /*widget*/)
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

void pqMultiView::restoreWidget(QWidget* /*widget*/)
{

}
bool pqMultiView::eventFilter(QObject*, QEvent* e)
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

void pqMultiView::cleanSplitter(QSplitter *splitter, QList<QWidget*> &removed)
{
  QWidget *widget = 0;
  QSplitter *subsplitter = 0;
  for(int i = splitter->count() - 1; i >= 0; i--)
    {
    widget = splitter->widget(i);
    subsplitter = qobject_cast<QSplitter *>(widget);
    if(subsplitter)
      {
      this->cleanSplitter(subsplitter, removed);
      }
    else if(widget)
      {
      widget->setParent(0);
      removed.append(widget);
      }
    }
}
void pqMultiView::setup(pqMultiViewFrame* frame)
{
  Q_ASSERT(frame != NULL);
  if(!frame)
    return;

  // Give the frame a name.
  QString name;
  pqMultiView::Index index=this->indexOf(frame);
  name.setNum(index.front());
  frame->setObjectName(name);

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
}
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

void pqMultiView::restoreSplitter(QWidget *widget,
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
  int count = 0;
  if(element->GetScalarAttribute("count", &count))
    {
    for(int i = 1; i < count; i++)
      {
      this->splitWidget(widget, orientation);
      }

    // Get the view sizes. Convert them to a list of ints to
    // restore the splitter sizes.
    QSplitter *splitter = qobject_cast<QSplitter *>(widget->parentWidget());
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

