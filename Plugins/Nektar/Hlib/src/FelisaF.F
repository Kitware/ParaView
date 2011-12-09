c*--------------------------------------------------------------------*
c*    [fgcurv] returns the 3D coordinates of a point on a Ferguson    *
c*    curve segment given the local coordinate 0 < u < nu-1 and the   *
c*    position and tangent vectors at the end points                  *
c*    ider = 0 ...... returns position vector R only                  *
c*           1 ...... also returns (R,u)                              *
c*           2 ...... also returns (R,uu)                             *
c*    nu   -- number of parametric points on curve                    *
c*    r    -- spline data in Ferguson format                          *
c*    un   -- local paramtric coordinate of point                     *
c*    rp   -- return vector of global coordinate                      *
c*    ier  -- error flag (0=successfull -1=failed)                    *
c*--------------------------------------------------------------------*
      subroutine fgcurv(ider,nu,r,un,rp,ier)
c
      parameter (NDIM =      3)
      implicit double precision (a-h,o-z)
      dimension r(2*NDIM,*), rp(*), r1(2*NDIM), r2(2*NDIM)
c
c *** out-of-bounds checking
c
      UBOT = 0.
      UTOP = nu-1
      ier = 0
      if(un.lt.UBOT.or.un.gt.UTOP) then
        ier = -1
        return
      endif
c
c *** finds the patch containing the point un,vn
c
      iu     = int(un)
      if(iu.eq.(nu-1)) iu = iu-1
      u      = iu
      u      = un-u 
      ip1    = iu+1
      ip2    = ip1+1
      do 100 id=1,2*NDIM
       r1(id) = r(id,ip1)
       r2(id) = r(id,ip2)
  100 continue
c
      do 1000 id=1,ndim
      id1    = id+ndim
      id2    = id1+ndim
      r12    = r2(id)-r1(id)
      a1     = r1(id)
      a2     = r1(id1)
      a3     =  3.*r12-2.*r1(id1)-r2(id1)
      a4     = -2.*r12+   r1(id1)+r2(id1)
      rp(id)  = a1+u*(a2+u*(a3+u*a4))
      if(ider.eq.0) goto 1000
      rp(id1) = a2+u*(2.*a3+3.*u*a4)
      if(ider.eq.1) goto 1000
      rp(id2) = 2.*a3+6.*u*a4
 1000 continue
c
      return
      end
c*--------------------------------------------------------------------*
c*    [fgsurf] returns the 3D coordinates of a point on a Ferguson    *
c*    partch given the global coordinates 0<un<nu-1; 0<vn<nv-1 and    *
c*    the vector r containing the values of (R,u); (R,v) and (R,uv)   *
c*    at the points defining the surface.                             *
c*    ider = 0 ...... returns position vector R only                  *
c*           1 ...... also returns (R,u); (R,v) and (R,uv)            *
c*           2 ...... also returns (R,uu) and (R,vv)                  *
c*    nu   -- number of u parametric points on surface (input)        *
c*    nv   -- number of v parametric points on surface (input)        *
c*    r    -- spline data in Furgeson format           (input)        *
c*    un   -- local u parametric coordinate of point   (input)        *
c*    vn   -- local v parametric coordinate of point   (input)        *
c*    rp   -- return vector of global coordinate                      *
c*    ier  -- error flag (0=successfull -1=failed)                    *
c*--------------------------------------------------------------------*
c
      subroutine fgsurf(ider,nu,nv,r,un,vn,rp,ier)
c
      implicit double precision (a-h,o-z)
      parameter (ndim =      3)
      parameter (nd4  = 4*ndim)
      dimension      r(nd4,*) ,rp(*)
      dimension      r1(nd4)  ,r2(nd4)  ,r3(nd4)  ,r4(nd4)
c
c *** expression of a Ferguson surface patch and its derivatives.
c     the vector r stores the values:  r, ru, rv, ruv, ruu, rvv
c
c                    (0,1)           (1,1)
c                      +---------------+
c                      | 3           4 |       ^ v
c                      |               |       |
c                      | 1           2 |       |
c                      +---------------+       +----> u
c                    (0,0)           (1,0)
c
c *** out-of-bounds checking
c
      UBOT = 0.
      VBOT = 0.
      UTOP = nu-1 
      VTOP = nv-1 
      ier = 0
      if(un.lt.UBOT.or.un.gt.UTOP.or.vn.lt.VBOT.or.vn.gt.VTOP) then
        ier = -1
c        stop 'Stopped in fgsurf'
      return
      endif
c
c *** finds the patch containing the point un,vn
c
      iu     = int(un)
      iv     = int(vn)
      if(iu.eq.(nu-1)) iu = iu-1
      if(iv.eq.(nv-1)) iv = iv-1
      u      = un-iu
      v      = vn-iv
      ip1    = iu+iv*nu+1
      ip2    = ip1+1
      ip3    = ip1+nu
      ip4    = ip3+1
      do 100 id=1,nd4
      r1(id) = r(id,ip1)
      r2(id) = r(id,ip2)
      r3(id) = r(id,ip3)
      r4(id) = r(id,ip4)
  100 continue
c
c *** computes the values of the position, tangent and
c     and twist vectors from the local coordinates 0<u,v<1 
c
      do 1000 id=1,ndim
      id1     = id +ndim
      id2     = id1+ndim
      id3     = id2+ndim
      id4     = id3+ndim
      id5     = id4+ndim
      q11     = r1(id)
      q12     = r3(id)
      q13     = r1(id2)
      q14     = r3(id2)
      q21     = r2(id)
      q22     = r4(id)
      q23     = r2(id2)
      q24     = r4(id2)
      q31     = r1(id1)
      q32     = r3(id1)
      q33     = r1(id3)
      q34     = r3(id3)
      q41     = r2(id1)
      q42     = r4(id1)
      q43     = r2(id3)
      q44     = r4(id3)
      s1      = -3.*q11+3.*q12-2.*q13-q14
      s2      = -3.*q21+3.*q22-2.*q23-q24
      s3      = -3.*q31+3.*q32-2.*q33-q34
      s4      = -3.*q41+3.*q42-2.*q43-q44
      t1      = 2.*q11-2.*q12+q13+q14
      t2      = 2.*q21-2.*q22+q23+q24
      t3      = 2.*q31-2.*q32+q33+q34
      t4      = 2.*q41-2.*q42+q43+q44
      a11     = q11
      a12     = q13
      a13     = s1
      a14     = t1
      a21     = q31
      a22     = q33
      a23     = s3
      a24     = t3
      a31     = -3.*q11+3.*q21-2.*q31-q41
      a32     = -3.*q13+3.*q23-2.*q33-q43
      a33     = -3.*s1 +3.*s2 -2.*s3 -s4
      a34     = -3.*t1 +3.*t2 -2.*t3 -t4
      a41     = 2.*q11-2.*q21+q31+q41
      a42     = 2.*q13-2.*q23+q33+q43
      a43     = 2.*s1 -2.*s2 +s3 +s4
      a44     = 2.*t1 -2.*t2 +t3 +t4
      s10     = a11+v*(a12+v*(a13+v*a14))
      s20     = a21+v*(a22+v*(a23+v*a24))
      s30     = a31+v*(a32+v*(a33+v*a34))
      s40     = a41+v*(a42+v*(a43+v*a44))
      rp(id)  = s10+u*(s20+u*(s30+u*s40))
      if(ider.eq.0) goto 1000
      rp(id1) = s20+u*(2.*s30+u*3.*s40)
      s1      = a12+v*(2.*a13+v*3.*a14)
      s2      = a22+v*(2.*a23+v*3.*a24)
      s3      = a32+v*(2.*a33+v*3.*a34)
      s4      = a42+v*(2.*a43+v*3.*a44)
      rp(id2) = s1+u*(s2+u*(s3+u*s4))
      rp(id3) = s2+u*(2.*s3+u*3.*s4)
      if(ider.eq.1) goto 1000
      rp(id4) = 2.*s30+u*6.*s40
      s1      = 2.*a13+v*6.*a14
      s2      = 2.*a23+v*6.*a24
      s3      = 2.*a33+v*6.*a34
      s4      = 2.*a43+v*6.*a44
      rp(id5) = s1+u*(s2+u*(s3+u*s4))
 1000 continue
c
      return
      end
c
      subroutine settolg(tol)
      implicit double precision (a-h,o-z)
      common  /GEOTOL/ tolg,spmin
      tolg = tol
      return
      end

c*--------------------------------------------------------------------*
c*                                                                    *
c*    [locuv] calculates the coordinates u,v in the parameter plane   *
c*    of a point on a Ferguson surface given by its 3D coordinates    *
c*                                                                    *
c*                        Modified MAR-93                             *
c*                                                                    *
c*    np   -- number of points to find in face                        *
c*    xg   -- global coordinate of points                             *
c*    xl   -- returned local coordaintes                              *
c*    nu   -- number of u parameters in surface                       *
c*    nv   -- number of v parameters in surface                       *
c*    r    -- data of Ferguson surface space                          *
c*                                                                    *
c*--------------------------------------------------------------------*
      subroutine locuv( np, xg, xl, nu, nv, r )
