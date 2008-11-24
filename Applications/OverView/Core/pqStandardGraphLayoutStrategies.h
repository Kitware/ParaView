/*=========================================================================

   Program: ParaView
   Module:    pqStandardGraphLayoutStrategies.h

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

#ifndef _pqStandardGraphLayoutStrategies_h
#define _pqStandardGraphLayoutStrategies_h

#include "OverViewCoreExport.h"
#include <pqCoreExport.h>
#include <pqGraphLayoutStrategyInterface.h>
#include <vtkSmartPointer.h>

#include <QObject>

class vtkCircularLayoutStrategy;
class vtkClustering2DLayoutStrategy;
class vtkCommunity2DLayoutStrategy;
class vtkFast2DLayoutStrategy;
class vtkForceDirectedLayoutStrategy;
class vtkRandomLayoutStrategy;
class vtkSimple2DLayoutStrategy;
class vtkPassThroughLayoutStrategy;

/// interface class for plugins that create graph layout strategies
class OVERVIEW_CORE_EXPORT pqStandardGraphLayoutStrategies :
  public QObject, 
  public pqGraphLayoutStrategyInterface
{
  Q_OBJECT
  Q_INTERFACES(pqGraphLayoutStrategyInterface)
public:
  
  pqStandardGraphLayoutStrategies(QObject* o);
  ~pqStandardGraphLayoutStrategies();

  // Description:
  // A list of the types of layout strategies
  virtual QStringList graphLayoutStrategies() const;
 
  // Description:
  // Return the actual layout strategy object given a the name.
  // i.e. "Fast2D" will return a vtkFast2DGraphLayoutStrategy object
  vtkGraphLayoutStrategy* getGraphLayoutStrategy(const QString& layoutStrategy);
  
protected:
  vtkSmartPointer<vtkRandomLayoutStrategy>         RandomStrategy;
  vtkSmartPointer<vtkForceDirectedLayoutStrategy>  ForceDirectedStrategy;
  vtkSmartPointer<vtkSimple2DLayoutStrategy>       Simple2DStrategy;
  vtkSmartPointer<vtkClustering2DLayoutStrategy>   Clustering2DStrategy;
  vtkSmartPointer<vtkCommunity2DLayoutStrategy>    Community2DStrategy;
  vtkSmartPointer<vtkFast2DLayoutStrategy>         Fast2DStrategy;
  vtkSmartPointer<vtkCircularLayoutStrategy>       CircularStrategy;
  vtkSmartPointer<vtkPassThroughLayoutStrategy>    PassThroughStrategy;

};

#endif

