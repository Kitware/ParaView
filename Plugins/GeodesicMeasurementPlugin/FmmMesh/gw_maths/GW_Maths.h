/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_Maths.h
 *  \brief  Definition of class \c GW_Maths
 *  \author Gabriel Peyré
 *  \date   10-28-2002
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_MATHS_H_
#define _GW_MATHS_H_

#include "GW_MathsConfig.h"
#include "GW_Vector3D.h" // here
#include "GW_Vector2D.h"
//#include "GW_MatrixNxP.h"
// #include "GW_VectorND.h"
// #include "GW_MatrixStatic.h"
// #include "GW_VectorStatic.h"

GW_BEGIN_NAMESPACE


/** Numerical storage methods */
GW_Float **matrix(long nrl, long nrh, long ncl, long nch);
void free_matrix(GW_Float **m, long nrl, long nrh, long ncl, long nch);
GW_Float *fvector(long nl, long nh);
void free_vector(GW_Float *v, long nl, long nh);
void ludcmp(GW_Float **a, GW_I32 n, GW_I32 *indx, GW_Float *d);
void lubksb(GW_Float **a, GW_I32 n, GW_I32 *indx, GW_Float b[]);



/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_Maths
 *  \brief  Basic Maths functions.
 *  \author Gabriel Peyré
 *  \date   10-28-2002
 *
 *  A bunch of useful functions to work on a sphere. Warning : All
 *  vectors are assumed to live un the sphere of radius 1 : the must
 *  be normalized.
 */
/*------------------------------------------------------------------------------*/
class GW_Maths
{

public:

    /*------------------------------------------------------------------------------*/
    // Name : GW_Maths::SphericalTriangleArea
    /**
    *  \param  v1 [GW_Vector3D&] Point of the triangle.
    *  \param  v2 [GW_Vector3D&] Point of the triangle.
    *  \param  v3 [GW_Vector3D&] Point of the triangle.
    *  \return [GW_Float] The area.
    *  \author Gabriel Peyré
    *  \date   10-28-2002
    *
    *  Compute the area of a spherical triangle.
    */
    /*------------------------------------------------------------------------------*/
    GW_Float SphericalTriangleArea(const GW_Vector3D& v1, const GW_Vector3D& v2, const GW_Vector3D& v3)
    {
        // first find the cosine of the arc lengths
        GW_Real64 cos_a = v2 * v3;
        GW_Real64 cos_b = v1 * v3;
        GW_Real64 cos_c = v1 * v2;

        // now the sine (positive since 0 <= a <= pi)
        GW_Real64 sin_a = sqrt( 1. - cos_a * cos_a );
        GW_Real64 sin_b = sqrt( 1. - cos_b * cos_b );
        GW_Real64 sin_c = sqrt( 1. - cos_c * cos_c );

        // now find the angles A, B, and C
        GW_Real64 A = acos( ( cos_a - cos_b * cos_c ) / ( sin_b * sin_c ) );
        GW_Real64 B = acos( ( cos_b - cos_c * cos_a ) / ( sin_c * sin_a ) );
        GW_Real64 C = acos( ( cos_c - cos_a * cos_b ) / ( sin_a * sin_b ) );

        return (GW_Float) ( A + B + C - GW_PI );
    }


    /*------------------------------------------------------------------------------*/
    // Name : GW_Maths::FlatTriangleArea
    /**
    *  \param  v1 [GW_Vector2D&] a point of the triangle.
    *  \param  v2 [GW_Vector2D&] a point of the triangle.
    *  \param  v3 [GW_Vector2D&] a point of the triangle.
    *  \return [GW_Float] The area
    *  \author Gabriel Peyré
    *  \date   10-28-2002
    *
    *  Compute the area of a plane triangle.
    */
    /*------------------------------------------------------------------------------*/
    static GW_Float FlatTriangleArea(const GW_Vector2D& v1, const GW_Vector2D& v2, const GW_Vector2D& v3)
    {
        /* half the Z component of the cross product (v2-v1)^(v3-v1) */
        return 0.5f * GW_ABS( (v2[0]-v1[0])*(v3[1]-v1[1])-(v2[1]-v1[1])*(v3[0]-v1[0]) );
    }

    /*------------------------------------------------------------------------------*/
    // Name : GW_Maths::FlatTriangleArea
    /**
    *  \param  v1 [GW_Vector3D&] a point of the triangle.
    *  \param  v2 [GW_Vector3D&] a point of the triangle.
    *  \param  v3 [GW_Vector3D&] a point of the triangle.
    *  \return [GW_Float] The area
    *  \author Gabriel Peyré
    *  \date   10-28-2002
    *
    *  Compute the area of a plane triangle.
    */
    /*------------------------------------------------------------------------------*/
    static GW_Float FlatTriangleArea(const GW_Vector3D& v1, const GW_Vector3D& v2, const GW_Vector3D& v3)
    {
        return 0.5f * ~( (v2-v1)^(v3-v1) );
    }

