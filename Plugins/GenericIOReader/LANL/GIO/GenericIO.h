/*
 *                    Copyright (C) 2015, UChicago Argonne, LLC
 *                               All Rights Reserved
 *
 *                               Generic IO (ANL-15-066)
 *                     Hal Finkel, Argonne National Laboratory
 *
 *                              OPEN SOURCE LICENSE
 *
 * Under the terms of Contract No. DE-AC02-06CH11357 with UChicago Argonne,
 * LLC, the U.S. Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. Neither the names of UChicago Argonne, LLC or the Department of Energy
 *      nor the names of its contributors may be used to endorse or promote
 *      products derived from this software without specific prior written
 *      permission.
 *
 * *****************************************************************************
 *
 *                                  DISCLAIMER
 * THE SOFTWARE IS SUPPLIED “AS IS” WITHOUT WARRANTY OF ANY KIND.  NEITHER THE
 * UNTED STATES GOVERNMENT, NOR THE UNITED STATES DEPARTMENT OF ENERGY, NOR
 * UCHICAGO ARGONNE, LLC, NOR ANY OF THEIR EMPLOYEES, MAKES ANY WARRANTY,
 * EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL LIABILITY OR RESPONSIBILITY FOR THE
 * ACCURACY, COMPLETENESS, OR USEFULNESS OF ANY INFORMATION, DATA, APPARATUS,
 * PRODUCT, OR PROCESS DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT INFRINGE
 * PRIVATELY OWNED RIGHTS.
 *
 * *****************************************************************************
 */

#ifndef GENERICIO_H
#define GENERICIO_H

#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdint.h>
#include <string>
#include <vector>

#ifndef LANL_GENERICIO_NO_MPI
#include <mpi.h>
#else
#include <fstream>
#endif

#ifndef _WIN32
#include <unistd.h>
#endif

namespace lanl
{

namespace gio
{

class GenericFileIO
{
public:
  virtual ~GenericFileIO() {}

public:
  virtual void open(const std::string& FN, bool ForReading = false) = 0;
  virtual void setSize(size_t sz) = 0;
  virtual void read(void* buf, size_t count, off_t offset, const std::string& D) = 0;
  virtual void write(const void* buf, size_t count, off_t offset, const std::string& D) = 0;

protected:
  std::string FileName;
};

#ifndef LANL_GENERICIO_NO_MPI
class GenericFileIO_MPI : public GenericFileIO
{
public:
  GenericFileIO_MPI(const MPI_Comm& C)
    : FH(MPI_FILE_NULL)
    , Comm(C)
  {
  }
  virtual ~GenericFileIO_MPI();

public:
  virtual void open(const std::string& FN, bool ForReading = false);
  virtual void setSize(size_t sz);
  virtual void read(void* buf, size_t count, off_t offset, const std::string& D);
  virtual void write(const void* buf, size_t count, off_t offset, const std::string& D);

protected:
  MPI_File FH;
  MPI_Comm Comm;
};

class GenericFileIO_MPICollective : public GenericFileIO_MPI
{
public:
  GenericFileIO_MPICollective(const MPI_Comm& C)
    : GenericFileIO_MPI(C)
  {
  }

public:
  void read(void* buf, size_t count, off_t offset, const std::string& D);
  void write(const void* buf, size_t count, off_t offset, const std::string& D);
};
#endif

class GenericFileIO_POSIX : public GenericFileIO
{
public:
  GenericFileIO_POSIX()
    : FH(-1)
  {
  }
  ~GenericFileIO_POSIX();

public:
  void open(const std::string& FN, bool ForReading = false);
  void setSize(size_t sz);
  void read(void* buf, size_t count, off_t offset, const std::string& D);
  void write(const void* buf, size_t count, off_t offset, const std::string& D);

protected:
  int FH;
};

class GenericIO
{
public:
  enum VariableFlags
  {
    VarHasExtraSpace = (1 << 0), // Note that this flag indicates that the
                                 // extra space is available, but the GenericIO
                                 // implementation is required to
                                 // preserve its contents.
    VarIsPhysCoordX = (1 << 1),
    VarIsPhysCoordY = (1 << 2),
    VarIsPhysCoordZ = (1 << 3),
    VarMaybePhysGhost = (1 << 4)
  };

