/*=========================================================================

  Program:   ParaView
  Plugin:    NodeEditor

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  ParaViewPluginsNodeEditor - BSD 3-Clause License - Copyright (C) 2021 Jonas Lukasczyk

  See the Copyright.txt file provided
  with ParaViewPluginsNodeEditor for license information.
-------------------------------------------------------------------------*/

#ifndef pqNodeEditorUtils_h
#define pqNodeEditorUtils_h

#include <QColor>
#include <QObject>

class pqProxy;

namespace pqNodeEditorUtils
{
namespace CONSTS
{
constexpr int NODE_WIDTH = 300;
constexpr int NODE_BORDER_WIDTH = 3;
constexpr int NODE_BORDER_RADIUS = 6;
constexpr int NODE_FONT_SIZE = 13;
constexpr int PORT_RADIUS = 8;
constexpr int PORT_HEIGHT = 20; // radius + width + padding
constexpr int EDGE_WIDTH = 4;
constexpr int EDGE_OUTLINE = 1;
const QColor COLOR_DARK_GREEN = QColor("#078a17");
const QColor COLOR_SWEET_GREEN = QColor("#3fb55d");
const QColor COLOR_DARK_ORANGE = QColor("#e9783d"); // complement to blue

constexpr int NODE_LAYER = 10;
constexpr int EDGE_LAYER = 20;
constexpr int PORT_LAYER = 30;
constexpr int VIEW_NODE_LAYER = 50;
constexpr int FOREGROUND_LAYER = 40;

constexpr unsigned long DOUBLE_CLICK_DELAY = 2.5e8;
};

template <typename F>
/**
 * Intercept all events from a particular QObject and process them using the
 * given @c functor. This is usually used with the QObjects::installEventFilter()
 * function.
 */
class Interceptor final : public QObject
{
public:
  /**
   * Create an Interceptor that process all events of @c parent using @c functor.
   */
  Interceptor(QObject* parent, F fn)
    : QObject(parent)
    , functor(fn)
  {
  }
  ~Interceptor() = default;

protected:
  /**
   * Filters events if this object has been installed as an event filter for the watched object.
   */
  bool eventFilter(QObject* object, QEvent* event) { return this->functor(object, event); }

  F functor;
};

/**
 * Create a new Interceptor instance.
 */
template <typename F>
Interceptor<F>* createInterceptor(QObject* parent, F functor)
{
  return new Interceptor<F>(parent, functor);
};

int getID(pqProxy* proxy);

std::string getLabel(pqProxy* proxy);

//@{
/**
 * Utility function for custom double click capabilities.
 *
 * XXX: One could investiguate if it's possible / worth it to use
 * Qt capabilities for handling double click event in our case.
 */
unsigned long getTimeDelta();
bool isDoubleClick();
//@}
};

#endif // pqNodeEditorUtils_h