    /*------------------------------------------------------------------------------*/
    // Name : GW_Maths::FlatQuadrilaterArea
    /**
    *  \param  v1 [GW_Vector3D&] 1st point.
    *  \param  v2 [GW_Vector3D&] 2nd point.
    *  \param  v3 [GW_Vector3D&] 3rd point.
    *  \param  v4 [GW_Vector3D&] 4th point.
    *  \return [GW_Float] Area.
    *  \author Gabriel Peyré
    *  \date   5-30-2003
    *
    *  Compute the area of a CONVEX quadrilater.
    */
    /*------------------------------------------------------------------------------*/
    static GW_Float FlatQuadrilaterArea( const GW_Vector3D& v1, const GW_Vector3D& v2, const GW_Vector3D& v3, const GW_Vector3D& v4 )
    {
        GW_Float rArea = 0;
        rArea += GW_Maths::FlatTriangleArea(v1, v2, v3);
        rArea += GW_Maths::FlatTriangleArea(v3, v4, v1);
        return rArea;
    }

    /*------------------------------------------------------------------------------*/
    // Name : GW_Maths::FlatTriangleNormal
    /**
    *  \param  Normal [GW_Vector3D&] The return value.
    *  \param  v1 [GW_Vector3D&] A point in the triangle.
    *  \param  v2 [GW_Vector3D&] A point in the triangle.
    *  \param  v3 [GW_Vector3D&] A point in the triangle.
    *  \author Gabriel Peyré
    *  \date   10-28-2002
    *
    *  Compute the normal of a triangle, assuming CW orientation.
    */
    /*------------------------------------------------------------------------------*/
    static void FlatTriangleNormal(GW_Vector3D& Normal, const GW_Vector3D& v1, const GW_Vector3D& v2, const GW_Vector3D& v3)
    {
        Normal = (v2-v1)^(v3-v1);
    }

    /*------------------------------------------------------------------------------*/
    // Name : GW_Maths::ComputeTriangleArea
    /**
    *  \param  a [GW_Float] Length of a side.
    *  \param  b [GW_Float] Length of a side.
    *  \param  c [GW_Float] Length of a side.
    *    \return Area of the triangle.
    *  \author Gabriel Peyré
    *  \date   4-27-2003
    *
    *  Compute the area of a triangle using Heron rule.
    */
    /*------------------------------------------------------------------------------*/
    static GW_Float ComputeTriangleArea( GW_Float a, GW_Float b, GW_Float c )
    {
        GW_Float p = (GW_Float) 0.5*(a+b+c);
        if( p*(p-a)*(p-b)*(p-c)<0 )
            return 0;
        return (GW_Float) sqrt( p*(p-a)*(p-b)*(p-c) );
    }


    /*------------------------------------------------------------------------------*/
    // Name : GW_Maths::ConvertCartesianToSpherical
    /**
    *  \param  Pos [GW_Vector3D] Absolute position of the point.
    *  \param  rLong [GW_Float&] Return value : longitude.
    *  \param  rLat [GW_Float&] Return value : latitude.
    *  \author Gabriel Peyré
    *  \date   10-27-2002
    *
    *  Convert from absolute cartesian coords to polar coords
    */
    /*------------------------------------------------------------------------------*/
    static void ConvertCartesianToSpherical(const GW_Vector3D& Pos, GW_Float& rLong, GW_Float& rLat)
    {
        GW_ASSERT( Pos[0] <= 1 );
        GW_ASSERT( Pos[0] >= -1 );
        GW_ASSERT( Pos[2] <= 1 );
        GW_ASSERT( Pos[2] >= -1 );

        rLat = (GW_Float) asin( Pos[2] );

        GW_Float rVal = (GW_Float) sqrt( Pos[0]*Pos[0] + Pos[1]*Pos[1] );
        if( rVal==0 )
            rLong = 0;
        else
        {
            rVal = Pos[0]/rVal;
            GW_ASSERT( rVal >= -1 );
            GW_ASSERT( rVal <= 1 );
            if( rVal < -1 )
                rVal = -1;
            if( rVal > 1 )
                rVal = 1;
            rLong = (GW_Float) acos( rVal );
            if( Pos[1]<0  )
                rLong = GW_TWOPI-rLong;
        }
    }

