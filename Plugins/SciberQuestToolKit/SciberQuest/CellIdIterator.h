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
#ifndef CellIdIterator_h
#define CellIdIterator_h

#include <cstdlib> // for size_t

/// cell id iterator
/**
Iterate over an inclusive range of cell ids.
*/
class CellIdIterator
{
public:
  CellIdIterator();
  CellIdIterator(size_t startId, size_t endId);
  CellIdIterator(const CellIdIterator &other);
  virtual ~CellIdIterator(){}

  // copy
  virtual CellIdIterator &operator=(const CellIdIterator &other);

  // initialize
  virtual void SetStartId(size_t id){ this->StartId=id; }
  virtual void SetEndId(size_t id){ this->EndId=id; }
  // begin
  virtual void Reset(){ this->Id=this->StartId; }
  // validate
  virtual int Good(){ return this->Id<=this->EndId; }
  // access
  virtual size_t Index(){ return this->Id; }
  virtual size_t operator*(){ return this->Index(); }
  // increment
  virtual CellIdIterator &Increment();
  virtual CellIdIterator &operator++(){ return this->Increment(); }

  // size of the extent
  virtual size_t Size(){ return this->EndId-this->StartId+1; }

private:
  size_t StartId;
  size_t EndId;
  size_t Id;
};

//-----------------------------------------------------------------------------
inline
CellIdIterator &CellIdIterator::Increment()
{
  this->Id+=1;
  return *this;
}

#endif

// VTK-HeaderTest-Exclude: CellIdIterator.h
