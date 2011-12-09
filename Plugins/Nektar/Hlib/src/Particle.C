#include <stdlib.h>
#include <math.h>
#include <veclib.h>
#include <hotel.h>
#include "nekstruct.h"

#include <stdio.h>

/* This function inspects point Xi to see if it lies the other side of
   the line defined by the normal direction n and with a point X in the plane.

   If it does lie on the other side of the plane the normal velocity
   is calculated. Providing that the normal velocity multiplied by dt (the
   distance travelled normal to the plane) is larger than  a tolerance P_TOL
   the time taken to travel the distance from the plane to Xi is
   returned in dt.

   if there is no intersection the dt is set to zero
*/
static double intersect_line(double *Xi, double *n, double *Xo, double *V,
           double dt_full){
  double  d, un, dt = 0.0;

  d = (Xi[0] - Xo[0])*n[0]+(Xi[1] - Xo[1])*n[1];
  if(d < 0){// point intersect face
    un = V[0]*n[0] + V[1]*n[1];
    if(-un*dt_full > P_TOL)
      return dt = d/un;
  }
  return dt;
}


// macros for intersection problems
// set dt and face for intersection with side 'a'
#define set_1_side(a)\
{dt  = intersect_line(Xi[0].x,n[a],Xo[a],vp,dt_remain[0]);fac[0] = a;}

// set maximum dt and face for intersection with sides 'a'  and 'b'
#define set_2_sides(a,b)\
{dt  = intersect_line(Xi[0].x,n[a],Xo[a],vp,dt_remain[0]);\
 dt1 = intersect_line(Xi[0].x,n[b],Xo[b],vp,dt_remain[0]);\
 if(dt*dt > dt1*dt1)  fac[0] = a;  else{dt = dt1; fac[0] = b;}}

int Element::intersect_bnd(Coord *Xi,double *vp,double *dt_remain,int *fac)
     {ERR; return -1;}

/* Given an initial point Xi, which has moved a linear velocity vp and
   a timestep dt this function will check to see if the point intersects
   a boundary. If no intersection is found a '0' is returned. If an
   intersection is found then a '1' is returned Xi and
   face are set to the boundary value and dt is set to the remaining
   time to complete the time step */

int Tri::intersect_bnd(Coord *Xi, double *vp, double *dt_remain, int *fac){
  // check whether it is outside or on element boundary
  if((Xi->x[0] < -1) || (Xi->y[0] < -1) ||(Xi->x[0] + Xi->y[0] > 0)){
    double dt,dt1,sum;
    static double n [3][2] = {{0,1},{-1/sqrt(2.),-1/sqrt(2.)},{1,0}};
    static double Xo[3][2] = {{0,-1},{0,0},{-1,0}};

    sum = vp[0] + vp[1];

    if(vp[0] > 0)
      if(vp[1] > 0) // must be side 1
  {set_1_side(1);}
      else  // check to see if velocity is less than 45 degrees angle
  if(sum > 0)  // side 0 or 1
    {set_2_sides(0,1);}
  else // side 0
    {set_1_side(0);}
    else
      if(vp[1] > 0)
  if(sum > 0) // sides 1 or 2
    {set_2_sides(1,2);}
  else // side 2
    {set_1_side(2);}
      else // sides 0 or 2
  {set_2_sides(0,2);}


    if(dt){
      // Move particle in parametric space
      daxpy(2,-dt,vp,1,Xi[0].x,1);
      dt_remain[0] = dt;
      return 1;
    }
    else
      return 0;
  }
  else
    return 0;
}

int Quad::intersect_bnd(Coord *Xi, double *vp, double *dt_remain, int *fac){
  // check whether it is outside of  element
  if(Xi->x[0] < -1 || Xi->y[0] < -1 || Xi->x[0] > 1 || Xi->y[0] > 1){
    double dt,dt1;
    static double n [4][2] = {{0,1},{-1,0},{0,-1},{1,0}};
    static double Xo[4][2] = {{0,-1},{1,0},{0,1},{-1,0}};

    if(vp[0] > 0)
      if(vp[1] > 0)  // side 1 or 2
  {set_2_sides(1,2);}
      else  // side 0 or 1
  {set_2_sides(0,1);}
    else
      if(vp[1] > 0)  // sides 2 or 3
  {set_2_sides(2,3);}
      else // sides 0 or 3
  {set_2_sides(0,3);}

    if(dt){
      // Move particle in parametric space
      daxpy(2,-dt,vp,1,Xi[0].x,1);
      dt_remain[0] = dt;
      return 1;
    }
    else
      return 0;
  }
  else
    return 0;
}