    static void SolveLU( GW_Float **a, GW_I32 n, GW_Float *b )
    {
        GW_Float d;
        GW_I32* indx = new GW_I32[n];
        ludcmp(a,n,indx-1,&d);
        lubksb(a,n,indx-1,b);
        GW_DELETEARRAY( indx );
    }

    /*------------------------------------------------------------------------------*/
    // Name : GW_Maths::Fit2ndOrderPolynomial2D
    /**
    *  \param  Points[2][6] [GW_Float] Coordinates of the points.
    *  \param  Values[6] [GW_Float] Value of the function.
    *  \param  Coeffs[6] [GW_Float] Coefficients.
    *  \author Gabriel Peyré
    *  \date   4-12-2003
    *
    *  Find the coefficients of a 2nd order 2D polynomial that pass
    *  through 6 points. The coefficients are given in that order :
    *    cst, X, Y, XY, X^2, Y^2.
    */
    /*------------------------------------------------------------------------------*/
    static void Fit2ndOrderPolynomial2D( const GW_Float Points[6][2], const GW_Float Values[6], GW_Float Coeffs[6] )
    {
        /* build the matrix */
        GW_Float** M = matrix(1,6,1,6);

        for( GW_U32 i=0; i<6; ++i )
            Coeffs[i] = Values[i];
        for( GW_U32 i=1; i<=6; ++i )
        {
            GW_Float x = Points[i-1][0];
            GW_Float y = Points[i-1][1];
            M[i][1] = 1;
            M[i][2] = x;
            M[i][3] = y;
            M[i][4] = x*y;
            M[i][5] = x*x;
            M[i][6] = y*y;
        }
        GW_Maths::SolveLU( (GW_Float**) M, 6, Coeffs-1 );

        free_matrix( M, 1,6,1,6 );
    }

    /*------------------------------------------------------------------------------*/
    // Name : GW_Maths::ComputeLocalGeodesicParameter
    /**
    *  \param  x [GW_Float&] 1st geodesic coord in LOCAL COORDS
    *  \param  y [GW_Float&] 2nd geodesic coord in LOCAL COORDS
    *  \param  z [GW_Float&] 3rd geodesic coord in LOCAL COORDS
    *  \param  a [GW_Float] 1st geodesic coord GLOBAL COORDS
    *  \param  b [GW_Float] 2nd geodesic coord GLOBAL COORDS
    *  \param  c [GW_Float] 3rd geodesic coord GLOBAL COORDS
    *  \param  a0 [GW_Float] 1st geodesic coord GLOBAL COORDS
    *  \param  b0 [GW_Float] 2nd geodesic coord GLOBAL COORDS
    *  \param  c0 [GW_Float] 3rd geodesic coord GLOBAL COORDS
    *  \param  a1 [GW_Float] 1st geodesic coord GLOBAL COORDS
    *  \param  b1 [GW_Float] 2nd geodesic coord GLOBAL COORDS
    *  \param  c1 [GW_Float] 3rd geodesic coord GLOBAL COORDS
    *  \param  a2 [GW_Float] 2nd geodesic coord GLOBAL COORDS
    *  \param  b2 [GW_Float] 3rd geodesic coord GLOBAL COORDS
    *  \param  c2 [GW_Float] 3rd geodesic coord GLOBAL COORDS
    *  \return >=0 if the conversion was successful, <0 otherwise.
    *  \author Gabriel Peyré
    *  \date   4-30-2003
    *
    *  Convert from global parametric coords to local ones.
    */
    /*------------------------------------------------------------------------------*/
    static GW_I32 ComputeLocalGeodesicParameter( GW_Float& x, GW_Float& y, GW_Float& z,
                                                        GW_Float a, GW_Float b, GW_Float /*c*/,
                                                        GW_Float a0, GW_Float b0, GW_Float /*c0*/,
                                                        GW_Float a1, GW_Float b1, GW_Float /*c1*/,
                                                        GW_Float a2, GW_Float b2, GW_Float /*c2*/ )
    {
        /* the system to solve is :
            [a0 a1 a2] [x]   [a]
            [b0 b1 b2]*[y] = [b]
            [c0 c1 c2] [z]   [c]
        Using x+y+z = 1, this becomes :
            [a0-a2 a1-a2] [x]   [a-a2]
            [b0-b2 b1-b2]*[y] = [b-b2]
        */
        GW_Float m00 = a0-a2;
        GW_Float m01 = a1-a2;
        GW_Float m10 = b0-b2;
        GW_Float m11 = b1-b2;
        GW_Float rDet = m00*m11-m01*m10;
        a = a - a2;    // rhs modification
        b = b - b2;
    //    GW_ASSERT( rDet!=0 );
        /* The inverse of the matrix is :
            [m11  -m01]
            [-m10  m00] * 1/rDet */
        if( GW_ABS(rDet)>GW_EPSILON )
        {
            x = 1/rDet * ( m11*a - m01*b );
            y = 1/rDet * (-m10*a + m00*b );
            z = 1 - x - y;
        }
        else
        {
            x = y = z = -GW_INFINITE;
            return -1;
        }
        return 0;
    }

