#ifndef CDI_READER_PROJECTIONS_
#define CDI_READER_PROJECTIONS_

enum projection
{
  SPHERICAL = 0,
  CYLINDRICAL_EQUIDISTANT = 1,
  CASSINI = 2,
  MOLLWEIDE = 3,
  CATALYST = 4,
  SPILHOUSE = 5
};

inline bool isproj(const int p)
{
  return p >= projection::SPHERICAL && p <= projection::SPILHOUSE;
}

void get_scaling(const projection ProjectionMode, const bool ShowMultilayerView,
  const double VertLev, const double adjustedLayerThickness, double& sx, double& sy, double& sz);

int CartesianToSpherical(double x, double y, double z, double* rho, double* phi, double* theta);

int SphericalToCartesian(double rho, double phi, double theta, double* x, double* y, double* z);

int LLtoXYZ(double lon, double lat, double* x, double* y, double* z, int projectionMode);

double iterate_theta(const double lon, const double lat, const double theta);

double get_theta(const double lon, const double lat);

static double ell_int_5(double phi);

double aasin(double v);

double aacos(double v);

void crossProduct(double vect_A[], double vect_B[], double cross_P[]);

double dotProduct(double vect_A[], double vect_B[]);

void rotateVec(double v[], double r[]);

#endif /* CDI_READER_PROJECTIONS_ */