/* This function inspects point Xi to see if it lies the other side of
   the plane defined by the normal direction n and with a point X in the plane.

   If it does lie on the other side of the plane the normal velocity
   is calculated. Providing that the normal velocity multiplied by dt (the
   distance travelled normal to the plane) is larger than  a tolerance P_TOL
   the time taken to travel the distance from the plane to Xi is return in dt.

   if there is no intersection the dt is set to zero
*/
static double intersect_plane(double *Xi, double *n, double *Xo, double *V,
            double dt_full){
  double  d, un, dt = 0.0;

  d = (Xi[0] - Xo[0])*n[0]+(Xi[1] - Xo[1])*n[1]+(Xi[2] - Xo[2])*n[2];
  if(d < 0){// point intersects face
    un = V[0]*n[0] + V[1]*n[1] + V[2]*n[2];
    if(-un*dt_full > P_TOL)
      return dt = d/un;
  }
  return dt;
}

// macros for intersection problems
// set dt and face for intersection with face 'a'
#define set_1_face(a)\
{dt = intersect_plane(Xi[0].x,n[a],Xo[a],vp,dt_remain[0]); fac[0] = a;}

// set maximum dt and face for intersection with faces 'a  and 'b'
#define set_2_faces(a,b)\
{dt  = intersect_plane(Xi[0].x,n[a],Xo[a],vp,dt_remain[0]);\
 dt1 = intersect_plane(Xi[0].x,n[b],Xo[b],vp,dt_remain[0]);\
 if(dt*dt > dt1*dt1)  fac[0] = a;  else{  dt = dt1; fac[0] = b;}}

// set maximum dt and face for intersection with faces 'a','b  and 'c'
#define set_3_faces(a,b,c)\
{dt  = intersect_plane(Xi[0].x,n[a],Xo[a],vp,dt_remain[0]);\
 dt1 = intersect_plane(Xi[0].x,n[b],Xo[b],vp,dt_remain[0]);\
 if(dt*dt > dt1*dt1)  fac[0] = a;  else{dt = dt1; fac[0] = b;}\
 dt1   = intersect_plane(Xi[0].x,n[c],Xo[c],vp,dt_remain[0]);\
 if(dt1*dt1 > dt*dt){ dt = dt1; fac[0]=c;}}


int Tet::intersect_bnd(Coord *Xi, double *vp, double *dt_remain, int *fac){
  // check whether it is outside or on  element boundary
  if(Xi->x[0] < -1 || Xi->y[0] < -1  || Xi->z[0] < -1 ||
     (Xi->x[0] +  Xi->y[0] + Xi->z[0]) > -1){
    double dt,dt1,sum;
    static double n [4][3] = {{0,0,1}, {0,1,0},
            {-1/sqrt(3.),-1/sqrt(3.),-1/sqrt(3.)},{1,0,0}};
    static double Xo[4][3] = {{-0.5,-0.5,-1},{-0.5,-1,-0.5},
            {-1/3.0,-1/3.0,-1/3.0}, {-1,-0.5,-0.5}};

    sum = vp[0] + vp[1] + vp[2];

    if(vp[0] > 0)
      if(vp[1] > 0)
  if(vp[2] > 0)// face 2
    {set_1_face(2);}
  else
    if(sum > 0) // face 0 or 2
      {set_2_faces(0,2);}
    else //face 0
      {set_1_face(0);}
      else
  if(vp[2] > 0)// face 2
    if(sum > 0) // face 1 or 2
      {set_2_faces(1,2);}
    else //face 1
      {set_1_face(1);}
  else
    if(sum > 0) // face 0, 1 or 2
      {set_3_faces(0,1,2);}
    else //face 0 or 1
      {set_2_faces(0,1);}
    else
      if(vp[1] > 0)
  if(vp[2] > 0)
    if(sum > 0) // face 2 or 3
      {set_2_faces(2,3);}
    else //face 3
      {set_1_face(3);}
  else
    if(sum > 0) // face 0,2 or 3
      {set_3_faces(0,2,3);}
    else //face 0,3
      {set_2_faces(0,3);}
      else
  if(vp[2] > 0)// face 2
    if(sum > 0) // face 1,2 or 3
      {set_3_faces(1,2,3);}
    else //face 1 or 3
      {set_2_faces(1,3);}
  else  // face 0, 1 or 3
    {set_3_faces(0,1,3);}

    if(dt){
      // Move particle in parametric space
      daxpy(3,-dt,vp,1,Xi[0].x,1);
      dt_remain[0] = dt;
      return 1;
    }
    else
      return 0;
  }
  else
  return 0;
}

