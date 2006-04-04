/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqMultiViewManager.h"

#include <QSignalMapper>
#include <QSplitter>
#include <QString>
#include <QBuffer>
#include <QDataStream>

#include "pqMultiView.h"
#include "pqMultiViewFrame.h"
#include "pqXMLUtil.h"

#include "vtkPVXMLElement.h"

static int gNameNum = 0;

pqMultiViewManager::pqMultiViewManager(QWidget* p)
  : pqMultiView(p)
{
  pqMultiViewFrame* frame = new pqMultiViewFrame(this);
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

void pqMultiViewManager::reset(QList<QWidget*> &removed, QWidget *)
{
  // Create a new multi-view frame for the reset view.
  pqMultiViewFrame* frame = new pqMultiViewFrame();
  pqMultiView::reset(removed, frame);
  this->setup(frame);
}

void pqMultiViewManager::saveState(vtkPVXMLElement *root)
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

void pqMultiViewManager::loadState(vtkPVXMLElement *root)
{
  if(!root)
    {
    return;
    }

  // Look for the multi-view element in the xml.
  vtkPVXMLElement *multiView = ParaQ::FindNestedElementByName(root, "MultiView");
  if(multiView)
    {
    QSplitter *splitter = qobject_cast<QSplitter *>(
        this->layout()->itemAt(0)->widget());
    if(splitter)
      {
      QWidget *widget = splitter->widget(0);
      vtkPVXMLElement *element = ParaQ::FindNestedElementByName(multiView, "Splitter");
      if(element && widget)
        {
        // This will be called recursively to restore the multi-view.
        this->restoreSplitter(widget, element);
        }
      }
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

void pqMultiViewManager::splitWidget(QWidget* widget, Qt::Orientation o)
{
  pqMultiView::Index index = this->indexOf(widget);
  pqMultiView::Index newindex = this->splitView(index, o);
  pqMultiViewFrame* frame = new pqMultiViewFrame;
  QWidget *old = this->replaceView(newindex, frame);
  delete old;
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

void pqMultiViewManager::maximizeWidget(QWidget* /*widget*/)
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

void pqMultiViewManager::restoreWidget(QWidget* /*widget*/)
{

}


void pqMultiViewManager::setup(pqMultiViewFrame* frame)
{
  Q_ASSERT(frame != NULL);
  if(!frame)
    return;

  // Give the frame a name.
  QString name;
  name.setNum(gNameNum);
  gNameNum++;
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

void pqMultiViewManager::saveSplitter(vtkPVXMLElement *element,
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
      ParaQ::GetStringFromIntList(splitter->sizes()).toAscii().data());

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

void pqMultiViewManager::restoreSplitter(QWidget *widget,
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
      QList<int> sizes = ParaQ::GetIntListFromString(element->GetAttribute("sizes"));
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

