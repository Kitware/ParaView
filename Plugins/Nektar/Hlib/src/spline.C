

/*all that follows is to set up a spline fitting routine from a data file*/

typedef struct geomf  {    /* Curve defined in a file */
  int           npts  ;    /* number of points        */
  int           pos   ;    /* last confirmed position */
  char         *name  ;    /* File/curve name         */
  double       *x, *y ;    /* coordinates             */
  double       *sx,*sy;    /* spline coefficients     */
  double       *arclen;    /* arclen along the curve  */
  struct geomf *next  ;    /* link to the next        */
} Geometry;

typedef struct vector {    /* A 2-D vector */
  double     x, y     ;    /* components   */
  double     length   ;    /* length       */
} Vector;

#define _MAX_NC         1024   /* Points describing a curved side   */
static int    closest    (Point p, Geometry *g);
static void   bracket    (double s[], double f[], Geometry *g, Point a,
        Vector ap);
static Vector setVector  (Point p1, Point p2);

static double searchGeom (Point a, Point p, Geometry *g),
              brent      (double s[], Geometry *g, Point a, Vector ap,
        double tol);

static Geometry *lookupGeom (char *name),
                *loadGeom   (char *name);

static Geometry *geomlist;

void Tri::genFile (Curve *cur, double *x, double *y){
  register int i;
  Geometry *g;
  Point    p1, p2, a;
  double   *z, *w, *eta, xoff, yoff;
  int      fac;

  fac = cur->face;

  p1.x = vert[vnum(fac,0)].x;  p1.y = vert[vnum(fac,0)].y;
  p2.x = vert[vnum(fac,1)].x;  p2.y = vert[vnum(fac,1)].y;

  getzw(qa,&z,&w,'a');

  eta    = dvector (0, qa);
  if ((g = lookupGeom (cur->info.file.name)) == (Geometry *) NULL)
       g = loadGeom   (cur->info.file.name);


  /* If the current edge has an offset, apply it now */
  xoff = cur->info.file.xoffset;
  yoff = cur->info.file.yoffset;
  if (xoff != 0.0 || yoff != 0.0) {
    dsadd (g->npts, xoff, g->x, 1, g->x, 1);
    dsadd (g->npts, yoff, g->y, 1, g->y, 1);
    if (option("verbose") > 1)
      printf ("shifting current geometry by (%g,%g)\n", xoff, yoff);
  }

  /* get the end points which are assumed to lie on the curve */
  /* set up search direction in normal to element point -- This
     assumes that vertices already lie on spline */

  a.x      = p1.x  - (p2.y - p1.y);
  a.y      = p1.y  + (p2.x - p1.x);
  eta[0]   = searchGeom (a, p1, g);
  a.x      = p2.x  - (p2.y - p1.y);
  a.y      = p2.y  + (p2.x - p1.x);
  eta[qa-1] = searchGeom (a, p2, g);

  /* Now generate the points where we'll evaluate the geometry */

  for (i = 1; i < qa-1; i++)
    eta [i] = eta[0] + 0.5 * (eta[qa-1] - eta[0]) * (z[i] + 1.);

  for (i = 0; i < qa; i++) {
    x[i] = splint (g->npts, eta[i], g->arclen, g->x, g->sx);
    y[i] = splint (g->npts, eta[i], g->arclen, g->y, g->sy);
  }

  g->pos = 0;     /* reset the geometry */
  if (xoff != 0.)
    dvsub (g->npts, g->x, 1, &xoff, 0, g->x, 1);
  if (yoff != 0.)
    dvsub (g->npts, g->y, 1, &yoff, 0, g->y, 1);

  free (eta);    /* free the workspace */

  return;
}

static Point setPoint (double x, double y)
{
  Point p;
  p.x = x;
  p.y = y;
  return p;
}


