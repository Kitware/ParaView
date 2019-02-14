
/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_PolygonIntersector.h
 *  \brief  Definition of class \c GW_PolygonIntersector
 *  \author Gabriel Peyré
 *  \date   6-11-2003
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_POLYGONINTERSECTOR_H_
#define _GW_POLYGONINTERSECTOR_H_

#include "GW_Config.h"

namespace GW {

/*------------------------------------------------------------------------------*/
/**
 *  \class  GW_PolygonIntersector
 *  \brief  Compute the intersection of 2 CONVEX polygon.
 *  \author Gabriel Peyré
 *  \date   6-11-2003
 *
 *  This is based on the source code from the book <i>Computational Geometry in C</i>
 *    by Joseph O'Rourke, see
 *    <a href="http://cs.smith.edu/~orourke/books/compgeom.html">http://cs.smith.edu/~orourke/books/compgeom.html</a>.
 */
/*------------------------------------------------------------------------------*/

class FMMMESH_EXPORT GW_PolygonIntersector
{

public:

    /*------------------------------------------------------------------------------*/
    /** \name Constructor and destructor */
    /*------------------------------------------------------------------------------*/
    //@{
    GW_PolygonIntersector()
    {}
    //@}

    T_Vector2DList& PerformIntersection( const T_Vector2DList& P1, const T_Vector2DList& P2 )
    {
        int n = (int) P1.size();
        int m = (int) P2.size();
        tPolygoni PolyList1 = new tPointi[n+10];
        tPolygoni PolyList2 = new tPointi[m+10];
        GW_Float scale_factor = 50000;        // this is because the algorithm works with int
        /* fill the 2 polygons */
        GW_U32 num = 0;
        for( CIT_Vector2DList it = P1.begin(); it!=P1.end(); ++it )
        {
            const GW_Vector2D& v = *it;
            PolyList1[num][0] = (int) (v[0]*scale_factor);
            PolyList1[num][1] = (int) (v[1]*scale_factor);
            num++;
        }
        num = 0;
        for( CIT_Vector2DList it = P2.begin(); it!=P2.end(); ++it )
        {
            const GW_Vector2D& v = *it;
            PolyList2[num][0] = (int) (v[0]*scale_factor);
            PolyList2[num][1] = (int) (v[1]*scale_factor);
            num++;
        }

        ConvexIntersect( PolyList1, PolyList2, n, m, IntersectionPolygon_ );

        /** rescale back the polygon */
        for( IT_Vector2DList it = IntersectionPolygon_.begin(); it!=IntersectionPolygon_.end(); ++it )
            *it /= scale_factor;

        GW_DELETEARRAY(PolyList1);
        GW_DELETEARRAY(PolyList2);

        return IntersectionPolygon_;
    }

    T_Vector2DList& GetIntersectionPolygon()
    {
        return IntersectionPolygon_;
    }

    GW_Float GetAreaIntersection()
    {
        return GW_PolygonIntersector::ComputePolygonArea( IntersectionPolygon_ );
    }


    static GW_Float ComputePolygonArea( T_Vector2DList& Poly )
    {
        if( Poly.size()<3 )
            return 0;
        IT_Vector2DList it = Poly.begin();
        GW_Vector2D& v0 = *it;
        it++;
        GW_Vector2D& v1 = *it;
        it++;
        GW_Float area = 0;
        while( it!=Poly.end() )
        {
            GW_Vector2D v2 = *it;
            area += GW_Maths::FlatTriangleArea( v0, v1, v2 );
            v1 = v2;
            it++;
        }
        return area;
    }

    /** return true if the polygon is in positive orientation */
    static GW_Bool CheckOrientation( T_Vector2DList& Poly )
    {
        if( Poly.size()<3 )
        {
            GW_ASSERT( GW_False );
            return GW_True;
        }
        GW_Vector2D center;
        for( IT_Vector2DList it=Poly.begin(); it!=Poly.end(); ++it )
            center += *it;
        center /= Poly.size();
        IT_Vector2DList itpoly=Poly.begin();
        GW_Vector2D v0 = *itpoly;
        itpoly++;
        GW_I32 orientation = 0;
        while( itpoly!=Poly.end() )
        {
            GW_Vector2D v1 = *itpoly;
            GW_Float crossp = (v0[0]-center[0])*(v1[1]-center[1])-(v0[1]-center[1])*(v1[0]-center[0]);
            GW_ASSERT( (crossp>0 && orientation>=0) || (crossp<0 && orientation<=0) );
            if( crossp>0 )
                orientation++;
            else
                orientation--;
            v0 = v1;
            itpoly++;
        }
        return orientation>0;
    }

