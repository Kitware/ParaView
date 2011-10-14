/**************************************************************************/
//                                                                        //
//   Author:    T.Warburton                                               //
//   Design:    T.Warburton && S.Sherwin                                  //
//   Date  :    12/4/96                                                   //
//                                                                        //
//   Copyright notice:  This code shall not be replicated or used without //
//                      the permission of the author.                     //
//                                                                        //
/**************************************************************************/

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <polylib.h>
#include "veclib.h"
#include "hotel.h"

#include <stdio.h>

using namespace polylib;


#define TANTOL 1e-10
/* new atan2 function to stop Nan on atan(0,0)*/
static double atan2_proof (double x, double y)
{
  if (fabs(x) + fabs(y) > TANTOL) return (atan2(x,y));
  else return (0.);
}
#define atan2 atan2_proof



typedef struct point    {  /* A 2-D point  */
  double  x,y;             /* coordinate   */
} Point;


void Load_HO_Surface(char *name);
void genSurfFile(Element *E, double *x, double *y, double *z, Curve *curve);

static void Tet_fill_curvx(Bndry *Bc, Cmodes *cx, int fac);
       void Pyr_fill_curvx(Bndry *Bc, Cmodes *cx);
static void Prism_fill_curvx(Bndry *Bc, Cmodes *cx);
static void Hex_fill_curvx(Bndry *Bc, Cmodes *cx);
       void Tet_FreeMemBndry(Bndry *B);
       void Pyr_FreeMemBndry(Bndry *B);
       void Prism_FreeMemBndry(Bndry *B);
static void Hex_FreeMemBndry(Bndry *B);
static Cmodes *SetCXMem(Element *E);
static void Tet_fill_curvx(Bndry *Bc, Cmodes *cx, int fac);
static void Prism_fill_curvx(Bndry *Bc, Cmodes *cx);
static void Hex_fill_curvx(Bndry *Bc, Cmodes *cx);
static void Hex_FreeMemBndry(Bndry *B);
static void Tet_JacProj(Bndry *B, Coord *S, double nx, double ny, double nz);
static void Tet_JacProj(Bndry *B);

/*

Function name: Element::set_curved_elmt

Function Purpose:

Argument 1: Element_List *U
Purpose:

Function Notes:

*/

void Tri::set_curved_elmt(Element_List *U){
//fprintf(stderr, "Coords.C:Tri::set_curved_elmt(): ENTER\n");
  U=U;
  // set up known curved element -- only allowed one curved side per element

  if(curve){
    int     f;
    double  *x = dvector(0,QGmax-1);
    double  *y = dvector(0,QGmax-1);
    Curve   *cur;

    if(!curvX) curvX = SetCXMem(this);
      /* set up curved face */

    for(cur=curve;cur;cur=cur->next){
      f = cur->face;

      switch(cur->type){
      case T_Straight:
  break; /* do nothing since mode are all zero */
      case T_Arc:
  genArc        (cur, x,y);
  goto TransformEdge;
      case T_Naca4:
  genNaca4      (cur, x,y);
  goto TransformEdge;
      case T_Naca2d:
  Tri_genNaca(this, cur, x,y);
  goto TransformEdge;
      case T_File:
  genFile       (cur, x,y);
  //      CheckVertLoc(U, (Element*)this,f);
  goto TransformEdge;
      case T_Sin:
  gen_sin(this, cur, x, y);
  goto TransformEdge;
      case T_Ellipse:
  gen_ellipse(this, cur, x, y);
  goto TransformEdge;
      TransformEdge:
  CoordTransEdge(x,curvX[0].Cedge[f],f);
  CoordTransEdge(y,curvX[1].Cedge[f],f);
  break;
      default:
  error_msg(unknown curved side type);
  break;
      }
    }
    free(x); free(y);
  }
}




void Quad::set_curved_elmt(Element_List *){
  int     f;
  double  *x = dvector(0,QGmax-1);
  double  *y = dvector(0,QGmax-1);
  Curve   *cur;
  //fprintf(stderr, "Coords.C:Quad::set_curved_elmt(): ENTER\n");
  /* set up known curved element -- only allowed one curved side per element */
  if(!curvX) curvX = SetCXMem(this);

  /* set up curved face */
  for(cur=curve;cur;cur=cur->next){
    f = cur->face;
    //fprintf(stderr, "Coords.C: set_curved_elmt(): cur->type = %s\n", cur->type);
    switch(cur->type){
    case T_Straight: // default for quad.s
      break; /* do nothing since mode are all zero */
    case T_Arc:
      //    error_msg(Quad::set_curved_elmt arcs not implemented for quads);
      genArc  (cur,x,y);
      goto TransformEdge;
    case T_Naca4:
      //    error_msg(Quad::set_curved_elmt naca4 not implemented for quads);
      genNaca4(cur,x,y);
      goto TransformEdge;
    case T_Naca2d:
      Quad_genNaca(this, cur, x,y);
      if(fabs(x[0]-vert[vnum(f,0)].x)>1e-8)
  fprintf(stderr, "Quad: %d ::Naca is      %lf away from surface\n",
    id+1,fabs(x[0]-vert[vnum(f,0)].x));
      if(fabs(y[0]-vert[vnum(f,0)].y)>1e-8)
  fprintf(stderr, "Quad: %d ::Naca is      %lf away from surface\n",
    id+1,fabs(y[0]-vert[vnum(f,0)].y));

      goto TransformEdge;
    case T_File:
      genFile       (cur,x,y);
      goto TransformEdge;
    case T_Sin:
      gen_sin(this, cur, x, y);
      goto TransformEdge;
    case T_Ellipse:
      gen_ellipse(this, cur, x, y);
      goto TransformEdge;
    TransformEdge:
      CoordTransEdge(x,curvX[0].Cedge[f],f);
      CoordTransEdge(y,curvX[1].Cedge[f],f);
      break;
    default:
      error_msg(unknown curved side type);
      break;
    }
  }

  free(x); free(y);
  //fprintf(stderr, "Coords.C:Quad::set_curved_elmt(): EXIT\n");
}




void Tet::set_curved_elmt(Element_List *U){
//fprintf(stderr, "Coords.C:Tet::set_curved_elmt(): ENTER\n");
  if(!curve)
  {
      //fprintf(stderr, "Coords.C:Tet::set_curved_elmt(): EXIT\n");
    return;
  }

  Bndry *Bc;
  Edge *e;
  Element *E;
  int i;
  Coord X;
  double *x, *y, *z;
  double v[3][3];
  double *tmp;
  Basis *b = getbasis();
  if(!curvX) curvX = SetCXMem(this);
#if 0
  if(curve->type == T_Straight)
    return;
#endif
  /*
      fprintf(stderr, "Tet::set_curved_elmt curving element 0 face 0\n",
      id+1, curve->face+1);
  */

  X.x = x = dvector(0, QGmax*QGmax-1);
  X.y = y = dvector(0, QGmax*QGmax-1);
  X.z = z = dvector(0, QGmax*QGmax-1);

  tmp = dvector(0, QGmax*QGmax-1);

  Curve *cur;
  for(cur=curve;cur;cur=cur->next){
    if(cur->type != T_Straight && cur->type != T_Curved){
    int fac = cur->face;
    int vn;

    dzero(qa*qb,X.x,1);
    dzero(qa*qb,X.y,1);
    dzero(qa*qb,X.z,1);

    for(i = 0; i < Nfverts(fac); ++i){
      vn = vnum(fac,i);
      Tet_faceMode(this,0,b->vert + i,tmp);
      daxpy(qa*qb,vert[vn].x,tmp,1,X.x,1);
      daxpy(qa*qb,vert[vn].y,tmp,1,X.y,1);
      daxpy(qa*qb,vert[vn].z,tmp,1,X.z,1);
      v[i][0] = vert[vn].x;
      v[i][1] = vert[vn].y;
      v[i][2] = vert[vn].z;

    }

    switch(cur->type){
    case T_Straight:
      break; /* do nothing since mode are all zero */
    case T_Cylinder:
      genCylinder  (cur,x,y,z,qa*qb);
      goto      TransformFace;
    case T_Cone:
      genCone      (cur,x,y,z,qa*qb);
      goto      TransformFace;
    case T_Sphere:
      genSphere    (cur,x,y,z,qa*qb);
      goto      TransformFace;
    case T_Sheet:
      genSheet     (cur,x,y,z,qa*qb);
      goto      TransformFace;
    case T_Spiral:
      genSpiral    (cur,x,y,z,qa*qb);
      goto      TransformFace;
    case T_Taurus:
      genTaurus    (cur,x,y,z,qa*qb);
      goto      TransformFace;
    case T_Free:
      //first 3 members of x,y,z contains coordinates of vertises
      for(i = 0; i < Nfverts(fac); ++i){
        vn = vnum(fac,i);
        x[i] = vert[vn].x;
        y[i] = vert[vn].y;
        z[i] = vert[vn].z;
      }
     // fprintf(stderr,"projecting E->id+1 = %d  face = %d xyz[2] = %f %f %f \n",id+1, fac, x[2], y[2], z[2]);
      genFree    (cur, x,y,z,'a','b',qa,qb);
      goto      TransformFace;
     case T_Recon:
      genRecon     (cur, v[0], v[1], v[2], x, y, z, qa*qb);
      goto      TransformFace;
    case T_Naca3d:
#if 1
      for(i=0;i < Nfverts(fac); ++i)
  genNaca3d(cur, &vert[vnum(fac,i)].x,
      &vert[vnum(fac,i)].y,
      &vert[vnum(fac,i)].z, 1);
#endif
      genNaca3d    (cur,x,y,z,qa*qb);
      goto      TransformFace;
    case T_File:
#if HOSURF
      Load_HO_Surface(cur->info.file.name);
      genSurfFile    (this, x,y,z,cur);
#else
      Load_Felisa_Surface(cur->info.file.name);
      genFelFile   (this, x,y,z,cur);
#endif
      goto      TransformFace;
    TransformFace:
      Bc = gen_bndry('X',0,x);
      for(i=0;i<Nfverts(fac);++i)
  Bc->bvert[i] = vert[vnum(fac,i)].x;

      JtransFace(Bc,x);
      Tet_fill_curvx(Bc, curvX, fac);
      for(i=0;i<Nfverts(fac);++i)
  Bc->bvert[i] = vert[vnum(fac,i)].y;
      JtransFace(Bc,y);
      Tet_fill_curvx(Bc, curvX+1, fac);

      for(i=0;i<Nfverts(fac);++i)
  Bc->bvert[i] = vert[vnum(fac,i)].z;
      JtransFace(Bc,z);
      Tet_fill_curvx(Bc, curvX+2, fac);

      Tet_FreeMemBndry(Bc);

      break;
   default:
      error_msg(unknown curved side type);
      break;
    }

    /* set up elements with edges that just touches curved surfaces */
    for(i = 0; i < Nfverts(fac); ++i)
      for(e = edge[ednum(fac,i)].base; e; e = e->link){
  E = U->flist[e->eid];
  if(!E->curvX) E->curvX = SetCXMem(E);
  if(E->curve && E->curve->type == T_Straight)
    E->curve->type = T_Curved;

  dcopy(e->l,curvX[0].Cedge[ednum(fac,i)], 1, E->curvX[0].Cedge[e->id],1);
  dcopy(e->l,curvX[1].Cedge[ednum(fac,i)], 1, E->curvX[1].Cedge[e->id],1);
  dcopy(e->l,curvX[2].Cedge[ednum(fac,i)], 1, E->curvX[2].Cedge[e->id],1);

  /* invert odd modes if necessary */
  if(edge[ednum(fac,i)].con != e->con){
    dscal(e->l/2,-1.0,E->curvX[0].Cedge[e->id]+1,2);
    dscal(e->l/2,-1.0,E->curvX[1].Cedge[e->id]+1,2);
    dscal(e->l/2,-1.0,E->curvX[2].Cedge[e->id]+1,2);
  }
      }
    }
  }
  free(x);  free(y);  free(z);
  free(tmp);

}




void Pyr::set_curved_elmt(Element_List *U){
  Bndry *Bc;
  Edge *e;
  Element *E;
  int i;
  Coord X;
  double *x, *y, *z;
  double *tmp;
  Curve  *cur;

  if(!curvX) curvX = SetCXMem(this);

#if 0
  if(curve->type == T_Straight)
    return;
#endif

  X.x = x = dvector(0, QGmax*QGmax-1);
  X.y = y = dvector(0, QGmax*QGmax-1);
  X.z = z = dvector(0, QGmax*QGmax-1);


  for(cur=curve;cur;cur=cur->next){
    if(cur->type != T_Straight && cur->type != T_Curved){
      int fac = cur->face, q;

    if(fac == 0)
      q = qa*qb;
    else if(fac == 1 || fac == 3)
      q = qa*qc;
    else
      q = qb*qc;


    straight_face(&X, cur->face,0);

    switch(cur->type){
    case T_Straight:
      break; /* do nothing since mode are all zero */
    case T_Cylinder:
      genCylinder  (cur,x,y,z,q);
      goto      TransformFace;
    case T_Cone:
      genCone      (cur,x,y,z,q);
    goto      TransformFace;
    case T_Sphere:
      genSphere    (cur,x,y,z,q);
      goto      TransformFace;
    case T_Sheet:
      genSheet     (cur,x,y,z,q);
      goto      TransformFace;
    case T_Spiral:
      genSpiral    (cur,x,y,z,q);
      goto      TransformFace;
    case T_Taurus:
      genTaurus    (cur,x,y,z,q);
      goto      TransformFace;
    TransformFace:

      tmp = dvector(0, QGmax*QGmax-1);

      Bc = gen_bndry('X',fac,x);

      for(i=0;i<Nfverts(fac);++i)
  Bc->bvert[i] = vert[vnum(fac,i)].x;
      InterpToFace1(fac,x,tmp);
      JtransFace(Bc,tmp);
      Pyr_fill_curvx(Bc, curvX);

      for(i=0;i<Nfverts(fac);++i)
  Bc->bvert[i] = vert[vnum(fac,i)].y;
      InterpToFace1(fac,y,tmp);
      JtransFace(Bc,tmp);
      Pyr_fill_curvx(Bc, curvX+1);

      for(i=0;i<Nfverts(fac);++i)
  Bc->bvert[i] = vert[vnum(fac,i)].z;
      InterpToFace1(fac,z,tmp);
      JtransFace(Bc,tmp);
      Pyr_fill_curvx(Bc, curvX+2);

      Pyr_FreeMemBndry(Bc);

      free(tmp);

      break;
    default:
      error_msg(unknown curved side type);
      break;
    }

    /* set up elements with edges that just touches curved surfaces */
    for(i = 0; i < Nfverts(fac); ++i)
      for(e = edge[ednum(fac,i)].base; e; e = e->link){
  E = U->flist[e->eid];
  if(E->curve && E->curve->type == T_Straight)
    E->curve->type = T_Curved;
  if(!E->curvX) E->curvX = SetCXMem(E);
  dcopy(e->l,curvX[0].Cedge[ednum(fac,i)],1,E->curvX[0].Cedge[e->id],1);
  dcopy(e->l,curvX[1].Cedge[ednum(fac,i)],1,E->curvX[1].Cedge[e->id],1);
  dcopy(e->l,curvX[2].Cedge[ednum(fac,i)],1,E->curvX[2].Cedge[e->id],1);

  /* invert odd modes if necessary */
  if(edge[ednum(fac,i)].con != e->con){
    dscal(e->l/2,-1.0,E->curvX[0].Cedge[e->id]+1,2);
    dscal(e->l/2,-1.0,E->curvX[1].Cedge[e->id]+1,2);
    dscal(e->l/2,-1.0,E->curvX[2].Cedge[e->id]+1,2);
  }
      }
    }
  }

  free(x);  free(y);  free(z);
}




