/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef IntersectionSet_h
#define IntersectionSet_h

#include "SQMacros.h" // for sqErrorMacro

#ifdef SQTK_WITHOUT_MPI
typedef void * MPI_Datatype;
#else
#include "SQMPICHWarningSupression.h" // for suppressing MPI warnings
#include "mpi.h" // for MPI_Datatype
#endif

#include <string> // for string

/**
/// Data associated with a stream line surface intersection set.
Data associated with a stream line surface intersection set. Streamlines
are identified by their seedpoint ids. Each streamline is treated internally
in two parts a forward running part and a backward running part, each of which
may have an intersection. Nothing says that streamline may not intersect
multiple surfaces multiple times. We use the integration time as the parameter
to decide which surface is closest to the seed point and discard others.
*/
class IntersectData
{
public:
  /// Construct in an invalid state.
  IntersectData()
    :
  seedPointId(-1),
  fwdSurfaceId(-1),
  bwdSurfaceId(-1),
  fwdIntersectTime(-1.0),
  bwdIntersectTime(-1.0)
    {
    #ifdef SQTK_WITHOUT_MPI
    sqErrorMacro(
      std::cerr,
      "This class requires MPI however it was built without MPI.");
    #endif
    }

  /// Construct in an initialized state.
  IntersectData(
    int seed, int fwdSurface, double fwdTime, int bwdSurface, double bwdTime)
    :
  seedPointId(seed),
  fwdSurfaceId(fwdSurface),
  bwdSurfaceId(bwdSurface),
  fwdIntersectTime(fwdTime),
  bwdIntersectTime(bwdTime)
    {
    #ifdef SQTK_WITHOUT_MPI
    sqErrorMacro(
      std::cerr,
      "This class requires MPI however it was built without MPI.");
    #endif
    }

  /// Copy construct
  IntersectData(const IntersectData &other);

  /// Assignment
  const IntersectData &operator=(const IntersectData &other);

  /// Create the MPI compatible description of this.
  int CommitType(MPI_Datatype *classType);

  /// Print the object state to a string.
  std::string Print();

public:
  int seedPointId;
  int fwdSurfaceId;
  int bwdSurfaceId;
  double fwdIntersectTime;
  double bwdIntersectTime;
};

/**
/// Container for data associated with a streamline surface intersection set.
Provides an interface to operate on the data associated with the intersection
set of a streamline (uniquely associated with a seed point).
*/
class IntersectionSet
{
public:
  /// Construct with id's marked invlaid.
  IntersectionSet();
  ~IntersectionSet();

  /// Copy construct
  IntersectionSet(const IntersectionSet &other);

  /// Assignment
  const IntersectionSet &operator=(const IntersectionSet &other);

  /// Set/Get seed point ids.
  void SetSeedPointId(int id){
    this->Data.seedPointId=id;
    }
  int GetSeedPointId(){
    return this->Data.seedPointId;
    }

  /// Set/Get intersection set values.
  void SetForwardIntersection(int surfaceId, double time){
    this->Data.fwdSurfaceId=surfaceId;
    this->Data.fwdIntersectTime=time;
    }
  int GetForwardIntersectionId() const {
    return this->Data.fwdSurfaceId;
    }
  void SetBackwardIntersection(int surfaceId, double time){
    this->Data.bwdSurfaceId=surfaceId;
    this->Data.bwdIntersectTime=time;
    }
  int GetBackwardIntersectionId() const {
    return this->Data.bwdSurfaceId;
    }

  /// Insert an intersection set only if its closer than the current.
  void InsertForwardIntersection(int surfaceId, double time){
    IntersectData other(-1,surfaceId,time,-1,-1.0);
    this->Reduce(other);
    }
  void InsertBackwardIntersection(int surfaceId, double time){
    IntersectData other(-1,-1,-1.0,surfaceId,time);
    this->Reduce(other);
    }

  /// Assign color based on the intersecting surface ids, and
  /// the number of surfaces. The value returned is a coordinate
  /// in a 2-D plane with 0 to nSurfaces values on each axis.
  int Color(int nSurfaces){
    int x=this->Data.fwdSurfaceId+1;
    int y=this->Data.bwdSurfaceId+1;
    return x+(nSurfaces+1)*y;
    }

  /// Synchronize intersection sets across all processes, such that
  /// all reflect the intersection set closest to this seed point.
  int AllReduce();

  /// Print object state into a string.
  std::string Print();

private:
  /// Reduce in place.
  void Reduce(IntersectData &other);

private:
  IntersectData Data;     // describes surface intersections for a seed
  MPI_Datatype DataType;  // something MPI understands
};

#endif

// VTK-HeaderTest-Exclude: IntersectionSet.h