    void TestClass(std::ostream &s = cout)
    {
        TestClassHeader("GW_PolygonIntersector", s);
        GW_PolygonIntersector PolyInt;
        T_Vector2DList P1, P2;
        P1.push_back(GW_Vector2D(0,0));
        P1.push_back(GW_Vector2D(2,0));
        P1.push_back(GW_Vector2D(2,2));
        P1.push_back(GW_Vector2D(0,2));
        P2.push_back(GW_Vector2D(1,1));
        P2.push_back(GW_Vector2D(3,1));
        P2.push_back(GW_Vector2D(3,3));
        P2.push_back(GW_Vector2D(1,3));

        T_Vector2DList& R = PolyInt.PerformIntersection( P1,P2 );
        for( IT_Vector2DList it=R.begin(); it!=R.end(); ++it )
            cout << *it << endl;

        cout << "Area : " << PolyInt.GetAreaIntersection() << endl;
        TestClassFooter("GW_PolygonIntersector", s);
    }

private:

    /** the computed polygon */
    T_Vector2DList IntersectionPolygon_;

    /** internal definition */
    #define EXIT_SUCCESS 0
    #define EXIT_FAILURE 1
    #undef X
    #undef Y
    #define X       0
    #define Y       1
    enum tInFlag
    { Pin, Qin, Unknown } ;

    /** Polygon and points */
    #define DIM     2               /* Dimension of points */
    typedef int     tPointi[DIM];   /* type integer point */
    typedef double  tPointd[DIM];   /* type double point */
    typedef tPointi* tPolygoni;/* type integer polygon */

    void debug_printf(const char*, ...)
    {}


