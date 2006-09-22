/*=========================================================================

   Program: ParaView
   Module:    pqSourceDisplayEditor.cxx

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

// this include
#include "pqSourceDisplayEditor.h"

// Qt includes
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QComboBox>

// VTK includes

// ParaView Server Manager includes

// ParaView widget includes

// ParaView core includes
#include "pqPipelineSource.h"
#include "pqPipelineDisplay.h"

// ParaView components includes
#include "pqDisplayProxyEditor.h"


//-----------------------------------------------------------------------------
pqSourceDisplayEditor::pqSourceDisplayEditor(QWidget* p)
  : QWidget(p)
{
  QVBoxLayout* l = new QVBoxLayout(this);
  this->DisplayCombo = new QComboBox(this);
  l->addWidget(this->DisplayCombo);
  this->StackWidget = new QStackedWidget(this);
  l->addWidget(this->StackWidget);
  QObject::connect(this->DisplayCombo,
                   SIGNAL(currentIndexChanged(int)),
                   this,
                   SLOT(changePage()));
}

//-----------------------------------------------------------------------------
pqSourceDisplayEditor::~pqSourceDisplayEditor()
{
}

//-----------------------------------------------------------------------------
void pqSourceDisplayEditor::setProxy(pqProxy* proxy) 
{
  if(this->Proxy == proxy)
    {
    return;
    }

  // clean out old stuff
  pqPipelineSource* source;
  source = qobject_cast<pqPipelineSource*>(this->Proxy);
  if(source)
    {
    int num = source->getDisplayCount();
    for(int i=0; i<num; i++)
      {
      pqPipelineDisplay* display = source->getDisplay(i);
      this->removePage(display);
      }
    QObject::disconnect(
              source, 
              SIGNAL(displayAdded(pqPipelineSource*, pqPipelineDisplay*)),
              this,
              SLOT(displayAdded(pqPipelineSource*, pqPipelineDisplay*)));
    QObject::disconnect(
              source, 
              SIGNAL(destroyed(QObject*)),
              this,
              SLOT(proxyDestroyed()));
    }

  this->Proxy = proxy;
  
  source = qobject_cast<pqPipelineSource*>(this->Proxy);
  if(source)
    {
    int num = source->getDisplayCount();
    for(int i=0; i<num; i++)
      {
      pqPipelineDisplay* display = source->getDisplay(i);
      this->addPage(display);
      }
    QObject::connect(
              source, 
              SIGNAL(displayAdded(pqPipelineSource*, pqPipelineDisplay*)),
              this,
              SLOT(displayAdded(pqPipelineSource*, pqPipelineDisplay*)),
              Qt::QueuedConnection);
              // allow chance for updates before really adding the page
              // or change the emission of the signal to be after the display is
              // usable
    QObject::connect(
              source, 
              SIGNAL(destroyed(QObject*)),
              this,
              SLOT(proxyDestroyed()));
    }

}

void pqSourceDisplayEditor::proxyDestroyed()
{
  this->setProxy(NULL);
}

//-----------------------------------------------------------------------------
/// get the proxy for which properties are displayed
pqProxy* pqSourceDisplayEditor::getProxy()
{
  return this->Proxy;
}

void pqSourceDisplayEditor::displayAdded(pqPipelineSource*, 
                                         pqPipelineDisplay* display)
{
  this->addPage(display);
}

void pqSourceDisplayEditor::displayRemoved(pqPipelineSource*, 
                                           pqPipelineDisplay* display)
{
  this->removePage(display);
}

void pqSourceDisplayEditor::removePage(pqPipelineDisplay* display)
{
  // remove display from combo box
  int idx = this->DisplayCombo->findData(display);
  if(idx != -1)
    {
    this->DisplayCombo->removeItem(idx);
    }

  // remove page for display
  for(int i=0; i<this->StackWidget->count(); i++)
    {
    pqDisplayProxyEditor* editor;
    editor = qobject_cast<pqDisplayProxyEditor*>(this->StackWidget->widget(i));
    if(editor && editor->getDisplay() == display)
      {
      this->StackWidget->removeWidget(editor);
      delete editor;
      }
    }
}

void pqSourceDisplayEditor::addPage(pqPipelineDisplay* display)
{
  // add display to combo box
  QString name = QString("Display %1").arg(this->DisplayCombo->count());
  QVariant d = qVariantFromValue(static_cast<QObject*>(display));
  this->DisplayCombo->insertItem(this->DisplayCombo->count(), name, d);
  
  // add page for display
  pqDisplayProxyEditor* editor = new pqDisplayProxyEditor(this->StackWidget);
  editor->setDisplay(display);
  this->StackWidget->addWidget(editor);
}

void pqSourceDisplayEditor::changePage()
{
  int idx = this->DisplayCombo->currentIndex();
  if(idx != -1)
    {
    QVariant d = this->DisplayCombo->itemData(idx);
    for(int i=0; i<this->StackWidget->count(); i++)
      {
      pqDisplayProxyEditor* editor;
      editor = qobject_cast<pqDisplayProxyEditor*>(this->StackWidget->widget(i));
      if(editor)
        {
        if(d.value<QObject*>() == editor->getDisplay())
          {
          this->StackWidget->setCurrentIndex(i);
          break;
          }
        }
      }
    }
}