void Prism::set_curved_elmt(Element_List *U){

  // if(!curvX) curvX = SetCXMem(this);

  Bndry *Bc;
  Edge *e;
  Element *E;
  int i,vn;
  Coord X;
  double *x, *y, *z;
  double *tmp;

  if(!curvX) curvX = SetCXMem(this);
#if 0
  if(curve->type == T_Straight)
    return;
#endif
  X.x = x = dvector(0, QGmax*QGmax-1);
  X.y = y = dvector(0, QGmax*QGmax-1);
  X.z = z = dvector(0, QGmax*QGmax-1);


  Curve *cur;
  for(cur=curve;cur;cur=cur->next){
    if(cur->type != T_Straight && cur->type != T_Curved){
    int fac = cur->face, q;

    if(fac == 0)
      q = qa*qb;
    else if(fac == 1 || fac == 3)
      q = qa*qc;
    else
      q = qb*qc;

    straight_face(&X, cur->face,0);

    switch(cur->type){
    case T_Straight:
      break; /* do nothing since mode are all zero */
    case T_Cylinder:
      genCylinder  (cur,x,y,z,q);
      goto      TransformFace;
    case T_Cone:
      genCone      (cur,x,y,z,q);
      goto      TransformFace;
    case T_Sphere:
      genSphere    (cur,x,y,z,q);
      goto      TransformFace;
    case T_Sheet:
      genSheet     (cur,x,y,z,q);
      goto      TransformFace;
    case T_Spiral:
      genSpiral    (cur,x,y,z,q);
      goto      TransformFace;
    case T_Taurus:
      genTaurus    (cur,x,y,z,q);
      goto      TransformFace;
    case T_Free:
      //first 3 members of x,y,z contains coordinates of vertises
      for(i = 0; i < Nfverts(fac); ++i){
        vn = vnum(fac,i);
        x[i] = vert[vn].x;
        y[i] = vert[vn].y;
        z[i] = vert[vn].z;
      }
     if(fac == 0)
       genFree      (cur, x,y,z,'a','b',qa,qb);
      else if(fac == 1 || fac == 3)
        genFree      (cur, x,y,z,'a','c',qa,qc);
      else
        genFree      (cur, x,y,z,'b','c',qb,qc);
      goto      TransformFace;
    case T_File:
#if HOSURF
      Load_HO_Surface(cur->info.file.name);
      genSurfFile    (this, x,y,z,cur); // returns point in face 1 format
#else
      Load_Felisa_Surface(cur->info.file.name);
      genFelFile   (this, x,y,z,cur);
#endif
      goto      TransformFace;
    case T_Naca3d:
      genNaca3d    (cur,x,y,z,q);
      goto      TransformFace;
    TransformFace:

      tmp = dvector(0, QGmax*QGmax-1);

      Bc = gen_bndry('X',fac,x);

      for(i=0;i<Nfverts(fac);++i)
  Bc->bvert[i] = vert[vnum(fac,i)].x;
      InterpToFace1(fac,x,tmp);
      tmp[0] = Bc->bvert[0]; // set to stop JtranFace warning
      JtransFace(Bc,tmp);
      Prism_fill_curvx(Bc, curvX);

      for(i=0;i<Nfverts(fac);++i)
  Bc->bvert[i] = vert[vnum(fac,i)].y;
      InterpToFace1(fac,y,tmp);
      tmp[0] = Bc->bvert[0]; // set to stop JtranFace warning
      JtransFace(Bc,tmp);
      Prism_fill_curvx(Bc, curvX+1);

      for(i=0;i<Nfverts(fac);++i)
  Bc->bvert[i] = vert[vnum(fac,i)].z;
      InterpToFace1(fac,z,tmp);
      tmp[0] = Bc->bvert[0]; // set to stop JtranFace warning
      JtransFace(Bc,tmp);
      Prism_fill_curvx(Bc, curvX+2);

      Prism_FreeMemBndry(Bc);

      free(tmp);

      break;
    default:
      error_msg(unknown curd side type);
      break;
    }

    /* set up elements with edges that just touches curve surfaces */
    for(i = 0; i < Nfverts(fac); ++i)
      for(e = edge[ednum(fac,i)].base; e; e = e->link){
  E = U->flist[e->eid];

  if(!E->curvX) E->curvX = SetCXMem(E);

  if(E->curve && E->curve->type == T_Straight)
    E->curve->type = T_Curved;

  dcopy(e->l,curvX[0].Cedge[ednum(fac,i)], 1, E->curvX[0].Cedge[e->id],1);
  dcopy(e->l,curvX[1].Cedge[ednum(fac,i)], 1, E->curvX[1].Cedge[e->id],1);
  dcopy(e->l,curvX[2].Cedge[ednum(fac,i)], 1, E->curvX[2].Cedge[e->id],1);

  /* invert odd modes if necessary */
#if 1
  if(edge[ednum(fac,i)].con != e->con){
    dscal(e->l/2,-1.0,E->curvX[0].Cedge[e->id]+1,2);
    dscal(e->l/2,-1.0,E->curvX[1].Cedge[e->id]+1,2);
    dscal(e->l/2,-1.0,E->curvX[2].Cedge[e->id]+1,2);
  }
#endif
      }
    }
  }
  free(x);  free(y);  free(z);
}




void Hex::set_curved_elmt(Element_List *U){
  Bndry *Bc;
  Edge *e;
  Element *E;
  int i;
  Coord X;
  double *x, *y, *z;
  double *tmp;

  if(!curvX) curvX = SetCXMem(this);

#if 0
  if(curve->type == T_Straight)
    return;
#endif
  Curve *cur;

  X.x = x = dvector(0, QGmax*QGmax-1);
  X.y = y = dvector(0, QGmax*QGmax-1);
  X.z = z = dvector(0, QGmax*QGmax-1);

  for(cur=curve;cur;cur=cur->next){
    if(cur->type != T_Straight && cur->type != T_Curved){
    int fac = cur->face, q;

    if(fac == 0 || fac == 5)
      q = qa*qb;
    else if(fac == 1 || fac == 3)
      q = qa*qc;
    else
      q = qb*qc;

    straight_face(&X, cur->face,0);

    switch(cur->type){
    case T_Straight:
      break; /* do nothing since mode are all zero */
    case T_Cylinder:
      genCylinder  (cur,x,y,z,q);
      goto      TransformFace;
    case T_Cone:
      genCone      (cur,x,y,z,q);
      goto      TransformFace;
    case T_Sphere:
      genSphere    (cur,x,y,z,q);
      goto      TransformFace;
    case T_Sheet:
      genSheet     (cur,x,y,z,q);
      goto      TransformFace;
    case T_Spiral:
      genSpiral    (cur,x,y,z,q);
      goto      TransformFace;
    case T_Taurus:
      genTaurus    (cur,x,y,z,q);
      goto      TransformFace;
    case T_Naca3d:
      genNaca3d    (cur,x,y,z,q);
      goto      TransformFace;
    TransformFace:

      tmp = dvector(0, QGmax*QGmax-1);

      Bc = gen_bndry('X',fac,x);

      for(i=0;i<Nfverts(fac);++i)
  Bc->bvert[i] = vert[vnum(fac,i)].x;
      InterpToFace1(fac,x,tmp);
      JtransFace(Bc,tmp);
      Hex_fill_curvx(Bc, curvX);
      for(i=0;i<Nfverts(fac);++i)
  vert[vnum(fac,i)].x = Bc->bvert[i];

      for(i=0;i<Nfverts(fac);++i)
  Bc->bvert[i] = vert[vnum(fac,i)].y;
      InterpToFace1(fac,y,tmp);
      JtransFace(Bc,tmp);
      Hex_fill_curvx(Bc, curvX+1);
      for(i=0;i<Nfverts(fac);++i)
  vert[vnum(fac,i)].y = Bc->bvert[i];

      for(i=0;i<Nfverts(fac);++i)
  Bc->bvert[i] = vert[vnum(fac,i)].z;
      InterpToFace1(fac,z,tmp);
      JtransFace(Bc,tmp);
      Hex_fill_curvx(Bc, curvX+2);
      for(i=0;i<Nfverts(fac);++i)
  vert[vnum(fac,i)].z = Bc->bvert[i];
      Hex_FreeMemBndry(Bc);

      free(tmp);

      break;
    default:
      error_msg(unknown curd side type);
      break;
    }

    /* set up elements with edges that just touches curd surfaces */
    for(i = 0; i < Nfverts(fac); ++i)
      for(e = edge[ednum(fac,i)].base; e; e = e->link){
  E = U->flist[e->eid];

  if(E->curve && E->curve->type == T_Straight)
    E->curve->type = T_Curved;

  if(!E->curvX) E->curvX = SetCXMem(E);
  dcopy(e->l,curvX[0].Cedge[ednum(fac,i)], 1, E->curvX[0].Cedge[e->id],1);
  dcopy(e->l,curvX[1].Cedge[ednum(fac,i)], 1, E->curvX[1].Cedge[e->id],1);
  dcopy(e->l,curvX[2].Cedge[ednum(fac,i)], 1, E->curvX[2].Cedge[e->id],1);

  /* invert odd modes if necessary */
  if(edge[ednum(fac,i)].con != e->con){
    dscal(e->l/2,-1.0,E->curvX[0].Cedge[e->id]+1,2);
    dscal(e->l/2,-1.0,E->curvX[1].Cedge[e->id]+1,2);
    dscal(e->l/2,-1.0,E->curvX[2].Cedge[e->id]+1,2);
  }
    }
    }
  }
  free(x);  free(y);  free(z);
}




void Element::set_curved_elmt(Element_List*){ERR;}



static Cmodes *SetCXMem(Element *E){
  register int i,j;
  Cmodes *cx;
  Curve *cur;

  cx = (Cmodes *)calloc(E->dim(),sizeof(Cmodes));
  for(i = 0; i < E->dim(); ++i){
    for(j = 0; j < E->Nedges; ++j)
      cx[i].Cedge[j] = (double *)calloc(max(E->edge[j].l,1),sizeof(double));
    if(E->dim() == 3){
      for(j = 0; j < E->Nfaces; ++j){
  cx[i].Cface[j] = dmatrix(0, QGmax-1, 0, QGmax-1);
  dzero(QGmax*QGmax, cx[i].Cface[j][0], 1);
      }
    }
#if 0
    if(cur = E->curve)
      while(cur){
  cx[i].Cface[cur->face] = dmatrix(0, QGmax-1, 0, QGmax-1);
    cur=cur->next;
  }
#endif
  }
  return cx;
}



static void Tet_fill_curvx(Bndry *Bc, Cmodes *cx, int fac){
  int i;
  Element *E = Bc->elmt;

  for(i=0;i<E->Nfverts(fac);++i)
    dcopy(E->edge[E->ednum(fac,i)].l, Bc->bedge[i], 1,
    cx->Cedge[E->ednum(fac,i)], 1);

  i = E->face[fac].l*(E->face[fac].l+1)/2;
  if(i)
    dcopy(i, Bc->bface[0], 1, cx->Cface[fac][0], 1);
}


void Pyr_fill_curvx(Bndry *Bc, Cmodes *cx){
  int i, fac = Bc->face;
  Element *E = Bc->elmt;

  for(i=0;i<E->Nfverts(fac);++i)
    dcopy(E->edge[E->ednum(fac,i)].l, Bc->bedge[i], 1,
    cx->Cedge[E->ednum(fac,i)], 1);

  i = (E->Nfverts(fac) == 4) ?
    E->face[fac].l*E->face[fac].l : E->face[fac].l*(E->face[fac].l+1)/2;

  dcopy(i, Bc->bface[0], 1, cx->Cface[fac][0], 1);
}



static void Prism_fill_curvx(Bndry *Bc, Cmodes *cx){
  int i, fac = Bc->face;
  Element *E = Bc->elmt;

  for(i=0;i<E->Nfverts(fac);++i)
    dcopy(E->edge[E->ednum(fac,i)].l, Bc->bedge[i], 1,
    cx->Cedge[E->ednum(fac,i)], 1);

  i = (E->Nfverts(fac) == 4) ?
    E->face[fac].l*E->face[fac].l : E->face[fac].l*(E->face[fac].l+1)/2;

  if(i)
    dcopy(i, Bc->bface[0], 1, cx->Cface[fac][0], 1);
}



static void Hex_fill_curvx(Bndry *Bc, Cmodes *cx){
  int i, fac = Bc->face;
  Element *E = Bc->elmt;

  for(i=0;i<E->Nfverts(fac);++i)
    dcopy(E->edge[E->ednum(fac,i)].l, Bc->bedge[i], 1,
    cx->Cedge[E->ednum(fac,i)], 1);

  i = (E->Nfverts(fac) == 4) ?
    E->face[fac].l*E->face[fac].l : E->face[fac].l*(E->face[fac].l+1)/2;

  if(i)
    dcopy(i, Bc->bface[0], 1, cx->Cface[fac][0], 1);
}


void Tet_FreeMemBndry(Bndry *B){
  int i;
  Element *E = B->elmt;
  free(B->bvert);

  for(i = 0; i < E->Nfverts(B->face); ++i)
    if(E->edge[E->ednum(B->face,i)].l)
      free(B->bedge[i]);

  if(E->face[B->face].l){
    free(B->bface[0]);
    free(B->bface);
  }
  free(B);
}



void Pyr_FreeMemBndry(Bndry *B){
  int i;
  Element *E = B->elmt;
  free(B->bvert);

  for(i = 0; i < E->Nfverts(B->face); ++i)
    if(E->edge[E->ednum(B->face,i)].l)
      free(B->bedge[i]);

  if(E->face[B->face].l){
    free(B->bface[0]);
    free(B->bface);
  }
  free(B);
}



void Prism_FreeMemBndry(Bndry *B){
  int i;
  Element *E = B->elmt;
  free(B->bvert);

  for(i = 0; i < E->Nfverts(B->face); ++i)
    if(E->edge[E->ednum(B->face,i)].l)
      free(B->bedge[i]);

  if(E->face[B->face].l){
    free(B->bface[0]);
    free(B->bface);
  }

  if(option("OpSplit"))
    free(B->phys_val_g);

  free(B);
}



static void Hex_FreeMemBndry(Bndry *B){
  int i;
  Element *E = B->elmt;
  free(B->bvert);

  for(i = 0; i < E->Nfverts(B->face); ++i)
    if(E->edge[E->ednum(B->face,i)].l)
      free(B->bedge[i]);

  if(E->face[B->face].l){
    free(B->bface[0]);
    free(B->bface);
  }
  free(B);
}







/*

Function name: Element::coord

Function Purpose:

Argument 1: Coord *X
Purpose:

Function Notes:

*/

void Tri::coord(Coord *X){
  if(curvX)
    curved_elmt  (X);
  else
    straight_elmt(X);
}




void Quad::coord(Coord *X){
    curved_elmt  (X);
}




void Tet::coord(Coord *X){
  if(curvX)
    curved_elmt  (X);
  else
    straight_elmt(X);
}




void Pyr::coord(Coord *X){
    curved_elmt  (X);
}




void Prism::coord(Coord *X){
    curved_elmt  (X);
}




void Hex::coord(Coord *X){
    curved_elmt  (X);
}




void Element::coord(Coord *){ERR;}                 // get quadrature coords




/*

Function name: Element::straight_elmt

Function Purpose:
 Calculate coordinates of this element assuming that it is straight sided.

Argument 1: Coord *X
Purpose:
 Provides storage for the coordinate output.

Function Notes:

*/

void Tri::straight_elmt(Coord *X){
  register int i;
  const    int qt =  qa*qb;
  double  *tmp;
  Vert    *v = vert;
  Mode    *bv = getbasis()->vert;

  tmp = dvector(0,qt-1);

  dzero(qt,X->x,1);
  dzero(qt,X->y,1);

  for(i = 0; i < Nverts; ++i){
    fillvec(bv+i,tmp);
    daxpy(qt,v[i].x,tmp,1,X->x,1);
    daxpy(qt,v[i].y,tmp,1,X->y,1);
  }

  free(tmp);
}




void Quad::straight_elmt(Coord *X){
  register int i;
  const    int qt =  qa*qb;
  double  *tmp;
  Vert    *v = vert;
  Mode    *bv = getbasis()->vert;

  tmp =  dvector(0,qt-1);

  dzero(qt,X->x,1);
  dzero(qt,X->y,1);

  for(i = 0; i < Nverts; ++i){
    fillvec(bv+i,tmp);
    daxpy(qt,v[i].x,tmp,1,X->x,1);
    daxpy(qt,v[i].y,tmp,1,X->y,1);
  }
  free(tmp);
}




void Tet::straight_elmt(Coord *X){
  register int i;
  const    int qt =  qtot;
  double  *tmp;
  Vert    *v = vert;
  Mode    *bv = getbasis()->vert;

  tmp = dvector(0,qt-1);

  dzero(qt,X->x,1);
  dzero(qt,X->y,1);
  dzero(qt,X->z,1);


  for(i = 0; i < Nverts; ++i){
    fillvec(bv+i,tmp);
    daxpy(qt,v[i].x,tmp,1,X->x,1);
    daxpy(qt,v[i].y,tmp,1,X->y,1);
    daxpy(qt,v[i].z,tmp,1,X->z,1);
  }

  free(tmp);
}




void Pyr::straight_elmt(Coord *X){
  register int i;
  const    int qt =  qtot;
  Vert    *v = vert;
  Mode    *bv = getbasis()->vert;

  double *save = dvector(0, qtot-1);
  dcopy(qtot, h_3d[0][0], 1, save, 1);

  dzero(qt,X->x,1);
  dzero(qt,X->y,1);
  dzero(qt,X->z,1);

  for(i = 0; i < NPyr_verts; ++i){
    fillElmt(bv+i);
    daxpy(qt,v[i].x,h_3d[0][0],1,X->x,1);
    daxpy(qt,v[i].y,h_3d[0][0],1,X->y,1);
    daxpy(qt,v[i].z,h_3d[0][0],1,X->z,1);
  }

  dcopy(qtot, save, 1, h_3d[0][0], 1);
  free(save);
}




void Prism::straight_elmt(Coord *X){
  register int i;
  const    int qt =  qtot;
  Vert    *v = vert;
  Mode    *bv = getbasis()->vert;

  double *save = dvector(0, qtot-1);
  dcopy(qtot, h_3d[0][0], 1, save, 1);

  dzero(qt,X->x,1);
  dzero(qt,X->y,1);
  dzero(qt,X->z,1);

  for(i = 0; i < NPrism_verts; ++i){
    fillElmt(bv+i);
    daxpy(qt,v[i].x,h_3d[0][0],1,X->x,1);
    daxpy(qt,v[i].y,h_3d[0][0],1,X->y,1);
    daxpy(qt,v[i].z,h_3d[0][0],1,X->z,1);
  }

  dcopy(qtot, save, 1, h_3d[0][0], 1);
  free(save);
}




