#ifndef FEADAPTOR_HEADER
#define FEADAPTOR_HEADER

#ifdef __cplusplus
extern "C" {
#endif
void CatalystCoProcess(unsigned int numberOfPoints, double* pointsData, unsigned int numberOfCells,
  unsigned int* cellsDAta, double* velocityData, float* pressureData, double time,
  unsigned int timeStep, int lastTimeStep);
void CatalystFinalize();
#ifdef __cplusplus
}
#endif
#endif
