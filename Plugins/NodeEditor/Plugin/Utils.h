/*=========================================================================
Copyright (c) 2021, Jonas Lukasczyk
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

#pragma once

#include <QObject>

class QColor;
class pqProxy;

// forward declarations
namespace NE
{

void log(std::string content, bool force = false);
int getID(pqProxy* proxy);
std::string getLabel(pqProxy* proxy);

namespace CONSTS
{
extern bool DEBUG;
extern int NODE_WIDTH;
extern int NODE_PADDING;
extern int NODE_BORDER_WIDTH;
extern int NODE_BORDER_RADIUS;
extern int NODE_BR_PADDING;
extern int NODE_DEFAULT_VERBOSITY;
extern int EDGE_WIDTH;
extern QColor COLOR_ORANGE;
extern QColor COLOR_GREEN;
extern double DOUBLE_CLICK_DELAY;
};

template <typename F>
class Interceptor : public QObject
{
public:
  F functor;
  Interceptor(QObject* parent, F functor)
    : QObject(parent)
    , functor(functor)
  {
  }
  ~Interceptor() {}

  bool eventFilter(QObject* object, QEvent* event) { return this->functor(object, event); }
};

template <typename F>
Interceptor<F>* createInterceptor(QObject* parent, F functor)
{
  return new Interceptor<F>(parent, functor);
};

double getTimeDelta();
double getTimeStamp();

bool isDoubleClick();
}
