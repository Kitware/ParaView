Subroutine CoProcessPV
!==============================================================================
! Description:  ParaView CoProcessing, using PagosaAdaptor.cxx by Chris Sewell,
!               for ParaView version 3.98.1
!
! Called From:  FLIP_Tecwrt
!
!   Externals:  CoProcessorInitialize, SetCoProcessorGeometry,
!               SetCoProcessorField, RequestDataDescription, CoProcess
!
! History:
! Version   Date         Comment
! -------   ----------   ------------------------------------------------------
! 00        04/09/2013   B. A. Kashiwa
! 17.4b     04/20/2018   B. A. Kashiwa  changed volume fraction definition in
!                                       grid output to improve frag counting
! 17.4c     05/03/2018   B. A. Kashiwa  reverted volume fraction definition to
!                                       circumvent ParaView type conversion
!                                       snafu.
! 17.4d     05/04/2018   B. A. Kashiwa  changed Volume Fraction output to a more
!                                       accurate name "Fraction [0-255]" in
!                                       to distinguish the data from the added
!                                       grid output for Volume Fraction.
! v17.4a    05/21/2018   B. A. Kashiwa  resolve array out-of-bounds due to logic
!                                       error in Load_UC_m (formerly Load_UC_kf)
! 17.4b     11/08/2018   Andrew F Nelson fix xlf syntax warning
! 17.4c     05/09/2019   David Culp     Added cases for I1-J2 fracture
! 17.4d     05/15/2019   Andrew F Nelson replace timer with get_wtime
!==============================================================================
use metrics, only: get_wtime

Implicit None

Integer(kind=kint) :: m, kf, i, j, k, kp, nvp, idfrac_m
Real(kind=kreal)   :: c0sqr, pois, twoG, &
                      c_ref, e_ref, p_ref, s_ref, T_ref, v_ref, &
                      sig_m, sig_e

Real(kind=kdbl)    :: elapsed, sec0, sec1, CPtime

Real(kind=kreal), Dimension(0:mx,0:my,0:mz) :: Tmp, SCL
Real(kind=ksngl), Dimension(0:mx,0:my,0:mz) :: Tms
Real(kind=kreal), Dimension(0:mx,0:my,0:mz,1:3) :: uc_Tmp

Real(kind=kreal), Allocatable, Dimension(:,:) :: avg
Real(kind=ksngl), Allocatable, Dimension(:,:) :: avs
Real(kind=ksngl), Allocatable, Dimension(:,:,:,:) :: XXs

Integer :: doprocess, status

Character(len=02) :: mtail
Character(len=40) :: var_label
Character(len=06) :: mfilet
Character(len=22) :: DateNow
Character(len=256) :: vline1
Character(len=256) :: vline2
Character(len=256) :: vline3
Character(len=256) :: vline4
Character(len=1024):: vline
Character(len=9) :: ncycle
Character(len=9) :: nframe
Character(len=9) :: nPEs

Logical :: file_exists
!==============================================================================

!if (ALL(CoProcessPV_Point .eq. .false.) .and. &
!    ALL(CoProcessPV_Grid  .eq. .false.)) return ! gets fortran warning

   do m = 1,nmat
     if(CoProcessPV_Point(m) .or. CoProcessPV_Grid(m)) goto 10
   enddo
   return ! there are no fields to output
10 continue

if(my_id == 0) then
  write(*,*) 'CoProcessPV ', nfilet, ' at Step = ', istep, ' time = ', t
  write(2,*) 'CoProcessPV ', nfilet, ' at Step = ', istep, ' time = ', t
endif

