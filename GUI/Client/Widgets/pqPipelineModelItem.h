/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineModelItem.h

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

/// \file pqPipelineModelItem.h
/// \date 4/14/2006

#ifndef _pqPipelineModelItem_h
#define _pqPipelineModelItem_h


#include "pqWidgetsExport.h"
#include <QObject>

#include "pqPipelineModel.h" // Needed for ModelType member


class PQWIDGETS_EXPORT pqPipelineModelItem : public QObject
{
  Q_OBJECT

public:
  pqPipelineModelItem(QObject* parent=NULL);
  virtual ~pqPipelineModelItem();

  // Get the type for the model item. Type determines the way the 
  // item is displayed.
  // TODO: May be type must be only for pqPipelineSource, 
  // everything is directly subclassed, hence we can use dynamic casts..
  pqPipelineModel::ItemType getType() const {return this->ModelType;}

signals:
  // This signal is fired when data associated with the item,
  // such as type/name is changed. 
  void dataModified();

protected:
  void setType(pqPipelineModel::ItemType type); 

protected slots:
  // called when input property on display changes. We must detect if
  // (and when) the display is connected to a new proxy.
  virtual void onInputChanged() { };
private:
  pqPipelineModel::ItemType ModelType;
};

#endif