void Hex::straight_elmt(Coord *X){
  register int i;
  const    int qt =  qtot;
  Vert    *v = vert;
  Mode    *bv = getbasis()->vert;

  double *save = dvector(0, qtot-1);
  dcopy(qtot, h_3d[0][0], 1, save, 1);

  dzero(qt,X->x,1);
  dzero(qt,X->y,1);
  dzero(qt,X->z,1);

  for(i = 0; i < NHex_verts; ++i){
    fillElmt(bv+i);
    daxpy(qt,v[i].x,h_3d[0][0],1,X->x,1);
    daxpy(qt,v[i].y,h_3d[0][0],1,X->y,1);
    daxpy(qt,v[i].z,h_3d[0][0],1,X->z,1);
  }

  dcopy(qtot, save, 1, h_3d[0][0], 1);
  free(save);
}




void Element::straight_elmt(Coord *){ERR;}         //




/*

Function name: Element::curved_elmt

Function Purpose:

Argument 1: Coord *X
Purpose:

Function Notes:

*/

void Tri::curved_elmt(Coord *X){

  int i,st;
  Basis *B = getbasis();

  double *save = dvector(0, qtot-1);
  double *save_hj = dvector(0, Nmodes-1);

  st = state;

  dcopy(qtot,   h[0], 1,    save, 1);
  dcopy(Nmodes, vert[0].hj, 1, save_hj, 1);

  dzero(Nmodes, vert[0].hj, 1);

  // x
  for(i=0;i<NTri_verts;++i)
    vert[i].hj[0] = vert[i].x;

  if(curvX)
    for(i=0;i<NTri_edges;++i)
      dcopy(edge[i].l, curvX[0].Cedge[i], 1, edge[i].hj, 1);

  Jbwd(this, B);
  dcopy(qtot, *h, 1, X->x, 1);

  dzero(Nmodes, vert[0].hj, 1);
  // y
  for(i=0;i<NTri_verts;++i)
    vert[i].hj[0] = vert[i].y;

  if(curvX)
    for(i=0;i<NTri_edges;++i)
      dcopy(edge[i].l, curvX[1].Cedge[i], 1, edge[i].hj, 1);

  Jbwd(this, B);
  dcopy(qtot, *h, 1, X->y, 1);

  dcopy(qtot,    save, 1, h[0], 1);
  dcopy(Nmodes, save_hj, 1, vert[0].hj, 1);

  state = st;

  free(save);  free(save_hj);
}




void Quad::curved_elmt(Coord *X){
#if 0
  // CURVED_ELMT
  register int i;
  Cmodes   *Cx = curvX;
  Vert     *v  = vert;
  Basis    *b  = getbasis();

  double   *f  = dvector(0,QGmax*QGmax-1);
  double   *tmpx = dvector(0,QGmax-1);
  double   *tmpy = dvector(0,QGmax-1);

  /*----------------------------*/
  /* vertex A, vertex D, edge 4 */
  /*----------------------------*/

  dsmul(qb,v[0].x,b->vert[0].b,1,tmpx,1);
  dsmul(qb,v[0].y,b->vert[0].b,1,tmpy,1);

  daxpy(qb,v[3].x,b->vert[3].b,1,tmpx,1);
  daxpy(qb,v[3].y,b->vert[3].b,1,tmpy,1);

  for(i = 0; i < edge[3].l; ++i){
    daxpy(qb,Cx[0].Cedge[3][i],b->edge[3][i].b,1,tmpx,1);
    daxpy(qb,Cx[1].Cedge[3][i],b->edge[3][i].b,1,tmpy,1);
  }

  /* blend info */
  for(i = 0; i < qb; ++i){
    dsmul(qa, tmpx[i] ,b->vert[0].a,1,X->x+i*qa,1);
    dsmul(qa, tmpy[i] ,b->vert[0].a,1,X->y+i*qa,1);
  }

  /*----------------------------*/
  /* vertex B, vertex C, edge 2 */
  /*----------------------------*/

  dsmul(qb,v[1].x,b->vert[1].b,1,tmpx,1);
  dsmul(qb,v[1].y,b->vert[1].b,1,tmpy,1);

  daxpy(qb,v[2].x,b->vert[2].b,1,tmpx,1);
  daxpy(qb,v[2].y,b->vert[2].b,1,tmpy,1);

  for(i = 0; i < edge[1].l; ++i){
    daxpy(qb,Cx[0].Cedge[1][i],b->edge[1][i].b,1,tmpx,1);
    daxpy(qb,Cx[1].Cedge[1][i],b->edge[1][i].b,1,tmpy,1);
  }

  /* blend info */
  for(i = 0; i < qb; ++i){
    daxpy(qa, tmpx[i] ,b->vert[1].a,1,X->x+i*qa,1);
    daxpy(qa, tmpy[i] ,b->vert[1].a,1,X->y+i*qa,1);
  }

  /*----------*/
  /* edge   1 */
  /*----------*/

  if(edge[0].l){
    dsmul(qa, Cx[0].Cedge[0][0],b->edge[0][0].a,1,tmpx,1);
    dsmul(qa, Cx[1].Cedge[0][0],b->edge[0][0].a,1,tmpy,1);
    for(i=1;i<edge[0].l;++i){
      daxpy(qa,Cx[0].Cedge[0][i],b->edge[0][i].a,1,tmpx,1);
      daxpy(qa,Cx[1].Cedge[0][i],b->edge[0][i].a,1,tmpy,1);
    }

    /* blend info */
    for(i = 0; i < qa; ++i){
      daxpy(qb, tmpx[i] ,b->vert[0].b,1,X->x+i,qa);
      daxpy(qb, tmpy[i] ,b->vert[0].b,1,X->y+i,qa);
    }
  }

  /*----------*/
  /* edge   3 */
  /*----------*/

  if(edge[2].l){
    dsmul(qa, Cx[0].Cedge[2][0],b->edge[2][0].a,1,tmpx,1);
    dsmul(qa, Cx[1].Cedge[2][0],b->edge[2][0].a,1,tmpy,1);
    for(i=1;i<edge[2].l;++i){
      daxpy(qa,Cx[0].Cedge[2][i],b->edge[2][i].a,1,tmpx,1);
      daxpy(qa,Cx[1].Cedge[2][i],b->edge[2][i].a,1,tmpy,1);
    }

    /* blend info */
    for(i = 0; i < qa; ++i){
      daxpy(qb, tmpx[i] ,b->vert[2].b,1,X->x+i,qa);
      daxpy(qb, tmpy[i] ,b->vert[2].b,1,X->y+i,qa);
    }
  }

  free(f); free(tmpx); free(tmpy);
#endif
  int i,st;
  Basis *B = getbasis();

  double *save = dvector(0, qtot-1);
  double *save_hj = dvector(0, Nmodes-1);

  st = state;

  dcopy(qtot,   h[0], 1,    save, 1);
  dcopy(Nmodes, vert[0].hj, 1, save_hj, 1);

  dzero(Nmodes, vert[0].hj, 1);

  // x
  for(i=0;i<NQuad_verts;++i)
    vert[i].hj[0] = vert[i].x;

  if(curvX)
    for(i=0;i<NQuad_edges;++i)
      dcopy(edge[i].l, curvX[0].Cedge[i], 1, edge[i].hj, 1);

  Jbwd(this, B);
  dcopy(qtot, *h, 1, X->x, 1);

  dzero(Nmodes, vert[0].hj, 1);
  // y
  for(i=0;i<NQuad_verts;++i)
    vert[i].hj[0] = vert[i].y;

  if(curvX)
    for(i=0;i<NQuad_edges;++i)
      dcopy(edge[i].l, curvX[1].Cedge[i], 1, edge[i].hj, 1);

  Jbwd(this, B);
  dcopy(qtot, *h, 1, X->y, 1);

  dcopy(qtot,    save, 1, h[0], 1);
  dcopy(Nmodes, save_hj, 1, vert[0].hj, 1);

  state = st;

  free(save);  free(save_hj);
}




void Tet::curved_elmt(Coord *X){

  int i,st,ll =0;
  Basis *B = getbasis();
  Curve *cur;
  double *save = dvector(0, qtot-1);
  double *save_hj = dvector(0, Nmodes-1);

  st = state;

  dcopy(qtot,   h_3d[0][0], 1,    save, 1);
  dcopy(Nmodes, vert[0].hj, 1, save_hj, 1);

  dzero(Nmodes, vert[0].hj, 1);

  // x
  for(i=0;i<NTet_verts;++i)
    vert[i].hj[0] = vert[i].x;

  for(i=0;i<NTet_edges;++i)
    dcopy(edge[i].l, curvX[0].Cedge[i], 1, edge[i].hj, 1);

  if(curvX)
    for(i=0;i<Nfaces;++i){
      ll = face[i].l;
      ll = (Nfverts(i) == 3) ? ll*(ll+1)/2 : ll*ll;
      if(ll)
  dcopy(ll, curvX[0].Cface[i][0], 1, face[i].hj[0], 1);
    }

  Jbwd(this, B);
  dcopy(qtot, **h_3d, 1, X->x, 1);

  dzero(Nmodes, vert[0].hj, 1);
  // y
  for(i=0;i<NTet_verts;++i)
    vert[i].hj[0] = vert[i].y;

  for(i=0;i<NTet_edges;++i)
    dcopy(edge[i].l, curvX[1].Cedge[i], 1, edge[i].hj, 1);

  if(curvX)
    for(i=0;i<Nfaces;++i){
      ll = face[i].l;
      ll = (Nfverts(i) == 3) ? ll*(ll+1)/2 : ll*ll;
      if(ll)
  dcopy(ll, curvX[1].Cface[i][0], 1, face[i].hj[0], 1);
    }

  Jbwd(this, B);
  dcopy(qtot, **h_3d, 1, X->y, 1);

  dzero(Nmodes, vert[0].hj, 1);
  // z
  for(i=0;i<NTet_verts;++i)
    vert[i].hj[0] = vert[i].z;

  for(i=0;i<NTet_edges;++i)
    dcopy(edge[i].l, curvX[2].Cedge[i], 1, edge[i].hj, 1);

  if(curvX)
    for(i=0;i<Nfaces;++i){
      ll = face[i].l;
      ll = (Nfverts(i) == 3) ? ll*(ll+1)/2 : ll*ll;
      if(ll)
  dcopy(ll, curvX[2].Cface[i][0], 1, face[i].hj[0], 1);
    }


  Jbwd(this, B);
  dcopy(qtot, **h_3d, 1, X->z, 1);

  dcopy(qtot,    save, 1, h_3d[0][0], 1);
  dcopy(Nmodes, save_hj, 1, vert[0].hj, 1);

  state = st;

  free(save);  free(save_hj);

}




void Pyr::curved_elmt(Coord *X){
  int i,st,ll =0;
  Basis *B = getbasis();

  double *save = dvector(0, qtot-1);
  double *save_hj = dvector(0, Nmodes-1);

  st = state;

  dcopy(qtot,   h_3d[0][0], 1,    save, 1);
  dcopy(Nmodes, vert[0].hj, 1, save_hj, 1);

  Curve *cur;

  dzero(Nmodes, vert[0].hj, 1);

  // x
  for(i=0;i<NPyr_verts;++i)
    vert[i].hj[0] = vert[i].x;

  for(i=0;i<NPyr_edges;++i)
    dcopy(edge[i].l, curvX[0].Cedge[i], 1, edge[i].hj, 1);

 if(curvX)
    for(i=0;i<Nfaces;++i){
      ll = face[i].l;
      ll = (Nfverts(i) == 3) ? ll*(ll+1)/2 : ll*ll;
      if(ll)
  dcopy(ll, curvX[0].Cface[i][0], 1, face[i].hj[0], 1);
    }


  Jbwd(this, B);
  dcopy(qtot, **h_3d, 1, X->x, 1);

  // y
  for(i=0;i<NPyr_verts;++i)
    vert[i].hj[0] = vert[i].y;

  for(i=0;i<NPyr_edges;++i)
    dcopy(edge[i].l, curvX[1].Cedge[i], 1, edge[i].hj, 1);

  if(curvX)
    for(i=0;i<Nfaces;++i){
      ll = face[i].l;
      ll = (Nfverts(i) == 3) ? ll*(ll+1)/2 : ll*ll;
      if(ll)
  dcopy(ll, curvX[1].Cface[i][0], 1, face[i].hj[0], 1);
    }

  Jbwd(this, B);
  dcopy(qtot, **h_3d, 1, X->y, 1);


  // z
  for(i=0;i<NPyr_verts;++i)
    vert[i].hj[0] = vert[i].z;

  for(i=0;i<NPyr_edges;++i)
    dcopy(edge[i].l, curvX[2].Cedge[i], 1, edge[i].hj, 1);

  if(curvX)
    for(i=0;i<Nfaces;++i){
      ll = face[i].l;
      ll = (Nfverts(i) == 3) ? ll*(ll+1)/2 : ll*ll;
      if(ll)
  dcopy(ll, curvX[2].Cface[i][0], 1, face[i].hj[0], 1);
    }


  Jbwd(this, B);
  dcopy(qtot, **h_3d, 1, X->z, 1);
  dcopy(qtot,    save, 1, h_3d[0][0], 1);
  dcopy(Nmodes, save_hj, 1, vert[0].hj, 1);

  state = st;

  free(save);  free(save_hj);
}




void Prism::curved_elmt(Coord *X){


  int i,st,ll =0;
  Basis *B = getbasis();
  Curve *cur;

  double *save = dvector(0, qtot-1);
  double *save_hj = dvector(0, Nmodes-1);

  st = state;


  dcopy(qtot,   h_3d[0][0], 1,    save, 1);
  dcopy(Nmodes, vert[0].hj, 1, save_hj, 1);

  dzero(Nmodes, vert[0].hj, 1);

  // x
  for(i=0;i<NPrism_verts;++i)
    vert[i].hj[0] = vert[i].x;

  for(i=0;i<NPrism_edges;++i)
    dcopy(edge[i].l, curvX[0].Cedge[i], 1, edge[i].hj, 1);

  if(curvX)
    for(i=0;i<Nfaces;++i){
      ll = face[i].l;
      ll = (Nfverts(i) == 3) ? ll*(ll+1)/2 : ll*ll;
      if(ll)
  dcopy(ll, curvX[0].Cface[i][0], 1, face[i].hj[0], 1);
    }


  Jbwd(this, B);
  dcopy(qtot, **h_3d, 1, X->x, 1);

  dzero(Nmodes, vert[0].hj, 1);
  // y
  for(i=0;i<NPrism_verts;++i)
    vert[i].hj[0] = vert[i].y;

  for(i=0;i<NPrism_edges;++i)
    dcopy(edge[i].l, curvX[1].Cedge[i], 1, edge[i].hj, 1);

  if(curvX)
    for(i=0;i<Nfaces;++i){
      ll = face[i].l;
      ll = (Nfverts(i) == 3) ? ll*(ll+1)/2 : ll*ll;
      if(ll)
  dcopy(ll, curvX[1].Cface[i][0], 1, face[i].hj[0], 1);
    }


  Jbwd(this, B);
  dcopy(qtot, **h_3d, 1, X->y, 1);

  dzero(Nmodes, vert[0].hj, 1);
  // z
  for(i=0;i<NPrism_verts;++i)
    vert[i].hj[0] = vert[i].z;

  for(i=0;i<NPrism_edges;++i)
    dcopy(edge[i].l, curvX[2].Cedge[i], 1, edge[i].hj, 1);

  if(curvX)
    for(i=0;i<Nfaces;++i){
      ll = face[i].l;
      ll = (Nfverts(i) == 3) ? ll*(ll+1)/2 : ll*ll;
      if(ll)
  dcopy(ll, curvX[2].Cface[i][0], 1, face[i].hj[0], 1);
    }

  Jbwd(this, B);
  dcopy(qtot, **h_3d, 1, X->z, 1);

  dcopy(qtot,    save, 1, h_3d[0][0], 1);
  dcopy(Nmodes, save_hj, 1, vert[0].hj, 1);

  state = st;

  free(save);  free(save_hj);
}