call get_wtime(sec0)

  Call Day_Time(DateNow)

  vline1 = 'Title: '//ADJUSTL(Title)
  write(ncycle,'(i9)') istep
  write(nframe,'(i9)') nfilet
  write(nPEs,  '(i9)') tot_pes
  write(vline2,'(5a,1pe10.3,a)') 'Frame: ',TRIM(ADJUSTL(nframe)),'  n = ',TRIM(ADJUSTL(ncycle)),'  t =',t,' [us]'
  vline3 = 'Write: '//TRIM(DateNow)//'   HeadPE: '//TRIM(HostName)//'  nPEs:  '//TRIM(ADJUSTL(nPEs))
  vline4 = 'Build: '//TRIM(ProgName)//'  '//TRIM(CodeVersion)//'  '//TRIM(CodeDate(5:))
  vline = TRIM(vline1)//crlf//TRIM(vline2)//crlf//TRIM(vline3)//crlf//TRIM(vline4)

! Note:  Character(len=2) :: crlf = char(13)//char(10) ! is used in constructing multiline strings

! One time initialize of coprocessor
if(nfilet .eq. nfile0) then
  Call CoProcessorInitializeWithPython(PVScript, LEN_TRIM(PVScript)) ! v4.3.1
endif

! One time create of coprocessor geometry
if(nfilet .eq. nfile0) then

#ifdef DBL_PREC
! if(ALLOCATED(XXs)) DEALLOCATE(XXs)
! ALLOCATE(XXs(0:mx,0:my,0:mx,1:6))
! XXs(:,:,:,1) = Real( Xv(:,:,:,1), kind=ksngl)
! XXs(:,:,:,2) = Real( Xv(:,:,:,2), kind=ksngl)
! XXs(:,:,:,3) = Real( Xv(:,:,:,3), kind=ksngl)
! XXs(:,:,:,4) = Real(dXc(:,:,:,1), kind=ksngl)
! XXs(:,:,:,5) = Real(dXc(:,:,:,2), kind=ksngl)
! XXs(:,:,:,6) = Real(dXc(:,:,:,3), kind=ksngl)
! Call SetCoProcessorGeometry(mx, my, mz, &
!   DBLE(XXs(0,0,0,1)), DBLE(XXs(0,0,0,2)), DBLE(XXs(0,0,0,3)), &
!   DBLE(XXs(0,0,0,4)), DBLE(XXs(0,0,0,5)), DBLE(XXs(0,0,0,6)), &
!   my_id, tot_pes)
! DEALLOCATE(XXs)
  Call SetCoProcessorGeometry(mx, my, mz, &
     Xv(0,0,0,1),  Xv(0,0,0,2),  Xv(0,0,0,3), &
    dXc(0,0,0,1), dXc(0,0,0,2), dXc(0,0,0,3), &
    my_id, tot_pes, TRIM(vline2), LEN(TRIM(vline2)), TRIM(vline), LEN(TRIM(vline)))
#else
  Call SetCoProcessorGeometry(mx, my, mz, &
    DBLE( Xv(0,0,0,1)), DBLE( Xv(0,0,0,2)), DBLE( Xv(0,0,0,3)), &
    DBLE(dXc(0,0,0,1)), DBLE(dXc(0,0,0,2)), DBLE(dXc(0,0,0,3)), &
    my_id, tot_pes, TRIM(vline2), LEN(TRIM(vline2)), TRIM(vline), LEN(TRIM(vline)))
#endif

endif

!function in PagosaAdaptor.cxx:
!extern "C" void setcoprocessorgeometry_(int* mx, int* my, int* mz, double* x0, double* y0,
!  double* z0, double* dx, double* dy, double* dz, unsigned int* my_id, const int* tot_pes,
!  int* stepNum, float* simTime, char* version, int* versionlen)

   do m = 1,nmat
     if(CoProcessPV_Point(m)) goto 20
   enddo
   goto 30 ! there are no marker (point) fields to output

20 continue ! output Point data

! Count the total number of markers to coprocess
nvp = 0
do kf = 1,nf ; if(.not. CoProcessPV_Point((mkf(kf)))) cycle
  nvp = nvp + nkp(kf)
enddo

! Update grid structure with frame, cycle and sim time
!Call SetGridGeometry(vline2, LEN(vline2), istep, t)
 Call SetGridGeometry( &
   TRIM(vline2), LEN(TRIM(vline2)), &
   TRIM(vline ), LEN(TRIM(vline )), istep, DBLE(t))