int Prism::intersect_bnd(Coord *Xi, double *vp, double *dt_remain, int *fac){
  // check whether it is outside or on  element
  if(Xi->x[0] < -1 || Xi->y[0] < -1  || Xi->z[0] < -1 || Xi->y[0] > 1 ||
     (Xi->x[0] + Xi->z[0]) > 0){
    double dt,dt1,sum;
    static double n [5][3] = {{0,0,1}, {0,1,0}, {-1/sqrt(2.),0,-1/sqrt(2.)},
            {0,-1,0},{1,0,0}};
    static double Xo[5][3] = {{0,0,-1},{-0.5,-1,-0.5},
            {0,0,0}, {-0.5,1,-0.5},{-1,0,0}};

    sum = vp[0] + vp[2];

    if(vp[0] > 0)
      if(vp[1] > 0)
  if(vp[2] > 0)
    if(sum > 0) // face 2 or 3
      {set_2_faces(2,3);}
    else //face 3
      {set_1_face(3);}
  else
    if(sum > 0) // face 0,2 or 3
      {set_3_faces(0,2,3);}
    else //face 0 or 3
      {set_2_faces(0,3);}
      else
  if(vp[2] > 0)
    if(sum > 0) // face 1 or 2
      {set_2_faces(1,2);}
    else //face 1
      {set_1_face(1);}
  else
    if(sum > 0) // face 0,1 or 2
      {set_3_faces(0,1,2);}
    else //face 0 or 1
      {set_2_faces(0,1);}
    else
      if(vp[1] > 0)
  if(vp[2] > 0)
    if(sum > 0) // face 2, 3 or 4
      {set_3_faces(2,3,4);}
    else //face 3 or 4
      {set_2_faces(3,4);}
  else
    // face 0,3 or 4
    {set_3_faces(0,3,4);}
      else
  if(vp[2] > 0)
    if(sum > 0) // face 1, 2 or 4
      {set_3_faces(1,2,4);}
    else //face 1 or 4
      {set_2_faces(1,4);}
  else
    // face 0,1 or 4
    {set_3_faces(0,3,4);}

    if(dt){
      // Move particle in parametric space
      daxpy(3,-dt,vp,1,Xi[0].x,1);
      dt_remain[0] = dt;
      return 1;
    }
    else
      return 0;
  }
  else
    return 0;
}

// set maximum dt and face for intersection with faces 'a','b','c'  and 'd'
#define set_4_faces(a,b,c,d)\
{dt  = intersect_plane(Xi[0].x,n[a],Xo[a],vp,dt_remain[0]);\
 dt1 = intersect_plane(Xi[0].x,n[b],Xo[b],vp,dt_remain[0]);\
 if(dt*dt > dt1*dt1)  fac[0] = a;  else{dt = dt1; fac[0] = b;}\
 dt1   = intersect_plane(Xi[0].x,n[c],Xo[c],vp,dt_remain[0]);\
 if(dt1*dt1 > dt*dt){ dt = dt1; fac[0]=c;}\
 dt1   = intersect_plane(Xi[0].x,n[d],Xo[d],vp,dt_remain[0]);\
 if(dt1*dt1 > dt*dt){ dt = dt1; fac[0]=d;}}