void Hex::curved_elmt(Coord *X){

  int i,st,ll =0;
  Basis *B = getbasis();

  double *save = dvector(0, qtot-1);
  double *save_hj = dvector(0, Nmodes-1);
  Curve *cur;

  st = state;

  dcopy(qtot,   h_3d[0][0], 1,    save, 1);
  dcopy(Nmodes, vert[0].hj, 1, save_hj, 1);

  dzero(Nmodes, vert[0].hj, 1);

  // x
  for(i=0;i<NHex_verts;++i)
    vert[i].hj[0] = vert[i].x;

  for(i=0;i<NHex_edges;++i)
    dcopy(edge[i].l, curvX[0].Cedge[i], 1, edge[i].hj, 1);

  if(curvX)
    for(i=0;i<Nfaces;++i){
      ll = face[i].l;
      ll = (Nfverts(i) == 3) ? ll*(ll+1)/2 : ll*ll;
      if(ll)
  dcopy(ll, curvX[0].Cface[i][0], 1, face[i].hj[0], 1);
    }

  Jbwd(this, B);
  dcopy(qtot, **h_3d, 1, X->x, 1);


  dzero(Nmodes, vert[0].hj, 1);
  // y
  for(i=0;i<NHex_verts;++i)
    vert[i].hj[0] = vert[i].y;

  for(i=0;i<NHex_edges;++i)
    dcopy(edge[i].l, curvX[1].Cedge[i], 1, edge[i].hj, 1);

  if(curvX)
    for(i=0;i<Nfaces;++i){
      ll = face[i].l;
      ll = (Nfverts(i) == 3) ? ll*(ll+1)/2 : ll*ll;
      if(ll)
  dcopy(ll, curvX[1].Cface[i][0], 1, face[i].hj[0], 1);
    }

  Jbwd(this, B);
  dcopy(qtot, **h_3d, 1, X->y, 1);

  dzero(Nmodes, vert[0].hj, 1);
  // z
  for(i=0;i<NHex_verts;++i)
    vert[i].hj[0] = vert[i].z;

  for(i=0;i<NHex_edges;++i)
    dcopy(edge[i].l, curvX[2].Cedge[i], 1, edge[i].hj, 1);

  if(curvX)
    for(i=0;i<Nfaces;++i){
      ll = face[i].l;
      ll = (Nfverts(i) == 3) ? ll*(ll+1)/2 : ll*ll;
      if(ll)
  dcopy(ll, curvX[2].Cface[i][0], 1, face[i].hj[0], 1);
    }


  Jbwd(this, B);
  dcopy(qtot, **h_3d, 1, X->z, 1);


  dcopy(qtot,    save, 1, h_3d[0][0], 1);
  dcopy(Nmodes, save_hj, 1, vert[0].hj, 1);

  state = st;

  free(save);  free(save_hj);
}




void Element::curved_elmt(Coord *){ERR;}           //




/*

Function name: Element::straight_edge

Function Purpose:
Calculate the quadrature points along a required edge.

Argument 1: Coord *X
Purpose:
Provides storage for the coordinate output.

Argument 2: int edg
Purpose:
Which edge is to be calculated.

Function Notes:

*/

void Tri::straight_edge(Coord *X, int edg){
  int    q;
  double *z,*w,v1[3],v2[3];
  double *tmp;

  switch(edg){
  case 0:
    q = qa;
    getzw(q,&z,&w,'a');
    v1[0] = vert[0].x; v2[0] = vert[1].x;
    v1[1] = vert[0].y; v2[1] = vert[1].y;
    break;
  case 1:
    q = qb;
    getzw(q,&z,&w,'b');
    v1[0] = vert[1].x; v2[0] = vert[2].x;
    v1[1] = vert[1].y; v2[1] = vert[2].y;
    break;
  case 2:
    q = qb;
    getzw(q,&z,&w,'b');
    v1[0] = vert[0].x; v2[0] = vert[2].x;
    v1[1] = vert[0].y; v2[1] = vert[2].y;
    break;
  default:
    fprintf(stderr,"Tri::straight_edge unkown edge: %d\n", edg);
    break;
  }

  tmp = dvector(0,q-1);
  dsadd(q,1.0,z,1,tmp,1);

  dsmul(q,0.5*(v2[0]-v1[0]),tmp,1,X->x,1);
  dsadd(q,v1[0],X->x,1,X->x,1);

  dsmul(q,0.5*(v2[1]-v1[1]),tmp,1,X->y,1);
  dsadd(q,v1[1],X->y,1,X->y,1);

  free(tmp);
}




void Quad::straight_edge(Coord *X, int edg){
  int    q;
  double *z,*w,v1[3],v2[3],*tmp;

  switch(edg){
  case 0:
    q = qa;
    getzw(q,&z,&w,'a');
    v1[0] = vert[0].x; v2[0] = vert[1].x;
    v1[1] = vert[0].y; v2[1] = vert[1].y;
    break;
  case 1:
    q = qb;
    getzw(q,&z,&w,'a');
    v1[0] = vert[1].x; v2[0] = vert[2].x;
    v1[1] = vert[1].y; v2[1] = vert[2].y;
    break;
  case 2:
    q = qa;
    getzw(q,&z,&w,'a');
    v1[0] = vert[3].x; v2[0] = vert[2].x;
    v1[1] = vert[3].y; v2[1] = vert[2].y;
    break;
  case 3:
    q = qb;
    getzw(q,&z,&w,'a');
    v1[0] = vert[0].x; v2[0] = vert[3].x;
    v1[1] = vert[0].y; v2[1] = vert[3].y;
    break;
  }

  tmp = dvector(0,q-1);
  dsadd(q,1.0,z,1,tmp,1);

  dsmul(q,0.5*(v2[0]-v1[0]),tmp,1,X->x,1);
  dsadd(q,v1[0],X->x,1,X->x,1);

  dsmul(q,0.5*(v2[1]-v1[1]),tmp,1,X->y,1);
  dsadd(q,v1[1],X->y,1,X->y,1);

  free(tmp);
}




void Tet::straight_edge(Coord *X, int edg){
  int    q;
  double *z,*w,v1[3],v2[3];
  double *tmp;

  switch(edg){
  case 0:
    q = qa;
    getzw(q,&z,&w,'a');
    v1[0] = vert[0].x; v2[0] = vert[1].x;
    v1[1] = vert[0].y; v2[1] = vert[1].y;
    v1[2] = vert[0].z; v2[2] = vert[1].z;
    break;
  case 1:
    q = qb;
    getzw(q,&z,&w,'b');
    v1[0] = vert[1].x; v2[0] = vert[2].x;
    v1[1] = vert[1].y; v2[1] = vert[2].y;
    v1[2] = vert[1].z; v2[2] = vert[2].z;
    break;
  case 2:
    q = qb;
    getzw(q,&z,&w,'b');
    v1[0] = vert[0].x; v2[0] = vert[2].x;
    v1[1] = vert[0].y; v2[1] = vert[2].y;
    v1[2] = vert[0].z; v2[2] = vert[2].z;
    break;
  case 3:
    q = qc;
    getzw(q,&z,&w,'c');
    v1[0] = vert[0].x; v2[0] = vert[3].x;
    v1[1] = vert[0].y; v2[1] = vert[3].y;
    v1[2] = vert[0].z; v2[2] = vert[3].z;
    break;
  case 4:
    q = qc;
    getzw(q,&z,&w,'c');
    v1[0] = vert[1].x; v2[0] = vert[3].x;
    v1[1] = vert[1].y; v2[1] = vert[3].y;
    v1[2] = vert[1].z; v2[2] = vert[3].z;
    break;
  case 5:
    q = qc;
    getzw(q,&z,&w,'c');
    v1[0] = vert[2].x; v2[0] = vert[3].x;
    v1[1] = vert[2].y; v2[1] = vert[3].y;
    v1[2] = vert[2].z; v2[2] = vert[3].z;
    break;
  }

  tmp = dvector(0,q-1);
  dsadd(q,1.0,z,1,tmp,1);

  dsmul(q,0.5*(v2[0]-v1[0]),tmp,1,X->x,1);
  dsadd(q,v1[0],X->x,1,X->x,1);

  dsmul(q,0.5*(v2[1]-v1[1]),tmp,1,X->y,1);
  dsadd(q,v1[1],X->y,1,X->y,1);

  dsmul(q,0.5*(v2[2]-v1[2]),tmp,1,X->z,1);
  dsadd(q,v1[2],X->z,1,X->z,1);

  free(tmp);
}




void Pyr::straight_edge(Coord *X, int edg){
  int    q;
  double *z,*w,v1[3],v2[3];
  double *tmp;

  switch(edg){
  case 0:
    q = qa;
    getzw(q,&z,&w,'a');
    v1[0] = vert[0].x; v2[0] = vert[1].x;
    v1[1] = vert[0].y; v2[1] = vert[1].y;
    v1[2] = vert[0].z; v2[2] = vert[1].z;
    break;
  case 1:
    q = qb;
    getzw(q,&z,&w,'a');
    v1[0] = vert[1].x; v2[0] = vert[2].x;
    v1[1] = vert[1].y; v2[1] = vert[2].y;
    v1[2] = vert[1].z; v2[2] = vert[2].z;
    break;
  case 2:
    q = qb;
    getzw(q,&z,&w,'a');
    v1[0] = vert[0].x; v2[0] = vert[2].x;
    v1[1] = vert[0].y; v2[1] = vert[2].y;
    v1[2] = vert[0].z; v2[2] = vert[2].z;
    break;
  case 3:
    q = qc;
    getzw(q,&z,&w,'a');
    v1[0] = vert[0].x; v2[0] = vert[3].x;
    v1[1] = vert[0].y; v2[1] = vert[3].y;
    v1[2] = vert[0].z; v2[2] = vert[3].z;
    break;
  case 4:
    q = qc;
    getzw(q,&z,&w,'a');
    v1[0] = vert[1].x; v2[0] = vert[3].x;
    v1[1] = vert[1].y; v2[1] = vert[3].y;
    v1[2] = vert[1].z; v2[2] = vert[3].z;
    break;
  case 5:
    q = qc;
    getzw(q,&z,&w,'a');
    v1[0] = vert[2].x; v2[0] = vert[3].x;
    v1[1] = vert[2].y; v2[1] = vert[3].y;
    v1[2] = vert[2].z; v2[2] = vert[3].z;
    break;
  }

  tmp = dvector(0,q-1);
  dsadd(q,1.0,z,1,tmp,1);

  dsmul(q,0.5*(v2[0]-v1[0]),tmp,1,X->x,1);
  dsadd(q,v1[0],X->x,1,X->x,1);

  dsmul(q,0.5*(v2[1]-v1[1]),tmp,1,X->y,1);
  dsadd(q,v1[1],X->y,1,X->y,1);

  dsmul(q,0.5*(v2[2]-v1[2]),tmp,1,X->z,1);
  dsadd(q,v1[2],X->z,1,X->z,1);

  free(tmp);
}




void Prism::straight_edge(Coord *X, int edg){
  int    q;
  double *z,*w,v1[3],v2[3];
  double *tmp;

  switch(edg){
  case 0:
    q = qa;
    getzw(q,&z,&w,'a');
    v1[0] = vert[0].x; v2[0] = vert[1].x;
    v1[1] = vert[0].y; v2[1] = vert[1].y;
    v1[2] = vert[0].z; v2[2] = vert[1].z;
    break;
  case 1:
    q = qb;
    getzw(q,&z,&w,'a');
    v1[0] = vert[1].x; v2[0] = vert[2].x;
    v1[1] = vert[1].y; v2[1] = vert[2].y;
    v1[2] = vert[1].z; v2[2] = vert[2].z;
    break;
  case 2:
    q = qb;
    getzw(q,&z,&w,'a');
    v1[0] = vert[0].x; v2[0] = vert[2].x;
    v1[1] = vert[0].y; v2[1] = vert[2].y;
    v1[2] = vert[0].z; v2[2] = vert[2].z;
    break;
  case 3:
    q = qc;
    getzw(q,&z,&w,'a');
    v1[0] = vert[0].x; v2[0] = vert[3].x;
    v1[1] = vert[0].y; v2[1] = vert[3].y;
    v1[2] = vert[0].z; v2[2] = vert[3].z;
    break;
  case 4:
    q = qc;
    getzw(q,&z,&w,'a');
    v1[0] = vert[1].x; v2[0] = vert[3].x;
    v1[1] = vert[1].y; v2[1] = vert[3].y;
    v1[2] = vert[1].z; v2[2] = vert[3].z;
    break;
  case 5:
    q = qc;
    getzw(q,&z,&w,'a');
    v1[0] = vert[2].x; v2[0] = vert[3].x;
    v1[1] = vert[2].y; v2[1] = vert[3].y;
    v1[2] = vert[2].z; v2[2] = vert[3].z;
    break;
  }

  tmp = dvector(0,q-1);
  dsadd(q,1.0,z,1,tmp,1);

  dsmul(q,0.5*(v2[0]-v1[0]),tmp,1,X->x,1);
  dsadd(q,v1[0],X->x,1,X->x,1);

  dsmul(q,0.5*(v2[1]-v1[1]),tmp,1,X->y,1);
  dsadd(q,v1[1],X->y,1,X->y,1);

  dsmul(q,0.5*(v2[2]-v1[2]),tmp,1,X->z,1);
  dsadd(q,v1[2],X->z,1,X->z,1);

  free(tmp);
}




void Hex::straight_edge(Coord *X, int edg){
  fprintf(stderr,"Hex::straight_edge not implemented\n");
}




void Element::straight_edge(Coord *, int ){ERR;}   //




/*

Function name: Element::CoordTransEdge

Function Purpose:

Argument 1: double *f
Purpose:

Argument 2: double *fhat
Purpose:

Argument 3: int ctedge
Purpose:

Function Notes:

*/

void Tri::CoordTransEdge(double *f, double *fhat, int ctedge){
  register int i;
  Basis  *b = getbasis();
  double *w,*imat;
  int    L = edge[ctedge].l,info;

  getzw(qa,&w,&w,'a');

  /*subtract off vertices */
  daxpy(qa,-f[0]   ,b->vert[0].a,1,f,1);
  daxpy(qa,-f[qa-1],b->vert[1].a,1,f,1);

  /* inner product */
  dvmul(qa,w,1,f,1,f,1);
  for(i = 0; i < L; ++i)
    fhat[i] = ddot(qa,b->edge[0][i].a,1,f,1);

  if(L){
    get_mmat1d(&imat,L);

    if(L>3)
      dpbtrs('L',L,2,1,imat,3,fhat,L,info);
    else
      dpptrs('L',L,1,imat,fhat,L,info);
  }
}




void Quad::CoordTransEdge(double *f, double *fhat, int ctedge){
  register int i;
  Basis  *b = getbasis();
  double *w,*imat;
  int    L = edge[ctedge].l,info;

  getzw(qa,&w,&w,'a');

  /*subtract off vertices */
  daxpy(qa,-f[0]   ,b->vert[0].a,1,f,1);
  daxpy(qa,-f[qa-1],b->vert[1].a,1,f,1);

  /* inner product */
  dvmul(qa,w,1,f,1,f,1);
  for(i = 0; i < L; ++i)
    fhat[i] = ddot(qa,b->edge[0][i].a,1,f,1);

  if(L){
    get_mmat1d(&imat,L);
    if(L>3)
      dpbtrs('L',L,2,1,imat,3,fhat,L,info);
    else
      dpptrs('L',L,1,imat,fhat,L,info);
  }
}




void Tet::CoordTransEdge(double *, double *, int ){
  return;
}




void Pyr::CoordTransEdge(double *, double *, int ){
  return;
}




void Prism::CoordTransEdge(double *, double *, int ){
  return;
}




void Hex::CoordTransEdge(double *, double *, int ){
  return;
}




void Element::CoordTransEdge(double *, double *, int ){ERR;}




/*

Function name: Element::GetFaceCoord

Function Purpose:

Argument 1: int fac
Purpose:

Argument 2: Coord *X
Purpose:

Function Notes:
*/

void Tri::GetFaceCoord(int fac, Coord *X){
  if(curvX){
    register int i;
    Basis *b = getbasis();
    switch(fac){
    case 0:
      dsmul(qa,vert[0].x,b->vert[0].a,1,X->x,1);
      dsmul(qa,vert[0].y,b->vert[0].a,1,X->y,1);
      daxpy(qa,vert[1].x,b->vert[1].a,1,X->x,1);
      daxpy(qa,vert[1].y,b->vert[1].a,1,X->y,1);
      for(i = 0; i < edge->l; ++i){
        daxpy(qa,curvX[0].Cedge[0][i],b->edge[0][i].a,1,X->x,1);
        daxpy(qa,curvX[1].Cedge[0][i],b->edge[0][i].a,1,X->y,1);
      }
      break;
    case 1: case 2:
      dsmul(qb,vert[fac%2].x,b->vert[0].b,1,X->x,1);
      dsmul(qb,vert[fac%2].y,b->vert[0].b,1,X->y,1);
      daxpy(qb,vert[2].x,b->vert[2].b,1,X->x,1);
      daxpy(qb,vert[2].y,b->vert[2].b,1,X->y,1);

      // recent change
      for(i = 0; i < edge[fac].l; ++i){
        daxpy(qb,curvX[0].Cedge[fac][i],b->edge[fac][i].b,1,X->x,1);
        daxpy(qb,curvX[1].Cedge[fac][i],b->edge[fac][i].b,1,X->y,1);
      }
    }
  }
  else
    straight_edge(X,fac);
}