static Vector setVector (Point p1, Point p2)
{
  Vector v;

  v.x      = p2.x - p1.x;
  v.y      = p2.y - p1.y;
  v.length = sqrt (v.x*v.x + v.y*v.y);

  return v;
}

/* Compute the angle between the vector ap and the vector from a to
 * a point s on the curv.  Uses the small-angle approximation */

static double getAngle (double s, Geometry *g, Point a, Vector ap)
{
  Point  c;
  Vector ac;

  c  = setPoint (splint(g->npts, s, g->arclen, g->x, g->sx),
                 splint(g->npts, s, g->arclen, g->y, g->sy));
  ac = setVector(a, c);

  return 1. - ((ap.x * ac.x + ap.y * ac.y) / (ap.length * ac.length));
}

/* Search for the named Geometry */

static Geometry *lookupGeom (char *name)
{
  Geometry *g = geomlist;

  while (g) {
    if (strcmp(name, g->name) == 0)
      return g;
    g = g->next;
  }

  return (Geometry *) NULL;
}

/* Load a geometry file */

static Geometry *loadGeom (char *name){
  const int verbose = option("verbose");
  Geometry *g   = (Geometry *) calloc (1, sizeof(Geometry));
  char      buf [BUFSIZ];
  double    tmp[_MAX_NC];
  Point     p1, p2, p3, p4;
  FILE     *fp;
  register  int i;

  if (verbose > 1)
    printf ("Loading geometry file %s...", name);
  if ((fp = fopen(name, "r")) == (FILE *) NULL) {
    fprintf (stderr, "couldn't find the curved-side file %s", name);
    exit (-1);
  }

  while (fgets (buf, BUFSIZ, fp))    /* Read past the comments */
    if (*buf != '#') break;

  /* Allocate space for the coordinates */

  g -> x = (double*) calloc (_MAX_NC, sizeof(double));
  g -> y = (double*) calloc (_MAX_NC, sizeof(double));

  strcpy (g->name = (char *) malloc (strlen(name)+1), name);

  /* Read the coordinates.  The first line is already in *
   * the input buffer from the comment loop above.       */

  i = 0;
  while (i <= _MAX_NC && sscanf (buf,"%lf%lf", g->x + i, g->y + i) == 2) {
    i++;
    if (!fgets(buf, BUFSIZ, fp)) break;
  }
  g->npts = i;

  if (i < 2 ) error_msg (geometry file does not have enough points);

  if (i > _MAX_NC) error_msg (geometry file has too many points);

  if (verbose > 1) printf ("%d points", g->npts);

  /* Allocate memory for the other quantities */

  g->sx     = (double*) calloc (g->npts, sizeof(double));
  g->sy     = (double*) calloc (g->npts, sizeof(double));
  g->arclen = (double*) calloc (g->npts, sizeof(double));

  /* Compute spline information for the (x,y)-coordinates.  The vector "tmp"
     is a dummy independent variable for the function x(eta), y(eta).  */

  tmp[0] = 0.;
  tmp[1] = 1.;
  dramp  (g->npts, tmp, tmp + 1, tmp, 1);
  spline (g->npts, 1.e30, 1.e30, tmp, g->x, g->sx);
  spline (g->npts, 1.e30, 1.e30, tmp, g->y, g->sy);

  /* Compute the arclength of the curve using 4 points per segment */

  for (i = 0; i < (*g).npts-1; i++) {
    p1 = setPoint (g->x[i], g->y[i] );
    p2 = setPoint (splint (g->npts, i+.25, tmp, g->x, g->sx),
       splint (g->npts, i+.25, tmp, g->y, g->sy));
    p3 = setPoint (splint (g->npts, i+.75, tmp, g->x, g->sx),
       splint (g->npts, i+.75, tmp, g->y, g->sy));
    p4 = setPoint (g->x[i+1], g->y[i+1]);

    g->arclen [i+1] = g->arclen[i] + distance (p1, p2) + distance (p2, p3) +
                                     distance (p3, p4);
  }

  /* Now that we have the arclength, compute x(s), y(s) */

  spline (g->npts, 1.e30, 1.e30, g->arclen, g->x, g->sx);
  spline (g->npts, 1.e30, 1.e30, g->arclen, g->y, g->sy);

  if (verbose > 1)
    printf (", arclength  = %f\n", g->arclen[i]);


  /* add to the list of geometries */

  g ->next = geomlist;
  geomlist = g;

  fclose (fp);
  return g;
}