c
      implicit double precision (a-h,o-z)
      dimension      r(12,*), xg(3,*), xl(2,*)
      dimension        xp(3),   uk(2),  uk1(2),  box(4)
      dimension      us(2,4), up(2,4), xd(2,4), bl(2,4)
      dimension         w(4),  rp(12) 
      integer   kp(2,4), verbose
      logical    feasib,   newdr,    flag,      ok
      common /GEOTOL/ tolg,spmin
      common /MACPAR/ SEPSIL, SLARGE, SSMALL
      common /jp/ kb
c
c *** Determines machine parameters (LAPACK routines)
c
      SEPSIL = dlamch('E') ! Round-off Error
      SLARGE = dlamch('O') ! Overflow
      SSMALL = dlamch('U') ! Underflow
c
c *** Tolerances for the iterative process
c
c     tolg := distance in 3D coordinates
c     EPS3 := modulus of search direction
c
      EPS  = tolg 
      EPS2 = EPS**2
c
c *** verbose = 1 turns on warnings 
      verbose = 0
c
c *** Stores values along the boundary
c
      call bval( nu, nv, box, us, kp, up, xd, bl )
c
c *** Loop over the points on the curve
c
      do ip=1,np
c
c *** Target point
c
c         if(verbose .eq. 1) then 
c            if( (ip/20)*20 .eq. ip ) then 
c               write(*,'(i5)') ip
c            else
c               write(*,'(i5,$)') ip
c            endif
c         endif
         xp(1) = xg(1,ip)
         xp(2) = xg(2,ip)
         xp(3) = xg(3,ip)
c
c *** First, checks for singular points.
c
        rp(1) = 0.0
        rp(2) = 0.0
        rp(3) = 0.0
        call chsing( nu, nv, r, xp, us, uk, rp, EPS, drm, ok )
        if( ok ) goto 100
c
c *** Finds the minimum along the boundary
c
        drm = SLARGE 
        kb  = 0
        do j = 1,4
          dst = ( r(1,kp(1,j)) - r(1,kp(2,j)) )**2+
     &          ( r(2,kp(1,j)) - r(2,kp(2,j)) )**2+
     &          ( r(3,kp(1,j)) - r(3,kp(2,j)) )**2
	  if( dst .gt. EPS2 ) then
	    call localm(     nu,      nv,       r,      xp, 
     &                  up(1,j), xd(1,j), bl(1,j), bl(2,j), 
     &                       al,      rp,     dr2)
            if( dr2 .lt. drm ) then
              drm   = dr2
              uk(1) = up(1,j)+al*xd(1,j)
              uk(2) = up(2,j)+al*xd(2,j)
              kb    = j    
            endif
            if( drm .lt. EPS ) then 
              goto 100
            endif
          endif
        enddo
c
c *** If the minimum has not been found on the boundary, the point 
c     with minimum distance is used as the initial guess for a 
c     conjugate gradient minimization process. 
c
        call cgmin( nu, nv, r, xp, uk, kp, up, xd, bl, box, 
     &              rp, EPS, drm, ok )
        if( ok ) then
          goto 100
        else
           if(verbose .eq. 1) then 
              write(*,'(/,a)') ' LOCUV > CG Iteration Failed'
              call info( ip, xp, rp, uk )
           endif
        endif
c
c *** Here tries a more robust but slower algorithm 
c     when at a boundary point the line search has taken the
c     iterative procedure out-of-bounds
c
        drm1   = drm
        uk1(1) = uk(1)
        uk1(2) = uk(2)
c
        call locu2( nu, nv, r, xp, uk, rp, EPS, drm, ok )
        if( ok ) then
           if(verbose .eq. 1) then 
              write(*,'(/,a)') ' LOCUV > Succesful 2nd Attempt'
              call info( ip, xp, rp, uk )
           endif
           goto 100
        else
           if(verbose .eq. 1) then 
              write(*,'(/,a)') ' LOCUV > Brute Force Iteration Failed'
              call info( ip, xp, rp, uk )
           endif
        endif
c
c *** Compare both methods and retain the best
c
        if ( drm .gt. drm1 ) then
          drm   = drm1
          uk(1) = uk1(1)
          uk(2) = uk1(2)
        endif
c
  100   continue
        if( drm .gt. EPS ) then 
           if(verbose .eq. 1) then 
              
              write(*,'(/,a,//,2(a,/,a,1pe12.5,/),2(a,/))') 
     &       ' LOCUV >        !!! *** WARNING *** !!!           ',
     &       '        The distance between the target point and ',
     &       '        and the last point in the iteration is:   ',drm,
     &       '        This is BIGGER than the tolerance used by ',
     &       '        SURFACE for coincident points which is:   ',tolg,
     &       '        The run continues but you are advised to  ',
     &       '        check your input geometry data for errors '
              endif
        endif
        xl(1,ip) = uk(1)
        xl(2,ip) = uk(2)
      enddo
c
c      write(*,'(/)')
c
      return
      end

c*---------------------------------------------------------------*
c*                                                               *
c*    [info] Displays a status message for the last iteration    *
c*                                                               *
c*---------------------------------------------------------------*
      subroutine info( ip, xp, rp, uk ) 
      implicit double precision (a-h,o-z)
      dimension xp(*), rp(*), uk(*)
      drm = dist3D( xp, rp )
      write(*,'( a,i5,/,2(a,3(1x,1pe12.5),/),
     &           a,2(1x,1pe12.5),/,a,1x,1pe12.5,/ )')
     &      '         Target point .. ',ip,
     &      '         Coordinates ... ',(xp(i),i=1,3),
     &      '         Last .......... ',(rp(i),i=1,3),
     &      '         Local ......... ',(uk(i),i=1,2),
     &      '         Distance ...... ',drm
      return
      end
c*---------------------------------------------------------------*
c*                                                               *
c*    [dist3D] Calculates the 3D distance between two points     *
c*                                                               *
c*---------------------------------------------------------------*
      double precision function dist3D( x1, x2 )
      implicit double precision (a-h,o-z)
      dimension x1(*), x2(*)
      dist3D = (x1(1)-x2(1))**2+(x1(2)-x2(2))**2+(x1(3)-x2(3))**2
      dist3D = sqrt(dist3D)
      return
      end
c*---------------------------------------------------------------*
c*                                                               *
c*    [dist2D] Calculates the 2D distance between two points     *
c*                                                               *
c*---------------------------------------------------------------*
      double precision function dist2D( x1, x2 )
      implicit double precision (a-h,o-z)
      dimension x1(*), x2(*)
      dist2D = (x1(1)-x2(1))**2+(x1(2)-x2(2))**2
      dist2D = sqrt(dist2D)
      return
      end
c*---------------------------------------------------------------*
c*                                                               *
c*    [chsing] Checks the target coordinates "xp" against a set  *
c*             of four candidate singular points whose local     *
c*             coordinates are stored in us(2,4).                * 
c*                                                               *
c*---------------------------------------------------------------*
      subroutine chsing( nu, nv, r, xp, us, uk, rp, EPS, drm, ok )
c
      implicit double precision (a-h,o-z)
      dimension  r(12,*), xp(*), us(2,*) 
      dimension  uk(*), rp(*) 
      logical ok
c
      ok = .false.
c
      do j = 1,4
        call fgsurf( 0, nu, nv, r, us(1,j), us(2,j), rp, ier )
        drm = dist3D(xp,rp)
        if( drm .lt. EPS ) then
          uk(1) = us(1,j)
          uk(2) = us(2,j)
          ok    = .true.
          return 
        endif
      enddo
c
      return
      end
c*---------------------------------------------------------------*
c*                                                               *
c*    [cgmin] Obtains the closest point "rp" on the surface to   *
c*            the target point "xp" via minimization using a     *
c*            Conjugate Gradient algorithm.                      *
c*                                                               *
c*---------------------------------------------------------------*
      subroutine cgmin( nu, nv, r, xp, uk, kp, up, xd, bl, box, 
     &                  rp, EPS, drm, ok )
c
      implicit double precision (a-h,o-z)
      parameter ( NIT =    200 )
      dimension      r(12,*),   xp(*),   uk(*),   rp(*)
      dimension      up(2,*), xd(2,*), bl(2,*),  box(*)
      integer   kp(2,*)
c
      dimension        gk(2),  gk1(2),   dr(2),    w(4)
c
      logical    feasib,   newdr,    flag,      ok
c
      common /MACPAR/ SEPSIL, SLARGE, SSMALL
      common /jp/ kb
c
c *** Tolerances set for algorithm termination
c
c     EPS3 := Perpendicularity
c     EPS4 := parallel directions ( 6 deg )
c
      TOL  = 10.0*SEPSIL
      EPS3 = SEPSIL
c     EPS3 = 1.e-20
      EPS4 = 0.10
c
c *** Success flag
c
      ok = .false.
c
c *** First we check that the first search direction does not 
c     coincide with the current one on the
c     boundary. If it does we select an alternative first guess.
c
      call fgsurf(1,nu,nv,r,uk(1),uk(2),rp,ier)
      drx = rp(1)-xp(1)
      dry = rp(2)-xp(2)
      drz = rp(3)-xp(3)
      drm = drx**2+dry**2+drz**2
      drm = sqrt(drm)
      if( drm .lt. EPS ) then
        ok = .true.
        return 
      endif