void Quad::GetFaceCoord(int fac, Coord *X){
  // CURVED_ELMT
  if(curvX){
    register int i;
    Basis *b = getbasis();
    int va = fac;
    int vb = (fac+1)%Nverts;

    switch(fac){
    case 0: case 2:
      dsmul(qa,vert[va].x,b->vert[va].a,1,X->x,1);
      dsmul(qa,vert[va].y,b->vert[va].a,1,X->y,1);
      daxpy(qa,vert[vb].x,b->vert[vb].a,1,X->x,1);
      daxpy(qa,vert[vb].y,b->vert[vb].a,1,X->y,1);
      for(i = 0; i < edge[fac].l; ++i){
  daxpy(qa,curvX[0].Cedge[fac][i],b->edge[fac][i].a,1,X->x,1);
  daxpy(qa,curvX[1].Cedge[fac][i],b->edge[fac][i].a,1,X->y,1);
      }
      break;
    case 1: case 3:
      dsmul(qb,vert[va].x,b->vert[va].b,1,X->x,1);
      dsmul(qb,vert[va].y,b->vert[va].b,1,X->y,1);
      daxpy(qb,vert[vb].x,b->vert[vb].b,1,X->x,1);
      daxpy(qb,vert[vb].y,b->vert[vb].b,1,X->y,1);
      for(i = 0; i < edge[fac].l; ++i){
  daxpy(qb,curvX[0].Cedge[fac][i],b->edge[fac][i].b,1,X->x,1);
  daxpy(qb,curvX[1].Cedge[fac][i],b->edge[fac][i].b,1,X->y,1);
      }
      break;
    }
  }
  else
    straight_edge(X,fac);
}

void Tet::GetFaceCoord(int fac, Coord *X){
  Coord Xf;
  Xf.x = dvector(0, 3*QGmax*QGmax*QGmax-1);
  Xf.y = Xf.x + QGmax*QGmax*QGmax;
  Xf.z = Xf.y + QGmax*QGmax*QGmax;

  this->coord(&Xf);
  GetFace(Xf.x, fac, X->x);
  GetFace(Xf.y, fac, X->y);
  GetFace(Xf.z, fac, X->z);

  // need to stack singular edge after qb*qc values
  if(fac > 1){
    double **im;
    getim(qb,qb+1,&im,b2a);
    int i;
    for(i=0;i<qc;++i){
      X->x[qb*qc+i] = ddot(qb,im[qb],1,X->x+i*qb,1);
      X->y[qb*qc+i] = ddot(qb,im[qb],1,X->y+i*qb,1);
      X->z[qb*qc+i] = ddot(qb,im[qb],1,X->z+i*qb,1);
    }
  }

  free(Xf.x);
}


void Pyr::GetFaceCoord(int fac, Coord *X){

  //  straight_face(X,fac,0);
  Coord Xf;
  Xf.x = dvector(0, 3*QGmax*QGmax*QGmax-1);
  Xf.y = Xf.x + QGmax*QGmax*QGmax;
  Xf.z = Xf.y + QGmax*QGmax*QGmax;

  this->coord(&Xf);
  GetFace(Xf.x, fac, X->x);
  GetFace(Xf.y, fac, X->y);
  GetFace(Xf.z, fac, X->z);

  free(Xf.x);
}




void Prism::GetFaceCoord(int fac, Coord *X){
  //  straight_face(X,fac,0);
  Coord Xf;
  Xf.x = dvector(0, 3*QGmax*QGmax*QGmax-1);
  Xf.y = Xf.x + QGmax*QGmax*QGmax;
  Xf.z = Xf.y + QGmax*QGmax*QGmax;

  this->coord(&Xf);
  GetFace(Xf.x, fac, X->x);
  GetFace(Xf.y, fac, X->y);
  GetFace(Xf.z, fac, X->z);

  free(Xf.x);
}




void Hex::GetFaceCoord(int fac, Coord *X){
  //  straight_face(X,fac,0);
  Coord Xf;
  Xf.x = dvector(0, 3*QGmax*QGmax*QGmax-1);
  Xf.y = Xf.x + QGmax*QGmax*QGmax;
  Xf.z = Xf.y + QGmax*QGmax*QGmax;

  this->coord(&Xf);
  GetFace(Xf.x, fac, X->x);
  GetFace(Xf.y, fac, X->y);
  GetFace(Xf.z, fac, X->z);

  free(Xf.x);
}




void Element::GetFaceCoord(int , Coord *){ERR;}    //

/*
Function name: Element::Surface_geofac

Function Purpose:

Argument 1: Bndry *B
Purpose:

Function Notes:

*/

void Tri::Surface_geofac(Bndry *B){
  Geom    *G = geom;
  Vert    *v = vert;
  double   nx,ny,sjac,fac,*f, *x, *y;
  int      i;

  if(curvX){
    Coord  X;
    f    = dvector(0,QGmax-1);
    x    = dvector(0,QGmax-1);
    y    = dvector(0,QGmax-1);

    /* get coordinates along edge and interp to edge1 */
    X.x  = dvector(0,QGmax-1);
    X.y  = dvector(0,QGmax-1);

    GetFaceCoord (B->face, &X);
    InterpToFace1(B->face, X.x, x);
    InterpToFace1(B->face, X.y, y);
    free(X.x); free(X.y);

    if(!B->nx.p){
      B->nx.p   = dvector(0,qa-1);
      B->ny.p   = dvector(0,qa-1);
      B->sjac.p = dvector(0,qa-1);
      B->K.p    = dvector(0,qa-1);
    }
  }

  switch (B->face){
  case 0:
    nx =  (v[1].y - v[0].y);
    ny = -(v[1].x - v[0].x);

    if(curvX){
      dcopy(qa,G->sx.p,1,B->nx.p,1);
      dcopy(qa,G->sy.p,1,B->ny.p,1);
      dvneg(qa,B->nx.p,1,B->nx.p,1);
      dvneg(qa,B->ny.p,1,B->ny.p,1);
    }
    break;
  case 1:
    nx =  (v[2].y - v[1].y);
    ny = -(v[2].x - v[1].x);

    if(curvX){
      dcopy(qb,G->rx.p+qa-1,qa,f,1);
      dvadd(qb,G->sx.p+qa-1,qa,f,1,f,1);
      InterpToFace1(B->face,f,B->nx.p);
      dcopy(qb,G->ry.p+qa-1,qa,f,1);
      dvadd(qb,G->sy.p+qa-1,qa,f,1,f,1);
      InterpToFace1(B->face,f,B->ny.p);
    }
    break;
  case 2:
    nx = -(v[2].y - v[0].y);
    ny =  (v[2].x - v[0].x);

    if(curvX){
      dcopy(qb,G->rx.p,qa,f,1);
      InterpToFace1(B->face,f,B->nx.p);
      dcopy(qb,G->ry.p,qa,f,1);
      InterpToFace1(B->face,f,B->ny.p);

      dvneg(qa,B->nx.p,1,B->nx.p,1);
      dvneg(qa,B->ny.p,1,B->ny.p,1);
    }
    break;
  }

  /* straight surface jacobean */
  sjac = 0.5*sqrt(fabs((v[vnum(B->face,0)].x - v[vnum(B->face,1)].x)*
           (v[vnum(B->face,0)].x - v[vnum(B->face,1)].x) +
           (v[vnum(B->face,0)].y - v[vnum(B->face,1)].y)*
           (v[vnum(B->face,0)].y - v[vnum(B->face,1)].y)));

  /* normalise normal components */
  fac  = sqrt(fabs(nx*nx + ny*ny));
  nx  /= fac;
  ny  /= fac;

  if(curvX){
    // NEW
    double **da,**dt;
    double *dx  = dvector(0, QGmax-1);
    double *dy  = dvector(0, QGmax-1);
    double *ddx = dvector(0, QGmax-1);
    double *ddy = dvector(0, QGmax-1);

    getD(&da,&dt,&dt,&dt,&dt,&dt);

    dvmul  (qa,B->nx.p,1,B->nx.p,1,f,1);
    dvvtvp (qa,B->ny.p,1,B->ny.p,1,f,1,f,1);
    dvsqrt (qa,f,1,f,1);
    dvdiv  (qa,B->nx.p,1,f,1,B->nx.p,1);
    dvdiv  (qa,B->ny.p,1,f,1,B->ny.p,1);

    /* calculate the surface jacobian as sjac = sqrt((dx/da)^2 + (dy/da)^2) */
    mxva (*da, qa, 1, x, 1, dx, 1, qa, qa);
    dcopy( qa, dx, 1, x, 1);
    mxva (*da, qa, 1, y, 1, dy, 1, qa, qa);
    dcopy( qa, dy, 1, y, 1);

    dvmul (qa,x,1,x,1,B->sjac.p,1);
    dvvtvp(qa,y,1,y,1,B->sjac.p,1,B->sjac.p,1);
    dvsqrt(qa,B->sjac.p,1,B->sjac.p,1);

    /* calculate the surface curvature as ... */
    mxva (*da, qa, 1, dx, 1, ddx, 1, qa, qa);
    mxva (*da, qa, 1, dy, 1, ddy, 1, qa, qa);

    double d;
    for(i=0;i<qa;++i){
      B->K.p[i] = fabs(dx[i]*ddy[i] - ddx[i]*dy[i]);
      d         =  pow(dx[i]*dx[i]  +  dy[i]*dy[i],1.5);

      if(d<1e-5 && B->K.p[i] >1e10)
  fprintf(stderr, "Tri::Surface_geofac.. singular curvature k=%lf\n",
    B->K.p[i]/d );

      B->K.p[i] /= d;
    }

    free(f); free(x); free(y); free(dx); free(dy); free(ddx); free(ddy);
  }
  else{
    B->nx.d   = nx;
    B->ny.d   = ny;
    B->sjac.d = sjac;
  }
}




void Quad::Surface_geofac(Bndry *B){
  Geom    *G = geom;
  Vert    *v = vert;
  double   nx,ny,fac,*f, *x, *y;
  int      i;

  Coord  X;
  f    = dvector(0,QGmax-1);
  x    = dvector(0,QGmax-1);
  y    = dvector(0,QGmax-1);

  /* get coordinates along edge and interp to edge1 */
  X.x  = dvector(0,QGmax-1);
  X.y  = dvector(0,QGmax-1);

  GetFaceCoord (B->face, &X);
  InterpToFace1(B->face, X.x, x);
  InterpToFace1(B->face, X.y, y);
  free(X.x); free(X.y);

  if(!B->nx.p){
    B->nx.p   = dvector(0,qa-1);
    B->ny.p   = dvector(0,qa-1);
    B->sjac.p = dvector(0,qa-1);
    B->K.p    = dvector(0,qa-1);
  }

  // fix for qa != qb

  switch (B->face){
  case 0:{
    nx =  (v[1].y - v[0].y);
    ny = -(v[1].x - v[0].x);

    dcopy(qa,G->sx.p,1,B->nx.p,1);
    dcopy(qa,G->sy.p,1,B->ny.p,1);
    dvneg(qa,B->nx.p,1,B->nx.p,1);
    dvneg(qa,B->ny.p,1,B->ny.p,1);
    break;
  }
  case 1:{
    nx =  (v[2].y - v[1].y);
    ny = -(v[2].x - v[1].x);
    dcopy(qb,G->rx.p+qa-1,qa,B->nx.p,1);
    dcopy(qb,G->ry.p+qa-1,qa,B->ny.p,1);
    break;
  }
  case 2:{
    nx =  (v[3].y - v[2].y);
    ny = -(v[3].x - v[2].x);

    dcopy(qa,G->sx.p+qa*(qb-1),1,B->nx.p,1);
    dcopy(qa,G->sy.p+qa*(qb-1),1,B->ny.p,1);
    break;
  }
  case 3:{
    nx = -(v[3].y - v[0].y);
    ny =  (v[3].x - v[0].x);

    dcopy(qb,G->rx.p,qa,B->nx.p,1);
    dcopy(qb,G->ry.p,qa,B->ny.p,1);
    dvneg(qb,B->nx.p,1,B->nx.p,1);
    dvneg(qb,B->ny.p,1,B->ny.p,1);
    break;
  }

  }
#if 0

  /* straight surface jacobean */
  double sjac =
    0.5*sqrt(fabs((v[vnum(B->face,0)].x - v[vnum(B->face,1)].x)*
      (v[vnum(B->face,0)].x - v[vnum(B->face,1)].x) +
      (v[vnum(B->face,0)].y - v[vnum(B->face,1)].y)*
      (v[vnum(B->face,0)].y - v[vnum(B->face,1)].y)));
#endif
  /* normalise normal components */
  fac  = sqrt(fabs(nx*nx + ny*ny));
  nx  /= fac;
  ny  /= fac;

   // NEW
  double **da,**dt;
  double *dx  = dvector(0, QGmax-1);
  double *dy  = dvector(0, QGmax-1);
  double *ddx = dvector(0, QGmax-1);
  double *ddy = dvector(0, QGmax-1);

  getD(&da,&dt,&dt,&dt,NULL,NULL);

  dvmul  (qa,B->nx.p,1,B->nx.p,1,f,1);
  dvvtvp (qa,B->ny.p,1,B->ny.p,1,f,1,f,1);
  dvsqrt (qa,f,1,f,1);
  dvdiv  (qa,B->nx.p,1,f,1,B->nx.p,1);
  dvdiv  (qa,B->ny.p,1,f,1,B->ny.p,1);

  /* calculate the surface jacobian as sjac = sqrt((dx/da)^2 + (dy/da)^2) */
  mxva (*da, qa, 1, x, 1, dx, 1, qa, qa);
  dcopy( qa, dx, 1, x, 1);
  mxva (*da, qa, 1, y, 1, dy, 1, qa, qa);
  dcopy( qa, dy, 1, y, 1);

  dvmul (qa,x,1,x,1,B->sjac.p,1);
  dvvtvp(qa,y,1,y,1,B->sjac.p,1,B->sjac.p,1);
  dvsqrt(qa,B->sjac.p,1,B->sjac.p,1);

  /* calculate the surface curvature as ... */
  mxva (*da, qa, 1, dx, 1, ddx, 1, qa, qa);
  mxva (*da, qa, 1, dy, 1, ddy, 1, qa, qa);

  double d;
  for(i=0;i<qa;++i){
    B->K.p[i] = fabs(dx[i]*ddy[i] - ddx[i]*dy[i]);
    d         =  pow(dx[i]*dx[i]  +  dy[i]*dy[i],1.5);

    if(d<1e-5 && B->K.p[i] >1e10)
       fprintf(stderr, "Tri::Surface_geofac.. singular curvature k=%lf\n",
        B->K.p[i]/d );

    B->K.p[i] /= d;
  }

  free(f); free(x); free(y); free(dx); free(dy); free(ddx); free(ddy);
#if 0
  dvmul  (qa,B->nx.p,1,B->nx.p,1,f,1);
  dvvtvp (qa,B->ny.p,1,B->ny.p,1,f,1,f,1);
  dvsqrt (qa,f,1,f,1);
  dvdiv  (qa,B->nx.p,1,f,1,B->nx.p,1);
  dvdiv  (qa,B->ny.p,1,f,1,B->ny.p,1);

  /* set deformed jacobean as sjac/( B->nx.p*nx + B->ny.p*ny) */
  dsmul  (qa,nx,B->nx.p,1,B->sjac.p,1);
  daxpy  (qa,ny,B->ny.p,1,B->sjac.p,1);
  dvdiv  (qa,&sjac,0,B->sjac.p,1,B->sjac.p,1);

  free(f);
#endif
}


Geom *Tet_gen_geofac(Element *E, int id);