int Pyr::intersect_bnd(Coord *Xi, double *vp, double *dt_remain, int *fac){

  fprintf(stderr,"intersect_bnd not set up form prisms yet \n");
  exit(-1);

  // check whether it is outside of  element
  if(Xi->x[0] < -1 || Xi->y[0] < -1 || (Xi->x[0] + Xi->y[0])> 1 ||
     (Xi->y[0] + Xi->z[0]) > 1){

    double dt,dt1;
    static double n [5][3] = {{0,0,1}, {0,1,0}, {-1/sqrt(2.),0,-1/sqrt(2.)},
            {0,-1/sqrt(2.),-1/sqrt(2.)}, {1,0,0}};
    static double Xo[5][3] = {{0,0,-1},{-0.5,-1,-0.5},{0,-0.5,0} ,{-0.5,0,0},
            {-1,-0.5,-0.5}};

    if(vp[0] > 0)
      if(vp[1] > 0)
  if(vp[2] > 0)// face 2,3
    {set_2_faces(2,3);}
  else  // face 0,2 or 3
    {set_3_faces(0,2,3);}
      else
  if(vp[2] > 0)// face 1,2,3
    {set_3_faces(1,2,3);}
  else  // face 0,1 or 2
    {set_3_faces(0,1,2);}
    else
      if(vp[1] > 0)
  if(vp[2] > 0)// face 2,3 or 4
    {set_3_faces(2,3,4);}
  else  // face 0,3 or 4
    {set_3_faces(0,3,4);}
      else
  if(vp[2] > 0)// face 1,2,3 or 4
    {set_4_faces(1,2,3,4);}
  else  // face 0,1 or 4
    {set_3_faces(0,1,4);}

    if(dt){
      // Move particle in parametric space
      daxpy(3,-dt,vp,1,Xi[0].x,1);
      dt_remain[0] = dt;
      return 1;
    }
    else
      return 0;
  }
  else
    return 0;
}

int Hex::intersect_bnd(Coord *Xi, double *vp, double *dt_remain, int *fac){
  // check whether it is outside or on  element boundary
  if(Xi->x[0] < -1 || Xi->y[0] < -1 || Xi->x[0] > 1 || Xi->y[0] > 1
     || Xi->z[0] < -1 || Xi->z[0] > 1){

    double dt,dt1;
    static double n [6][3] = {{0,0,1}, {0,1,0}, {-1,0,0},{0,-1,0},
            {1,0,0}, {0,0,-1}};
    static double Xo[6][3] = {{0,0,-1},{0,-1,0},{1,0,0} ,{0,1,0},
            {-1,0,0},{0,0,1}};

    if(vp[0] > 0)
      if(vp[1] > 0)
  if(vp[2] > 0)// face 2,3 or 5
    {set_3_faces(2,3,5);}
  else  // face 0,2 or 3
    {set_3_faces(0,2,3);}
      else
  if(vp[2] > 0)// face 1,2 or 5
    {set_3_faces(1,2,5);}
  else  // face 0,1 or 2
    {set_3_faces(0,1,2);}
    else
      if(vp[1] > 0)
  if(vp[2] > 0)// face 3,4 or 5
    {set_3_faces(3,4,5);}
  else  // face 0,3 or 4
    {set_3_faces(0,3,4);}
      else
  if(vp[2] > 0)// face 1,4 or 5
    {set_3_faces(1,4,5);}
  else  // face 0,1 or 4
    {set_3_faces(0,1,4);}

    if(dt){
      // Move particle in parametric space
      daxpy(3,-dt,vp,1,Xi[0].x,1);
      dt_remain[0] = dt;
      return 1;
    }
    else
      return 0;
  }
  else
    return 0;
}

int  Element::lcoords2face(Coord *Xi, int *fac){ERR;return -1;}
void Element::face2lcoords(Coord *Xi, int *fac){ERR;}


/** Given the local coordinates Xi, determine their value in terms of
   the face coordinates on face 'fac'. Returns the element id of the
   adjacent face if it exists otherwise -1 is returned. Xi is also
   filled with the face coordinates in its first entries */