/*
 * Find the point at which a line passing from the anchor point "a"
 * through the search point "p" intersects the curve defined by "g".
 * Always searches from the last point found to the end of the curve.
 */

static double searchGeom (Point a, Point p, Geometry *g)
{
  Vector   ap;
  double   tol = dparam("TOLCURV"), s[3], f[3];
  register int ip;

  /* start the search at the closest point */

  ap   = setVector (a, p);
  s[0] = g -> arclen[ip = closest (p, g)];
  s[1] = g -> arclen[ip + 1];

  bracket (s, f, g, a, ap);
  if (fabs(f[1]) > tol)
    brent (s, g, a, ap, tol);

  return s[1];
}

int id_min(int n, double *d, int ){
  if(!n)
    return 0;

  int    cnt = 0,i;
  for(i=1; i<n;++i)
    cnt = (d[i] < d[cnt]) ? i: cnt;
  return cnt;
}
/* ---------------  Bracketing and Searching routines  --------------- */

static int closest (Point p, Geometry *g)
{
  const
  double  *x = g->x    + g->pos,
          *y = g->y    + g->pos;
  const    int n = g->npts - g->pos;
  double   len[_MAX_NC];
  register int i;

  for (i = 0; i < n; i++)
    len[i] = sqrt (pow(p.x - x[i],2.) + pow(p.y - y[i],2.));

  i = id_min (n, len, 1) + g->pos;
  i = min(i, g->npts-2);

  /* If we found the same position and it's not the very first *
   * one, start the search over at the beginning again.  The   *
   * test for i > 0 makes sure we only do the recursion once.  */

  if (i && i == g->pos) { g->pos = 0; i = closest (p, g); }

  return g->pos = i;
}

#define GOLD      1.618034
#define CGOLD     0.3819660
#define GLIMIT    100.
#define TINY      1.e-20
#define ZEPS      1.0e-10
#define ITMAX     100

#define SIGN(a,b)     ((b) > 0. ? fabs(a) : -fabs(a))
#define SHFT(a,b,c,d)  (a)=(b);(b)=(c);(c)=(d);
#define SHFT2(a,b,c)   (a)=(b);(b)=(c);

#define fa f[0]
#define fb f[1]
#define fc f[2]
#define xa s[0]
#define xb s[1]
#define xc s[2]

static void bracket (double s[], double f[], Geometry *g, Point a, Vector ap)
{
  double ulim, u, r, q, fu;

  fa = getAngle (xa, g, a, ap);
  fb = getAngle (xb, g, a, ap);

  if (fb > fa) { SHFT (u, xa, xb, u); SHFT (fu, fb, fa, fu); }

  xc = xb + GOLD*(xb - xa);
  fc = getAngle (xc, g, a, ap);

  while (fb > fc) {
    r = (xb - xa) * (fb - fc);
    q = (xb - xc) * (fb - fa);
    u =  xb - ((xb - xc) * q - (xb - xa) * r) /
              (2.*SIGN(max(fabs(q-r),TINY),q-r));
    ulim = xb * GLIMIT * (xc - xb);

    if ((xb - u)*(u - xc) > 0.) {      /* Parabolic u is bewteen b and c */
      fu = getAngle (u, g, a, ap);
      if (fu < fc) {                    /* Got a minimum between b and c */
  SHFT2 (xa,xb, u);
  SHFT2 (fa,fb,fu);
  return;
      } else if (fu > fb) {             /* Got a minimum between a and u */
  xc = u;
  fc = fu;
  return;
      }
      u  = xc + GOLD*(xc - xb);    /* Parabolic fit was no good. Use the */
      fu = getAngle (u, g, a, ap);             /* default magnification. */

    } else if ((xc-u)*(u-ulim) > 0.) {   /* Parabolic fit is bewteen c   */
      fu = getAngle (u, g, a, ap);                         /* and ulim   */
      if (fu < fc) {
  SHFT  (xb, xc, u, xc + GOLD*(xc - xb));
  SHFT  (fb, fc, fu, getAngle(u, g, a, ap));
      }
    } else if ((u-ulim)*(ulim-xc) >= 0.) {  /* Limit parabolic u to the  */
      u   = ulim;                           /* maximum allowed value     */
      fu  = getAngle (u, g, a, ap);
    } else {                                       /* Reject parabolic u */
      u   = xc + GOLD * (xc - xb);
      fu  = getAngle (u, g, a, ap);
    }
    SHFT  (xa, xb, xc, u);      /* Eliminate the oldest point & continue */
    SHFT  (fa, fb, fc, fu);
  }
  return;
}

