/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __BinaryStream_hxx
#define __BinaryStream_hxx

#include "SharedArray.hxx"

#include <cstdlib>
#include <string>
#include <map>
#include <vector>

/// Serialize objects into a binary stream.
class BinaryStream
{
public:
  BinaryStream();
  BinaryStream(const BinaryStream &s);
  ~BinaryStream();
  const BinaryStream &operator=(const BinaryStream &other);

  /**
  Release all resources, set to a uninitialized state.
  */
  void Clear();

  /**
  Alolocate nBytes for the stream.
  */
  void Resize(size_t nBytes);

  /**
  Add nBytes to the stream.
  */
  void Grow(size_t nBytes);

  /**
  Get apointer to the stream internal representation.
  */
  char *GetData(){ return this->Data; }

  /**
  Get the size of the data in the stream.
  */
  size_t GetSize(){ return this->DataP-this->Data; }

  /**
  Set the stream position to the head of the strem
  */
  void Rewind()
    {
    this->DataP=this->Data;
    }

  /**
  Insert/Extract to/from the stream.
  */
  template <typename T> void Pack(T *val);
  template <typename T> void Pack(T val);
  template <typename T> void UnPack(T &val);
  template <typename T> void Pack(const T *val, size_t n);
  template <typename T> void UnPack(T *val, size_t n);

  // specializations
  void Pack(const std::string &str);
  void UnPack(std::string &str);
  template<typename T> void Pack(std::vector<T> &v);
  template<typename T> void UnPack(std::vector<T> &v);
  template<typename T> void Pack(SharedArray<T> &v);
  template<typename T> void UnPack(SharedArray<T> &v);
  void Pack(std::map<std::string,int> &m);
  void UnPack(std::map<std::string,int> &m);

private:
  size_t Size;
  char *Data;
  char *DataP;
};


//-----------------------------------------------------------------------------
inline
BinaryStream::BinaryStream()
     :
  Size(0),
  Data(0),
  DataP(0)
    {
    // this->Size=64;
    // this->Data=(char*)malloc(this->Size);
    // this->DataP=this->Data;
    // for (int i=0; i<this->Size; ++i)
    //   {
    //   this->Data[i]='\0';
    //   }
    }

//-----------------------------------------------------------------------------
inline
BinaryStream::~BinaryStream()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
inline
BinaryStream::BinaryStream(const BinaryStream &other)
{
  *this=other;
}

//-----------------------------------------------------------------------------
inline
const BinaryStream &BinaryStream::operator=(const BinaryStream &other)
{
  if (&other==this) return *this;

  this->Clear();

  this->Resize(other.Size);

  this->DataP=other.DataP;

  for (size_t i=0; i<other.Size; ++i)
    {
    this->Data[i]=other.Data[i];
    }

  return *this;
}

//-----------------------------------------------------------------------------
inline
void BinaryStream::Clear()
{
  free(this->Data);
  this->Data=0;
  this->DataP=0;
  this->Size=0;
}

//-----------------------------------------------------------------------------
inline
void BinaryStream::Resize(size_t nBytes)
{
  char *origData=this->Data;
  this->Data=(char *)realloc(this->Data,nBytes);

  // update the stream pointer
  if (this->Data!=origData)
    {
    this->DataP=this->Data+(this->DataP-origData);
    }

  this->Size=nBytes;
}

//-----------------------------------------------------------------------------
inline
void BinaryStream::Grow(size_t nBytes)
{
  this->Resize(this->Size+nBytes);
}

//-----------------------------------------------------------------------------
template <typename T>
void BinaryStream::Pack(T *val)
{
  (void)val;
  std::cerr << "Error: Packing a pointer." << std::endl;
}

//-----------------------------------------------------------------------------
template <typename T>
void BinaryStream::Pack(T val)
{
  this->Grow(sizeof(T));

  *((T *)this->DataP)=val;

  this->DataP+=sizeof(T);
}

//-----------------------------------------------------------------------------
template <typename T>
void BinaryStream::UnPack(T &val)
{
  val=*((T *)this->DataP);

  this->DataP+=sizeof(T);
}

//-----------------------------------------------------------------------------
template <typename T>
void BinaryStream::Pack(const T *val, size_t n)
{
  size_t nBytes=n*sizeof(T);

  this->Grow(nBytes);

  for (size_t i=0; i<n; ++i,this->DataP+=sizeof(T))
    {
    *((T *)this->DataP)=val[i];
    }
}

//-----------------------------------------------------------------------------
template <typename T>
void BinaryStream::UnPack(T *val, size_t n)
{
  for (size_t i=0; i<n; ++i, this->DataP+=sizeof(T))
    {
    val[i]=*((T *)this->DataP);
    }
}

//-----------------------------------------------------------------------------
inline
void BinaryStream::Pack(const std::string &str)
{
  size_t strLen=str.size();
  this->Pack(strLen);
  this->Pack(str.c_str(),strLen);
}

//-----------------------------------------------------------------------------
inline
void BinaryStream::UnPack(std::string &str)
{
  size_t strLen=0;
  this->UnPack(strLen);

  str.resize(strLen);
  str.assign(this->DataP,strLen);

  this->DataP+=strLen;
}

//-----------------------------------------------------------------------------
template<typename T>
void BinaryStream::Pack(std::vector<T> &v)
{
  const size_t vLen=v.size();
  this->Pack(vLen);
  this->Pack(&v[0],vLen);
}

//-----------------------------------------------------------------------------
template<typename T>
void BinaryStream::UnPack(std::vector<T> &v)
{
  size_t vLen;
  this->UnPack(vLen);
  v.resize(vLen);
  this->UnPack(&v[0],vLen);
}

//-----------------------------------------------------------------------------
template<typename T>
void BinaryStream::Pack(SharedArray<T> &v)
{
  const size_t vLen=v.Size();
  this->Pack(vLen);
  this->Pack(v.GetPointer(),vLen);
}

//-----------------------------------------------------------------------------
template<typename T>
void BinaryStream::UnPack(SharedArray<T> &v)
{
  size_t vLen;
  this->UnPack(vLen);
  v.Resize(vLen);
  this->UnPack(v.GetPointer(),vLen);
}

//-----------------------------------------------------------------------------
inline
void BinaryStream::Pack(std::map<std::string,int> &m)
{
  size_t mLen=m.size();
  this->Pack(mLen);

  std::map<std::string,int>::iterator it=m.begin();
  std::map<std::string,int>::iterator end=m.end();
  for (;it!=end; ++it)
    {
    this->Pack(it->first);
    this->Pack(it->second);
    }
}

//-----------------------------------------------------------------------------
inline
void BinaryStream::UnPack(std::map<std::string,int> &m)
{
  size_t mLen=0;
  this->UnPack(mLen);

  for (size_t i=0; i<mLen; ++i)
    {
    std::string key;
    this->UnPack(key);
    int val;
    this->UnPack(val);

    m[key]=val;
    }
}

#endif