int Tri::lcoords2face(Coord *Xi, int *fac){
  int   neid;
  double eta;
  Edge   *e;

  if(!edge[fac[0]].base) return -1;

  if(fac[0] == 0)
    eta = Xi->x[0];
  else
    eta = Xi->y[0];

  e = (edge[fac[0]].link)? e = edge[fac[0]].link : edge[fac[0]].base;

  if(edge[fac[0]].con)
    eta = -eta;

  neid     = e->eid;
  fac[0]   = e->id;
  Xi->x[0] = eta;

  return neid;
}

/** evaluate the local coordinates from the face coordinates of face fac
  stored   in Xi */

void Tri::face2lcoords(Coord *Xi, int *fac){
  double eta = Xi->x[0];

  if(edge[fac[0]].con)
    eta = -eta;

  switch(fac[0]){
  case 0:
    Xi->x[0] =  eta;
    Xi->y[0] = -1;
    break;
  case 1:
    Xi->x[0] = -eta;
    Xi->y[0] =  eta;
    break;
  case 2:
    Xi->x[0] = -1;
    Xi->y[0] = eta;
    break;
  }
}

int Quad::lcoords2face(Coord *Xi, int *fac){
  int   neid;
  double eta;
  Edge   *e;

  if(!edge[fac[0]].base) return -1;

  if((fac[0] == 0)||(fac[0] == 3))
    eta = Xi->x[0];
  else
    eta = Xi->y[0];

  e = (edge[fac[0]].link)? e = edge[fac[0]].link : edge[fac[0]].base;


  if(edge[fac[0]].con)
    eta = -eta;

  neid     = e->eid;
  fac[0]   = e->id;
  Xi->x[0] = eta;

  return neid;
}

void Quad::face2lcoords(Coord *Xi, int *fac){
  double eta = Xi->x[0];

  if(edge[fac[0]].con)
    eta = -eta;

  switch(fac[0]){
  case 0:
    Xi->x[0] =  eta;
    Xi->y[0] = -1;
    break;
  case 1:
    Xi->x[0] = 1;
    Xi->y[0] = eta;
    break;
  case 2:
    Xi->x[0] =  eta;
    Xi->y[0] = 1;
    break;
  case 3:
    Xi->x[0] = -1;
    Xi->y[0] = eta;
    break;
  }
}

int Tet::lcoords2face(Coord *Xi, int *fac){
  int   neid;
  double eta1,eta2;
  Face *f;

  if(!(f = face[fac[0]].link)) return -1;

  switch(fac[0]){
  case 0:
    eta1 = (1-Xi->y[0])? 2*(1+Xi->x[0])/(1-Xi->y[0])-1:0.0;
    eta2 = Xi->y[0];
    break;
  case 1:
    eta1 = (1-Xi->z[0])? 2*(1+Xi->x[0])/(1-Xi->z[0])-1:0.0;
    eta2 = Xi->z[0];
    break;
  case 2:
    eta1 = (1-Xi->z[0])? 2*(1+Xi->y[0])/(1-Xi->z[0])-1:0.0;
    eta2 = Xi->z[0];
    break;
  case 3:
    eta1 = (1-Xi->z[0])? 2*(1+Xi->y[0])/(1-Xi->z[0])-1:0.0;
    eta2 = Xi->z[0];
    break;
  }

  if(face[fac[0]].con)
    eta1 = -eta1;

  neid   = f->eid;
  fac[0] = f->id;

  Xi->x[0] = eta1;
  Xi->y[0] = eta2;

  return neid;
}

void Tet::face2lcoords(Coord *Xi, int *fac){
  double eta1 = Xi->x[0];
  double eta2 = Xi->y[0];

  if(face[fac[0]].con)
    eta1 = -eta1;

  switch(fac[0]){
  case 0:
    Xi->x[0] = 0.5*(eta1+1)*(1-eta2)-1;
    Xi->y[0] = eta2;
    Xi->z[0] = -1;
    break;
  case 1:
    Xi->x[0] = 0.5*(eta1+1)*(1-eta2)-1;
    Xi->y[0] = -1;
    Xi->z[0] = eta2;
    break;
  case 2:
    Xi->z[0] = eta2;
    Xi->y[0] = 0.5*(eta1+1)*(1-eta2)-1;
    Xi->x[0] = -Xi->y[0] -Xi->z[0]-1;
    break;
  case 3:
    Xi->x[0] = -1;
    Xi->y[0] = 0.5*(eta1+1)*(1-eta2)-1;
    Xi->z[0] = eta2;
    break;
  }
}