    /** P has n vertices, Q has m vertices. */
    void ConvexIntersect( tPolygoni P, tPolygoni Q, int n, int m, T_Vector2DList& R )
    {
        int     a, b;           /* indices on P and Q (resp.) */
        int     a1, b1;         /* a-1, b-1 (resp.) */
        tPointi A, B;           /* directed edges on P and Q (resp.) */
        int     cross;          /* sign of z-component of A x B */
        int     bHA, aHB;       /* b in H(A); a in H(b). */
        tPointi Origin = {0,0}; /* (0,0) */
        tPointd p;              /* double point of intersection */
        tPointd q;              /* second point of intersection */
        tInFlag inflag;         /* {Pin, Qin, Unknown}: which inside */
        int     aa, ba;         /* # advances on a & b indices (after 1st inter.) */
        bool    FirstPoint;     /* Is this the first point? (used to initialize).*/
        tPointd p0;             /* The first point. */
        int     code;           /* SegSegInt return code. */

        /* Initialize variables. */
        a = 0; b = 0; aa = 0; ba = 0;
        inflag = Unknown; FirstPoint = GW_True;

        do {
            /* Computations of key variables. */
            a1 = (a + n - 1) % n;
            b1 = (b + m - 1) % m;

            SubVec( P[a], P[a1], A );
            SubVec( Q[b], Q[b1], B );

            cross = AreaSign( Origin, A, B );
            aHB   = AreaSign( Q[b1], Q[b], P[a] );
            bHA   = AreaSign( P[a1], P[a], Q[b] );
            debug_printf("%%cross=%d, aHB=%d, bHA=%d\n", cross, aHB, bHA );

            /* If A & B intersect, update inflag. */
            code = SegSegInt( P[a1], P[a], Q[b1], Q[b], p, q );
            debug_printf("%%SegSegInt: code = %c\n", code );
            if ( code == '1' || code == 'v' ) {
                if ( inflag == Unknown && FirstPoint ) {
                    aa = ba = 0;
                    FirstPoint = GW_False;
                    p0[X] = p[X]; p0[Y] = p[Y];
                    debug_printf("%8.2lf %8.2lf moveto\n", p0[X], p0[Y] );
                }
                inflag = InOut( p, inflag, aHB, bHA );
                R.push_back( GW_Vector2D(p[X], p[Y]) );
                debug_printf("%%InOut sets inflag=%d\n", inflag);
            }

            /*-----Advance rules-----*/
            /* Special case: A & B overlap and oppositely oriented. */
            if ( ( code == 'e' ) && (Dot( A, B ) < 0) )
            {
//                GW_ASSERT( GW_False );
                return;
            }

            /* Special case: A & B parallel and separated. */
            if ( (cross == 0) && ( aHB < 0) && ( bHA < 0 ) )
            {
                debug_printf("%%P and Q are disjoint.\n");
                return;
            }

            /* Special case: A & B collinear. */
            else if ( (cross == 0) && ( aHB == 0) && ( bHA == 0 ) ) {
                    /* Advance but do not output point. */
                    if ( inflag == Pin )
                    b = Advance( b, &ba, m, inflag == Qin, Q[b], R );
                    else
                    a = Advance( a, &aa, n, inflag == Pin, P[a], R );
                }

            /* Generic cases. */
            else if ( cross >= 0 ) {
                if ( bHA > 0)
                    a = Advance( a, &aa, n, inflag == Pin, P[a], R );
                else
                    b = Advance( b, &ba, m, inflag == Qin, Q[b], R );
            }
            else /* if ( cross < 0 ) */{
                if ( aHB > 0)
                    b = Advance( b, &ba, m, inflag == Qin, Q[b], R );
                else
                    a = Advance( a, &aa, n, inflag == Pin, P[a], R );
            }
            debug_printf("%%After advances:a=%d, b=%d; aa=%d, ba=%d; inflag=%d\n", a, b, aa, ba, inflag);

        /* Quit when both adv. indices have cycled, or one has cycled twice. */
        } while ( ((aa < n) || (ba < m)) && (aa < 2*n) && (ba < 2*m) );

        if ( !FirstPoint ) /* If at least one point output, close up. */
                debug_printf("%8.2lf %8.2lf lineto\n", p0[X], p0[Y] );

        /* Deal with special cases: not implemented. */
        if ( inflag == Unknown)
            debug_printf("%%The boundaries of P and Q do not cross.\n");
    }


    /** Prints out the double point of intersection, and toggles in/out flag. */
    tInFlag InOut( tPointd p, tInFlag inflag, int aHB, int bHA )
    {
        debug_printf("%8.2lf %8.2lf lineto\n", p[X], p[Y] );

        /* Update inflag. */
        if      ( aHB > 0)
            return Pin;
        else if ( bHA > 0)
            return Qin;
        else    /* Keep status quo. */
            return inflag;
    }
    /** Advances and prints out an inside vertex if appropriate. */
    int Advance( int a, int *aa, int n, bool inside, tPointi v, T_Vector2DList& R )
    {
        if ( inside )
        {
            debug_printf("%5d    %5d    lineto\n", v[X], v[Y] );
            R.push_back( GW_Vector2D(v[X], v[Y]) );
        }
        (*aa)++;
        return  (a+1) % n;
    }
    /** a - b ==> c. */
    void    SubVec( tPointi a, tPointi b, tPointi c )
    {
        int i;
        for( i = 0; i < DIM; i++ )
            c[i] = a[i] - b[i];
    }
    int    AreaSign( tPointi a, tPointi b, tPointi c )
    {
        double area2;

        area2 = ( b[0] - a[0] ) * (double)( c[1] - a[1] ) -
                ( c[0] - a[0] ) * (double)( b[1] - a[1] );

        /* The area should be an integer. */
        if      ( area2 >  0.5 ) return  1;
        else if ( area2 < -0.5 ) return -1;
        else                     return  0;
    }

