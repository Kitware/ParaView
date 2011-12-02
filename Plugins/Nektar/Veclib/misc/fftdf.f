      subroutine fftdf(a,n,nskip,mtrn,mskip,isign,fac,
     >                 irev,btrv,ierr,ireal)
      double precision a(1), fac(1)
      integer          btrv(1), n, nskip, mtrn, mskip, isign
      integer          irev, ierr, ireal
c
c     fftdf performs multiple complex-complex or real-complex fast 
c     fourier transforms by decimation in frequency. initialization is 
c     performed by calling fftdf with isign = 0.  Note that the fortran 
c     version does not actually use the btrv array.
c
c     a            double precision complex n + 1, real 2*(n+1)
c                  the array of transforms.
c     
c     nskip        skip between adjacent points of a transform (complex)
c
c     mtrn         number of transforms to do.
c
c     mskip        skip between starting elements of adjacent transforms
c
c     isign        sign (+/-1) of exponent/direction of transform
c                  forward = -1, back = +1
c
c     fac          array of exponential factors of length 6*n calculated 
c                  during the initialization call (isign=0)
c
c     irev         1 to do the bitreverse, 0 to skip it
c
c     btrv         index array for bitreverse on length n
c
c     ierr         return error condition  
c
c     ireal        0 = complex-complex, 1 = real-complex
c
c
c
c
c     fourier-space representation
c
c     real-complex    
c
c     for a real array of length n, an array of length n+2 (n/2+1 
c     complex numbers) is needed in fourier space
c
c         k:      0  1   2  ...   n/2-1  n/2
c         index:  1  2   3  ...   n/2    n/2+1
c
c     complex-complex: 
c
c         k:      0   1   2  ...  n/2-1  +-n/2  -(n/2-1) ...  -2   -1
c         index:  1   2   3  ...  n/2    n/2+1   n/2+2   ...  n-1   n
c
c     the index n/2+1 in the complex-complex fft corresponds to the 
c     aliased (+/-)n/2 mode and should be ignored (set to zero)
c
c
c
c     Things to be kept in mind:
c
c     1. To transform from Physical-Fourier space, use isign = -1,
c        which corresponds to a negative exponent exp(-i*pi*n/N).
c
c     2. After the forward and back transforms, the data must be div-
c        ided through by N for the complex-complex routines and 4*N
c        for the real-complex routines.
c
c=====================================================================
c
      if (isign.eq.0) then
         call facinit(fac,n)
         return
      endif
      if (ireal.eq.1) then
         call fealft(a,n,nskip,mtrn,mskip,isign,fac,
     >               irev,btrv,ierr)
      else
         call ftsdf(a,n,nskip,mtrn,mskip,isign,fac,
     >              irev,btrv,ierr)
      endif
      return
      end
c
c=====================================================================
c
c     real-complex decimation-in-frequency
c 
      subroutine fealft (data,n,nskip,mtrn,mskip,isign,
     >                   fac,irev,btrv,ierr)
      double precision  data(1)
      double precision  fac (1)
      integer           btrv(1), n, nskip, mtrn, mskip, isign
     >                  irev, ierr
c
c     internal variables
c
      double precision  wr, wi, wpr, wpi, wtemp, theta
      double precision  h1r, h1i, h2r, h2i
      double precision  c1, sfac
      integer           itrn, ib, i, i1, i2, i3, i4, n2p3
c
c     constants
c
      double precision  pi
      parameter (pi=3.141592653589793238463d0)
c
      theta = pi / dble(n)
      c1    = dble(isign)
      if (isign.eq.-1) then
         call ftsdf(data,n,nskip,mtrn,mskip,isign,fac,
     >        irev,btrv,ierr)
      endif
c     
c     main loop over transforms
c     
      do itrn = 1, mtrn
         wpr  = dcos(theta)
         wpi  = dsin(theta)
         wr   = wpr
         wi   = wpi
         ib   = (itrn-1)*mskip
         n2p3 = n + 2
         do i = 2, ishft(n,-1)
c
            i1  = ishft((i-1)*nskip+ib,1) + 1
            i2  = i1 + 1
            i3  = ishft((n2p3-i-1)*nskip + ib,1) + 1
            i4  = i3 + 1
c
            h1r      =  data(i1) + data(i3)
            h1i      =  data(i2) - data(i4)
            h2r      =  data(i2) + data(i4)
            h2i      =  data(i1) - data(i3)
c
            data(i1) =  h1r - wi * h2i - c1 * wr * h2r
            data(i2) =  h1i - wi * h2r + c1 * wr * h2i
            data(i3) =  h1r + wi * h2i + c1 * wr * h2r
            data(i4) = -h1i - wi * h2r + c1 * wr * h2i
c
            wtemp = wr
            wr    = wr * wpr - wi    * wpi
            wi    = wi * wpr + wtemp * wpi
         enddo
c
         if (isign.eq.-1) then
            i1       = ishft(ib,1) + 1
            i2       = i1 + 1
            i3       = i1 + n*nskip
            i4       = i3 + 1
            h1r      = data(i1)
            h1i      = data(i2)
            data(i1) = 2.d0 * (h1r + h1i)
            data(i2) = 2.d0 * (h1r - h1i) 
            data(i3) =  2.d0* data(i3)
            data(i4) = -2.d0* data(i4)
         else
            i1       = ishft(ib,1) + 1
            i2       = i1 + 1
            i3       = i1 + n*nskip
            i4       = i3 + 1
            h1r      = data(i1)
            h1i      = data(i2)
            data(i1) = h1r + h1i
            data(i2) = h1r - h1i
            data(i3) =  2.d0 * data(i3)
            data(i4) = -2.d0 * data(i4)
         endif
      enddo
      if (isign.eq.1) then
            call ftsdf(data,n,nskip,mtrn,mskip,isign,fac,
     >           irev,btrv,ierr)
         endif
      return
      end