int Prism::lcoords2face(Coord *Xi, int *fac){
  int   neid;
  double eta1,eta2;
  int con;
  Face   *f;

  if(!(f = face[fac[0]].link)) return -1;

  switch(fac[0]){
  case 0:
    eta1 = Xi->x[0];
    eta2 = Xi->y[0];
    break;
  case 1:
    eta1 = (1-Xi->z[0])? 2*(1+Xi->x[0])/(1-Xi->z[0])-1:0.0;
    eta2 = Xi->z[0];
    break;
  case 2:
    eta1 = Xi->y[0];
    eta2 = Xi->z[0];
    break;
  case 3:
    eta1 = (1-Xi->z[0])? 2*(1+Xi->x[0])/(1-Xi->z[0])-1:0.0;
    eta2 = Xi->z[0];
    break;
  case 4:
    eta1 = Xi->y[0];
    eta2 = Xi->z[0];
    break;
  }

  con = face[fac[0]].con;

  if(con)
    if((fac[0] == 1)||(fac[0] == 3)) // sort out connectivity
      eta1 = -eta1;
    else{ // sort out orientation and connectivity
      double tmp;
      switch(con){
      case 1:  // negate 'a'
  eta1 = -eta1;
  break;
      case 2:// negate 'b'
  eta2 = -eta2;
  break;
      case 3:  // negate 'a' and 'b'
  eta1 = -eta1;
  eta2 = -eta2;
  break;
      case 4: // transpose
  tmp =  eta1;
  eta1 = eta2;
  eta2 = tmp;
  break;
      case 5: // negate 'a' and transpose
  tmp  = -eta1;
  eta1 =  eta2;
  eta2 =  tmp;
  break;
      case 6: // negate 'b' and transpose
  tmp  = -eta2;
  eta2 =  eta1;
  eta1 =  tmp;
  break;
      case 7:// negate 'a' and 'b' and transpose
  tmp  = -eta1;
  eta1 = -eta2;
  eta2 =  tmp;
    break;
      }
    }

  neid     = f->eid;
  fac[0]   = f->id;
  Xi->x[0] = eta1;
  Xi->y[0] = eta2;

  return neid;
}

void Prism::face2lcoords(Coord *Xi, int *fac){
  int con;
  double eta1 = Xi->x[0];
  double eta2 = Xi->y[0];

  con = face[fac[0]].con;

  if(con)
    if((fac[0] == 1)||(fac[0] == 3)) // sort out connectivity
      eta1 = -eta1;
    else{ // sort out orientation and connectivity
      double tmp;
      switch(con){
      case 1: // negate 'a'
  eta1 = -eta1;
  break;
      case 2: // negate 'b'
  eta2 = -eta2;
  break;
      case 3: // negate 'a' and 'b'
  eta1 = -eta1;
  eta2 = -eta2;
  break;
      case 4: // transpose
  tmp =  eta1;
  eta1 = eta2;
  eta2 = tmp;
  break;
      case 5: // negate 'b' and transpose
  tmp  = -eta2;
  eta2 =  eta1;
  eta1 =  tmp;
  break;
      case 6: // negate 'a' and transpose
  tmp  = -eta1;
  eta1 =  eta2;
  eta2 =  tmp;
  break;
      case 7: // negate 'a' and 'b' and transpose
  tmp  = -eta1;
  eta1 = -eta2;
  eta2 =  tmp;
    break;
      }
    }

  switch(fac[0]){
  case 0:
    Xi->x[0] = eta1;
    Xi->y[0] = eta2;
    Xi->z[0] = -1;
    break;
  case 1:
    Xi->x[0] = 0.5*(eta1+1)*(1-eta2)-1;
    Xi->y[0] = -1;
    Xi->z[0] = eta2;
    break;
  case 2:
    Xi->z[0] = eta2;
    Xi->y[0] = eta1;
    Xi->x[0] = -Xi->z[0];
    break;
  case 3:
    Xi->x[0] = 0.5*(eta1+1)*(1-eta2)-1;
    Xi->y[0] = 1;
    Xi->z[0] = eta2;
    break;
  case 4:
    Xi->x[0] = -1;
    Xi->y[0] = eta1;
    Xi->z[0] = eta2;
    break;
  }
}