void Tet::Surface_geofac(Bndry *B){
  int     fac = B->face;
  Element *E = B->elmt;
  Geom    *G = E->geom;
  Vert    *v = B->elmt->vert;
  double   nx,ny,sjac,jfac,*f;
  register int i;
  double   nz,a,b,ab;
  Coord    S;
  Geom     *Gtmp;

  /* get linear element characteristics */
  Gtmp = Tet_gen_geofac(E,0);

  if(E->curvX){
    f = dvector(0,QGmax*QGmax-1);
    if(!B->nx.p){
      B->sjac.p = dvector(0,qa*qb-1);
      B->nx.p   = dvector(0,qa*qb-1);
      B->ny.p   = dvector(0,qa*qb-1);
      B->nz.p   = dvector(0,qa*qb-1);
    }
    S.x = dvector(0,qa*qb*qc-1);
    S.y = dvector(0,qa*qb*qc-1);
    S.z = dvector(0,qa*qb*qc-1);

    this->coord(&S);
  }

  /* normals */
  switch(fac){
  case 0:
    a  = (v[1].x - v[0].x)*(v[1].x - v[0].x) +
   (v[1].y - v[0].y)*(v[1].y - v[0].y) +
   (v[1].z - v[0].z)*(v[1].z - v[0].z);
    b =  (v[2].x - v[0].x)*(v[2].x - v[0].x) +
   (v[2].y - v[0].y)*(v[2].y - v[0].y) +
   (v[2].z - v[0].z)*(v[2].z - v[0].z);
    ab = (v[1].x - v[0].x)*(v[2].x - v[0].x) +
         (v[1].y - v[0].y)*(v[2].y - v[0].y) +
   (v[1].z - v[0].z)*(v[2].z - v[0].z);
    sjac = sqrt(a*b - ab*ab)/4.0;

    nx = -Gtmp->tx.d;
    ny = -Gtmp->ty.d;
    nz = -Gtmp->tz.d;

    if(E->curvX){
      InterpToFace1(fac,G->tx.p,B->nx.p);
      dvneg(qa*qb,B->nx.p,1,B->nx.p,1);

      InterpToFace1(fac,G->ty.p,B->ny.p);
      dvneg(qa*qb,B->ny.p,1,B->ny.p,1);

      InterpToFace1(fac,G->tz.p,B->nz.p);
      dvneg(qa*qb,B->nz.p,1,B->nz.p,1);

      dcopy(qa*qb,S.x,1,f,1);
      InterpToFace1(fac,f,S.x);
      dcopy(qa*qb,S.y,1,f,1);
      InterpToFace1(fac,f,S.y);
      dcopy(qa*qb,S.z,1,f,1);
      InterpToFace1(fac,f,S.z);

    }
    break;
  case 1:
    a  = (v[1].x - v[0].x)*(v[1].x - v[0].x) +
      (v[1].y - v[0].y)*(v[1].y - v[0].y) +
   (v[1].z - v[0].z)*(v[1].z - v[0].z);
    b  = (v[3].x - v[0].x)*(v[3].x - v[0].x) +
   (v[3].y - v[0].y)*(v[3].y - v[0].y) +
   (v[3].z - v[0].z)*(v[3].z - v[0].z);
    ab = (v[1].x - v[0].x)*(v[3].x - v[0].x) +
         (v[1].y - v[0].y)*(v[3].y - v[0].y) +
         (v[1].z - v[0].z)*(v[3].z - v[0].z);
    sjac = sqrt(a*b - ab*ab)/4.0;

    nx = -Gtmp->sx.d;
    ny = -Gtmp->sy.d;
    nz = -Gtmp->sz.d;

    if(E->curvX){
      for(i = 0; i < qc; ++i) dcopy(qa,G->sx.p+i*qa*qb,1,f+i*qa,1);
      InterpToFace1(fac,f,B->nx.p);
      dvneg(qa*qb,B->nx.p,1,B->nx.p,1);

      for(i = 0; i < qc; ++i) dcopy(qa,G->sy.p+i*qa*qb,1,f+i*qa,1);
      InterpToFace1(fac,f,B->ny.p);
      dvneg(qa*qb,B->ny.p,1,B->ny.p,1);

      for(i = 0; i < qc; ++i) dcopy(qa,G->sz.p+i*qa*qb,1,f+i*qa,1);
      InterpToFace1(fac,f,B->nz.p);
      dvneg(qa*qb,B->nz.p,1,B->nz.p,1);

      for(i = 0; i < qc; ++i) dcopy(qa,S.x+i*qa*qb,1,f+i*qa,1);
      InterpToFace1(fac,f,S.x);
      for(i = 0; i < qc; ++i) dcopy(qa,S.y+i*qa*qb,1,f+i*qa,1);
      InterpToFace1(fac,f,S.y);
      for(i = 0; i < qc; ++i) dcopy(qa,S.z+i*qa*qb,1,f+i*qa,1);
      InterpToFace1(fac,f,S.z);

    }
    break;
  case 2:
    a  = (v[2].x - v[1].x)*(v[2].x - v[1].x) +
         (v[2].y - v[1].y)*(v[2].y - v[1].y) +
   (v[2].z - v[1].z)*(v[2].z - v[1].z);
    b  = (v[3].x - v[1].x)*(v[3].x - v[1].x) +
   (v[3].y - v[1].y)*(v[3].y - v[1].y) +
   (v[3].z - v[1].z)*(v[3].z - v[1].z);
    ab = (v[2].x - v[1].x)*(v[3].x - v[1].x) +
         (v[2].y - v[1].y)*(v[3].y - v[1].y) +
         (v[2].z - v[1].z)*(v[3].z - v[1].z);
    sjac = sqrt(a*b - ab*ab)/4.0;

    nx = Gtmp->rx.d + Gtmp->sx.d + Gtmp->tx.d;
    ny = Gtmp->ry.d + Gtmp->sy.d + Gtmp->ty.d;
    nz = Gtmp->rz.d + Gtmp->sz.d + Gtmp->tz.d;

    if(E->curvX){
      for(i = 0; i < qc; ++i){
  dcopy(qb,G->rx.p+qa*(i*qb+1)-1,qa,f+i*qb,1);
  dvadd(qb,G->sx.p+qa*(i*qb+1)-1,qa,f+i*qb,1,f+i*qb,1);
  dvadd(qb,G->tx.p+qa*(i*qb+1)-1,qa,f+i*qb,1,f+i*qb,1);
      }
      InterpToFace1(fac,f,B->nx.p);

      for(i = 0; i < qc; ++i){
  dcopy(qb,G->ry.p+qa*(i*qb+1)-1,qa,f+i*qb,1);
  dvadd(qb,G->sy.p+qa*(i*qb+1)-1,qa,f+i*qb,1,f+i*qb,1);
  dvadd(qb,G->ty.p+qa*(i*qb+1)-1,qa,f+i*qb,1,f+i*qb,1);
      }
      InterpToFace1(fac,f,B->ny.p);

      for(i = 0; i < qc; ++i){
  dcopy(qb,G->rz.p+qa*(i*qb+1)-1,qa,f+i*qb,1);
  dvadd(qb,G->sz.p+qa*(i*qb+1)-1,qa,f+i*qb,1,f+i*qb,1);
  dvadd(qb,G->tz.p+qa*(i*qb+1)-1,qa,f+i*qb,1,f+i*qb,1);
      }
      InterpToFace1(fac,f,B->nz.p);

      for(i = 0; i < qc; ++i) dcopy(qb,S.x+qa*(i*qb+1)-1,qa,f+i*qb,1);
      InterpToFace1(fac,f,S.x);
      for(i = 0; i < qc; ++i) dcopy(qb,S.y+qa*(i*qb+1)-1,qa,f+i*qb,1);
      InterpToFace1(fac,f,S.y);
      for(i = 0; i < qc; ++i) dcopy(qb,S.z+qa*(i*qb+1)-1,qa,f+i*qb,1);
      InterpToFace1(fac,f,S.z);

    }
    break;
  case 3:
    a  = (v[2].x - v[0].x)*(v[2].x - v[0].x) +
         (v[2].y - v[0].y)*(v[2].y - v[0].y) +
         (v[2].z - v[0].z)*(v[2].z - v[0].z);
    b  = (v[3].x - v[0].x)*(v[3].x - v[0].x) +
         (v[3].y - v[0].y)*(v[3].y - v[0].y) +
         (v[3].z - v[0].z)*(v[3].z - v[0].z);
    ab = (v[2].x - v[0].x)*(v[3].x - v[0].x) +
         (v[2].y - v[0].y)*(v[3].y - v[0].y) +
         (v[2].z - v[0].z)*(v[3].z - v[0].z);
    sjac = sqrt(a*b - ab*ab)/4.0;

    nx = -Gtmp->rx.d;
    ny = -Gtmp->ry.d;
    nz = -Gtmp->rz.d;

    if(E->curvX){
      for(i = 0; i < qc; ++i) dcopy(qb,G->rx.p+i*qa*qb,qa,f+i*qb,1);
      InterpToFace1(fac,f,B->nx.p);
      dvneg(qa*qb,B->nx.p,1,B->nx.p,1);

      for(i = 0; i < qc; ++i) dcopy(qb,G->ry.p+i*qa*qb,qa,f+i*qb,1);
      InterpToFace1(fac,f,B->ny.p);
      dvneg(qa*qb,B->ny.p,1,B->ny.p,1);

      for(i = 0; i < qc; ++i) dcopy(qb,G->rz.p+i*qa*qb,qa,f+i*qb,1);
      InterpToFace1(fac,f,B->nz.p);
      dvneg(qa*qb,B->nz.p,1,B->nz.p,1);

      for(i = 0; i < qc; ++i) dcopy(qb,S.x+i*qa*qb,qa,f+i*qb,1);
      InterpToFace1(fac,f,S.x);
      for(i = 0; i < qc; ++i) dcopy(qb,S.y+i*qa*qb,qa,f+i*qb,1);
      InterpToFace1(fac,f,S.y);
      for(i = 0; i < qc; ++i) dcopy(qb,S.z+i*qa*qb,qa,f+i*qb,1);
      InterpToFace1(fac,f,S.z);

    }
    break;
  }

  /* normalise planar normal components */
  jfac = sqrt(fabs(nx*nx + ny*ny + nz*nz));
  nx /= jfac;
  ny /= jfac;
  nz /= jfac;

  /* normalise normals */
  if(E->curvX){

    B->nx.d   = nx;
    B->ny.d   = ny;
    B->nz.d   = nz;

    dvmul  (qa*qb,B->nx.p,1,B->nx.p,1,f,1);
    dvvtvp (qa*qb,B->ny.p,1,B->ny.p,1,f,1,f,1);
    dvvtvp (qa*qb,B->nz.p,1,B->nz.p,1,f,1,f,1);
    dvsqrt (qa*qb,f,1,f,1);
    dvdiv  (qa*qb,B->nx.p,1,f,1,B->nx.p,1);
    dvdiv  (qa*qb,B->ny.p,1,f,1,B->ny.p,1);
    dvdiv  (qa*qb,B->nz.p,1,f,1,B->nz.p,1);

    /* set up jaobean of surface projected into triangle */
#if 1
    Tet_JacProj(B);
#else
    Tet_JacProj(B,&S,nx,ny,nz);

    /* set deformed jacobean as sjac/(B->nx.p*nx + B->ny.p*ny + B->nz.p*nz) */
    dsmul  (qa*qb,nx,B->nx.p,1,f,1);
    daxpy  (qa*qb,ny,B->ny.p,1,f,1);
    daxpy  (qa*qb,nz,B->nz.p,1,f,1);
    dvdiv  (qa*qb,B->sjac.p,1,f,1,B->sjac.p,1);
#endif

    free(f);
    free(S.x); free(S.y); free(S.z);
  }
  else{
    B->nx.d   = nx;
    B->ny.d   = ny;
    B->nz.d   = nz;
    B->sjac.d = sjac;
  }

  free(Gtmp);

}




void Pyr::Surface_geofac(Bndry *B){
  int     fac = B->face,nq;
  double  *tmp, *tmp1;

  tmp  = dvector(0,QGmax*QGmax-1);
  tmp1 = dvector(0,QGmax*QGmax-1);

  nq = (Nfverts(fac) == 3) ? qa*qc: qa*qb;
  if(!B->nx.p){
    B->sjac.p = dvector(0,nq-1);
    B->nx.p   = dvector(0,nq-1);
    B->ny.p   = dvector(0,nq-1);
    B->nz.p   = dvector(0,nq-1);
  }

  /* normals */
  switch(fac){
  case 0:
    GetFace(geom->tx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);
    dvneg(qa*qb,B->nx.p,1,B->nx.p,1);

    GetFace(geom->ty.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);
    dvneg(qa*qb,B->ny.p,1,B->ny.p,1);

    GetFace(geom->tz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);
    dvneg(qa*qb,B->nz.p,1,B->nz.p,1);

    break;
  case 1:

    GetFace(geom->sx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);
    dvneg(qa*qc,B->nx.p,1,B->nx.p,1);

    GetFace(geom->sy.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);
    dvneg(qa*qc,B->ny.p,1,B->ny.p,1);

    GetFace(geom->sz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);
    dvneg(qa*qc,B->nz.p,1,B->nz.p,1);

    break;
  case 2:
    GetFace(geom->rx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);
    GetFace(geom->sx.p, fac, tmp);
    InterpToFace1(fac, tmp, tmp1);
    dvadd(qa*qc, tmp1, 1, B->nx.p, 1, B->nx.p, 1);
    GetFace(geom->tx.p, fac, tmp);
    InterpToFace1(fac, tmp, tmp1);
    dvadd(qa*qc, tmp1, 1, B->nx.p, 1, B->nx.p, 1);

    GetFace(geom->ry.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);
    GetFace(geom->sy.p, fac, tmp);
    InterpToFace1(fac, tmp, tmp1);
    dvadd(qa*qc, tmp1, 1, B->ny.p, 1, B->ny.p, 1);
    GetFace(geom->ty.p, fac, tmp);
    InterpToFace1(fac, tmp, tmp1);
    dvadd(qa*qc, tmp1, 1, B->ny.p, 1, B->ny.p, 1);

    GetFace(geom->rz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);
    GetFace(geom->sz.p, fac, tmp);
    InterpToFace1(fac, tmp, tmp1);
    dvadd(qa*qc, tmp1, 1, B->nz.p, 1, B->nz.p, 1);
    GetFace(geom->tz.p, fac, tmp);
    InterpToFace1(fac, tmp, tmp1);
    dvadd(qa*qc, tmp1, 1, B->nz.p, 1, B->nz.p, 1);

    break;
  case 3:
    GetFace(geom->sx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);

    GetFace(geom->sy.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);

    GetFace(geom->sz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);

    break;

  case 4:
    GetFace(geom->rx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);
    dvneg(qa*qc,B->nx.p,1,B->nx.p,1);

    GetFace(geom->ry.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);
    dvneg(qa*qc,B->ny.p,1,B->ny.p,1);

    GetFace(geom->rz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);
    dvneg(qa*qc,B->nz.p,1,B->nz.p,1);

    break;
  }

  // normalise normals
  dvmul  (nq,B->nx.p,1,B->nx.p,1,tmp,1);
  dvvtvp (nq,B->ny.p,1,B->ny.p,1,tmp,1,tmp,1);
  dvvtvp (nq,B->nz.p,1,B->nz.p,1,tmp,1,tmp,1);
  dvsqrt (nq,tmp,1,tmp,1);
  dvdiv  (nq,B->nx.p,1,tmp,1,B->nx.p,1);
  dvdiv  (nq,B->ny.p,1,tmp,1,B->ny.p,1);
  dvdiv  (nq,B->nz.p,1,tmp,1,B->nz.p,1);

  /* set up surface jaobian */
  if(fac == 0)
    Quad_Face_JacProj(B);
  else
    Tri_Face_JacProj(B);

  free(tmp);   free(tmp1);
}




void Prism::Surface_geofac(Bndry *B){
  int     fac = B->face,nq;
  double  *tmp, *tmp1;

  tmp  = dvector(0,QGmax*QGmax-1);
  tmp1 = dvector(0,QGmax*QGmax-1);

  nq = (Nfverts(fac) == 3) ? qa*qc: qa*qb;

  if(!B->nx.p){
    B->sjac.p = dvector(0,nq-1);
    B->nx.p   = dvector(0,nq-1);
    B->ny.p   = dvector(0,nq-1);
    B->nz.p   = dvector(0,nq-1);
  }

  /* normals */
  switch(fac){
  case 0:
    GetFace(geom->tx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);
    dvneg(nq,B->nx.p,1,B->nx.p,1);

    GetFace(geom->ty.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);
    dvneg(nq,B->ny.p,1,B->ny.p,1);

    GetFace(geom->tz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);
    dvneg(nq,B->nz.p,1,B->nz.p,1);

    break;
  case 1:

    GetFace(geom->sx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);
    dvneg(nq,B->nx.p,1,B->nx.p,1);

    GetFace(geom->sy.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);
    dvneg(nq,B->ny.p,1,B->ny.p,1);

    GetFace(geom->sz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);
    dvneg(nq,B->nz.p,1,B->nz.p,1);

    break;
  case 2:
    GetFace(geom->rx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);
    GetFace(geom->tx.p, fac, tmp);
    InterpToFace1(fac, tmp, tmp1);
    dvadd(nq, tmp1, 1, B->nx.p, 1, B->nx.p, 1);

    GetFace(geom->ry.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);
    GetFace(geom->ty.p, fac, tmp);
    InterpToFace1(fac, tmp, tmp1);
    dvadd(nq, tmp1, 1, B->ny.p, 1, B->ny.p, 1);

    GetFace(geom->rz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);
    GetFace(geom->tz.p, fac, tmp);
    InterpToFace1(fac, tmp, tmp1);
    dvadd(nq, tmp1, 1, B->nz.p, 1, B->nz.p, 1);

    break;
  case 3:
    GetFace(geom->sx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);

    GetFace(geom->sy.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);

    GetFace(geom->sz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);

    break;

  case 4:
    GetFace(geom->rx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);
    dvneg(nq,B->nx.p,1,B->nx.p,1);

    GetFace(geom->ry.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);
    dvneg(nq,B->ny.p,1,B->ny.p,1);

    GetFace(geom->rz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);
    dvneg(nq,B->nz.p,1,B->nz.p,1);

    break;
  }

  // normalise normals
  dvmul  (nq,B->nx.p,1,B->nx.p,1,tmp,1);
  dvvtvp (nq,B->ny.p,1,B->ny.p,1,tmp,1,tmp,1);
  dvvtvp (nq,B->nz.p,1,B->nz.p,1,tmp,1,tmp,1);
  dvsqrt (nq,tmp,1,tmp,1);
  dvdiv  (nq,B->nx.p,1,tmp,1,B->nx.p,1);
  dvdiv  (nq,B->ny.p,1,tmp,1,B->ny.p,1);
  dvdiv  (nq,B->nz.p,1,tmp,1,B->nz.p,1);

  /* set up surface jaobian */
  if(fac == 0 || fac == 2 || fac == 4)
    Quad_Face_JacProj(B);
  else
    Tri_Face_JacProj(B);

  free(tmp);   free(tmp1);
}