  struct VariableInfo
  {
    VariableInfo(
      const std::string& N, std::size_t S, bool IF, bool IS, bool PCX, bool PCY, bool PCZ, bool PG)
      : Name(N)
      , Size(S)
      , IsFloat(IF)
      , IsSigned(IS)
      , IsPhysCoordX(PCX)
      , IsPhysCoordY(PCY)
      , IsPhysCoordZ(PCZ)
      , MaybePhysGhost(PG)
    {
    }

    std::string Name;
    std::size_t Size;
    bool IsFloat;
    bool IsSigned;
    bool IsPhysCoordX, IsPhysCoordY, IsPhysCoordZ;
    bool MaybePhysGhost;
  };

public:
  struct Variable
  {
    template <typename T>
    Variable(const std::string& N, T* D, unsigned Flags = 0)
      : Name(N)
      , Size(sizeof(T))
      , IsFloat(!std::numeric_limits<T>::is_integer)
      , IsSigned(std::numeric_limits<T>::is_signed)
      , Data((void*)D)
      , HasExtraSpace((Flags & VarHasExtraSpace) != 0)
      , IsPhysCoordX((Flags & VarIsPhysCoordX) != 0)
      , IsPhysCoordY((Flags & VarIsPhysCoordY) != 0)
      , IsPhysCoordZ((Flags & VarIsPhysCoordZ) != 0)
      , MaybePhysGhost((Flags & VarMaybePhysGhost) != 0)
    {
    }

    Variable(const VariableInfo& VI, void* D, unsigned Flags = 0)
      : Name(VI.Name)
      , Size(VI.Size)
      , IsFloat(VI.IsFloat)
      , IsSigned(VI.IsSigned)
      , Data(D)
      , HasExtraSpace((Flags & VarHasExtraSpace) != 0)
      , IsPhysCoordX(((Flags & VarIsPhysCoordX) != 0) || VI.IsPhysCoordX)
      , IsPhysCoordY(((Flags & VarIsPhysCoordY) != 0) || VI.IsPhysCoordY)
      , IsPhysCoordZ(((Flags & VarIsPhysCoordZ) != 0) || VI.IsPhysCoordZ)
      , MaybePhysGhost(((Flags & VarMaybePhysGhost) != 0) || VI.MaybePhysGhost)
    {
    }

    std::string Name;
    std::size_t Size;
    bool IsFloat;
    bool IsSigned;
    void* Data;
    bool HasExtraSpace;
    bool IsPhysCoordX, IsPhysCoordY, IsPhysCoordZ;
    bool MaybePhysGhost;
  };

public:
  enum FileIO
  {
    FileIOMPI,
    FileIOPOSIX,
    FileIOMPICollective
  };

#ifndef LANL_GENERICIO_NO_MPI
  GenericIO(const MPI_Comm& C, const std::string& FN, unsigned FIOT = -1)
    : NElems(0)
    , FileIOType(FIOT == (unsigned)-1 ? DefaultFileIOType : FIOT)
    , Partition(DefaultPartition)
    , Comm(C)
    , FileName(FN)
    , Redistributing(false)
    , DisableCollErrChecking(false)
    , SplitComm(MPI_COMM_NULL)
  {
    std::fill(PhysOrigin, PhysOrigin + 3, 0.0);
    std::fill(PhysScale, PhysScale + 3, 0.0);
  }
#else
  GenericIO(const std::string& FN, unsigned FIOT = -1)
    : NElems(0)
    , FileIOType(FIOT == (unsigned)-1 ? DefaultFileIOType : FIOT)
    , Partition(DefaultPartition)
    , FileName(FN)
    , Redistributing(false)
    , DisableCollErrChecking(false)
  {
    std::fill(PhysOrigin, PhysOrigin + 3, 0.0);
    std::fill(PhysScale, PhysScale + 3, 0.0);
  }
#endif