! Update marker structure with frame, cycle and sim time and allocate total size
!Call SetMarkerGeometry(nvp, vline2, LEN(vline2), istep, t)
 Call SetMarkerGeometry(nvp, &
   TRIM(vline2), LEN(TRIM(vline2)), &
   TRIM(vline ), LEN(TRIM(vline )), istep, DBLE(t))

if(ALLOCATED(avg)) DEALLOCATE(avg)
if(ALLOCATED(avs)) DEALLOCATE(avs)

do kf = 1,nf ; if(.not. CoProcessPV_Point((mkf(kf)))) cycle

#ifdef DBL_PREC
  ALLOCATE(avs(nkp(kf),3))
  avs(1:nkp(kf),1) = Real(xp(1:nkp(kf),kf,1), kind=ksngl)
  avs(1:nkp(kf),2) = Real(xp(1:nkp(kf),kf,2), kind=ksngl)
  avs(1:nkp(kf),3) = Real(xp(1:nkp(kf),kf,3), kind=ksngl)
  Call AddMarkerGeometry(nkp(kf), avs(1,1), avs(1,2), avs(1,3))
  DEALLOCATE(avs)
#else
  Call AddMarkerGeometry(nkp(kf), xp(1,kf,1), xp(1,kf,2), xp(1,kf,3))
!         xp(1:nkp(kf),kf,1), xp(1:nkp(kf),kf,2), xp(1:nkp(kf),kf,3))
#endif
enddo

! Set the marker field data
! Material

do kf = 1,nf ; m = mkf(kf)

  if(.not. CoProcessPV_Point(m)) cycle

  ALLOCATE(avg(nkp(kf),3))
  ALLOCATE(avs(nkp(kf),3))

! load reference values
  twoG = two*g0(m)
  pois = poisson(m)
  idfrac_m = Int(Cmat(7,m), kind=kint) ! Pagosa fracture case
  if(idfrac_m > 2) then ! the material has a history stress, and star(3) = c^2
    c0sqr = (twoG/d0(m))*(one - pois)/(one - two*pois)
    c_ref = Sqrt(c0sqr)
    e_ref = Real(Cmat(33,m), kind=kreal) ! I1 activation energy, D_8
    p_ref = Real(Cmat(14,m), kind=kreal) ! sig_y (change to sig_u)
    s_ref = Real(Cmat(29,m), kind=kreal) ! efmin
    T_ref = 298._kreal
    v_ref = one/d0(m)
  else
    c_ref = one
    e_ref = e0(m)
    p_ref = one
    s_ref = one
    T_ref = 298._kreal
    v_ref = one/d0(m)
  endif

! field number
  avg(1:nkp(kf),1) = REAL(kf, kind=kreal)
  avs(1:nkp(kf),1) = Real(avg(1:nkp(kf),1), kind=ksngl)
  Call AddMarkerScalarField('${\rm Field\ Number}$', 21, nkp(kf), avs(1,1))

! <T>
  avg(1:nkp(kf),1) = Tp(1:nkp(kf),kf)
  Call MarkerAverage(avg(1,1),kf)
! avs(1:nkp(kf),1) = Real(Max(T_ref,avg(1:nkp(kf),1)), kind=ksngl)
  avs(1:nkp(kf),1) = Real(avg(1:nkp(kf),1), kind=ksngl)
  Call AddMarkerScalarField('${\rm T\ [K]}$', 14, nkp(kf), avs(1,1))

! e/v
! avg(1:nkp(kf),1) = ep(1:nkp(kf),kf)*d0(m)*thousand
! avs(1:nkp(kf),1) = Real(avg(1:nkp(kf),1), kind=ksngl)
! Call AddMarkerScalarField('${e/v_\circ}\ {\rm [kbar]}$', 27, nkp(kf), avs(1,1))