    /** SegSegInt: Finds the point of intersection p between two closed
        segments ab and cd.  Returns p and a char with the following meaning:
        'e': The segments collinearly overlap, sharing a point.
        'v': An endpoint (vertex) of one segment is on the other segment,
                but 'e' doesn't hold.
        '1': The segments intersect properly (i.e., they share a point and
                neither 'v' nor 'e' holds).
        '0': The segments do not intersect (i.e., they share no points).
        Note that two collinear segments that share just one point, an endpoint
        of each, returns 'e' rather than 'v' as one might expect. */
    char SegSegInt( tPointi a, tPointi b, tPointi c, tPointi d, tPointd p, tPointd q )
    {
        double  s, t;       /* The two parameters of the parametric eqns. */
        double num, denom;  /* Numerator and denoninator of equations. */
        char code = '?';    /* Return char characterizing intersection. */

        denom = a[X] * (double)( d[Y] - c[Y] ) +
                b[X] * (double)( c[Y] - d[Y] ) +
                d[X] * (double)( b[Y] - a[Y] ) +
                c[X] * (double)( a[Y] - b[Y] );

        /* If denom is zero, then segments are parallel: handle separately. */
        if (denom == 0.0)
            return  ParallelInt(a, b, c, d, p, q);

        num =    a[X] * (double)( d[Y] - c[Y] ) +
                    c[X] * (double)( a[Y] - d[Y] ) +
                    d[X] * (double)( c[Y] - a[Y] );
        if ( (num == 0.0) || (num == denom) ) code = 'v';
        s = num / denom;

        num = -( a[X] * (double)( c[Y] - b[Y] ) +
                    b[X] * (double)( a[Y] - c[Y] ) +
                    c[X] * (double)( b[Y] - a[Y] ) );
        if ( (num == 0.0) || (num == denom) ) code = 'v';
        t = num / denom;

        if      ( (0.0 < s) && (s < 1.0) &&
                    (0.0 < t) && (t < 1.0) )
            code = '1';
        else if ( (0.0 > s) || (s > 1.0) ||
                    (0.0 > t) || (t > 1.0) )
            code = '0';

        p[X] = a[X] + s * ( b[X] - a[X] );
        p[Y] = a[Y] + s * ( b[Y] - a[Y] );

        return code;
    }
    char   ParallelInt( tPointi a, tPointi b, tPointi c, tPointi d, tPointd p, tPointd q )
    {
        if ( !Collinear( a, b, c) )
            return '0';

        if ( Between( a, b, c ) && Between( a, b, d ) ) {
            Assigndi( p, c );
            Assigndi( q, d );
            return 'e';
        }
        if ( Between( c, d, a ) && Between( c, d, b ) ) {
            Assigndi( p, a );
            Assigndi( q, b );
            return 'e';
        }
        if ( Between( a, b, c ) && Between( c, d, b ) ) {
            Assigndi( p, c );
            Assigndi( q, b );
            return 'e';
        }
        if ( Between( a, b, c ) && Between( c, d, a ) ) {
            Assigndi( p, c );
            Assigndi( q, a );
            return 'e';
        }
        if ( Between( a, b, d ) && Between( c, d, b ) ) {
            Assigndi( p, d );
            Assigndi( q, b );
            return 'e';
        }
        if ( Between( a, b, d ) && Between( c, d, a ) ) {
            Assigndi( p, d );
            Assigndi( q, a );
            return 'e';
        }
        return '0';
    }
    void    Assigndi( tPointd p, tPointi a )
    {
        int i;
        for ( i = 0; i < DIM; i++ )
            p[i] = a[i];
    }
    /** Returns TRUE iff point c lies on the closed segment ab.
        Assumes it is already known that abc are collinear. */
    bool    Between( tPointi a, tPointi b, tPointi c )
    {
        /* If ab not vertical, check betweenness on x; else on y. */
        if ( a[X] != b[X] )
            return ((a[X] <= c[X]) && (c[X] <= b[X])) ||
                    ((a[X] >= c[X]) && (c[X] >= b[X]));
        else
            return ((a[Y] <= c[Y]) && (c[Y] <= b[Y])) ||
                    ((a[Y] >= c[Y]) && (c[Y] >= b[Y]));
    }

    /** Returns the dot product of the two input vectors. */
    double  Dot( tPointi a, tPointi b )
    {
        int i;
        double sum = 0.0;

        for( i = 0; i < DIM; i++ )
        sum += a[i] * b[i];

        return  sum;
    }

    bool Collinear( tPointi a, tPointi b, tPointi c )
    {
            return  AreaSign( a, b, c ) == 0;
    }


};

} // End namespace GW


#endif // _GW_POLYGONINTERSECTOR_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