c     
c
c=====================================================================
c
c     complex-complex decimation-in-frequency
c 
      subroutine ftsdf(data,n,nskip,mtrn,mskip,isign,
     >                 fac,irev,btrv,ierr)
      double precision  data(1)
      double precision  fac (1)
      integer           btrv(1), n, nskip, mtrn, mskip, isign
      integer           irev, ierr
c
c     internal variables
c
      double precision  tmpr, tmpi, exr, exi, hr, hi
      integer           itrn, ill, ilog, idexj, iss, i, j, mmax
      integer           istep, ib, m, i1, i2, j1, j2
c
c     constants
c
      double precision  pi, l2
      parameter (pi=3.141592653589793238463d0,
     >           l2=1.442695040888963407359d0)

c
c     calculate log(n) 
c
      ilog = int(dlog(dble(n))*l2 + 0.01d0)
c 
c     set starting point in exponential factor array
c
      if (isign.eq.1) then
         iss = ishft(n-1,1)
      else
         iss = 0
      endif
c     
c     main loop over transforms
c
      do itrn = 1, mtrn
         mmax  = n
         ill   = ilog + 1
         ib    = (itrn-1) * mskip
 101     continue
         ill   = ill - 1
         istep = mmax
         mmax  = ishft(mmax,-1)
         idexj = ishft(2,ill-1) - 1 + iss
         do m  = 1, mmax
            exr  = fac(idexj)
            exi  = fac(idexj+1)
            do i = m, n, istep
c
               j  = i + mmax
               i1 = ishft(ib + (i - 1) * nskip,1) + 1
               i2 = i1 + 1
               j1 = ishft(ib + (j - 1) * nskip,1) + 1
               j2 = j1 + 1
c     
               tmpr     = data(i1) - data(j1)
               tmpi     = data(i2) - data(j2)
               data(i1) = data(i1) + data(j1)
               data(i2) = data(i2) + data(j2)
               data(j1) = exr * tmpr - exi * tmpi
               data(j2) = exr * tmpi + exi * tmpr
c
            enddo
            idexj = idexj + 2 
         enddo
c
         if(mmax.gt.1) goto 101
c
c     do the bit reverse
c         
         if (irev.eq.0) goto 999
         j = 1
         do i = 1, n
            if(j.gt.i)then
               i1       = ishft(ib + (i-1) * nskip,1) + 1
               i2       = i1 + 1
               j1       = ishft(ib + (j-1) * nskip,1) + 1
               j2       = j1 + 1
               tmpr     = data(j1)
               tmpi     = data(j2)
               data(j1) = data(i1)
               data(j2) = data(i2)
               data(i1) = tmpr
               data(i2) = tmpi
            endif
            m = ishft(n,-1)
 888        if((m.ge.2).and.(j.gt.m)) then
               j = j - m
               m = ishft(m,-1)
               go to 888
            endif
            j = j + m
         enddo
 999     continue
      enddo
      return
      end
c
c
c=====================================================================
c
c     precalculate exponential factors
      subroutine facinit (fac, n)
      double precision fac(1)
      integer           n
c
c     internal variables
c
      double precision er,ei,epr,epi,etemp,theta,eps
      integer           nn, inf, isign, mmax, m
c
c     constants
c
      double precision  pi
      parameter (pi=3.141592653589793238463d0)
c
      nn    = ishft(n,1)
      eps   = 9.d-16
      inf   = 1
      isign = -1
 3    continue
      mmax  = 2
 2    if (nn.gt.mmax) then
         theta = 2.d0 * pi / dble(isign*mmax)
         epr   = dcos(theta)
         epi   = dsin(theta)
         er    = 1.d0
         ei    = 0.d0
         do m = 1, mmax, 2
            if (dabs(er).lt.eps) er = 0.0d0
            if (dabs(ei).lt.eps) ei = 0.0d0
            fac(inf)   = er
            fac(inf+1) = ei
            etemp      = er
            er         = er * epr - ei * epi
            ei         = ei * epr + etemp * epi
            inf        = inf + 2
         enddo
         mmax = ishft(mmax,1)
         go to 2
      endif
      if (isign.lt.0) then
         isign = 1
         goto 3
      endif
c     
c     this section is to generate the additional factors needed for 
c     real-complex
c
      inf   = ishft(n,2)
      isign = -1
 33   continue
      mmax  = nn
      theta = 2.d0 * pi / dble(isign*mmax)
      epr   = dcos(theta)
      epi   = dsin(theta)
      er    = epr
      ei    = epi
      do m = 2, ishft(n,-1)
         if (dabs(er).lt.eps) er = 0.0d0
         if (dabs(ei).lt.eps) ei = 0.0d0
         fac(inf)   = er
         fac(inf+1) = ei
         etemp      = er
         er         = er * epr - ei * epi
         ei         = ei * epr + etemp * epi
         inf        = inf + 2
      enddo
      if (isign.lt.0) then
         isign = 1 
         goto 33
      endif
      return
      end
