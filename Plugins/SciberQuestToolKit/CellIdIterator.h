/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __CellIdIterator_h
#define __CellIdIterator_h

/// cell id iterator
/**
Iterate over an inclusive range of cell ids.
*/
class CellIdIterator
{
public:
  CellIdIterator();
  CellIdIterator(int startId, int endId);
  CellIdIterator(const CellIdIterator &other);
  virtual ~CellIdIterator(){}

  // copy
  virtual CellIdIterator &operator=(const CellIdIterator &other);

  // initialize
  virtual void SetStartId(int id){ this->StartId=id; }
  virtual void SetEndId(int id){ this->EndId=id; }
  // begin
  virtual void Reset(){ this->Id=this->StartId; }
  // validate
  virtual int Good(){ return this->Id<=this->EndId; }
  // access
  virtual int Index(){ return this->Id; }
  virtual int operator*(){ return this->Index(); }
  // increment
  virtual CellIdIterator &Increment();
  virtual CellIdIterator &operator++(){ return this->Increment(); }

  // size of the extent
  virtual int Size(){ return this->EndId-this->StartId+1; }

private:
  int StartId;
  int EndId;
  int Id;
};

//-----------------------------------------------------------------------------
inline
CellIdIterator &CellIdIterator::Increment()
{
  this->Id+=1;
  return *this;
}

#endif
