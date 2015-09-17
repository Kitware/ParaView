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
#ifndef SimpleTerminationCondition_h
#define SimpleTerminationCondition_h

#include "TerminationCondition.h"

// The open termination condition asigns a unique color
// to permutation of surfaces and a noise category. Noise
// include the termnination cases problem domain, field null,
// short integration.
class SimpleTerminationCondition : public TerminationCondition
{
public:
  // Description:
  // Build the color mapper with the folowing scheme:
  //
  // 0   -> problem domain
  // 1   -> s1
  //    ...
  // n   -> sn
  // n+1 -> noise=(field null,short integration)
  virtual void InitializeColorMapper();

  // Description:
  // Return the indentifier for the special termination cases.
  virtual int GetProblemDomainSurfaceId(){ return 0; };
  virtual int GetFieldNullId(){ return this->Surfaces.size()+1; }
  virtual int GetShortIntegrationId(){ return this->Surfaces.size()+1; }
};

#endif

// VTK-HeaderTest-Exclude: SimpleTerminationCondition.h