c
c *** Is the vector perpendicular to the surface ?   
c
      gk(1) = 2.*( drx*rp(4)+dry*rp(5)+drz*rp(6) )
      gk(2) = 2.*( drx*rp(7)+dry*rp(8)+drz*rp(9) )
      vmd   = sqrt(gk(1)**2+gk(2)**2)
      if(vmd .lt. EPS3) then
        ok = .true.
        return 
      endif
      vmd   = 1./vmd
      dr(1) = -gk(1)*vmd
      dr(2) = -gk(2)*vmd
c
c *** If the direction is not valid uses a different initial
c     guess: the closest patch center point
c
      vsc   = abs( -dr(1)*xd(2,kb)+dr(2)*xd(1,kb) )
      if( vsc .lt. EPS4 ) then
        call fguess( nu, nv, r, xp, uk )
        call fgsurf(1,nu,nv,r,uk(1),uk(2),rp,ier)
        drx = rp(1)-xp(1)
        dry = rp(2)-xp(2)
        drz = rp(3)-xp(3)
        drm = drx**2+dry**2+drz**2
        drm = sqrt(drm)
        if( drm .lt. EPS ) then
          ok = .true.
          return 
        endif
        gk(1) = 2.*( drx*rp(4)+dry*rp(5)+drz*rp(6) )
        gk(2) = 2.*( drx*rp(7)+dry*rp(8)+drz*rp(9) )
        dr(1) = -gk(1)
        dr(2) = -gk(2)
      endif
c
c *** iteration for finding the local coordinates
c
      do it=1,NIT
        dmd   = sqrt(dr(1)**2+dr(2)**2)
        if(dmd .lt. EPS3) then
          ok = .true.
          return 
        endif
        dmd   = 1./dmd
        dr(1) = dr(1)*dmd
        dr(2) = dr(2)*dmd
c
c *** Is the current direction valid ?
c
        flag = feasib( uk, dr, box, TOL )
        if( .not. flag ) then
           flag = newdr( uk, gk, dr, box, TOL )
           if( .not. flag ) then
             ok = .false.
             return
           endif
        endif
c
c *** Max. values of the line parameter in the (du,dv) direction
c
        u1   = 1./sign( max(abs(dr(1)),EPS3) , dr(1) )
        v1   = 1./sign( max(abs(dr(2)),EPS3) , dr(2) )
        w(1) = (box(3)-uk(1))*u1
        w(2) = (box(1)-uk(1))*u1
        w(3) = (box(4)-uk(2))*v1
        w(4) = (box(2)-uk(2))*v1
        call sort( 4, w )
        ax   = 0.
        bx   = w(3)
        if( abs(bx) .lt. TOL ) bx = w(4)
c
c *** Brent's procedure localm for line search
c
        call localm(nu, nv, r, xp, uk, dr,
     &              ax, bx, al, rp, drm  )
        uk(1) = uk(1)+al*dr(1)
        uk(2) = uk(2)+al*dr(2)
        if( drm .lt. EPS ) then
          ok  = .true.
          return 
        endif
        call fgsurf(1, nu, nv, r, uk(1), uk(2), rp, ier)
        drx   = rp(1)-xp(1)
        dry   = rp(2)-xp(2)
        drz   = rp(3)-xp(3)
        gk1(1) = 2.*( drx*rp(4)+dry*rp(5)+drz*rp(6) )
        gk1(2) = 2.*( drx*rp(7)+dry*rp(8)+drz*rp(9) )
        if( mod(it,2) .eq. 0 ) then
          dr(1) = -gk1(1)
          dr(2) = -gk1(2)
        else
          bet   = (gk1(1)-gk(1))*gk1(1)+
     &            (gk1(2)-gk(2))*gk1(2)        ! Polak-Riviere
c         bet   = (gk1(1)**2+gk1(2)**2)        ! Fletcher-Reeves
          bet   = bet*vmd
          dr(1) = -gk1(1)+bet*dr(1)
          dr(2) = -gk1(2)+bet*dr(2)
        endif
        gk(1) = gk1(1)
        gk(2) = gk1(2)
        vmd   = sqrt(gk(1)**2+gk(2)**2)
        if(vmd .lt. EPS3) then
          drm = sqrt(drm)
          ok  = .true.
          return 
        endif
        vmd = 1./vmd
      enddo
c
      return
      end
c*---------------------------------------------------------------*
c*                                                               *
c*    [sort] Selection sort. Adapted from: R. Sedgewick (1988),  *
c*           "Algorithms".                                       *
c*                                                               *
c*---------------------------------------------------------------*
      subroutine sort(n, a)
      implicit double precision (a-h,o-z)
      dimension  a(*)
      do i = 1,n-1
        k = i
        do j = i+1,n
          if( a(j) .lt. a(k) ) k = j
        enddo
        tmp  = a(k)
        a(k) = a(i)
        a(i) = tmp
      enddo
      return
      end