! <sig_m>
! avg(1:nkp(kf),1) = (sigp(1:nkp(kf),kf,1) + &
!                     sigp(1:nkp(kf),kf,2) + &
!                     sigp(1:nkp(kf),kf,3))*third
! Call MarkerAverage(avg(1,1),kf)
! avg(1:nkp(kf),1) = (avg(1:nkp(kf),1) - Pp(1:nkp(kf),kf))*thousand
! avs(1:nkp(kf),1) = Real(avg(1:nkp(kf),1), kind=ksngl)
! Call AddMarkerScalarField('${\sigma_m}\ {\rm [kbar]}$', 26, nkp(kf), avs(1,1))

! u - not averaged
  avg(1:nkp(kf),1) = up(1:nkp(kf),kf,1) !/c_ref
  avg(1:nkp(kf),2) = up(1:nkp(kf),kf,2) !/c_ref
  avg(1:nkp(kf),3) = up(1:nkp(kf),kf,3) !/c_ref
  avs(1:nkp(kf),1) = Real(avg(1:nkp(kf),1), kind=ksngl)
  avs(1:nkp(kf),2) = Real(avg(1:nkp(kf),2), kind=ksngl)
  avs(1:nkp(kf),3) = Real(avg(1:nkp(kf),3), kind=ksngl)
  Call AddMarkerVectorField('${\bf u}\ {\rm[cm/\mu s]}$', 26, nkp(kf), &
                            avs(1,1), avs(1,2), avs(1,3))

! pressure p
  avg(1:nkp(kf),1) = Pp(1:nkp(kf),kf)*thousand
! Call MarkerAverage(avg(1,1),kf)
  avs(1:nkp(kf),1) = Real(avg(1:nkp(kf),1), kind=ksngl)
  Call AddMarkerScalarField('${\rm p\ [kbar]}$', 17, nkp(kf), avs(1,1))

! eps_p
  avg(1:nkp(kf),1) = star(1:nkp(kf),kf,1)
  Call MarkerAverage(avg(1,1),kf)
  avs(1:nkp(kf),1) = Real(avg(1:nkp(kf),1), kind=ksngl)
  Call AddMarkerScalarField('${\epsilon_p}$', 14, nkp(kf), avs(1,1))

! "Relative Porosity"
  avg(1:nkp(kf),1) = vp(1:nkp(kf),kf)/v_ref ! expansion eta = delta + 1
  Call MarkerAverage(avg(1,1),kf)
  Where(avg(1:nkp(kf),1) .gt. zero)
!   avg(1:nkp(kf),2) = one - one/avg(1:nkp(kf),1)
    avg(1:nkp(kf),2) = LOG(avg(1:nkp(kf),1))/avg(1:nkp(kf),1)
  ElseWhere
    avg(1:nkp(kf),2) = zero
  End Where
  avs(1:nkp(kf),2) = Real(avg(1:nkp(kf),2), kind=ksngl)
  Call AddMarkerScalarField('${\phi}\ \doteq\ \delta/\eta$', 37, nkp(kf),  avs(1,2))

! P_f
  avg(1:nkp(kf),1) = star(1:nkp(kf),kf,4)
  Call MarkerAverage(avg(1,1),kf)
  avs(1:nkp(kf),1) = Real(avg(1:nkp(kf),1), kind=ksngl)
  Call AddMarkerScalarField('$\rm\mathbb{P}_f$', 19, nkp(kf), avs(1,1))

! Wstar
! avg(1:nkp(kf),1) = 298.0_kreal + star(1:nkp(kf),kf,3)
  avg(1:nkp(kf),1) = star(1:nkp(kf),kf,3)
! Call MarkerAverage(avg(1,1),kf)
  avs(1:nkp(kf),1) = Real(avg(1:nkp(kf),1), kind=ksngl)
  Call AddMarkerScalarField('$W^\ast$', 8, nkp(kf), avs(1,1))

