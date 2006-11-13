/*=========================================================================

   Program: ParaView
   Module:    pqDisplayProxyEditorWidget.cxx

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
#include "pqDisplayProxyEditorWidget.h"

#include "vtkSMProxy.h"

#include <QPointer>
#include <QVBoxLayout>

#include "pqDisplayProxyEditor.h"
#include "pqXYPlotDisplayProxyEditor.h"
#include "pqPipelineDisplay.h"

class pqDisplayProxyEditorWidgetInternal
{
public:
  QPointer<pqDisplay> Display;
  QPointer<QWidget> DisplayWidget;
};

//-----------------------------------------------------------------------------
pqDisplayProxyEditorWidget::pqDisplayProxyEditorWidget(QWidget* p /*=0*/)
  : QWidget(p)
{
  new QVBoxLayout(this);
  this->Internal = new pqDisplayProxyEditorWidgetInternal;
}

//-----------------------------------------------------------------------------
pqDisplayProxyEditorWidget::~pqDisplayProxyEditorWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
pqDisplay* pqDisplayProxyEditorWidget::getDisplay() const
{
  return this->Internal->Display;
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::setDisplay(pqDisplay* display)
{
  this->Internal->Display = display;
  if (!display)
    {
    emit this->requestSetDisplay((pqDisplay*)0);
    emit this->requestSetDisplay((pqPipelineDisplay*)0);
    return;
    }

  pqPipelineDisplay* pd = qobject_cast<pqPipelineDisplay*>(display);
  if (pd)
    {
    pqDisplayProxyEditor* editor = qobject_cast<pqDisplayProxyEditor*>(
      this->Internal->DisplayWidget);

    if (!editor)
      {
      editor = new pqDisplayProxyEditor(this);
      this->layout()->addWidget(editor);
      if (this->Internal->DisplayWidget)
        {
        this->layout()->removeWidget(this->Internal->DisplayWidget);
        delete this->Internal->DisplayWidget;
        }      
      this->Internal->DisplayWidget = editor;
      QObject::connect(this, SIGNAL(requestReload()),
        editor, SLOT(reloadGUI()));
      QObject::connect(this, SIGNAL(requestSetDisplay(pqPipelineDisplay*)),
        editor, SLOT(setDisplay(pqPipelineDisplay*)));
      }
    editor->setDisplay(pd);
    }
  else if (display->getProxy() && 
    display->getProxy()->GetXMLName() == QString("XYPlotDisplay2"))
    {
    pqXYPlotDisplayProxyEditor* editor = 
      qobject_cast<pqXYPlotDisplayProxyEditor*>(
        this->Internal->DisplayWidget);
    if (!editor)
      {
      editor = new pqXYPlotDisplayProxyEditor(this);
      this->layout()->addWidget(editor);
      if (this->Internal->DisplayWidget)
        {
        this->layout()->removeWidget(this->Internal->DisplayWidget);
        delete this->Internal->DisplayWidget;
        }
      this->Internal->DisplayWidget = editor;
      QObject::connect(this, SIGNAL(requestSetDisplay(pqDisplay*)),
        editor, SLOT(setDisplay(pqDisplay*)));
      QObject::connect(this, SIGNAL(requestReload()),
        editor, SLOT(reloadGUI()));
      }
    editor->setDisplay(display);
    }
  else
    {
    emit this->requestSetDisplay((pqDisplay*)0);
    emit this->requestSetDisplay((pqPipelineDisplay*)0);
    }
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditorWidget::reloadGUI()
{
  emit this->requestReload();
}