/* Brent's algorithm for parabolic minimization */

static double brent (double s[], Geometry *g, Point ap, Vector app, double tol)
{
  int    iter;
  double a,b,d,etemp,fu,fv,fw,fx,p,q,r,tol1,tol2,u,v,w,x,xm;
  double e=0.0;

  a  = min (xa, xc);               /* a and b must be in decending order */
  b  = max (xa, xc);
  d  = 1.;
  x  = w  = v  = xb;
  fw = fv = fx = getAngle (x, g, ap, app);

  for (iter = 1; iter <= ITMAX; iter++) {    /* ....... Main Loop ...... */
    xm   = 0.5*(a+b);
    tol2 = 2.0*(tol1 = tol*fabs(x)+ZEPS);
    if (fabs(x-xm) <= (tol2-0.5*(b-a))) {             /* Completion test */
      xb = x;
      return fx;
    }
    if (fabs(e) > tol1) {             /* Construct a trial parabolic fit */
      r = (x-w) * (fx-fv);
      q = (x-v) * (fx-fw);
      p = (x-v) * q-(x-w) * r;
      q = (q-r) * 2.;
      if (q > 0.) p = -p;
      q = fabs(q);
      etemp=e;
      e = d;

      /* The following conditions determine the acceptability of the    */
      /* parabolic fit.  Following we take either the golden section    */
      /* step or the parabolic step.                                    */

      if (fabs(p) >= fabs(.5*q*etemp) || p <= q*(a-x) || p >= q*(b-x))
  d = CGOLD * (e = (x >= xm ? a-x : b-x));
      else {
  d = p / q;
  u = x + d;
  if (u-a < tol2 || b-u < tol2)
    d = SIGN(tol1,xm-x);
      }
    } else
      d = CGOLD * (e = (x >= xm ? a-x : b-x));

    u  = (fabs(d) >= tol1 ? x+d : x+SIGN(tol1,d));
    fu = getAngle(u,g,ap,app);

    /* That was the one function evaluation per step.  Housekeeping... */

    if (fu <= fx) {
      if (u >= x) a = x; else b = x;
      SHFT(v ,w ,x ,u );
      SHFT(fv,fw,fx,fu)
    } else {
      if (u < x) a=u; else b=u;
      if (fu <= fw || w == x) {
  v  = w;
  w  = u;
  fv = fw;
  fw = fu;
      } else if (fu <= fv || v == x || v == w) {
  v  = u;
  fv = fu;
      }
    }
  }                        /* .......... End of the Main Loop .......... */

  error_msg(too many iterations in brent());
  xb = x;
  return fx;
}

#undef ITMAX
#undef CGOLD
#undef ZEPS
#undef SIGN
#undef fa
#undef fb
#undef fc
#undef xa
#undef xb
#undef xc
