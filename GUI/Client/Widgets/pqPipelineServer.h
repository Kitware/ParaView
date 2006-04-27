/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineServer.h

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

/// \file pqPipelineServer.h
///
/// \date 11/16/2005

#ifndef _pqPipelineServer_h
#define _pqPipelineServer_h


#include "pqWidgetsExport.h"
#include "pqPipelineModelItem.h"

class pqMultiView;
class pqPipelineServerInternal;
class pqPipelineSource;
class pqPipelineWindow;
class pqServer;
class QWidget;
class vtkPVXMLElement;
class vtkSMProxy;


class PQWIDGETS_EXPORT pqPipelineServer : public pqPipelineModelItem
{
public:
  pqPipelineServer();
  ~pqPipelineServer();

  void SaveState(vtkPVXMLElement *root, pqMultiView *multiView);

  void SetServer(pqServer *server) {this->Server = server;}
  pqServer *GetServer() const {return this->Server;}

  /// \name Pipeline Methods
  //@{
  /// \brief
  ///   Adds a source to the object map and the source list.
  /// \param source The source object to add.
  /// \sa
  ///   pqPipelineServer::AddObject(pqPipelineSource *)
  void AddSource(pqPipelineSource *source);

  /// \brief
  ///   Adds a source to the object map.
  /// \param source The source object to add.
  /// \sa
  ///   pqPipelineServer::AddSource(pqPipelineSource *)
  void AddObject(pqPipelineSource *source);

  /// \brief
  ///   Removes a source object from the object map.
  ///
  /// If the object is on the source list, it is removed from that list
  /// as well.
  ///
  /// \param source The source object to remove.
  /// \sa
  ///   pqPipelineServer::RemoveFromSourceList(pqPipelineSource *)
  void RemoveObject(pqPipelineSource *source);

  /// \brief
  ///   Gets a source object from the object map.
  /// \param proxy The proxy pointer to look up.
  /// \return
  ///   A pointer to the object in the map or null if not present.
  pqPipelineSource *GetObject(vtkSMProxy *proxy) const;

  void ClearPipelines();
  //@}

  /// \name Source List Methods
  //@{
  int GetSourceCount() const;
  pqPipelineSource *GetSource(int index) const;
  int GetSourceIndexFor(pqPipelineSource *source) const;
  bool HasSource(pqPipelineSource *source) const;
  void AddToSourceList(pqPipelineSource *source);
  void RemoveFromSourceList(pqPipelineSource *source);
  //@}

  /// \name Window List Methods
  //@{
  int GetWindowCount() const;
  QWidget *GetWindow(int index) const;
  int GetWindowIndexFor(QWidget *window) const;
  bool HasWindow(QWidget *window) const;
  void AddToWindowList(QWidget *window);
  void RemoveFromWindowList(QWidget *window);
  //@}

private:
  void UnregisterObject(pqPipelineSource *source);

private:
  pqPipelineServerInternal *Internal;
  pqServer *Server;
};

#endif
