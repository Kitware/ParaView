// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef FEDataStructures_h
#define FEDataStructures_h

#include <array>
#include <vector>

class AMR
{
public:
  // on each MPI proc the coarse grid is 1 first AMR level cell in the i-dir, 1 first AMR level
  // cell in the j-dir, and numberOfAMRLevels first AMR level cells in the k-dir.
  // The grid gets finer as we go in the k-dir.
  AMR(int numberOfAMRLevels, int myRank, int numRanks);

  std::array<int, 6> GetLevelIndices(int level);
  std::array<double, 3> GetLevelOrigin(int level);

  int NumberOfAMRLevels;

  std::vector<std::array<int, 6>> LevelIndices; // inclusive min and max for point indices
  std::vector<int> CellsPerLevel;
  std::vector<int> BlockId; // We only have one child block under each parent block
  std::vector<std::array<double, 3>> LevelOrigin;
};

#endif
