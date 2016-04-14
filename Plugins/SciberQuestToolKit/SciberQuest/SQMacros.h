/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef SQMacros_h
#define SQMacros_h

#include <iostream> // for endl

#define safeio(s) (s?s:"NULL")

#define sqErrorMacro(os,estr)                            \
    os                                                   \
      << "Error in:" << std::endl                        \
      << __FILE__ << ", line " << __LINE__ << std::endl  \
      << "" estr << std::endl;

#define sqWarningMacro(os,estr)                          \
    os                                                   \
      << "Warning in:" << std::endl                      \
      << __FILE__ << ", line " << __LINE__ << std::endl  \
      << "" estr << std::endl;

#define SafeDelete(a)\
  if (a)\
    {\
    a->Delete();\
    }

// The vtkFloatTemplateMacro is like vtkTemplateMacro but
// expands only to floating point types.

#define vtkFloatTemplateMacroCase(typeN, type, call) \
  case typeN: { typedef type VTK_TT; call; }; break

#define vtkFloatTemplateMacro(call) \
  vtkFloatTemplateMacroCase(VTK_DOUBLE, double, call); \
  vtkFloatTemplateMacroCase(VTK_FLOAT, float, call);

#if defined(_MSC_VER)
// TODO - move this to the file where it ocurs
#pragma warning(disable : 4800) // int to bool performance
#endif
#endif

// VTK-HeaderTest-Exclude: SQMacros.h
