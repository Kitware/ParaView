
#ifndef vtkVisItDatabaseBridgeTypes_h
#define vtkVisItDatabaseBridgeTypes_h




#include "avtSILCollection.h"
// These are used to give a catagory to entries we have
// added to the SIL that VisIt doesn't currently support.
enum {
  SIL_USERD_MESH=SIL_USERD+1,
  SIL_USERD_ARRAY,
  SIL_USERD_EXPRESSION
};

// Used to idnetify what operation for a given SIL
// should be applied.
enum {
  SIL_SET_OP_USE_DOMAIN=0,
  SIL_SET_OP_INTERSECT_BLOCKS,
  SIL_SET_OP_INTERSECT_ASSEMBLIES,
  SIL_SET_OP_USE_MATERIALS,
  SIL_SET_OP_INTERSECT_MATERIALS,
  SIL_SET_OP_UNION_MATERIALS
};

// Used to classify edges in a SIL. The "structural" edge
// describe the user view of the SIL. The "domain link" edges
// describe dependencies between vertices who's role is "domain".
// This is used to facilitate subsetting.
enum {
  SIL_EDGE_STRUCTURAL=0,
  SIL_EDGE_LINK
};


#endif