! P_s
  avg(1:nkp(kf),1) = star(1:nkp(kf),kf,2)
  Call MarkerAverage(avg(1,1),kf)
  avs(1:nkp(kf),1) = Real(avg(1:nkp(kf),1), kind=ksngl)
  Call AddMarkerScalarField('$\rm\mathbb{P}_s$', 19, nkp(kf), avs(1,1))

! P_p
  avg(1:nkp(kf),1) = star(1:nkp(kf),kf,5)
  Call MarkerAverage(avg(1,1),kf)
  avs(1:nkp(kf),1) = Real(avg(1:nkp(kf),1), kind=ksngl)
  Call AddMarkerScalarField('$\rm\mathbb{P}_p$', 19, nkp(kf), avs(1,1))

! c_m(kf)
  do kp = 1,nkp(kf)
    i = Ip(kp,kf,1) ;   j = Ip(kp,kf,2) ;   k = Ip(kp,kf,3)
    avg(kp,1) = Sqrt(Cm(i,j,k,m))
  enddo
! Call MarkerAverage(avg(1,1),kf)
  avs(1:nkp(kf),1) = Real(avg(1:nkp(kf),1), kind=ksngl)
  Call AddMarkerScalarField('${\rm c\ [cm/\mu s]}$', 21, nkp(kf), avs(1,1))

! <sig_m>, <sig_e>, <sig^star>
  do kp = 1,nkp(kf)
    sig_m = (sigp(kp,kf,1) + sigp(kp,kf,2) + sigp(kp,kf,3))/three
    sig_e = (sigp(kp,kf,1) - sig_m)**2 &
          + (sigp(kp,kf,2) - sig_m)**2 &
          + (sigp(kp,kf,3) - sig_m)**2 &
          + two*(sigp(kp,kf,4)**2 + sigp(kp,kf,5)**2 + sigp(kp,kf,6)**2)
    sig_e = Sqrt(three*sig_e/two)
    avg(kp,2) = sig_e*thousand
    avg(kp,1) = sig_m*thousand
!   if(sig_e .gt. thousandth) then
!     avg(kp,3) = sig_m/sig_e
!   else
!     avg(kp,3) = -two
!   endif
    avg(kp,3) = Max(-ten, Min(ten, (sig_m/(sig_e + fuzz))))
  enddo
  Call MarkerAverage(avg(1,1),kf)
  avs(1:nkp(kf),1) = Real(avg(1:nkp(kf),1), kind=ksngl)
  Call AddMarkerScalarField('${\sigma_m}\ {\rm [kbar]}$', 26, nkp(kf), avs(1,1))
  Call MarkerAverage(avg(1,2),kf)
  avs(1:nkp(kf),2) = Real(avg(1:nkp(kf),2), kind=ksngl)
  Call AddMarkerScalarField('${\sigma_e}\ {\rm [kbar]}$', 26, nkp(kf), avs(1,2))
  Call MarkerAverage(avg(1,3),kf)
  avs(1:nkp(kf),3) = Real(avg(1:nkp(kf),3), kind=ksngl)
  Call AddMarkerScalarField('${\sigma^\ast}$', 15, nkp(kf), avs(1,3))

! Call AddMarkerScalarField('mass', 4, nkp(kf), mp(1:nkp(kf),kf))
! Call AddMarkerScalarField('sie', 3, nkp(kf), ep(1:nkp(kf),kf))
! Call AddMarkerScalarField('pressure', 8, nkp(kf), Pp(1:nkp(kf),kf))
! Call AddMarkerTensorField('stress', 6, nkp(kf), &
!         sigp(1:nkp(kf),kf,1), sigp(1:nkp(kf),kf,2), sigp(1:nkp(kf),kf,3), &
!         sigp(1:nkp(kf),kf,4), sigp(1:nkp(kf),kf,5), sigp(1:nkp(kf),kf,6))
!enddo

  DEALLOCATE(avg)
  DEALLOCATE(avs)

enddo

30 continue ! output Grid data

  Where(.NOT. Mgc)
    Tmp = P*thousand
  ElseWhere
    Tmp = zero
  End Where