c*-------------------------------------------------------------*
c*                                                             *
c*    [localm] Carries out the line search minimization from   *
c*             the point u0(1:2) along the direction dr(1:2)   *
c*             restricted to the interval [a,b]. eps is the    *
c*             tolerance for finishing the iteration (of the   *
c*             order of the square root of the computer        *
c*             precission and t is a very small number         *
c*             Adapted from: R. P. Brent (1973), "Algorithms   *
c*             for minimization without derivatives".          *
c*                                                             *
c*-------------------------------------------------------------*
c* Original Notes from the "Golden Oldies" NETLIB directory    *
c*-------------------------------------------------------------*
c*                                                             *
c* SFMIN gives an approximation "x" to the point where the     *
c*       function attains a minimum on the interval (x,b)      * 
c*                                                             *
c* Input:                                                      *
c* a     left endpoint of initial interval                     *
c* b     right endpoint of initial interval                    *
c* tol   desired length of the interval of uncertainty of the  *
c*       final result (.ge.0.)                                 *
c*                                                             *
c* Output:                                                     *
c* x     abcissa approximating the point where the function    *
c*       attains a minimum                                     *
c*                                                             *
c* The method used is a combination of golden section search   *
c* and successive parabolic interpolation. Its convergence is  *
c* never much slower than that for a Fibonacci search. If the  *
c* function has a continuous second derivative which is        *
c* positive at the minimum (which is not at "a" or "b"), then  *
c* convergence  is  superlinear, and usually of the order of   *
c* about  1.324.... The function is never evaluated at two     *
c* points closer together than eps*abs(sfmin)+(tol/3), where   *
c* eps is approximately the square root of the relative        *
c* machine precision. If f is a unimodal function and the      *
c* computed values of f are always unimodal when separated by  *
c* at least eps*abs(x)+(tol/3), then sfmin approximates the    *
c* abcissa of the global minimum of f on the interval (a,b)    *
c* with an error less than 3*eps*abs(sfmin)+tol. If f is not   *
c* unimodal, then sfmin may approximate a local, but perhaps   *
c* non-global, minimum to the same accuracy.                   *
c* This function subprogram is a slightly modified version of  *
c* the algol 60 procedure localmin given in Richard Brent,     *
c* "Algorithms for minimization without derivatives", (1973),  *
c* Prentice-Hall, Inc.                                         *
c*                                                             *
c*-------------------------------------------------------------*
      subroutine localm(nu, nv, rs, xp, u0, dr, a, b, x, rp, fx )
      implicit double precision (a-h,o-z)
      dimension     rs(12,*), u0(*), dr(*), xp(*), rp(*)
c
      common /MACPAR/ SEPSIL, SLARGE, SSMALL
c
c *** GOLD is the squared inverse of the golden ratio
c
      GOLD = 0.5*(3.0-sqrt(5.0)) 
c
c *** eps is approximately the square root of the relative machine
c     precision.
c
      tol1 = 1.0+SEPSIL
      tol2 = SEPSIL/1000.
      eps  = sqrt(SEPSIL)*1.0E-03
c
      sa   = a
      sb   = b
      x    = sa+GOLD*(sb-sa)
      w    = x
      v    = w
      e    = 0.0
      uc   = u0(1)+x*dr(1)
      vc   = u0(2)+x*dr(2)
      call fgsurf(0, nu, nv, rs, uc, vc, rp, ier)
      fx   = dist3D(xp,rp)
      fw   = fx
      fv   = fx
      tol3 = tol2/3.0
c
c *** Main loop
c
   10 continue
      xm   = 0.5*(sa+sb)
      tol1 = eps*abs(x)+tol3
      t2   = 2.0*tol1
c
c *** Check stopping criterion
c
      if( abs(x-xm) .le. t2-0.5*(sb-sa) )           goto 190
      r    = 0.0
      q    = 0.0
      p    = 0.0
      if( abs(e) .le. tol1 )                        goto  40
c
c *** Fit a parabola
c
      r    = (x-w)*(fx-fv)
      q    = (x-v)*(fx-fw)
      p    = (x-v)*q-(x-w)*r
      q    = 2.0*(q-r)
      if( q .le. 0.0 )                              goto  20
      p    = -p
                                                    goto  30
   20 q    = -q
   30 r    = e 
      e    = d
   40 if( abs(p) .ge. abs(0.5*q*r) .or. 
     &         p .le. q*(sa-x)     .or.  
     &         p .ge. q*(sb-x)         )            goto  60
c
c *** A parabolic-interpolation step
c
      d    = p/q
      u    = x+d
c
c *** f must not be evaluated too close to sa or sb
c
      if( u-sa .ge. t2 .and. sb-u .ge. t2 )         goto  90
      d    = tol1
      if( x .ge. xm ) d  = -d
                                                    goto  90
c
c *** A golden-section step
c
   60 if( x .ge. xm )                               goto  70
      e   = sb-x
                                                    goto  80
   70 e   = sa-x
   80 d   = GOLD*e
c
c *** f must not be evaluated too close to x
c
   90 if( abs(d) .lt. tol1 )                        goto 100
      u    = x+d
                                                    goto 120
  100 if( d .le. 0.0 )                              goto 110 
      u    = x+tol1
                                                    goto 120
  110 u    = x-tol1
  120 continue
      uc   = u0(1)+u*dr(1)
      vc   = u0(2)+u*dr(2)
      call fgsurf(0, nu, nv, rs, uc, vc, rp, ier)
      fu   = dist3D(xp,rp)
c
c *** Update  a, b, v, w, and x
c
      if( fx .gt. fu )                              goto 140
      if( u .ge. x )                                goto 130
      sa   = u
                                                    goto 140
  130 sb   = u
  140 if( fu .gt. fx )                              goto 170 
      if( u .ge. x )                                goto 150 
      sb   = x
                                                    goto 160
  150 sa   = x 
  160 v    = w
      fv   = fw
      w    = x
      x    = u
      fx   = fu
                                                    goto  10
  170 if( fu .gt. fw .and. w .ne. x )               goto 180
      v    = w
      fv   = fw
      w    = u
      fw   = fu
                                                    goto  10
  180 if( fu .gt. fv .and. v.ne.x .and. v.ne.w )    goto  10
      v    = u
      fv   = fu
                                                    goto  10      
  190 return
c
      end
c*---------------------------------------------------------------------*
c*                                                                     *
c*    [feasib] checks whether a direction of advance is admissible or  *
c*             not. The criterion used is that a direction is valid    *
c*             if advancing from (u0,v0) a distance "tol" along it     *
c*             the resulting point (u,v) lies within the bounds of     *
c*             the parameter plane, i.e. [0,nu-1]x[0,nv-1]             *
c*                                                                     *
c*---------------------------------------------------------------------*
      logical function feasib( uk, dr, box, tol )
      implicit double precision (a-h,o-z)
      dimension     uk(*), dr(*), box(*)
      dimension     up(2), dn(2)
c
c *** Bounds of the parameter plane
c
      UBOT = box(1) 
      VBOT = box(2)
      UTOP = box(3)
      VTOP = box(4)
c
c *** Normalize the direction
c
      vmd = 1./sqrt( dr(1)**2+dr(2)**2 )
      dn(1) = dr(1)*vmd
      dn(2) = dr(2)*vmd
c
c *** New position
c
      up(1) = uk(1)+tol*dn(1)      
      up(2) = uk(2)+tol*dn(2)      
c
c *** Still in the parameter plane ? 
c
      if( up(1) .ge. UBOT .and. up(1) .le. UTOP .and. 
     &    up(2) .ge. VBOT .and. up(2) .le. VTOP       ) then
        feasib = .true.
      else
        feasib = .false. 
      endif
c
      return
      end
c*--------------------------------------------------------*
c*                                                        *
c*    [newdr] finds an alternative feasible direction or  *
c*            returns a .false. if this is not possible   *
c*                                                        *
c*--------------------------------------------------------*
      logical function newdr( uk, gk, dr, box, tol )
      implicit double precision (a-h,o-z)
      dimension       uk(*), dr(*), gk(*), box(*)
      logical    flag, feasib
c
      common /MACPAR/ SEPSIL, SLARGE, SSMALL      
c
      newdr = .true.
      EPS   = SEPSIL
c
c *** Bounds of the parameter plane
c
      UBOT = box(1) 
      VBOT = box(2)
      UTOP = box(3)
      VTOP = box(4)
c
c *** Checks the active constraints
c
      dub = abs(uk(1)-UBOT)
      dut = abs(uk(1)-UTOP)
      dvb = abs(uk(2)-VBOT)
      dvt = abs(uk(2)-VTOP)
c
c *** Projects the descent direction ( opposite to the gradient )
c     along the active boundary and verifies whether the new 
c     direction is acceptable
c
      if( dub .le. tol .or. dut .le. tol ) then
        if( abs(gk(1)) .gt. EPS ) then 
          dr(1) = sign(1.D00,-gk(1))
          dr(2) = 0.
          flag  = feasib( uk, dr, box, tol ) 
          if( flag ) return
        endif
      endif
c
      if( dvb .le. tol .or. dvt .le. tol ) then
        if( abs(gk(2)) .gt. EPS ) then 
          dr(1) = 0.
          dr(2) = sign(1.D00,-gk(2))
          flag  = feasib( uk, dr, box, tol ) 
          if( flag ) return
        endif
      endif
c
c *** There is no feasible direction along the boundary 
c
      newdr = .false.
c
      return
      end
c*--------------------------------------------------------*
c*                                                        * 
c*    [bval] calculates some values along the boundaries  *
c*                                                        * 
c*--------------------------------------------------------*
      subroutine bval( nu, nv, box, us, kp, up, xd, bl )
c
      implicit double precision (a-h,o-z)
      dimension      us(2,4), up(2,4), xd(2,4), bl(2,4)
      dimension       box(4)
      integer   kp(2,4)
c
c *** Notation
c                    4         3
c                     +---<---+
c                     |       |
c                     v       ^
c                     |       |
c                     +--->---+
c                    1         2
c
      ipt1    = 1
      ipt2    = nu
      ipt3    = nu*nv
      ipt4    = (nv-1)*nu+1
      UBOT    = 0.0
      VBOT    = 0.0
      UTOP    = nu-1
      VTOP    = nv-1
      box(1)  = UBOT
      box(2)  = VBOT
      box(3)  = UTOP
      box(4)  = VTOP
c
c *** v = VBOT
c
      us(1,1) = 0.5*(UBOT+UTOP)
      us(2,1) = VBOT 
      kp(1,1) = ipt1
      kp(2,1) = ipt2
      up(1,1) = UBOT 
      up(2,1) = VBOT 
      xd(1,1) =  1.0 
      xd(2,1) =  0.0 
      bl(1,1) =  0.
      bl(2,1) = UTOP
c
c *** u = UTOP
c
      us(1,2) = UTOP
      us(2,2) = 0.5*(VBOT+VTOP)
      kp(1,2) = ipt2
      kp(2,2) = ipt3
      up(1,2) = UTOP 
      up(2,2) = VBOT 
      xd(1,2) =  0.0 
      xd(2,2) =  1.0 
      bl(1,2) =  0.
      bl(2,2) = VTOP
c
c *** v = VTOP
c
      us(1,3) = 0.5*(UBOT+UTOP)
      us(2,3) = VTOP
      kp(1,3) = ipt3
      kp(2,3) = ipt4
      up(1,3) = UTOP 
      up(2,3) = VTOP 
      xd(1,3) = -1.0 
      xd(2,3) =  0.0 
      bl(1,3) =  0. 
      bl(2,3) = UTOP
c
c *** u = UBOT
c
      us(1,4) = UBOT
      us(2,4) = 0.5*(VBOT+VTOP)
      kp(1,4) = ipt4
      kp(2,4) = ipt1
      up(1,4) = UBOT 
      up(2,4) = VTOP 
      xd(1,4) =  0. 
      xd(2,4) = -1. 
      bl(1,4) = 0. 
      bl(2,4) = VTOP
c
      return
      end
c*----------------------------------------*
c*                                        *
c*    [fguess] provides an initial guess  *
c*                                        *
c*----------------------------------------*
      subroutine fguess( nu, nv, r, xp, uk )
      implicit double precision (a-h,o-z)
      dimension   r(12,*), uk(*), xp(*)
      dimension   rp(3)
c
      common /MACPAR/ SEPSIL, SLARGE, SSMALL
c
c *** initial guess: parametric coordinates (u,v)
c     of the closest support point
c
      drm = SLARGE 
      do iu = 1,nu-1
        do iv = 1,nv-1
          i1 = (iv-1)*nu+iu 
          i2 = i1+1
          i3 = i1+nu
          i4 = i3+1
          rp(1) = 0.25*(r(1,i1)+r(1,i2)+r(1,i3)+r(1,i4))
          rp(2) = 0.25*(r(2,i1)+r(2,i2)+r(2,i3)+r(2,i4))
          rp(3) = 0.25*(r(3,i1)+r(3,i2)+r(3,i3)+r(3,i4))
          dr2   = (rp(1)-xp(1))**2+
     &            (rp(2)-xp(2))**2+
     &            (rp(3)-xp(3))**2
          if( dr2 .lt. drm ) then
            drm   = dr2
            uk(1) = iu-0.5
            uk(2) = iv-0.5
          endif
       enddo
      enddo
c
      return
      end
c*--------------------------------------------------------------------*
c*                                                                    *
c*    [locu2] calculates the coordinates u,v in the parameter plane   *
c*    of a point on a Ferguson surface given by its 3D coordinates    *
c*    using a selective splitting in u and v of the surface           *
c*                                                                    *
c*--------------------------------------------------------------------*
      subroutine locu2( nu, nv, r, xp, xl, rp, EPS3D, dis, ok )
c
      implicit double precision (a-h,o-z)
      parameter ( MBND = 5      )
      parameter ( NIT  = 1000   )
c
      dimension xp(*), xl(*), r(12,*), rp(*)
      dimension xuvt(2,(2*MBND+1)*(2*MBND+1))
c
      logical ok
c
      common /MACPAR/ SEPSIL, SLARGE, SSMALL
c
      EPS2D = 1.e-10
      EPS   = 1.e-10
c
      ok = .false.
c
c *** Finds the closest support point
c
      dis = SLARGE 
      au  = 0.0
      do iu = 1,nu 
        av = 0.0
        do iv = 1,nv
          call fgsurf( 0, nu, nv, r, au, av, rp, ier)
          dis1 = dist3D(rp,xp)
          if(dis1.lt.dis) then 
            aur = au
            avr = av
            dis = dis1
            if(dis.lt.EPS3D) then
              ok = .true.
              goto 100 
            endif
          endif
          av = av + 1.0
        enddo
        au = au + 1.0
      enddo
c
c *** Recursive subdivision
c
      au    = au - 1.0 + EPS
      av    = av - 1.0 + EPS
      du    = 1.0
      iter  = 0
  200 iter  = iter + 1
      du    = 0.5*du
      nbnd  = 3
      iter1 = 0
  300 iter1 = iter1 + 1 
      disk  = dis
      auk   = aur
      avk   = avr
      ntr   = 0 
      uc    = aur - du*nbnd
      do iu = 1,2*nbnd+1
        vc = avr - du*nbnd
        do iv = 1,2*nbnd+1
          if(min(uc,vc).gt.-EPS .and. uc.lt.au .and. vc.lt.av) then
            ntr = ntr + 1
            xuvt(1,ntr) = uc
            xuvt(2,ntr) = vc
          endif
          vc = vc + du
        enddo 
        uc = uc + du
      enddo
c
c *** Checks the selected points
c
      do it = 1,ntr
        call fgsurf( 0, nu, nv, r, xuvt(1,it), xuvt(2,it), rp, ier)
        dis1 = dist3D(rp,xp)
        if(dis1.lt.dis) then 
          aur = xuvt(1,it) 
          avr = xuvt(2,it) 
          dis = dis1
          if(dis.lt.EPS3D) then
            ok = .true.
            goto 100 
          endif
        endif
      enddo
      if(aur .lt. auk-du*nbnd+EPS .or.
     &   aur .gt. auk+du*nbnd-EPS .or.
     &   avr .lt. avk-du*nbnd+EPS .or.
     &   avr .gt. avk+du*nbnd-EPS )       goto 300 
      if(iter.gt.NIT) then 
        ok = .false.
        return
      endif
      if( dis .gt. disk ) then
        ok = .false.
        return
      else
        disk = dis
      endif
      if(du.gt.EPS2D)                     goto 200
  100 continue
      xl(1) = aur
      xl(2) = avr
c
      return
      end
c*---------------------------------------------------------------*
c*                                                               *
c*    [sign] Routine needed for alpha's since not supported      *
c*                                                               *
c*---------------------------------------------------------------*
      double precision function sign( x1, x2 )
      implicit double precision (a-h,o-z)  
      if(x2.lt.0.0) then
         sign  = -x1
      else
         sign = x1
      endif
      return
      end

#if !defined(NO_DLAMCH) && !defined(__APPLE__)
      DOUBLE PRECISION FUNCTION DLAMCH( CMACH )
*
*  -- LAPACK auxiliary routine (version 2.0) --
*     Univ. of Tennessee, Univ. of California Berkeley, NAG Ltd.,
*     Courant Institute, Argonne National Lab, and Rice University
*     October 31, 1992
*
*     .. Scalar Arguments ..
      CHARACTER          CMACH
*     ..
*
*  Purpose
*  =======
*
*  DLAMCH determines double precision machine parameters.
*
*  Arguments
*  =========
*
*  CMACH   (input) CHARACTER*1
*          Specifies the value to be returned by DLAMCH:
*          = 'E' or 'e',   DLAMCH := eps
*          = 'S' or 's ,   DLAMCH := sfmin
*          = 'B' or 'b',   DLAMCH := base
*          = 'P' or 'p',   DLAMCH := eps*base
*          = 'N' or 'n',   DLAMCH := t
*          = 'R' or 'r',   DLAMCH := rnd
*          = 'M' or 'm',   DLAMCH := emin
*          = 'U' or 'u',   DLAMCH := rmin
*          = 'L' or 'l',   DLAMCH := emax
*          = 'O' or 'o',   DLAMCH := rmax
*
*          where
*
*          eps   = relative machine precision
*          sfmin = safe minimum, such that 1/sfmin does not overflow
*          base  = base of the machine
*          prec  = eps*base
*          t     = number of (base) digits in the mantissa
*          rnd   = 1.0 when rounding occurs in addition, 0.0 otherwise
*          emin  = minimum exponent before (gradual) underflow
*          rmin  = underflow threshold - base**(emin-1)
*          emax  = largest exponent before overflow
*          rmax  = overflow threshold  - (base**emax)*(1-eps)
*
* =====================================================================
*
*     .. Parameters ..
      DOUBLE PRECISION   ONE, ZERO
      PARAMETER          ( ONE = 1.0D+0, ZERO = 0.0D+0 )
*     ..
*     .. Local Scalars ..
      LOGICAL            FIRST, LRND
      INTEGER            BETA, IMAX, IMIN, IT
      DOUBLE PRECISION   BASE, EMAX, EMIN, EPS, PREC, RMACH, RMAX, RMIN,
     $                   RND, SFMIN, SMALL, T
*     ..
*     .. External Functions ..
      LOGICAL            LSAME
      EXTERNAL           LSAME
*     ..
*     .. External Subroutines ..
      EXTERNAL           DLAMC2
*     ..
*     .. Save statement ..
      SAVE               FIRST, EPS, SFMIN, BASE, T, RND, EMIN, RMIN,
     $                   EMAX, RMAX, PREC
*     ..
*     .. Data statements ..
      DATA               FIRST / .TRUE. /
*     ..
*     .. Executable Statements ..
*
      IF( FIRST ) THEN
         FIRST = .FALSE.
         CALL DLAMC2( BETA, IT, LRND, EPS, IMIN, RMIN, IMAX, RMAX )
         BASE = BETA
         T = IT
         IF( LRND ) THEN
            RND = ONE
            EPS = ( BASE**( 1-IT ) ) / 2
         ELSE
            RND = ZERO
            EPS = BASE**( 1-IT )
         END IF
         PREC = EPS*BASE
         EMIN = IMIN
         EMAX = IMAX
         SFMIN = RMIN
         SMALL = ONE / RMAX
         IF( SMALL.GE.SFMIN ) THEN
*
*           Use SMALL plus a bit, to avoid the possibility of rounding
*           causing overflow when computing  1/sfmin.
*
            SFMIN = SMALL*( ONE+EPS )
         END IF
      END IF
*
      IF( LSAME( CMACH, 'E' ) ) THEN
         RMACH = EPS
      ELSE IF( LSAME( CMACH, 'S' ) ) THEN
         RMACH = SFMIN
      ELSE IF( LSAME( CMACH, 'B' ) ) THEN
         RMACH = BASE
      ELSE IF( LSAME( CMACH, 'P' ) ) THEN
         RMACH = PREC
      ELSE IF( LSAME( CMACH, 'N' ) ) THEN
         RMACH = T
      ELSE IF( LSAME( CMACH, 'R' ) ) THEN
         RMACH = RND
      ELSE IF( LSAME( CMACH, 'M' ) ) THEN
         RMACH = EMIN
      ELSE IF( LSAME( CMACH, 'U' ) ) THEN
         RMACH = RMIN
      ELSE IF( LSAME( CMACH, 'L' ) ) THEN
         RMACH = EMAX
      ELSE IF( LSAME( CMACH, 'O' ) ) THEN
         RMACH = RMAX
      END IF
*
      DLAMCH = RMACH
      RETURN
*
*     End of DLAMCH
*
      END
*
************************************************************************
*
      SUBROUTINE DLAMC1( BETA, T, RND, IEEE1 )
*
*  -- LAPACK auxiliary routine (version 2.0) --
*     Univ. of Tennessee, Univ. of California Berkeley, NAG Ltd.,
*     Courant Institute, Argonne National Lab, and Rice University
*     October 31, 1992
*
*     .. Scalar Arguments ..
      LOGICAL            IEEE1, RND
      INTEGER            BETA, T
*     ..
*
*  Purpose
*  =======
*
*  DLAMC1 determines the machine parameters given by BETA, T, RND, and
*  IEEE1.
*
*  Arguments
*  =========
*
*  BETA    (output) INTEGER
*          The base of the machine.
*
*  T       (output) INTEGER
*          The number of ( BETA ) digits in the mantissa.
*
*  RND     (output) LOGICAL
*          Specifies whether proper rounding  ( RND = .TRUE. )  or
*          chopping  ( RND = .FALSE. )  occurs in addition. This may not
*          be a reliable guide to the way in which the machine performs
*          its arithmetic.
*
*  IEEE1   (output) LOGICAL
*          Specifies whether rounding appears to be done in the IEEE
*          'round to nearest' style.
*
*  Further Details
*  ===============
*
*  The routine is based on the routine  ENVRON  by Malcolm and
*  incorporates suggestions by Gentleman and Marovich. See
*
*     Malcolm M. A. (1972) Algorithms to reveal properties of
*        floating-point arithmetic. Comms. of the ACM, 15, 949-951.
*
*     Gentleman W. M. and Marovich S. B. (1974) More on algorithms
*        that reveal properties of floating point arithmetic units.
*        Comms. of the ACM, 17, 276-277.
*
* =====================================================================
*
*     .. Local Scalars ..
      LOGICAL            FIRST, LIEEE1, LRND
      INTEGER            LBETA, LT
      DOUBLE PRECISION   A, B, C, F, ONE, QTR, SAVEC, T1, T2
*     ..
*     .. External Functions ..
      DOUBLE PRECISION   DLAMC3
      EXTERNAL           DLAMC3
*     ..
*     .. Save statement ..
      SAVE               FIRST, LIEEE1, LBETA, LRND, LT
*     ..
*     .. Data statements ..
      DATA               FIRST / .TRUE. /
*     ..
*     .. Executable Statements ..
*
      IF( FIRST ) THEN
         FIRST = .FALSE.
         ONE = 1
*
*        LBETA,  LIEEE1,  LT and  LRND  are the  local values  of  BETA,
*        IEEE1, T and RND.
*
*        Throughout this routine  we use the function  DLAMC3  to ensure
*        that relevant values are  stored and not held in registers,  or
*        are not affected by optimizers.
*
*        Compute  a = 2.0**m  with the  smallest positive integer m such
*        that
*
*           fl( a + 1.0 ) = a.
*
         A = 1
         C = 1
*
*+       WHILE( C.EQ.ONE )LOOP
   10    CONTINUE
         IF( C.EQ.ONE ) THEN
            A = 2*A
            C = DLAMC3( A, ONE )
            C = DLAMC3( C, -A )
            GO TO 10
         END IF
*+       END WHILE
*
*        Now compute  b = 2.0**m  with the smallest positive integer m
*        such that
*
*           fl( a + b ) .gt. a.
*
         B = 1
         C = DLAMC3( A, B )
*
*+       WHILE( C.EQ.A )LOOP
   20    CONTINUE
         IF( C.EQ.A ) THEN
            B = 2*B
            C = DLAMC3( A, B )
            GO TO 20
         END IF
*+       END WHILE
*
*        Now compute the base.  a and c  are neighbouring floating point
*        numbers  in the  interval  ( beta**t, beta**( t + 1 ) )  and so
*        their difference is beta. Adding 0.25 to c is to ensure that it
*        is truncated to beta and not ( beta - 1 ).
*
         QTR = ONE / 4
         SAVEC = C
         C = DLAMC3( C, -A )
         LBETA = C + QTR
*
*        Now determine whether rounding or chopping occurs,  by adding a
*        bit  less  than  beta/2  and a  bit  more  than  beta/2  to  a.
*
         B = LBETA
         F = DLAMC3( B / 2, -B / 100 )
         C = DLAMC3( F, A )
         IF( C.EQ.A ) THEN
            LRND = .TRUE.
         ELSE
            LRND = .FALSE.
         END IF
         F = DLAMC3( B / 2, B / 100 )
         C = DLAMC3( F, A )
         IF( ( LRND ) .AND. ( C.EQ.A ) )
     $      LRND = .FALSE.
*
*        Try and decide whether rounding is done in the  IEEE  'round to
*        nearest' style. B/2 is half a unit in the last place of the two
*        numbers A and SAVEC. Furthermore, A is even, i.e. has last  bit
*        zero, and SAVEC is odd. Thus adding B/2 to A should not  change
*        A, but adding B/2 to SAVEC should change SAVEC.
*
         T1 = DLAMC3( B / 2, A )
         T2 = DLAMC3( B / 2, SAVEC )
         LIEEE1 = ( T1.EQ.A ) .AND. ( T2.GT.SAVEC ) .AND. LRND
*
*        Now find  the  mantissa, t.  It should  be the  integer part of
*        log to the base beta of a,  however it is safer to determine  t
*        by powering.  So we find t as the smallest positive integer for
*        which
*
*           fl( beta**t + 1.0 ) = 1.0.
*
         LT = 0
         A = 1
         C = 1
*
*+       WHILE( C.EQ.ONE )LOOP
   30    CONTINUE
         IF( C.EQ.ONE ) THEN
            LT = LT + 1
            A = A*LBETA
            C = DLAMC3( A, ONE )
            C = DLAMC3( C, -A )
            GO TO 30
         END IF
*+       END WHILE
*
      END IF
*
      BETA = LBETA
      T = LT
      RND = LRND
      IEEE1 = LIEEE1
      RETURN
*
*     End of DLAMC1
*
      END
*
************************************************************************
*
      SUBROUTINE DLAMC2( BETA, T, RND, EPS, EMIN, RMIN, EMAX, RMAX )
*
*  -- LAPACK auxiliary routine (version 2.0) --
*     Univ. of Tennessee, Univ. of California Berkeley, NAG Ltd.,
*     Courant Institute, Argonne National Lab, and Rice University
*     October 31, 1992
*
*     .. Scalar Arguments ..
      LOGICAL            RND
      INTEGER            BETA, EMAX, EMIN, T
      DOUBLE PRECISION   EPS, RMAX, RMIN
*     ..
*
*  Purpose
*  =======
*
*  DLAMC2 determines the machine parameters specified in its argument
*  list.
*
*  Arguments
*  =========
*
*  BETA    (output) INTEGER
*          The base of the machine.
*
*  T       (output) INTEGER
*          The number of ( BETA ) digits in the mantissa.
*
*  RND     (output) LOGICAL
*          Specifies whether proper rounding  ( RND = .TRUE. )  or
*          chopping  ( RND = .FALSE. )  occurs in addition. This may not
*          be a reliable guide to the way in which the machine performs
*          its arithmetic.
*
*  EPS     (output) DOUBLE PRECISION
*          The smallest positive number such that
*
*             fl( 1.0 - EPS ) .LT. 1.0,
*
*          where fl denotes the computed value.
*
*  EMIN    (output) INTEGER
*          The minimum exponent before (gradual) underflow occurs.
*
*  RMIN    (output) DOUBLE PRECISION
*          The smallest normalized number for the machine, given by
*          BASE**( EMIN - 1 ), where  BASE  is the floating point value
*          of BETA.
*
*  EMAX    (output) INTEGER
*          The maximum exponent before overflow occurs.
*
*  RMAX    (output) DOUBLE PRECISION
*          The largest positive number for the machine, given by
*          BASE**EMAX * ( 1 - EPS ), where  BASE  is the floating point
*          value of BETA.
*
*  Further Details
*  ===============
*
*  The computation of  EPS  is based on a routine PARANOIA by
*  W. Kahan of the University of California at Berkeley.
*
* =====================================================================
*
*     .. Local Scalars ..
      LOGICAL            FIRST, IEEE, IWARN, LIEEE1, LRND
      INTEGER            GNMIN, GPMIN, I, LBETA, LEMAX, LEMIN, LT,
     $                   NGNMIN, NGPMIN
      DOUBLE PRECISION   A, B, C, HALF, LEPS, LRMAX, LRMIN, ONE, RBASE,
     $                   SIXTH, SMALL, THIRD, TWO, ZERO
*     ..
*     .. External Functions ..
      DOUBLE PRECISION   DLAMC3
      EXTERNAL           DLAMC3
*     ..
*     .. External Subroutines ..
      EXTERNAL           DLAMC1, DLAMC4, DLAMC5
*     ..
*     .. Intrinsic Functions ..
      INTRINSIC          ABS, MAX, MIN
*     ..
*     .. Save statement ..
      SAVE               FIRST, IWARN, LBETA, LEMAX, LEMIN, LEPS, LRMAX,
     $                   LRMIN, LT
*     ..
*     .. Data statements ..
      DATA               FIRST / .TRUE. / , IWARN / .FALSE. /
*     ..
*     .. Executable Statements ..
*
      IF( FIRST ) THEN
         FIRST = .FALSE.
         ZERO = 0
         ONE = 1
         TWO = 2
*
*        LBETA, LT, LRND, LEPS, LEMIN and LRMIN  are the local values of
*        BETA, T, RND, EPS, EMIN and RMIN.
*
*        Throughout this routine  we use the function  DLAMC3  to ensure
*        that relevant values are stored  and not held in registers,  or
*        are not affected by optimizers.
*
*        DLAMC1 returns the parameters  LBETA, LT, LRND and LIEEE1.
*
         CALL DLAMC1( LBETA, LT, LRND, LIEEE1 )
*
*        Start to find EPS.
*
         B = LBETA
         A = B**( -LT )
         LEPS = A
*
*        Try some tricks to see whether or not this is the correct  EPS.
*
         B = TWO / 3
         HALF = ONE / 2
         SIXTH = DLAMC3( B, -HALF )
         THIRD = DLAMC3( SIXTH, SIXTH )
         B = DLAMC3( THIRD, -HALF )
         B = DLAMC3( B, SIXTH )
         B = ABS( B )
         IF( B.LT.LEPS )
     $      B = LEPS
*
         LEPS = 1
*
*+       WHILE( ( LEPS.GT.B ).AND.( B.GT.ZERO ) )LOOP
   10    CONTINUE
         IF( ( LEPS.GT.B ) .AND. ( B.GT.ZERO ) ) THEN
            LEPS = B
            C = DLAMC3( HALF*LEPS, ( TWO**5 )*( LEPS**2 ) )
            C = DLAMC3( HALF, -C )
            B = DLAMC3( HALF, C )
            C = DLAMC3( HALF, -B )
            B = DLAMC3( HALF, C )
            GO TO 10
         END IF
*+       END WHILE
*
         IF( A.LT.LEPS )
     $      LEPS = A
*
*        Computation of EPS complete.
*
*        Now find  EMIN.  Let A = + or - 1, and + or - (1 + BASE**(-3)).
*        Keep dividing  A by BETA until (gradual) underflow occurs. This
*        is detected when we cannot recover the previous A.
*
         RBASE = ONE / LBETA
         SMALL = ONE
         DO 20 I = 1, 3
            SMALL = DLAMC3( SMALL*RBASE, ZERO )
   20    CONTINUE
         A = DLAMC3( ONE, SMALL )
         CALL DLAMC4( NGPMIN, ONE, LBETA )
         CALL DLAMC4( NGNMIN, -ONE, LBETA )
         CALL DLAMC4( GPMIN, A, LBETA )
         CALL DLAMC4( GNMIN, -A, LBETA )
         IEEE = .FALSE.
*
         IF( ( NGPMIN.EQ.NGNMIN ) .AND. ( GPMIN.EQ.GNMIN ) ) THEN
            IF( NGPMIN.EQ.GPMIN ) THEN
               LEMIN = NGPMIN
*            ( Non twos-complement machines, no gradual underflow;
*              e.g.,  VAX )
            ELSE IF( ( GPMIN-NGPMIN ).EQ.3 ) THEN
               LEMIN = NGPMIN - 1 + LT
               IEEE = .TRUE.
*            ( Non twos-complement machines, with gradual underflow;
*              e.g., IEEE standard followers )
            ELSE
               LEMIN = MIN( NGPMIN, GPMIN )
*            ( A guess; no known machine )
               IWARN = .TRUE.
            END IF
*
         ELSE IF( ( NGPMIN.EQ.GPMIN ) .AND. ( NGNMIN.EQ.GNMIN ) ) THEN
            IF( ABS( NGPMIN-NGNMIN ).EQ.1 ) THEN
               LEMIN = MAX( NGPMIN, NGNMIN )
*            ( Twos-complement machines, no gradual underflow;
*              e.g., CYBER 205 )
            ELSE
               LEMIN = MIN( NGPMIN, NGNMIN )
*            ( A guess; no known machine )
               IWARN = .TRUE.
            END IF
*
         ELSE IF( ( ABS( NGPMIN-NGNMIN ).EQ.1 ) .AND.
     $            ( GPMIN.EQ.GNMIN ) ) THEN
            IF( ( GPMIN-MIN( NGPMIN, NGNMIN ) ).EQ.3 ) THEN
               LEMIN = MAX( NGPMIN, NGNMIN ) - 1 + LT
*            ( Twos-complement machines with gradual underflow;
*              no known machine )
            ELSE
               LEMIN = MIN( NGPMIN, NGNMIN )
*            ( A guess; no known machine )
               IWARN = .TRUE.
            END IF
*
         ELSE
            LEMIN = MIN( NGPMIN, NGNMIN, GPMIN, GNMIN )
*         ( A guess; no known machine )
            IWARN = .TRUE.
         END IF
***
* Comment out this if block if EMIN is ok
         IF( IWARN ) THEN
            FIRST = .TRUE.
            WRITE( 6, FMT = 9999 )LEMIN
         END IF
***
*
*        Assume IEEE arithmetic if we found denormalised  numbers above,
*        or if arithmetic seems to round in the  IEEE style,  determined
*        in routine DLAMC1. A true IEEE machine should have both  things
*        true; however, faulty machines may have one or the other.
*
         IEEE = IEEE .OR. LIEEE1
*
*        Compute  RMIN by successive division by  BETA. We could compute
*        RMIN as BASE**( EMIN - 1 ),  but some machines underflow during
*        this computation.
*
         LRMIN = 1
         DO 30 I = 1, 1 - LEMIN
            LRMIN = DLAMC3( LRMIN*RBASE, ZERO )
   30    CONTINUE
*
*        Finally, call DLAMC5 to compute EMAX and RMAX.
*
         CALL DLAMC5( LBETA, LT, LEMIN, IEEE, LEMAX, LRMAX )
      END IF
*
      BETA = LBETA
      T = LT
      RND = LRND
      EPS = LEPS
      EMIN = LEMIN
      RMIN = LRMIN
      EMAX = LEMAX
      RMAX = LRMAX
*
      RETURN
*
 9999 FORMAT( / / ' WARNING. The value EMIN may be incorrect:-',
     $      '  EMIN = ', I8, /
     $      ' If, after inspection, the value EMIN looks',
     $      ' acceptable please comment out ',
     $      / ' the IF block as marked within the code of routine',
     $      ' DLAMC2,', / ' otherwise supply EMIN explicitly.', / )
*
*     End of DLAMC2
*
      END
*
************************************************************************
*
      DOUBLE PRECISION FUNCTION DLAMC3( A, B )
*
*  -- LAPACK auxiliary routine (version 2.0) --
*     Univ. of Tennessee, Univ. of California Berkeley, NAG Ltd.,
*     Courant Institute, Argonne National Lab, and Rice University
*     October 31, 1992
*
*     .. Scalar Arguments ..
      DOUBLE PRECISION   A, B
*     ..
*
*  Purpose
*  =======
*
*  DLAMC3  is intended to force  A  and  B  to be stored prior to doing
*  the addition of  A  and  B ,  for use in situations where optimizers
*  might hold one of these in a register.
*
*  Arguments
*  =========
*
*  A, B    (input) DOUBLE PRECISION
*          The values A and B.
*
* =====================================================================
*
*     .. Executable Statements ..
*
      DLAMC3 = A + B
*
      RETURN
*
*     End of DLAMC3
*
      END
*
************************************************************************
*
      SUBROUTINE DLAMC4( EMIN, START, BASE )
*
*  -- LAPACK auxiliary routine (version 2.0) --
*     Univ. of Tennessee, Univ. of California Berkeley, NAG Ltd.,
*     Courant Institute, Argonne National Lab, and Rice University
*     October 31, 1992
*
*     .. Scalar Arguments ..
      INTEGER            BASE, EMIN
      DOUBLE PRECISION   START
*     ..
*
*  Purpose
*  =======
*
*  DLAMC4 is a service routine for DLAMC2.
*
*  Arguments
*  =========
*
*  EMIN    (output) EMIN
*          The minimum exponent before (gradual) underflow, computed by
*          setting A = START and dividing by BASE until the previous A
*          can not be recovered.
*
*  START   (input) DOUBLE PRECISION
*          The starting point for determining EMIN.
*
*  BASE    (input) INTEGER
*          The base of the machine.
*
* =====================================================================
*
*     .. Local Scalars ..
      INTEGER            I
      DOUBLE PRECISION   A, B1, B2, C1, C2, D1, D2, ONE, RBASE, ZERO
*     ..
*     .. External Functions ..
      DOUBLE PRECISION   DLAMC3
      EXTERNAL           DLAMC3
*     ..
*     .. Executable Statements ..
*
      A = START
      ONE = 1
      RBASE = ONE / BASE
      ZERO = 0
      EMIN = 1
      B1 = DLAMC3( A*RBASE, ZERO )
      C1 = A
      C2 = A
      D1 = A
      D2 = A
*+    WHILE( ( C1.EQ.A ).AND.( C2.EQ.A ).AND.
*    $       ( D1.EQ.A ).AND.( D2.EQ.A )      )LOOP
   10 CONTINUE
      IF( ( C1.EQ.A ) .AND. ( C2.EQ.A ) .AND. ( D1.EQ.A ) .AND.
     $    ( D2.EQ.A ) ) THEN
         EMIN = EMIN - 1
         A = B1
         B1 = DLAMC3( A / BASE, ZERO )
         C1 = DLAMC3( B1*BASE, ZERO )
         D1 = ZERO
         DO 20 I = 1, BASE
            D1 = D1 + B1
   20    CONTINUE
         B2 = DLAMC3( A*RBASE, ZERO )
         C2 = DLAMC3( B2 / RBASE, ZERO )
         D2 = ZERO
         DO 30 I = 1, BASE
            D2 = D2 + B2
   30    CONTINUE
         GO TO 10
      END IF
*+    END WHILE
*
      RETURN
*
*     End of DLAMC4
*
      END
*
************************************************************************
*
      SUBROUTINE DLAMC5( BETA, P, EMIN, IEEE, EMAX, RMAX )
*
*  -- LAPACK auxiliary routine (version 2.0) --
*     Univ. of Tennessee, Univ. of California Berkeley, NAG Ltd.,
*     Courant Institute, Argonne National Lab, and Rice University
*     October 31, 1992
*
*     .. Scalar Arguments ..
      LOGICAL            IEEE
      INTEGER            BETA, EMAX, EMIN, P
      DOUBLE PRECISION   RMAX
*     ..
*
*  Purpose
*  =======
*
*  DLAMC5 attempts to compute RMAX, the largest machine floating-point
*  number, without overflow.  It assumes that EMAX + abs(EMIN) sum
*  approximately to a power of 2.  It will fail on machines where this
*  assumption does not hold, for example, the Cyber 205 (EMIN = -28625,
*  EMAX = 28718).  It will also fail if the value supplied for EMIN is
*  too large (i.e. too close to zero), probably with overflow.
*
*  Arguments
*  =========
*
*  BETA    (input) INTEGER
*          The base of floating-point arithmetic.
*
*  P       (input) INTEGER
*          The number of base BETA digits in the mantissa of a
*          floating-point value.
*
*  EMIN    (input) INTEGER
*          The minimum exponent before (gradual) underflow.
*
*  IEEE    (input) LOGICAL
*          A logical flag specifying whether or not the arithmetic
*          system is thought to comply with the IEEE standard.
*
*  EMAX    (output) INTEGER
*          The largest exponent before overflow
*
*  RMAX    (output) DOUBLE PRECISION
*          The largest machine floating-point number.
*
* =====================================================================
*
*     .. Parameters ..
      DOUBLE PRECISION   ZERO, ONE
      PARAMETER          ( ZERO = 0.0D0, ONE = 1.0D0 )
*     ..
*     .. Local Scalars ..
      INTEGER            EXBITS, EXPSUM, I, LEXP, NBITS, TRY, UEXP
      DOUBLE PRECISION   OLDY, RECBAS, Y, Z
*     ..
*     .. External Functions ..
      DOUBLE PRECISION   DLAMC3
      EXTERNAL           DLAMC3
*     ..
*     .. Intrinsic Functions ..
      INTRINSIC          MOD
*     ..
*     .. Executable Statements ..
*
*     First compute LEXP and UEXP, two powers of 2 that bound
*     abs(EMIN). We then assume that EMAX + abs(EMIN) will sum
*     approximately to the bound that is closest to abs(EMIN).
*     (EMAX is the exponent of the required number RMAX).
*
      LEXP = 1
      EXBITS = 1
   10 CONTINUE
      TRY = LEXP*2
      IF( TRY.LE.( -EMIN ) ) THEN
         LEXP = TRY
         EXBITS = EXBITS + 1
         GO TO 10
      END IF
      IF( LEXP.EQ.-EMIN ) THEN
         UEXP = LEXP
      ELSE
         UEXP = TRY
         EXBITS = EXBITS + 1
      END IF
*
*     Now -LEXP is less than or equal to EMIN, and -UEXP is greater
*     than or equal to EMIN. EXBITS is the number of bits needed to
*     store the exponent.
*
      IF( ( UEXP+EMIN ).GT.( -LEXP-EMIN ) ) THEN
         EXPSUM = 2*LEXP
      ELSE
         EXPSUM = 2*UEXP
      END IF
*
*     EXPSUM is the exponent range, approximately equal to
*     EMAX - EMIN + 1 .
*
      EMAX = EXPSUM + EMIN - 1
      NBITS = 1 + EXBITS + P
*
*     NBITS is the total number of bits needed to store a
*     floating-point number.
*
      IF( ( MOD( NBITS, 2 ).EQ.1 ) .AND. ( BETA.EQ.2 ) ) THEN
*
*        Either there are an odd number of bits used to store a
*        floating-point number, which is unlikely, or some bits are
*        not used in the representation of numbers, which is possible,
*        (e.g. Cray machines) or the mantissa has an implicit bit,
*        (e.g. IEEE machines, Dec Vax machines), which is perhaps the
*        most likely. We have to assume the last alternative.
*        If this is true, then we need to reduce EMAX by one because
*        there must be some way of representing zero in an implicit-bit
*        system. On machines like Cray, we are reducing EMAX by one
*        unnecessarily.
*
         EMAX = EMAX - 1
      END IF
*
      IF( IEEE ) THEN
*
*        Assume we are on an IEEE machine which reserves one exponent
*        for infinity and NaN.
*
         EMAX = EMAX - 1
      END IF
*
*     Now create RMAX, the largest machine number, which should
*     be equal to (1.0 - BETA**(-P)) * BETA**EMAX .
*
*     First compute 1.0 - BETA**(-P), being careful that the
*     result is less than 1.0 .
*
      RECBAS = ONE / BETA
      Z = BETA - ONE
      Y = ZERO
      DO 20 I = 1, P
         Z = Z*RECBAS
         IF( Y.LT.ONE )
     $      OLDY = Y
         Y = DLAMC3( Y, Z )
   20 CONTINUE
      IF( Y.GE.ONE )
     $   Y = OLDY
*
*     Now multiply by BETA**EMAX to get RMAX.
*
      DO 30 I = 1, EMAX
         Y = DLAMC3( Y*BETA, ZERO )
   30 CONTINUE
*
      RMAX = Y
      RETURN
*
*     End of DLAMC5
*
      END

      LOGICAL          FUNCTION LSAME( CA, CB )
*
*  -- LAPACK auxiliary routine (version 2.0) --
*     Univ. of Tennessee, Univ. of California Berkeley, NAG Ltd.,
*     Courant Institute, Argonne National Lab, and Rice University
*     September 30, 1994
*
*     .. Scalar Arguments ..
      CHARACTER          CA, CB
*     ..
*
*  Purpose
*  =======
*
*  LSAME returns .TRUE. if CA is the same letter as CB regardless of
*  case.
*
*  Arguments
*  =========
*
*  CA      (input) CHARACTER*1
*  CB      (input) CHARACTER*1
*          CA and CB specify the single characters to be compared.
*
* =====================================================================
*
*     .. Intrinsic Functions ..
      INTRINSIC          ICHAR
*     ..
*     .. Local Scalars ..
      INTEGER            INTA, INTB, ZCODE
*     ..
*     .. Executable Statements ..
*
*     Test if the characters are equal
*
      LSAME = CA.EQ.CB
      IF( LSAME )
     $   RETURN
*
*     Now test for equivalence if both characters are alphabetic.
*
      ZCODE = ICHAR( 'Z' )
*
*     Use 'Z' rather than 'A' so that ASCII can be detected on Prime
*     machines, on which ICHAR returns a value with bit 8 set.
*     ICHAR('A') on Prime machines returns 193 which is the same as
*     ICHAR('A') on an EBCDIC machine.
*
      INTA = ICHAR( CA )
      INTB = ICHAR( CB )
*
      IF( ZCODE.EQ.90 .OR. ZCODE.EQ.122 ) THEN
*
*        ASCII is assumed - ZCODE is the ASCII code of either lower or
*        upper case 'Z'.
*
         IF( INTA.GE.97 .AND. INTA.LE.122 ) INTA = INTA - 32
         IF( INTB.GE.97 .AND. INTB.LE.122 ) INTB = INTB - 32
*
      ELSE IF( ZCODE.EQ.233 .OR. ZCODE.EQ.169 ) THEN
*
*        EBCDIC is assumed - ZCODE is the EBCDIC code of either lower or
*        upper case 'Z'.
*
         IF( INTA.GE.129 .AND. INTA.LE.137 .OR.
     $       INTA.GE.145 .AND. INTA.LE.153 .OR.
     $       INTA.GE.162 .AND. INTA.LE.169 ) INTA = INTA + 64
         IF( INTB.GE.129 .AND. INTB.LE.137 .OR.
     $       INTB.GE.145 .AND. INTB.LE.153 .OR.
     $       INTB.GE.162 .AND. INTB.LE.169 ) INTB = INTB + 64
*
      ELSE IF( ZCODE.EQ.218 .OR. ZCODE.EQ.250 ) THEN
*
*        ASCII is assumed, on Prime machines - ZCODE is the ASCII code
*        plus 128 of either lower or upper case 'Z'.
*
         IF( INTA.GE.225 .AND. INTA.LE.250 ) INTA = INTA - 32
         IF( INTB.GE.225 .AND. INTB.LE.250 ) INTB = INTB - 32
      END IF
      LSAME = INTA.EQ.INTB
*
*     RETURN
*
*     End of LSAME
*
      END
#endif