void Hex::Surface_geofac(Bndry *B){
  int     fac = B->face;
  double  *tmp;
  // Leak
  tmp = dvector(0,QGmax*QGmax-1);
  if(!B->nx.p){
    B->sjac.p = dvector(0,qa*qb-1);
    B->nx.p   = dvector(0,qa*qb-1);
    B->ny.p   = dvector(0,qa*qb-1);
    B->nz.p   = dvector(0,qa*qb-1);
  }
  /* normals */
  switch(fac){
  case 0:

    GetFace(geom->tx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);
    dvneg(qa*qb,B->nx.p,1,B->nx.p,1);

    GetFace(geom->ty.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);
    dvneg(qa*qb,B->ny.p,1,B->ny.p,1);

    GetFace(geom->tz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);
    dvneg(qa*qb,B->nz.p,1,B->nz.p,1);

    break;
  case 1:

    GetFace(geom->sx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);
    dvneg(qa*qb,B->nx.p,1,B->nx.p,1);

    GetFace(geom->sy.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);
    dvneg(qa*qb,B->ny.p,1,B->ny.p,1);

    GetFace(geom->sz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);
    dvneg(qa*qb,B->nz.p,1,B->nz.p,1);

    break;
  case 2:

    GetFace(geom->rx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);

    GetFace(geom->ry.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);

    GetFace(geom->rz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);

    break;
  case 3:

    GetFace(geom->sx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);

    GetFace(geom->sy.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);

    GetFace(geom->sz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);

    break;

  case 4:

    GetFace(geom->rx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);
    dvneg(qa*qb,B->nx.p,1,B->nx.p,1);

    GetFace(geom->ry.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);
    dvneg(qa*qb,B->ny.p,1,B->ny.p,1);

    GetFace(geom->rz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);
    dvneg(qa*qb,B->nz.p,1,B->nz.p,1);

    break;
  case 5:

    GetFace(geom->tx.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nx.p);

    GetFace(geom->ty.p, fac, tmp);
    InterpToFace1(fac, tmp, B->ny.p);

    GetFace(geom->tz.p, fac, tmp);
    InterpToFace1(fac, tmp, B->nz.p);

    break;
  }

  dvmul  (qa*qb,B->nx.p,1,B->nx.p,1,tmp,1);
  dvvtvp (qa*qb,B->ny.p,1,B->ny.p,1,tmp,1,tmp,1);
  dvvtvp (qa*qb,B->nz.p,1,B->nz.p,1,tmp,1,tmp,1);
  dvsqrt (qa*qb,tmp,1,tmp,1);
  dvdiv  (qa*qb,B->nx.p,1,tmp,1,B->nx.p,1);
  dvdiv  (qa*qb,B->ny.p,1,tmp,1,B->ny.p,1);
  dvdiv  (qa*qb,B->nz.p,1,tmp,1,B->nz.p,1);

  /* set up surface jaobian */
  Quad_Face_JacProj(B);

  free(tmp);
}




void Element::Surface_geofac(Bndry *){ERR;}



/* this function projects the co-ordinates on a curved surface into
the plane of the triangle along the normal direction. Then rotates the
triangle to eliminate z and finally calculates the 2d Jacobean */

static void Tet_JacProj(Bndry *B, Coord *S, double nx, double ny, double nz){
  register int i;
  Element *E = B->elmt;
  const    int qa = E->qa, qb = E->qb;
  double   *x = S->x, *y = S->y, *z = S->z;
  double   **da,**db,**dc,**dt,t, theta, phi, cp,sp,ct,st;
  double   *xr = x + qa*qb, *xs = z, *yr = y+qa*qb, *ys = z+qa*qb;
  Mode     *v = E->getbasis()->vert;

  /* project co-ordinates along normal direction into plane of triangle */

  for(i = 0; i < qa*qb; ++i){
    t  = nx*(x[0]-x[i]) + ny*(y[0]-y[i]) + nz*(z[0]-z[i]);
    t /= nx*B->nx.p[i]  + ny*B->ny.p[i]  + nz*B->nz.p[i];
    x[i] += t*B->nx.p[i];
    y[i] += t*B->ny.p[i];
    z[i] += t*B->nz.p[i];
  }

  /* rotate surface so that it lies in x-y  plane
     ie. normal aligned with z axis */

  phi = atan2(ny,nx);
  cp  = cos(phi); sp = sin(phi);
  drot(1,&nx,1,&ny,1,cp,sp);
  theta = atan2(nx,nz);
  ct    = cos(theta); st = sin(theta);

  drot(qa*qb,x,1,y,1,cp,sp);
  drot(qa*qb,z,1,x,1,ct,st);

  /* calculate 2-d jacobean */
  E->getD(&da,&dt,&db,&dt,&dc,&dt);

  /* calculate dx/dr */
  dgemm('T','N',qa,qb,qa,1.0,*da,qa,x,qa,0.0,xr,qa);
  for(i = 0; i < qb; ++i)  dsmul(qa,1/v->b[i],xr+i*qa,1,xr+i*qa,1);

  /* calculate dx/ds */
  for(i = 0; i < qb; ++i) dvmul(qa,v[1].a,1,xr+i*qa,1,xs+i*qa,1);
  dgemm('N','N',qa,qb,qb,1.0,x,qa,*db,qb,1.0,xs,qa);

  /* calculate dy/dr */
  dgemm('T','N',qa,qb,qa,1.0,*da,qa,y,qa,0.0,yr,qa);
  for(i = 0; i < qb; ++i)  dsmul(qa,1/v->b[i],yr+i*qa,1,yr+i*qa,1);

  /* calculate dy/ds */
  for(i = 0; i < qb; ++i) dvmul(qa,v[1].a,1,yr+i*qa,1,ys+i*qa,1);
  dgemm('N','N',qa,qb,qb,1.0,y,qa,*db,qb,1.0,ys,qa);

  /* jacobean = | xr*ys - xs*yr| */
  dvmul (qa*qb,xr,1,ys,1,B->sjac.p,1);
  dvvtvm(qa*qb,xs,1,yr,1,B->sjac.p,1,B->sjac.p,1);
  dvabs (qa*qb,B->sjac.p,1,B->sjac.p,1);
}

/* calculate the surface jacobian which is defined for as 2D surface in
   a 3D space as:

   Surface Jac  = sqrt(Nx^2 + Ny^2 + Nz^2)

   where [Nx,Ny,Nz] is the vector normal to the surface given by the
   cross product of the two tangent vectors in the r and s direction,
   i.e.

   Nx = y_r z_s - z_r y_s
   Ny = z_r x_s - x_r z_s
   Nz = x_r y_s - y_r x_s

   */

static void Tet_JacProj(Bndry *B){
  register int i;
  Element *E = B->elmt;
  int      face = B->face;
  const    int qa = E->qa, qb = E->qb;
  Coord    S;
  double   **da,**db,**dc,**dt,t, theta, phi, cp,sp,ct,st;
  double   **D, *x, *y, *z, *xr, *xs, *yr, *ys, *zr, *zs;
  Mode     *v = E->getbasis()->vert;

  D = dmatrix(0,9,0,(QGmax+1)*QGmax-1);
  xr = D[0];  xs = D[1];
  yr = D[2];  ys = D[3];
  zr = D[4];  zs = D[5];

  S.x = D[0]; x = D[6];
  S.y = D[1]; y = D[7];
  S.z = D[2]; z = D[8];

  E->GetFaceCoord(face,&S);

  E->InterpToFace1(face,S.x,x);
  E->InterpToFace1(face,S.y,y);
  E->InterpToFace1(face,S.z,z);

  /* calculate derivatives */
  E->getD(&da,&dt,&db,&dt,&dc,&dt);

  /* calculate dx/dr */
  dgemm('T','N',qa,qb,qa,1.0,*da,qa,x,qa,0.0,xr,qa);
  for(i = 0; i < qb; ++i)  dsmul(qa,1/v->b[i],xr+i*qa,1,xr+i*qa,1);

  /* calculate dx/ds */
  for(i = 0; i < qb; ++i) dvmul(qa,v[1].a,1,xr+i*qa,1,xs+i*qa,1);
  dgemm('N','N',qa,qb,qb,1.0,x,qa,*db,qb,1.0,xs,qa);

  /* calculate dy/dr */
  dgemm('T','N',qa,qb,qa,1.0,*da,qa,y,qa,0.0,yr,qa);
  for(i = 0; i < qb; ++i)  dsmul(qa,1/v->b[i],yr+i*qa,1,yr+i*qa,1);

  /* calculate dy/ds */
  for(i = 0; i < qb; ++i) dvmul(qa,v[1].a,1,yr+i*qa,1,ys+i*qa,1);
  dgemm('N','N',qa,qb,qb,1.0,y,qa,*db,qb,1.0,ys,qa);

  /* calculate dz/dr */
  dgemm('T','N',qa,qb,qa,1.0,*da,qa,z,qa,0.0,zr,qa);
  for(i = 0; i < qb; ++i)  dsmul(qa,1/v->b[i],zr+i*qa,1,zr+i*qa,1);

  /* calculate dz/ds */
  for(i = 0; i < qb; ++i) dvmul(qa,v[1].a,1,zr+i*qa,1,zs+i*qa,1);
  dgemm('N','N',qa,qb,qb,1.0,z,qa,*db,qb,1.0,zs,qa);

  /* x = yr*zs - zr*ys*/
  dvmul (qa*qb,yr,1,zs,1,x,1);
  dvvtvm(qa*qb,zr,1,ys,1,x,1,x,1);

  /* y = zr*xs - xr*zs*/
  dvmul (qa*qb,zr,1,xs,1,y,1);
  dvvtvm(qa*qb,xr,1,zs,1,y,1,y,1);

  /* z = xr*ys - xs*yr*/
  dvmul (qa*qb,xr,1,ys,1,z,1);
  dvvtvm(qa*qb,xs,1,yr,1,z,1,z,1);

  /* Surface Jacobean = sqrt(x^2 + y^2 + z^2) */

  dvmul (qa*qb,x,1,x,1,B->sjac.p,1);
  dvvtvp(qa*qb,y,1,y,1,B->sjac.p,1,B->sjac.p,1);
  dvvtvp(qa*qb,z,1,z,1,B->sjac.p,1,B->sjac.p,1);

  dvsqrt(qa*qb,B->sjac.p,1,B->sjac.p,1);

  free_dmatrix(D,0,0);
}


/*

Function name: Element::free_geofac

Function Purpose:

Function Notes:

*/

void Tri::free_geofac(){
  if(geom && geom->elmt == this){
    if(curvX)
      free(geom->sy.p);

    free(geom);
    geom = (Geom*)0;
    iparam_set("FAMILIES",iparam("FAMILIES")-1);
  }
}




void Quad::free_geofac(){
  if(geom && geom->elmt == this){
    free(geom->sy.p);
    free(geom);
    geom = (Geom*)0;
    iparam_set("FAMILIES",iparam("FAMILIES")-1);
  }
}




void Tet::free_geofac(){
  if(geom && geom->elmt == this){
    if(curvX)
      free(geom->rx.p);

    free(geom);
    geom = (Geom*)0;
    iparam_set("FAMILIES",iparam("FAMILIES")-1);
  }
}




void Pyr::free_geofac(){

 if(geom && geom->elmt == this){
    if(curvX)
      free(geom->rx.p);

    free(geom);
    geom = (Geom*)0;
    iparam_set("FAMILIES",iparam("FAMILIES")-1);
  }
}




void Prism::free_geofac(){

  if(geom && geom->elmt == this){
    if(curvX)
      free(geom->rx.p);

    free(geom);
    geom = (Geom*)0;
    iparam_set("FAMILIES",iparam("FAMILIES")-1);
  }
}




void Hex::free_geofac(){

  if(geom && geom->elmt == this){
    if(curvX)
      free(geom->rx.p);

    free(geom);
    geom = (Geom*)0;
    iparam_set("FAMILIES",iparam("FAMILIES")-1);
  }
}




void Element::free_geofac(){ERR;}

void Element::move_vertices(Coord *X){
  int i;

  for(i=0;i<Nverts;++i){
    vert[i].x = X->x[i];
    vert[i].y = X->y[i];
    if(dim()==3)
      vert[i].z = X->z[i];
    iparam_set("FAMILIES",iparam("FAMILIES")-1);
  }
}

void Tet::straight_face(Coord *X, int fac, int trip){
  register int i;
  int      q,vn;
  double   *f = dvector(0,QGmax*QGmax-1);
  Mode     *v = getbasis()->vert;

  q = (fac == 0)? qa*qb : (fac == 1)? qa*qc: qb*qc;

  dzero(q,X->x,1);
  dzero(q,X->y,1);
  dzero(q,X->z,1);

  for(i = 0; i < Nfverts(fac); ++i){
    vn = vnum(fac,i);
    Tet_faceMode(this,fac,v + vn,f);
    daxpy(q,vert[vn].x,f,1,X->x,1);
    daxpy(q,vert[vn].y,f,1,X->y,1);
    daxpy(q,vert[vn].z,f,1,X->z,1);
  }

  if(trip){
    dsmul(qc,vert[2].x,v[2].c,1,X->x+q,1);
    daxpy(qc,vert[3].x,v[3].c,1,X->x+q,1);
    dsmul(qc,vert[2].y,v[2].c,1,X->y+q,1);
    daxpy(qc,vert[3].y,v[3].c,1,X->y+q,1);
    dsmul(qc,vert[2].z,v[2].c,1,X->z+q,1);
    daxpy(qc,vert[3].z,v[3].c,1,X->z+q,1);
  }

  free(f);
}

void Pyr::straight_face(Coord *X, int fac, int){
  register int i;
  int      q,vn;
  double   *f = dvector(0,QGmax*QGmax-1);
  Mode     *v = getbasis()->vert;

  if(fac == 0)
    q = qa*qb;
  else if(fac == 1 || fac == 3)
    q = qa*qc;
  else
    q = qb*qc;

  dzero(q,X->x,1);
  dzero(q,X->y,1);
  dzero(q,X->z,1);

  int nfv = Nfverts(fac);
  if(nfv == 4)
    for(i = 0; i < Nfverts(fac); ++i){
      vn = vnum(fac,i);
      Pyr_faceMode(this,0,v + i,f);

      daxpy(q,vert[vn].x,f,1,X->x,1);
      daxpy(q,vert[vn].y,f,1,X->y,1);
      daxpy(q,vert[vn].z,f,1,X->z,1);
    }
  else{
    vn = vnum(fac,0);
    Pyr_faceMode(this,1,v   ,f);

    daxpy(q,vert[vn].x,f,1,X->x,1);
    daxpy(q,vert[vn].y,f,1,X->y,1);
    daxpy(q,vert[vn].z,f,1,X->z,1);

    vn = vnum(fac,1);
    Pyr_faceMode(this,1,v+1 ,f);

    daxpy(q,vert[vn].x,f,1,X->x,1);
    daxpy(q,vert[vn].y,f,1,X->y,1);
    daxpy(q,vert[vn].z,f,1,X->z,1);

    vn = vnum(fac,2);
    Pyr_faceMode(this,1,v+4 ,f);

    daxpy(q,vert[vn].x,f,1,X->x,1);
    daxpy(q,vert[vn].y,f,1,X->y,1);
    daxpy(q,vert[vn].z,f,1,X->z,1);
  }



  free(f);
}


void Prism::straight_face(Coord *X, int fac, int){
  register int i;
  int      q,vn;
  double   *f = dvector(0,QGmax*QGmax-1);
  Mode     *v = getbasis()->vert;

  if(fac == 0)
    q = qa*qb;
  else if(fac == 1 || fac == 3)
    q = qa*qc;
  else
    q = qb*qc;

  dzero(q,X->x,1);
  dzero(q,X->y,1);
  dzero(q,X->z,1);

  int nfv = Nfverts(fac);
  if(nfv == 4)
    for(i = 0; i < Nfverts(fac); ++i){
      vn = vnum(fac,i);
      Prism_faceMode(this,0,v + i,f);

      daxpy(q,vert[vn].x,f,1,X->x,1);
      daxpy(q,vert[vn].y,f,1,X->y,1);
      daxpy(q,vert[vn].z,f,1,X->z,1);
    }
  else{
    vn = vnum(fac,0);
    Prism_faceMode(this,1,v   ,f);

    daxpy(q,vert[vn].x,f,1,X->x,1);
    daxpy(q,vert[vn].y,f,1,X->y,1);
    daxpy(q,vert[vn].z,f,1,X->z,1);

    vn = vnum(fac,1);
    Prism_faceMode(this,1,v+1 ,f);

    daxpy(q,vert[vn].x,f,1,X->x,1);
    daxpy(q,vert[vn].y,f,1,X->y,1);
    daxpy(q,vert[vn].z,f,1,X->z,1);

    vn = vnum(fac,2);
    Prism_faceMode(this,1,v+4 ,f);

    daxpy(q,vert[vn].x,f,1,X->x,1);
    daxpy(q,vert[vn].y,f,1,X->y,1);
    daxpy(q,vert[vn].z,f,1,X->z,1);
  }



  free(f);
}


void Hex::straight_face(Coord *X, int fac, int trip){
  register int i;
  int      q,vn;
  double   *f = dvector(0,QGmax*QGmax-1);
  Mode     *v = getbasis()->vert;

  q = qa*qb;

  dzero(q,X->x,1);
  dzero(q,X->y,1);
  dzero(q,X->z,1);

  for(i = 0; i < Nfverts(fac); ++i){
    vn = vnum(fac,i);
    Hex_faceMode(this,0,v + i,f);
    daxpy(q,vert[vn].x,f,1,X->x,1);
    daxpy(q,vert[vn].y,f,1,X->y,1);
    daxpy(q,vert[vn].z,f,1,X->z,1);
  }

  free(f);
}