#ifdef DBL_PREC
  Tms = Real(Tmp, kind=ksngl)
#else
  Tms = Tmp
#endif

  var_label = 'Total Pressure [kbar]'
    Call AddGridField (var_label, LEN_TRIM(var_label), mx, my, mz, &
      my_id, Tms, .false.)

  Where(.NOT. Mgc)
    Tmp = D
  ElseWhere
    Tmp = zero
  End Where

#ifdef DBL_PREC
  Tms = Real(Tmp, kind=ksngl)
#else
  Tms = Tmp
#endif

  var_label = 'Total Density [g/cc]'
    Call AddGridField (var_label, LEN_TRIM(var_label), mx, my, mz, &
      my_id, Tms, .false.)

!  load SCL with the total material volume per unit grid Volume

SCL = zero
do m = 1,nmat-1
! Where(.NOT. Mgc .and. mc(:,:,:,m) .gt. zero)
  Where(.NOT. Mgc .and. Dm(:,:,:,m) .gt. zero)
    SCL = SCL + mc(:,:,:,m)/(Dm(:,:,:,m)*Vol)
  End Where
enddo

Do m = 1,nmat-1
  if(.NOT. CoProcessPV_Grid(m)) cycle

  kf = kfm(m)

    If (m .lt. 10) then
      Write (mtail,'(i1)') m
      mtail = '0'//mtail
    Else
      Write (mtail,'(i2)') m
    End If

  Tmp = zero
! Where(.NOT. Mgc .and. mc(:,:,:,m) .gt. zero)
  Where(.NOT. Mgc .and. Dm(:,:,:,m) .gt. zero)
    Tmp = (mc(:,:,:,m)/(Dm(:,:,:,m)*Vol))/Max(one,SCL)
  End Where

#ifdef DBL_PREC
  Tms = Real(Tmp, kind=ksngl)
#else
  Tms = Tmp
#endif

  var_label = 'Fraction [0-255] Mat - '//mtail
    Call AddGridField (var_label, LEN_TRIM(var_label), mx, my, mz, &
      my_id, Tms, .true.)

  var_label = 'Volume Fraction [-] Mat - '//mtail
    Call AddGridField (var_label, LEN_TRIM(var_label), mx, my, mz, &
      my_id, Tms, .false.)

  Where(.NOT. Mgc)
    Tmp = mc(:,:,:,m)
  ElseWhere
    Tmp = zero
  End Where

#ifdef DBL_PREC
  Tms = Real(Tmp, kind=ksngl)
#else
  Tms = Tmp
#endif

  var_label = 'M [g] Mat - '//mtail
    Call AddGridField (var_label, LEN_TRIM(var_label), mx, my, mz, &
      my_id, Tms, .false.)

  Where (.NOT. Mgc .and. Dm(:,:,:,m) .gt. zero)
    Tmp = one - Dm(:,:,:,m)/d0(m)
  ElseWhere
    Tmp = zero
  End Where

#ifdef DBL_PREC
  Tms = Real(Tmp, kind=ksngl)
#else
  Tms = Tmp
#endif

  var_label = '$\phi\ \doteq\ \delta/\eta$ Mat - '//mtail
    Call AddGridField (var_label, LEN_TRIM(var_label), mx, my, mz, &
      my_id, Tms, .false.)

  Where(.NOT. Mgc)
    Tmp = Dm(:,:,:,m)
  ElseWhere
    Tmp = zero
  End Where

#ifdef DBL_PREC
  Tms = Real(Tmp, kind=ksngl)
#else
  Tms = Tmp
#endif

! var_label = 'v [cc/g] Mat - '//mtail
  var_label = '$\rho^\circ$ [g/cc] Mat - '//mtail
    Call AddGridField (var_label, LEN_TRIM(var_label), mx, my, mz, &
      my_id, Tms, .false.)

  Where(.NOT. Mgc)
    Tmp = Pm(:,:,:,m)*thousand
  ElseWhere
    Tmp = zero
  End Where

#ifdef DBL_PREC
  Tms = Real(Tmp, kind=ksngl)
