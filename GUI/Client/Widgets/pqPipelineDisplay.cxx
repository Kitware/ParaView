/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineDisplay.cxx

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

/// \file pqPipelineDisplay.cxx
/// \date 4/24/2006

#include "pqPipelineDisplay.h"

#include "pqMultiView.h"
#include "pqPipelineData.h"
#include "pqXMLUtil.h"

#include <QString>
#include "QVTKWidget.h"
#include <QWidget>

#include "vtkPVXMLElement.h"
#include "vtkSMDisplayProxy.h"


class pqPipelineDisplayItem
{
public:
  pqPipelineDisplayItem();
  ~pqPipelineDisplayItem();

  vtkSMDisplayProxy *GetDisplay() const {return this->Display;}
  void SetDisplay(vtkSMDisplayProxy *display);

  QString DisplayName;
  QWidget *Window;

private:
  vtkSMDisplayProxy *Display;
};


class pqPipelineDisplayInternal : public QList<pqPipelineDisplayItem *> {};


pqPipelineDisplayItem::pqPipelineDisplayItem()
  : DisplayName()
{
  this->Window = 0;
  this->Display = 0;
}

pqPipelineDisplayItem::~pqPipelineDisplayItem()
{
  // Make sure the proxy reference gets released.
  this->SetDisplay(0);
}

void pqPipelineDisplayItem::SetDisplay(vtkSMDisplayProxy *display)
{
  if(this->Display != display)
    {
    // Release the reference to the old proxy.
    if(this->Display)
      {
      this->Display->UnRegister(0);
      }

    // Save the pointer and add a reference to the new proxy.
    this->Display = display;
    if(this->Display)
      {
      this->Display->Register(0);
      }
    }
}


pqPipelineDisplay::pqPipelineDisplay()
{
  this->Internal = new pqPipelineDisplayInternal();
}

pqPipelineDisplay::~pqPipelineDisplay()
{
  if(this->Internal)
    {
    // Clean up the display proxy list.
    QList<pqPipelineDisplayItem *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      delete *iter;
      }

    delete this->Internal;
    }
}

void pqPipelineDisplay::UpdateWindows()
{
  if(this->Internal)
    {
    QList<pqPipelineDisplayItem *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      if((*iter)->Window)
        {
        (*iter)->Window->update();
        }
      }
    }
}

void pqPipelineDisplay::UnregisterDisplays()
{
  if(this->Internal)
    {
    pqPipelineData *pipeline = pqPipelineData::instance();
    QList<pqPipelineDisplayItem *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      if(!(*iter)->DisplayName.isEmpty())
        {
        pipeline->removeAndUnregisterDisplay((*iter)->GetDisplay(),
            (*iter)->DisplayName.toAscii().data(),
            pipeline->getRenderModule(qobject_cast<QVTKWidget *>(
            (*iter)->Window)));
        }

      delete *iter;
      *iter = 0;
      }

    this->Internal->clear();
    }
}

void pqPipelineDisplay::SaveState(vtkPVXMLElement *root,
    pqMultiView *multiView)
{
  if(!root || !multiView || !this->Internal)
    {
    return;
    }

  vtkPVXMLElement *element = 0;
  QList<pqPipelineDisplayItem *>::Iterator iter = this->Internal->begin();
  for( ; iter != this->Internal->end(); ++iter)
    {
    if((*iter)->Window && !(*iter)->DisplayName.isEmpty())
      {
      element = vtkPVXMLElement::New();
      element->SetName("Display");
      element->AddAttribute("name", (*iter)->DisplayName.toAscii().data());
      element->AddAttribute("windowID", pqXMLUtil::GetStringFromIntList(
          multiView->indexOf((*iter)->Window->parentWidget())).toAscii().data());
      root->AddNestedElement(element);
      element->Delete();
      }
    }
}

int pqPipelineDisplay::GetDisplayCount() const
{
  if(this->Internal)
    {
    return this->Internal->size();
    }

  return 0;
}

vtkSMDisplayProxy *pqPipelineDisplay::GetDisplayProxy(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->size())
    {
    return (*this->Internal)[index]->GetDisplay();
    }

  return 0;
}

void pqPipelineDisplay::GetDisplayName(int index, QString &buffer) const
{
  buffer.clear();
  if(this->Internal && index >= 0 && index < this->Internal->size())
    {
    buffer = (*this->Internal)[index]->DisplayName;
    }
}

QWidget *pqPipelineDisplay::GetDisplayWindow(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->size())
    {
    return (*this->Internal)[index]->Window;
    }

  return 0;
}

int pqPipelineDisplay::GetDisplayIndexFor(vtkSMDisplayProxy *display) const
{
  if(this->Internal)
    {
    QList<pqPipelineDisplayItem *>::Iterator iter = this->Internal->begin();
    for(int i = 0; iter != this->Internal->end(); ++iter, ++i)
      {
      if((*iter)->GetDisplay() == display)
        {
        return i;
        }
      }
    }

  return -1;
}

void pqPipelineDisplay::AddDisplay(vtkSMDisplayProxy *display,
    const QString &name, QWidget *window)
{
  // Make sure the display does not already exist.
  if(!this->Internal || this->GetDisplayIndexFor(display) != -1)
    {
    return;
    }

  // Create the display and add it to the list.
  pqPipelineDisplayItem *item = new pqPipelineDisplayItem();
  if(item)
    {
    item->SetDisplay(display);
    item->DisplayName = name;
    item->Window = window;
    this->Internal->append(item);
    }
}

void pqPipelineDisplay::RemoveDisplay(vtkSMDisplayProxy *display)
{
  if(this->Internal)
    {
    QList<pqPipelineDisplayItem *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      pqPipelineDisplayItem *item = *iter;
      if(item->GetDisplay() == display)
        {
        // Remove the display item from the list.
        *iter = 0;
        delete item;
        this->Internal->erase(iter);
        break;
        }
      }
    }
}