#define distance(p1,p2) (sqrt((p2.x-p1.x)*(p2.x-p1.x)+(p2.y-p1.y)*(p2.y-p1.y)))

void Tri::genArc(Curve *cur, double *x, double *y){
  Point    p1, p2, rc, rd;
  double   alpha, theta, rad, arclen;
  double   *z, *w;
  int      fac;
  register int i, q;

  fac = cur->face;
  p1.x = vert[fac].x;        p1.y = vert[fac].y;
  p2.x = vert[(fac+1)%Nverts].x;  p2.y = vert[(fac+1)%Nverts].y;


  q = qa;           /* always evaluate at gll points */
  getzw(q,&z,&w,'a');

  arclen = distance(p1,p2);

  if(fabs(alpha = cur->info.arc.radius / arclen) < 0.505)
    error_msg(arc radius is too small to fit an edge);

  alpha = alpha > 0. ? -sqrt(alpha*alpha - 0.25) * arclen :
                        sqrt(alpha*alpha - 0.25) * arclen ;

  rc.x = (p1.x + p2.x) * .5 + alpha * (p2.y - p1.y) / arclen;
  rc.y = (p1.y + p2.y) * .5 + alpha * (p1.x - p2.x) / arclen;

  alpha = fabs (alpha);
  rad   = fabs (cur->info.arc.radius);
  rd.x  = ((p1.x + p2.x) * .5 - rc.x) * (rad / alpha);
  rd.y  = ((p1.y + p2.y) * .5 - rc.y) * (rad / alpha);
  theta = cur->info.arc.radius > 0. ? atan(.5 * arclen / alpha) :
                                       -atan(.5 * arclen / alpha) ;

  theta = (fac == 2)? -theta : theta;

  for(i = 0; i < q; ++i){/* calulate points based on gll distribution */
    alpha = theta * z[i];
    x[i] = rc.x + rd.x * cos(alpha) - rd.y * sin(alpha);
    y[i] = rc.y + rd.x * sin(alpha) + rd.y * cos(alpha);
  }

  return;
}

#define ITERNACA 1000
#define TOLNACA  1e-5
#define c1       ( 0.29690)
#define c2       (-0.12600)
#define c3       (-0.35160)
#define c4       ( 0.28430)
// #define c5       (-0.10150)
#define c5       (-0.10360)
void Tri::genNaca4(Curve *cur, double *x, double *y){
  Point    p1, p2;
  double   *z, *w,theta, ct, st,sign,r,ct1,st1;
  double   x0,y0,n0,n1,n2,f,fp,t,chord,a,c;
  int      fac,q;
  register int i, j;

  fac = cur->face;
  p1.x = vert[fac].x - cur->info.nac.xl;
  p1.y = vert[fac].y - cur->info.nac.yl;
  p2.x = vert[(fac+1)%Nverts].x - cur->info.nac.xl;
  p2.y = vert[(fac+1)%Nverts].y - cur->info.nac.yl;

  q = qa;           /* always evaluate at gll points */
  getzw(q,&z,&w,'a');

  /* set up initial guess as straight line */

  for(i = 0; i < q; ++i){
    x[i] = 0.5*(1-z[i])*p1.x + 0.5*(1+z[i])*p2.x;
    y[i] = 0.5*(1-z[i])*p1.y + 0.5*(1+z[i])*p2.y;
  }

  /*rotate points so that chord is parallel to x axis */
  theta = atan2(cur->info.nac.yt - cur->info.nac.yl,
    cur->info.nac.xt - cur->info.nac.xl);
  ct = cos(theta); st = sin(theta);
  drot(q,x,1,y,1,ct,st);

  t     = cur->info.nac.thickness;
  chord = sqrt(pow((cur->info.nac.xt - cur->info.nac.xl),2) +
         pow((cur->info.nac.yt - cur->info.nac.yl),2));

  a = 0.05; /* region in which polar search is made */

  for(i = 0; i < q; ++i){
    sign = (y[i] > 0.0)? 1.0:-1.0;

    x[i] /= chord; y[i] /= chord;

    x0 = x[i];  y0 = y[i];


    if(x0 < a){ /* do polar coord search around leading edge */

      n0 = 5*t*(c1*sqrt(a)+ a*(c2 + a*(c3 + a*(c4 + c5*a))));
      n1 = 5*t*(0.5*c1/sqrt(a)+ c2+a*(2*c3 + a*(3*c4 + 4*c5*a)));

      c   = a + n0*n1; /* find centre of radius from point 'a' on surface */
      ct1 = atan2(sign*y0,x0-c); /* find theta of x0,y0 from c */
      st1 = sin(ct1);
      ct1 = cos(ct1);
      r = sqrt((x0-c)*(x0-c) + y0*y0);


      /* do newton iteration to find points where radial direction
   and surface intersect */

      if((x0 != 0.0)&&(y0 != 0.0)){
  for(j = 0; j < ITERNACA; ++j){
    x0 = r*ct1 + c;

    n0 = 5*t*(c1*sqrt(x0) + x0*(c2 + x0*(c3 + x0*(c4 + c5*x0))));
    n1 = 5*t*(0.5*c1/sqrt(x0)+ c2 + x0*(2*c3 + x0*(3*c4 + 4*c5*x0)))*ct1;

    f  = (r*st1 - n0)*(r*st1 - n0);
    fp = 2*(r*st1 - n0)*(st1 - n1);

    if(fabs(f/fp) < TOLNACA){
      r -= f/fp;
    break;
    }
    r -= f/fp;
  }
  if(j == ITERNACA) fprintf(stderr,"genNaca4: didn't converge\n");
  x[i] = r*ct1 + c;
  y[i] = sign*r*st1;
      }
    }
    else{ /* else find point on surface which so that x0,y0 lies along normal
       to surface */
      for(j = 0; j < ITERNACA; ++j){
  if(fabs(x[i]) < TOLNACA){
    x[i] = 0.0;
    y[i] = 0.0;
    break;
  }

  n0 = 5*t*(c1*sqrt(x[i]) + x[i]*(c2 + x[i]*(c3 + x[i]*(c4 + c5*x[i]))));
  n0 *= sign;
  n1 = 5*t*(0.5*c1/sqrt(x[i])+ c2+x[i]*(2*c3 + x[i]*(3*c4 + 4*c5*x[i])));
  n1 *= sign;
  n2 = 5*t*(-0.25*c1/(x[i]*sqrt(x[i]))+2*c3 + x[i]*(6*c4 + 12*c5*x[i]));
  n2 *= sign;

  f  = (x0 - x[i]) - (y0 - n0)*n1;
  fp = -1.0 - (y0 - n0)*n2 + n1*n1;

  if(fabs(f/fp) < TOLNACA){
    x[i] -= f/fp;
    y[i] = n0;
    break;
  }
  x[i] -= f/fp;
  y[i] = n0;
      }
    }

    x[i] *= chord;
    y[i] *= chord;
  }

  /* rotate back */
  drot(q,x,1,y,1,ct,-st);

  /* add back leading edge contribution */
  for(i= 0; i < q; ++i){
    x[i] += cur->info.nac.xl;
    y[i] += cur->info.nac.yl;
  }

}

void Quad::genArc(Curve *cur, double *x, double *y){
  Point    p1, p2, rc, rd;
  double   alpha, theta, rad, arclen;
  double   *z, *w;
  int      fac;
  register int i, q;

  fac = cur->face;
  p1.x = vert[fac].x;        p1.y = vert[fac].y;
  p2.x = vert[(fac+1)%Nverts].x;  p2.y = vert[(fac+1)%Nverts].y;

  q = qa;           /* always evaluate at gll points */
  getzw(q,&z,&w,'a');

  arclen = distance(p1,p2);

  if(fabs(alpha = cur->info.arc.radius / arclen) < 0.505)
    error_msg(arc radius is too small to fit an edge);

  alpha = alpha > 0. ? -sqrt(alpha*alpha - 0.25) * arclen :
                        sqrt(alpha*alpha - 0.25) * arclen ;

  rc.x = (p1.x + p2.x) * .5 + alpha * (p2.y - p1.y) / arclen;
  rc.y = (p1.y + p2.y) * .5 + alpha * (p1.x - p2.x) / arclen;

  alpha = fabs (alpha);
  rad   = fabs (cur->info.arc.radius);
  rd.x  = ((p1.x + p2.x) * .5 - rc.x) * (rad / alpha);
  rd.y  = ((p1.y + p2.y) * .5 - rc.y) * (rad / alpha);
  theta = cur->info.arc.radius > 0. ? atan(.5 * arclen / alpha) :
                                       -atan(.5 * arclen / alpha) ;

  theta = (fac == 2 ||fac == 3)? -theta : theta;

  for(i = 0; i < q; ++i){/* calulate points based on gll distribution */
    alpha = theta * z[i];
    x[i] = rc.x + rd.x * cos(alpha) - rd.y * sin(alpha);
    y[i] = rc.y + rd.x * sin(alpha) + rd.y * cos(alpha);
  }
  return;
}
#define ITERNACA 1000
#define TOLNACA  1e-5
#define c1       ( 0.29690)
#define c2       (-0.12600)
#define c3       (-0.35160)
#define c4       ( 0.28430)
// #define c5       (-0.10150)
#define c5       (-0.10360)
void Quad::genNaca4(Curve *cur, double *x, double *y){
  Point    p1, p2;
  double   *z, *w,theta, ct, st,sign,r,ct1,st1;
  double   x0,y0,n0,n1,n2,f,fp,t,chord,a,c;
  int      fac,q;
  register int i, j;

  fac = cur->face;
  p1.x = vert[fac].x - cur->info.nac.xl;
  p1.y = vert[fac].y - cur->info.nac.yl;
  p2.x = vert[(fac+1)%Nverts].x - cur->info.nac.xl;
  p2.y = vert[(fac+1)%Nverts].y - cur->info.nac.yl;

  q = qa;           /* always evaluate at gll points */
  getzw(q,&z,&w,'a');

  /* set up initial guess as straight line */

  for(i = 0; i < q; ++i){
    x[i] = 0.5*(1-z[i])*p1.x + 0.5*(1+z[i])*p2.x;
    y[i] = 0.5*(1-z[i])*p1.y + 0.5*(1+z[i])*p2.y;
  }

  /*rotate points so that chord is parallel to x axis */
  theta = atan2(cur->info.nac.yt - cur->info.nac.yl,
    cur->info.nac.xt - cur->info.nac.xl);
  ct = cos(theta); st = sin(theta);
  drot(q,x,1,y,1,ct,st);

  t     = cur->info.nac.thickness;
  chord = sqrt(pow((cur->info.nac.xt - cur->info.nac.xl),2) +
         pow((cur->info.nac.yt - cur->info.nac.yl),2));

  a = 0.05; /* region in which polar search is made */

  for(i = 0; i < q; ++i){
    sign = (y[i] > 0.0)? 1.0:-1.0;

    x[i] /= chord; y[i] /= chord;

    x0 = x[i];  y0 = y[i];


    if(x0 < a){ /* do polar coord search around leading edge */

      n0 = 5*t*(c1*sqrt(a)+ a*(c2 + a*(c3 + a*(c4 + c5*a))));
      n1 = 5*t*(0.5*c1/sqrt(a)+ c2+a*(2*c3 + a*(3*c4 + 4*c5*a)));

      c   = a + n0*n1; /* find centre of radius from point 'a' on surface */
      ct1 = atan2(sign*y0,x0-c); /* find theta of x0,y0 from c */
      st1 = sin(ct1);
      ct1 = cos(ct1);
      r = sqrt((x0-c)*(x0-c) + y0*y0);


      /* do newton iteration to find points where radial direction
   and surface intersect */

      if((x0 != 0.0)&&(y0 != 0.0)){
  for(j = 0; j < ITERNACA; ++j){
    x0 = r*ct1 + c;

    n0 = 5*t*(c1*sqrt(x0) + x0*(c2 + x0*(c3 + x0*(c4 + c5*x0))));
    n1 = 5*t*(0.5*c1/sqrt(x0)+ c2 + x0*(2*c3 + x0*(3*c4 + 4*c5*x0)))*ct1;

    f  = (r*st1 - n0)*(r*st1 - n0);
    fp = 2*(r*st1 - n0)*(st1 - n1);

    if(fabs(f/fp) < TOLNACA){
      r -= f/fp;
    break;
    }
    r -= f/fp;
  }
  if(j == ITERNACA) fprintf(stderr,"genNaca4: didn't converge\n");
  x[i] = r*ct1 + c;
  y[i] = sign*r*st1;
      }
    }
    else{ /* else find point on surface which so that x0,y0 lies along normal
       to surface */
      for(j = 0; j < ITERNACA; ++j){
  if(fabs(x[i]) < TOLNACA){
    x[i] = 0.0;
    y[i] = 0.0;
    break;
  }

  n0 = 5*t*(c1*sqrt(x[i]) + x[i]*(c2 + x[i]*(c3 + x[i]*(c4 + c5*x[i]))));
  n0 *= sign;
  n1 = 5*t*(0.5*c1/sqrt(x[i])+ c2+x[i]*(2*c3 + x[i]*(3*c4 + 4*c5*x[i])));
  n1 *= sign;
  n2 = 5*t*(-0.25*c1/(x[i]*sqrt(x[i]))+2*c3 + x[i]*(6*c4 + 12*c5*x[i]));
  n2 *= sign;

  f  = (x0 - x[i]) - (y0 - n0)*n1;
  fp = -1.0 - (y0 - n0)*n2 + n1*n1;

  if(fabs(f/fp) < TOLNACA){
    x[i] -= f/fp;
    y[i] = n0;
    break;
  }
  x[i] -= f/fp;
  y[i] = n0;
      }
    }

    x[i] *= chord;
    y[i] *= chord;
  }

  /* rotate back */
  drot(q,x,1,y,1,ct,-st);

  /* add back leading edge contribution */
  for(i= 0; i < q; ++i){
    x[i] += cur->info.nac.xl;
    y[i] += cur->info.nac.yl;
  }

}



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
  double  xscal = dparam("XSCALE");
  double  yscal = dparam("YSCALE");
  double  xmove = dparam("XMOVE");
  double  ymove = dparam("YMOVE");

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

  if(xmove)  dsadd(g->npts,xmove,g->x,1,g->x,1);
  if(ymove)  dsadd(g->npts,ymove,g->y,1,g->y,1);
  if(xscal)  dscal(g->npts,xscal,g->x,1);
  if(yscal)  dscal(g->npts,yscal,g->y,1);

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

/* make sure that any vertex touching 'face' in element E has the
   same 'xy' co-ordinates */

static void CheckVertLoc(Element *U, Element *E, int face){
  int     gid1,gid2,i;
  double  x1,x2,y1,y2;

  gid1 = E->vert[face].gid;
  x1   = E->vert[face].x;
  y1   = E->vert[face].y;
  gid2 = E->vert[(face+1)%E->Nverts].gid;
  x2   = E->vert[(face+1)%E->Nverts].x;
  y2   = E->vert[(face+1)%E->Nverts].y;

  for(;U; U=U->next)
    for(i = 0; i < U->Nverts; ++i){
      if(U->vert[i].gid == gid1){
  U->vert[i].x = x1;
  U->vert[i].y = y1;
      }
      if(U->vert[i].gid == gid2){
  U->vert[i].x = x2;
  U->vert[i].y = y2;
      }
    }
}

void Quad::genFile (Curve *cur, double *x, double *y){
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


#define c1       ( 0.29690)
#define c2       (-0.12600)
#define c3       (-0.35160)
#define c4       ( 0.28430)
#define c5       (-0.10360)

/* naca profile -- usage: naca t x  returns points on naca 00 aerofoil of
   thickness t at position x

   to compile

   cc -o naca naca.c -lm
   */

double naca(double L, double x, double t){
  x = x/L;
  if(L==0.)
    return 0.;
  //  return 5.*t*L*(c1*sqrt(x)+ x*(c2 + x*(c3 + x*(c4 + c5*x))));
  return 5.*t*L*(c1*sqrt(x)+ x*(c2 + x*(c3 + x*(c4 + c5*x))));
}

#undef c1
#undef c2
#undef c3
#undef c4
#undef c5

void Quad_genNaca(Element *E, Curve *curve, double *x, double *y){
  int   i;
  Coord X;
  X.x = dvector(0, QGmax-1);
  X.y = dvector(0, QGmax-1);

  E->GetFaceCoord(curve->face, &X);

  dcopy(E->qa, X.x, 1, x, 1);
  for(i=0;i<E->qa;++i){
    y[i] = naca(curve->info.nac2d.length,
    X.x[i]-curve->info.nac2d.xo,
    curve->info.nac2d.thickness);
    if(X.y[i] < curve->info.nac2d.yo)
      y[i] =  curve->info.nac2d.yo-y[i];
    else
      y[i] =  curve->info.nac2d.yo+y[i];
  }

  free(X.x);
  free(X.y);
}