#else
  Tms = Tmp
#endif

  var_label = 'Pm [kbar] Mat - '//mtail
    Call AddGridField (var_label, LEN_TRIM(var_label), mx, my, mz, &
      my_id, Tms, .false.)

  Where(.NOT. Mgc)
    Tmp = Sqrt(Cm(:,:,:,m))
  ElseWhere
    Tmp = zero
  End Where

#ifdef DBL_PREC
  Tms = Real(Tmp, kind=ksngl)
#else
  Tms = Tmp
#endif

  var_label = 'Cm [cm/us] Mat - '//mtail
    Call AddGridField (var_label, LEN_TRIM(var_label), mx, my, mz, &
      my_id, Tms, .false.)

  if(loc_tm(m) .gt. 0) then
    Where(.NOT. Mgc)
      Tmp = Tm(:,:,:,loc_tm(m))
    ElseWhere
      Tmp = zero
    End Where
  else
    Where(.NOT. Mgc)
      Tmp = Sqrt(Cm(:,:,:,m))
    ElseWhere
      Tmp = zero
    End Where
  endif

#ifdef DBL_PREC
  Tms = Real(Tmp, kind=ksngl)
#else
  Tms = Tmp
#endif

  var_label = 'T [K] Mat - '//mtail
    Call AddGridField (var_label, LEN_TRIM(var_label), mx, my, mz, &
      my_id, Tms, .false.)

  Call Load_UC_m(uc_Tmp, m)

  Where(.NOT. Mgc)
    Tmp = uc_Tmp(:,:,:,1)
  ElseWhere
    Tmp = zero
  End Where

#ifdef DBL_PREC
  Tms = Real(Tmp, kind=ksngl)
#else
  Tms = Tmp
#endif

  var_label = 'u_x [cm/us] Mat - '//mtail
    Call AddGridField (var_label, LEN_TRIM(var_label), mx, my, mz, &
      my_id, Tms, .false.)

  Where(.NOT. Mgc)
    Tmp = uc_Tmp(:,:,:,2)
  ElseWhere
    Tmp = zero
  End Where

#ifdef DBL_PREC
  Tms = Real(Tmp, kind=ksngl)
#else
  Tms = Tmp
#endif

  var_label = 'u_y [cm/us] Mat - '//mtail
    Call AddGridField (var_label, LEN_TRIM(var_label), mx, my, mz, &
      my_id, Tms, .false.)

  Where(.NOT. Mgc)
    Tmp = uc_Tmp(:,:,:,3)
  ElseWhere
    Tmp = zero
  End Where

#ifdef DBL_PREC
  Tms = Real(Tmp, kind=ksngl)
#else
  Tms = Tmp
#endif

  var_label = 'u_z [cm/us] Mat - '//mtail
    Call AddGridField (var_label, LEN_TRIM(var_label), mx, my, mz, &
      my_id, Tms, .false.)

End Do ! Do m = 1,nmat-1

 CPtime = DBLE(nfilet) ! XML readers and writers work fine.  "time" is identical to nfilet and all Group files are read
!CPtime = DBLE(t) ! XML writers fail to update the time after initialization, which confuses XML readers.
Call RequestDataDescription(nfilet, CPtime, doprocess)
Call CoProcess()

if (my_id .eq. 0) then
  INQUIRE(file='gp.tcsh', exist=file_exists)
    if(file_exists) then
      Write(mfilet,'(i6)') nfilet
      Call Execute_Command_Line('gp.tcsh '//mfilet, exitstat=i)
      print *, "Exit status of gp.tcsh was ", i
  endif
endif

call get_wtime(sec1)

if(my_id .eq. 0) then
  elapsed = sec1 - sec0
  write(*,*) 'CoProcessPV ', nfilet, ' done. ', elapsed, ' sec'
  write(2,*) 'CoProcessPV ', nfilet, ' done. ', elapsed, ' sec'
endif
!==============================================================================
End Subroutine CoProcessPV
