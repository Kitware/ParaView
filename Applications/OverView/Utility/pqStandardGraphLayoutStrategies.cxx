/*=========================================================================

   Program: ParaView
   Module:    pqStandardGraphLayoutStrategies.cxx

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

#include "pqStandardGraphLayoutStrategies.h"

#include "vtkRandomLayoutStrategy.h"
#include "vtkForceDirectedLayoutStrategy.h"
#include "vtkSimple2DLayoutStrategy.h"
#include "vtkClustering2DLayoutStrategy.h"
#include "vtkCommunity2DLayoutStrategy.h"
#include "vtkFast2DLayoutStrategy.h"
#include "vtkCircularLayoutStrategy.h"
#include "vtkPassThroughLayoutStrategy.h"

pqStandardGraphLayoutStrategies::pqStandardGraphLayoutStrategies(QObject* o)
  : QObject(o)
{
  this->RandomStrategy         = vtkSmartPointer<vtkRandomLayoutStrategy>::New();
  this->Simple2DStrategy       = vtkSmartPointer<vtkSimple2DLayoutStrategy>::New();
  this->Clustering2DStrategy   = vtkSmartPointer<vtkClustering2DLayoutStrategy>::New();
  this->Community2DStrategy    = vtkSmartPointer<vtkCommunity2DLayoutStrategy>::New();
  this->Fast2DStrategy         = vtkSmartPointer<vtkFast2DLayoutStrategy>::New();
  this->ForceDirectedStrategy  = vtkSmartPointer<vtkForceDirectedLayoutStrategy>::New();
  this->CircularStrategy       = vtkSmartPointer<vtkCircularLayoutStrategy>::New();
  this->PassThroughStrategy    = vtkSmartPointer<vtkPassThroughLayoutStrategy>::New();
}

pqStandardGraphLayoutStrategies::~pqStandardGraphLayoutStrategies()
{
}

QStringList pqStandardGraphLayoutStrategies::graphLayoutStrategies() const
{
  return QStringList() <<
  "Random" <<
  "ForceDirected" <<
  "Simple2D" <<
  "Clustering2D" <<
  "Community2D" <<
  "Fast2D" <<
  "Circular" <<
  "None"
  ;
}

vtkGraphLayoutStrategy* pqStandardGraphLayoutStrategies::getGraphLayoutStrategy(const QString& layoutStrategy)
{
  if(layoutStrategy == "Random")
    {
    return this->RandomStrategy;
    }
  else if(layoutStrategy == "ForceDirected")
    {
    return this->ForceDirectedStrategy;
    }
  else if(layoutStrategy == "Simple2D")
    {
    return this->Simple2DStrategy;
    }
  else if(layoutStrategy == "Clustering2D")
    {
    return this->Clustering2DStrategy;
    }
  else if(layoutStrategy == "Community2D")
    {
    return this->Community2DStrategy;
    }
  else if(layoutStrategy == "Fast2D")
    {
    return this->Fast2DStrategy;
    }
  else if(layoutStrategy == "Circular")
    {
    return this->CircularStrategy;
    }
  else if(layoutStrategy == "None")
    {
    return this->PassThroughStrategy;
    }

  return NULL;
}
