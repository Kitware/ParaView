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

#include <Utils.h>

#include <pqProxy.h>
#include <vtkSMProxy.h>

#include <QColor>
#include <iostream>

#ifdef _WIN32
#include <ciso646>
#include <cwchar>
#include <direct.h>
#include <stdint.h>
#include <time.h>
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// bool   NE::CONSTS::DEBUG = true;
bool NE::CONSTS::DEBUG = false;
int NE::CONSTS::NODE_WIDTH = 300;
int NE::CONSTS::NODE_BORDER_WIDTH = 4;
int NE::CONSTS::NODE_BORDER_RADIUS = 6;
int NE::CONSTS::NODE_DEFAULT_VERBOSITY = 1;
int NE::CONSTS::EDGE_WIDTH = 5;
QColor NE::CONSTS::COLOR_ORANGE = QColor("#e9763d");
QColor NE::CONSTS::COLOR_GREEN = QColor("#049a0a");
double NE::CONSTS::DOUBLE_CLICK_DELAY = 0.3;

void NE::log(std::string content, bool force)
{
  if (!force && !NE::CONSTS::DEBUG)
    return;

  std::cout << content << std::endl;
};

int NE::getID(pqProxy* proxy)
{
  if (proxy == nullptr)
    return -1;
  auto smProxy = proxy->getProxy();
  if (smProxy == nullptr)
    return -1;
  return smProxy->GetGlobalID();
};

std::string NE::getLabel(pqProxy* proxy)
{
  if (!proxy)
    return "PROXY IS NULL";

  return proxy->getSMName().toStdString() + "<" + std::to_string(NE::getID(proxy)) + ">";
};

double NE::getTimeStamp()
{
#ifdef _WIN32
  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);

  LARGE_INTEGER temp;
  QueryPerformanceCounter(&temp);

  return (double)temp.QuadPart / frequency.QuadPart;
#endif

#ifdef __APPLE__
  struct timeval stamp;
  gettimeofday(&stamp, NULL);
  return (stamp.tv_sec * 1000000 + stamp.tv_usec) / 1000000.0;
#endif

#ifdef __unix__
  struct timeval stamp;
  gettimeofday(&stamp, NULL);
  return (stamp.tv_sec * 1000000 + stamp.tv_usec) / 1000000.0;
#endif
};

double t0{ 0 };
double NE::getTimeDelta()
{
  double t1 = NE::getTimeStamp();
  double delta = t1 - t0;
  t0 = t1;
  return delta;
};

bool NE::isDoubleClick()
{
  return NE::getTimeDelta() < NE::CONSTS::DOUBLE_CLICK_DELAY;
};