  ~GenericIO()
  {
    close();

#ifndef LANL_GENERICIO_NO_MPI
    if (SplitComm != MPI_COMM_NULL)
      MPI_Comm_free(&SplitComm);
#endif
  }

public:
  std::size_t requestedExtraSpace() const { return 8; }

  void setNumElems(std::size_t E)
  {
    NElems = E;

#ifndef LANL_GENERICIO_NO_MPI
    int IsLarge = E >= CollectiveMPIIOThreshold;
    int AllIsLarge;
    MPI_Allreduce(&IsLarge, &AllIsLarge, 1, MPI_INT, MPI_SUM, Comm);
    if (!AllIsLarge)
      FileIOType = FileIOMPICollective;
#endif
  }

  void setPhysOrigin(double O, int Dim = -1)
  {
    if (Dim >= 0)
      PhysOrigin[Dim] = O;
    else
      std::fill(PhysOrigin, PhysOrigin + 3, O);
  }

  void setPhysScale(double S, int Dim = -1)
  {
    if (Dim >= 0)
      PhysScale[Dim] = S;
    else
      std::fill(PhysScale, PhysScale + 3, S);
  }

  template <typename T>
  void addVariable(const std::string& Name, T* Data, unsigned Flags = 0)
  {
    Vars.push_back(Variable(Name, Data, Flags));
  }

  template <typename T, typename A>
  void addVariable(const std::string& Name, std::vector<T, A>& Data, unsigned Flags = 0)
  {
    T* D = Data.empty() ? 0 : &Data[0];
    addVariable(Name, D, Flags);
  }

  void addVariable(const VariableInfo& VI, void* Data, unsigned Flags = 0)
  {
    Vars.push_back(Variable(VI, Data, Flags));
  }

#ifndef LANL_GENERICIO_NO_MPI
  // Writing
  void write();
#endif

  enum MismatchBehavior
  {
    MismatchAllowed,
    MismatchDisallowed,
    MismatchRedistribute
  };

  // Reading
  void openAndReadHeader(
    MismatchBehavior MB = MismatchDisallowed, int EffRank = -1, bool CheckPartMap = true);

  int readNRanks();
  void readDims(int Dims[3]);

  // Note: For partitioned inputs, this returns -1.
  uint64_t readTotalNumElems();

  void readPhysOrigin(double Origin[3]);
  void readPhysScale(double Scale[3]);

  void clearVariables() { this->Vars.clear(); };

  int getNumberOfVariables() { return static_cast<int>(this->Vars.size()); };

  void getVariableInfo(std::vector<VariableInfo>& VI);

  std::size_t readNumElems(int EffRank = -1);
  void readCoords(int Coords[3], int EffRank = -1);
  int readGlobalRankNumber(int EffRank = -1);

  void readData(int EffRank = -1, bool PrintStats = true, bool CollStats = true);
  void readDataSection(size_t readOffset, size_t readNumRows, int EffRank = -1,
    bool PrintStats = true, bool CollStats = true);

  void getSourceRanks(std::vector<int>& SR);

  template <typename T>
  T getValue(int variableID, size_t index)
  {
    T* dataPtr = static_cast<T*>(Vars[variableID].Data);
    return dataPtr[index];
  }

  void close() { FH.close(); }

  void setPartition(int P) { Partition = P; }

  static void setDefaultFileIOType(unsigned FIOT) { DefaultFileIOType = FIOT; }

  static void setDefaultPartition(int P) { DefaultPartition = P; }

  static void setNaturalDefaultPartition();

  static void setDefaultShouldCompress(bool C) { DefaultShouldCompress = C; }

#ifndef LANL_GENERICIO_NO_MPI
  static void setCollectiveMPIIOThreshold(std::size_t T)
  {
#ifndef GENERICIO_NO_NEVER_USE_COLLECTIVE_IO
    CollectiveMPIIOThreshold = T;
#endif
  }
#endif

private:
// Implementation functions templated on the Endianness of the underlying
// data.

#ifndef LANL_GENERICIO_NO_MPI
  template <bool IsBigEndian>
  void write();
#endif

  template <bool IsBigEndian>
  void readHeaderLeader(void* GHPtr, MismatchBehavior MB, int Rank, int NRanks, int SplitNRanks,
    std::string& LocalFileName, uint64_t& HeaderSize, std::vector<char>& Header);

