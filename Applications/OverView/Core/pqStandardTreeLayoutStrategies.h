/*=========================================================================

   Program: ParaView
   Module:    pqStandardTreeLayoutStrategies.h

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

#ifndef _pqStandardTreeLayoutStrategies_h
#define _pqStandardTreeLayoutStrategies_h

#include "OverViewCoreExport.h"
#include <pqCoreExport.h>
#include <pqTreeLayoutStrategyInterface.h>
#include <vtkSmartPointer.h>

#include <QObject>

class vtkBoxLayoutStrategy;
class vtkSliceAndDiceLayoutStrategy;
class vtkSquarifyLayoutStrategy;
class vtkStackedTreeLayoutStrategy;

/// interface class for plugins that create graph layout strategies
class OVERVIEW_CORE_EXPORT pqStandardTreeLayoutStrategies :
  public QObject, 
  public pqTreeLayoutStrategyInterface
{
  Q_OBJECT
  Q_INTERFACES(pqTreeLayoutStrategyInterface)
public:
  
  pqStandardTreeLayoutStrategies(QObject* o);
  ~pqStandardTreeLayoutStrategies();

  // Description:
  // A list of the types of layout strategies
  virtual QStringList treeLayoutStrategies() const;
 
  // Description:
  // Return the actual layout strategy object given a the name.
  // i.e. "Fast2D" will return a vtkFast2DGraphLayoutStrategy object
  vtkAreaLayoutStrategy* getTreeLayoutStrategy(const QString& layoutStrategy);
  
protected:
  vtkSmartPointer<vtkBoxLayoutStrategy>           BoxStrategy;
  vtkSmartPointer<vtkSliceAndDiceLayoutStrategy>  SliceAndDiceStrategy;
  vtkSmartPointer<vtkSquarifyLayoutStrategy>      SquarifyStrategy;
  vtkSmartPointer<vtkStackedTreeLayoutStrategy>   StackedTreeStrategy;
};

#endif

