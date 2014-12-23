#include "Definition.h"
#include "dims-local-init.h"
#include "dims.h"

#ifdef __bgq__
#include <spi/include/kernel/location.h>
#endif

#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdio>

using namespace std;
using namespace cosmotk;

void initTopologyFromString(const string &TopologyString) {
  int tmpDims[DIMENSION];
  int nnodes;
  MPI_Comm_size(MPI_COMM_WORLD, &nnodes);

  if (TopologyString == "dynamic") {
#ifdef __bgq__
    BG_JobCoords_t job;
    if (Kernel_JobCoords(&job) != 0) {
      fprintf(stderr, "cannot use dynamic topology with subblocks!\n");
      exit(1);
    }

    std::vector<int> blk;
    blk.push_back(job.shape.a);
    blk.push_back(job.shape.b);
    blk.push_back(job.shape.c);
    blk.push_back(job.shape.d);
    blk.push_back(job.shape.e);
    blk.push_back(job.shape.core /*t*/);

    if (Kernel_GetRank() == 0)
      printf("job block ABCDET = %d, %d, %d, %d, %d, %d\n",
             blk[0], blk[1], blk[2], blk[3], blk[4], blk[5]);

    // We need to choose a topology which will combine physical partition
    // dimensions, but not split them. In between the six physical dimensions
    // we need to choose two dividing locations. Of these, we try to choose
    // the most balanced (the one which minimized the distance in between the
    // largest and smallest dimension) [algorithm suggested by V.M.].

    unsigned int md = (unsigned int) -1;
    int mdi = 0, mdj = 0, mdd1 = 1, mdd2 = 1, mdd3 = 1;

    for (int i = 0; i < 5; ++i)
    for (int j = i+1; j < 6; ++j) {
      int d1 = 1, d2 = 1, d3 = 1;
      for (int k = 0; k <= i; ++k) d1 *= blk[k];
      for (int k = i+1; k <= j; ++k) d2 *= blk[k];
      for (int k = j+1; k <= 5; ++k) d3 *= blk[k];

      int dmin = std::min(std::min(d1, d2), d3);
      int dmax = std::max(std::max(d1, d2), d3);
      unsigned int ddist = (unsigned int) (dmax - dmin);

      if (ddist < md) {
        md = ddist;
        mdi = i;
        mdj = j;
        mdd1 = d1;
        mdd2 = d2;
        mdd3 = d3;
      }
    }

    if (Kernel_GetRank() == 0)
      printf("dynamic topology: %d x %d x %d\n", mdd1, mdd2, mdd3);
    tmpDims[0] = mdd1;
    tmpDims[1] = mdd2;
    tmpDims[2] = mdd3;
#else
    fprintf(stderr, "dynamic topology is not supported!\n");
    exit(1);
#endif
  } else {
    char *tmpDimsStr = (char *)TopologyString.c_str();
    char *tmpDimsTok;
    tmpDimsTok = strtok(tmpDimsStr,"x");
    tmpDims[0] = atoi(tmpDimsTok);
    tmpDimsTok = strtok(NULL,"x");
    tmpDims[1] = atoi(tmpDimsTok);
    tmpDimsTok = strtok(NULL,"x");
    tmpDims[2] = atoi(tmpDimsTok);
  }
  MY_Dims_init_3D(nnodes, DIMENSION, tmpDims);
}
