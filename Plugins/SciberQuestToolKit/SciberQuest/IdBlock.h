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
#ifndef IdBlock_h
#define IdBlock_h

#include <iostream> // for ostream

/// A block of adjecent indexes uint64_to a collection (typically of cells or pouint64_ts).
class IdBlock
{
public:
  IdBlock(){ this->clear(); }
  IdBlock(unsigned long long frst){ m_data[0]=frst; m_data[1]=1; }
  IdBlock(unsigned long long frst, unsigned long long n){ m_data[0]=frst; m_data[1]=n; }
  void clear(){ m_data[0]=m_data[1]=0; }
  unsigned long long &first(){ return m_data[0]; }
  unsigned long long &size(){ return m_data[1]; }
  unsigned long long last(){ return m_data[0]+m_data[1]; }
  unsigned long long *data(){ return m_data; }
  unsigned long long dataSize(){ return 2; }
  bool contains(unsigned long long id){ return ((id>=first()) && (id<last())); }
  bool empty(){ return m_data[1]==0; }
private:
  unsigned long long m_data[2]; // id, size
private:
  friend std::ostream &operator<<(std::ostream &os, const IdBlock &b);
};

#endif

// VTK-HeaderTest-Exclude: IdBlock.h