int Pyr::lcoords2face(Coord *Xi, int *fac){
  fprintf(stderr,"Pyramidic: lcoords2face not setup\n");
  exit(-1);
  return -1;
}

void Pyr::face2lcoords(Coord *Xi, int *fac){
  fprintf(stderr,"Pyramidic: face2lcoords not setup\n");
  exit(-1);
}

int Hex::lcoords2face(Coord *Xi, int *fac){
  fprintf(stderr,"Hex: lcoords2face not setup\n");
  exit(-1);
  return -1;
}

void Hex::face2lcoords(Coord *Xi, int *fac){
  fprintf(stderr,"Hex: face2lcoords not setup\n");
  exit(-1);
}

void Element::Cart_to_coll(Coord csi, Coord *A){ERR;}

void Tri::Cart_to_coll(Coord csi, Coord *A){

  A->x[0] = (1-csi.y[0])? 2*(1 + csi.x[0])/(1 - csi.y[0]) -1: 0;
  A->y[0] = csi.y[0];

}

void Quad::Cart_to_coll(Coord csi, Coord *A){
  A->x[0] = csi.x[0];
  A->y[0] = csi.y[0];
}

void Tet::Cart_to_coll(Coord csi, Coord *A){
  A->x[0] = (csi.y[0]+csi.z[0])? 2*(1+csi.x[0])/(-csi.y[0]-csi.z[0])-1:0.0;
  A->y[0] = (1-csi.z[0])? 2*(1+csi.y[0])/(1-csi.z[0])-1:0.0;
  A->z[0] = csi.z[0];

}

void Pyr::Cart_to_coll(Coord csi, Coord *A){
  fprintf(stderr,"Cart_to_coll not implemented\n");
}

void Prism::Cart_to_coll(Coord csi, Coord *A){

  A->x[0] = (1-csi.z[0])? 2*(1+csi.x[0])/(1-csi.z[0])-1:0.0;
  A->y[0] = csi.y[0];
  A->z[0] = csi.z[0];

}

void Hex::Cart_to_coll(Coord csi, Coord *A){

  A->x[0] = csi.x[0];
  A->y[0] = csi.y[0];
  A->z[0] = csi.z[0];
}

// Convert Collapsed to Cartesian reusing data
void Element::Coll_to_cart(Coord A, Coord *csi){ERR;}

void Tri::Coll_to_cart(Coord A, Coord *csi){

  csi->x[0] = 0.5*(1+A.x[0])*(1-A.y[0])-1;
  csi->y[0] = A.y[0];

}

void Quad::Coll_to_cart(Coord A, Coord *csi){

  csi->x[0] = A.x[0];
  csi->y[0] = A.y[0];

}

void Tet::Coll_to_cart(Coord A, Coord *csi){

  csi->x[0] = 0.25*(1+A.x[0])*(1-A.y[0])*(1-A.z[0]) - 1;
  csi->y[0] = 0.5*(1+A.y[0])*(1-A.z[0]) - 1;
  csi->z[0] = A.z[0];

}

void Prism::Coll_to_cart(Coord A, Coord *csi){

  csi->x[0] = 0.5*(1+A.x[0])*(1-A.z[0]) - 1;
  csi->y[0] = A.y[0];
  csi->z[0] = A.z[0];

}

void Pyr::Coll_to_cart(Coord A, Coord *csi){
  fprintf(stderr,"Pyramid: Coll_to_cart not implemented\n");
}

void Hex::Coll_to_cart(Coord A, Coord *csi){

  csi->x[0] = A.x[0];
  csi->y[0] = A.y[0];
  csi->z[0] = A.z[0];

}