    /*------------------------------------------------------------------------------*/
    // Name : GW_Maths::TestClass
    /**
    *  \author Gabriel Peyré
    *  \date   4-12-2003
    *
    *  Test the class.
    */
    /*------------------------------------------------------------------------------*/
    static void TestClass(std::ostream &s = cout)
    {
        TestClassHeader("GW_Maths", s);
        GW_Float Points[6][2];
        GW_Float Values[6];
        GW_Float Coeffs[6];
        for( GW_U32 i=0; i<6; ++i )
        {
            Points[i][0] = GW_RAND;
            Points[i][1] = GW_RAND;
            Values[i] = GW_RAND;
        }
        GW_Maths::Fit2ndOrderPolynomial2D( Points, Values, Coeffs );
        for( GW_U32 i=0; i<6; ++i )
        {
            GW_Float x = Points[i][0];
            GW_Float y = Points[i][1];
            GW_Float t = Coeffs[0] + Coeffs[1]*x + Coeffs[2]*y + Coeffs[3]*x*y +
                        Coeffs[4]*x*x + Coeffs[5]*y*y;
            GW_ASSERT( GW_ABS(t-Values[i])<GW_EPSILON );
        }
        TestClassFooter("GW_Maths", s);
    }


    /* Conversion methods **********************************************************/
    /** Conversion method, from static size to dynamic size. */
#if 0
    template<GW_U32 v_size, class v_type>
    inline
    static void Convert(GW_VectorND& a, const GW_VectorStatic<v_size,v_type>& b)
    {
        a.Reset( v_size );
        for( GW_U32 i=0; i<v_size; ++i )
            a.SetData( i, b.GetData(i) );
    }
    /** Conversion method, from static size to dynamic size. */
    template<GW_U32 r_size, GW_U32 c_size, class v_type>
    inline
    static void Convert(GW_MatrixNxP& a, const GW_MatrixStatic<r_size,c_size,v_type>& b)
    {
        a.Reset( r_size, c_size );
        for( GW_U32 i=0; i<r_size; ++i )
        for( GW_U32 j=0; j<c_size; ++j )
            a.SetData( i, j, b.GetData(i,j) );
    }
    /** Conversion method, from dynamic size to static size. */
    template<GW_U32 v_size,class v_type>
    inline
    static void Convert(GW_VectorStatic<v_size,v_type>& a, const GW_VectorND& b)
    {
        GW_ASSERT( v_size==b.GetDim() );
        for( GW_U32 i=0; i<GW_MIN(v_size,b.GetDim()); ++i )
            a.SetData( i, b.GetData(i) );
    }
    /** Conversion method, from dynamic size to static size. */
    template<GW_U32 r_size,GW_U32 c_size,class v_type>
    inline
    static void Convert( GW_MatrixStatic<r_size,c_size,v_type>& a, const GW_MatrixNxP& b)
    {
        GW_ASSERT( r_size==b.GetNbrRows() && c_size==b.GetNbrCols() );
        for( GW_U32 i=0; i<GW_MIN(v_size,b.GetNbrRows()); ++i )
        for( GW_U32 j=0; j<GW_MIN(c_size,b.GetNbrCols()); ++j )
            a.SetData( i,j, b.GetData(i,j) );
    }
#endif
private:

};

#define NR_END 1
#define FREE_ARG char*
/* allocate a GW_Float matrix with subscript range m[nrl..nrh][ncl..nch] */
inline
GW_Float **matrix(long nrl, long nrh, long ncl, long nch)
{
    long i, nrow=nrh-nrl+1,ncol=nch-ncl+1;
    GW_Float **m;

    /* allocate pointers to rows */
    m=(GW_Float **) malloc((size_t)((nrow+NR_END)*sizeof(GW_Float*)));
    GW_ASSERT( m!=NULL );
    m += NR_END;
    m -= nrl;

    /* allocate rows and set pointers to them */
    m[nrl]= (GW_Float *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(GW_Float)));
    GW_ASSERT( m[nrl]!=NULL );
    m[nrl] += NR_END;
    m[nrl] -= ncl;

    for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;

    /* return pointer to array of pointers to rows */
    return m;
}

