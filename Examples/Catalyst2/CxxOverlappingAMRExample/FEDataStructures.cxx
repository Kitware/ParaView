// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "FEDataStructures.h"
#include <math.h>

AMR::AMR(int numberOfAMRLevels, int myRank, int numRanks)
  : NumberOfAMRLevels(numberOfAMRLevels)
{
  int numberOfCells = 0;
  for (int level = 0; level < numberOfAMRLevels; level++)
  {
    std::array<int, 6> levelIndices;
    levelIndices[0] = 0;                                            // smallest i
    levelIndices[1] = std::pow(2, level);                           // largest i
    levelIndices[2] = 0;                                            // smallest j
    levelIndices[3] = std::pow(2, level);                           // largest j
    levelIndices[4] = level * std::pow(2, level);                   // smallest k
    levelIndices[5] = this->NumberOfAMRLevels * std::pow(2, level); // largest k
    this->LevelIndices.push_back(levelIndices);
    int cellsPerLevel = (levelIndices[1] - levelIndices[0]) * (levelIndices[3] - levelIndices[2]) *
      (levelIndices[5] - levelIndices[4]);
    this->CellsPerLevel.push_back(cellsPerLevel);
    std::array<double, 3> levelOrigin;
    levelOrigin[0] = myRank;
    levelOrigin[1] = 0;
    levelOrigin[2] = level;
    this->LevelOrigin.push_back(levelOrigin);
    numberOfCells += cellsPerLevel;
  }
  this->BlockId.resize(this->NumberOfAMRLevels, -1);
  for (int level = 0; level < this->NumberOfAMRLevels; level++)
  {
    this->BlockId[level] = myRank * this->NumberOfAMRLevels + level;
  }
}

std::array<int, 6> AMR::GetLevelIndices(int level)
{
  return this->LevelIndices[level];
}

std::array<double, 3> AMR::GetLevelOrigin(int level)
{
  return this->LevelOrigin[level];
}