  template <bool IsBigEndian>
  int readNRanks();

  template <bool IsBigEndian>
  void readDims(int Dims[3]);

  template <bool IsBigEndian>
  uint64_t readTotalNumElems();

  template <bool IsBigEndian>
  void readPhysOrigin(double Origin[3]);

  template <bool IsBigEndian>
  void readPhysScale(double Scale[3]);

  template <bool IsBigEndian>
  int readGlobalRankNumber(int EffRank);

  template <bool IsBigEndian>
  size_t readNumElems(int EffRank);

  template <bool IsBigEndian>
  void readCoords(int Coords[3], int EffRank);

  void readData(int EffRank, size_t RowOffset, int Rank, uint64_t& TotalReadSize, int NErrs[3]);

  template <bool IsBigEndian>
  void readData(int EffRank, size_t RowOffset, int Rank, uint64_t& TotalReadSize, int NErrs[3]);

  template <bool IsBigEndian>
  void readDataSection(size_t readOffset, size_t readNumRows, int EffRank, size_t RowOffset,
    int Rank, uint64_t& TotalReadSize, int NErrs[3]);

  void readDataSection(size_t readOffset, size_t readNumRows, int EffRank, size_t RowOffset,
    int Rank, uint64_t& TotalReadSize, int NErrs[3]);

  template <bool IsBigEndian>
  void getVariableInfo(std::vector<VariableInfo>& VI);

protected:
  std::vector<Variable> Vars;
  std::size_t NElems;

  double PhysOrigin[3], PhysScale[3];

  unsigned FileIOType;
  int Partition;
#ifndef LANL_GENERICIO_NO_MPI
  MPI_Comm Comm;
#endif
  std::string FileName;

  static unsigned DefaultFileIOType;
  static int DefaultPartition;
  static bool DefaultShouldCompress;

#ifndef LANL_GENERICIO_NO_MPI
  static std::size_t CollectiveMPIIOThreshold;
#endif

  // When redistributing, the rank blocks which this process should read.
  bool Redistributing, DisableCollErrChecking;
  std::vector<int> SourceRanks;

  std::vector<int> RankMap;
#ifndef LANL_GENERICIO_NO_MPI
  MPI_Comm SplitComm;
#endif
  std::string OpenFileName;

  // This reference counting mechanism allows the the GenericIO class
  // to be used in a cursor mode. To do this, make a copy of the class
  // after reading the header but prior to adding the variables.
  struct FHManager
  {
    FHManager()
      : CountedFH(0)
    {
      allocate();
    }

    FHManager(const FHManager& F)
    {
      CountedFH = F.CountedFH;
      CountedFH->Cnt += 1;
    }

    ~FHManager() { close(); }

    GenericFileIO*& get()
    {
      if (!CountedFH)
        allocate();

      return CountedFH->GFIO;
    }

    std::vector<char>& getHeaderCache()
    {
      if (!CountedFH)
        allocate();

      return CountedFH->HeaderCache;
    }

    bool isBigEndian() { return CountedFH ? CountedFH->IsBigEndian : false; }

    void setIsBigEndian(bool isBE) { CountedFH->IsBigEndian = isBE; }

    void allocate()
    {
      close();
      CountedFH = new FHWCnt;
    };

    void close()
    {
      if (CountedFH && CountedFH->Cnt == 1)
        delete CountedFH;
      else if (CountedFH)
        CountedFH->Cnt -= 1;

      CountedFH = 0;
    }

    struct FHWCnt
    {
      FHWCnt()
        : GFIO(0)
        , Cnt(1)
        , IsBigEndian(false)
      {
      }

      ~FHWCnt() { close(); }

    protected:
      void close()
      {
        delete GFIO;
        GFIO = 0;
      }

    public:
      GenericFileIO* GFIO;
      size_t Cnt;

      // Used for reading
      std::vector<char> HeaderCache;
      bool IsBigEndian;
    };

    FHWCnt* CountedFH;
  } FH;
};

} /* END namespace lanl */
} /* END namespace cosmotk */
#endif // GENERICIO_H