/* free a GW_Float matrix allocated by matrix() */
inline
void free_matrix(GW_Float **m, long nrl, long /*nrh*/, long ncl, long /*nch*/)
{
    free((FREE_ARG) (m[nrl]+ncl-NR_END));
    free((FREE_ARG) (m+nrl-NR_END));
}

/* allocate a GW_Float vector with subscript range v[nl..nh] */
inline
GW_Float *fvector(long nl, long nh)
{
    GW_Float *v;
    v=(GW_Float*) malloc((size_t) ((nh-nl+1+NR_END)*sizeof(GW_Float)));
    GW_ASSERT( v!=NULL );
    return v-nl+NR_END;
}

/* free a GW_Float vector allocated with vector() */
inline
void free_vector(GW_Float *v, long nl, long /*nh*/)
{
    free((FREE_ARG) (v+nl-NR_END));
}

/************************************************************************/
/*
    Given a matrix a[1..n][1..n], this routine replaces it by the LU decomposition of a rowwise
    permutation of itself. a and n are input. a is output, arranged as in equation (2.3.14) above;
    indx[1..n] is an output vector that records the row permutation elected by the partial
    pivoting; d is output as ±1 depending on whether the number of row interchanges was even
    or odd, respectively. This routine is used in combination with lubksb to solve linear equations
    or invert a matrix.
*/
/************************************************************************/
inline
void ludcmp(GW_Float **a, GW_I32 n, GW_I32 *indx, GW_Float *d)
{
    GW_I32 i,imax,j,k;
    GW_Float big,dum,sum,temp;
    GW_Float *vv;
    vv = fvector(1,n);
    *d=1.0;
    imax = 0;
    for (i=1;i<=n;i++)
    {
        big=0.0;
        for (j=1;j<=n;j++)
            if ((temp=fabs(a[i][j])) > big) big = (GW_Float) temp;
        GW_ASSERT(big != 0.0);
        vv[i] = (GW_Float) 1.0/big;
    }
    for (j=1;j<=n;j++)
    {
        for (i=1;i<j;i++)
        {
            sum=a[i][j];
            for (k=1;k<i;k++)
                sum -= a[i][k]*a[k][j];
            a[i][j]=sum;
        }
        big=0.0;
        for (i=j;i<=n;i++)
        {
            sum=a[i][j];
            for (k=1;k<j;k++)
                sum -= a[i][k]*a[k][j];
            a[i][j]=sum;
            if ( (dum=vv[i]*fabs(sum)) >= big)
            {
                big=dum;
                imax=i;
            }
        }
        if (j != imax)
        {
            for (k=1;k<=n;k++)
            {
                dum=a[imax][k];
                a[imax][k]=a[j][k];
                a[j][k]=dum;
            }
            *d = -(*d);
            vv[imax]=vv[j];
        }
        indx[j]=imax;
        if (a[j][j] == 0.0) a[j][j]=GW_EPSILON;
        if (j != n)
        {
            dum=1.0/(a[j][j]);
            for (i=j+1;i<=n;i++) a[i][j] *= dum;
        }
    }
    free_vector( vv, 1, n );
}

/************************************************************************/
/*
    Solves the set of n linear equations A·X = B. Here a[1..n][1..n] is input, not as the matrix
    A but rather as its LU decomposition, determined by the routine ludcmp. indx[1..n] is input
    as the permutation vector returned by ludcmp. b[1..n] is input as the right-hand side vector
    B, and returns with the solution vector X. a, n, and indx are not modi?ed by this routine
    and can be left in place for successive calls with di?erent right-hand sides b. This routine takes
    into account the possibility that b will begin with many zero elements, so it is e?cient for use
    in matrix inversion.
*/
/************************************************************************/
inline
void lubksb(GW_Float **a, GW_I32 n, GW_I32 *indx, GW_Float b[])
{
    GW_I32 i,ii=0,ip,j;
    GW_Float sum;
    for( i=1;i<=n;i++ )
    {
        ip=indx[i];
        sum=b[ip];
        b[ip]=b[i];
        if (ii)
        {
            for (j=ii;j<=i-1;j++)
                sum -= a[i][j]*b[j];
        }
        else
        {
            if (sum)
                ii=i;
        }
        b[i]=sum;
    }
    for( i=n;i>=1;i-- )
    {
        sum=b[i];
        for (j=i+1;j<=n;j++)
            sum -= a[i][j]*b[j];
        b[i]=sum/a[i][i];
    }
}


GW_END_NAMESPACE


#endif // _GW_MATHS_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
