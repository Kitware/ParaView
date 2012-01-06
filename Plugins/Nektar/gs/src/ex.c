
void
ssgl_radd(register REAL *vals, register REAL *work, register int level,
    register int *segs)
{
  register int edge, type, dest, source, mask;
  register int stage_n;


#ifdef DEBUG
  if (level > i_log2_num_nodes)
    {error_msg_fatal("sgl_add() :: level > log_2(P)!!!");}
#endif


#ifdef NXSRC
  /* all msgs are *NOT* the same length */
  /* implement the mesh fan in/out exchange algorithm */
  for (mask=0, edge=0; edge<level; edge++, mask++)
    {
      stage_n = (segs[level] - segs[edge]);
      if (stage_n && !(my_id & mask))
  {
    dest = edge_node[edge];
    type = MSGTAG3 + my_id + (num_nodes*edge);
    if (my_id>dest)
      {csend(type, vals+segs[edge],stage_n*REAL_LEN,dest,0);}
    else
      {
        type =  type - my_id + dest;
        crecv(type,work,stage_n*REAL_LEN);
        rvec_add(vals+segs[edge], work, stage_n);
/*            daxpy(vals+segs[edge], work, stage_n); */
      }
  }
      mask <<= 1;
    }
  mask>>=1;
  for (edge=0; edge<level; edge++)
    {
      stage_n = (segs[level] - segs[level-1-edge]);
      if (stage_n && !(my_id & mask))
  {
    dest = edge_node[level-edge-1];
    type = MSGTAG6 + my_id + (num_nodes*edge);
    if (my_id<dest)
      {csend(type,vals+segs[level-1-edge],stage_n*REAL_LEN,dest,0);}
    else
      {
        type =  type - my_id + dest;
        crecv(type,vals+segs[level-1-edge],stage_n*REAL_LEN);
      }
  }
      mask >>= 1;
    }
#endif
}



***********************************comm.c*************************************/
void
grop_hc_vvl(REAL *vals, REAL *work, int *segs, int *oprs, int dim)
{
  register int mask, edge, n;
  int type, dest;
  vfp fp;
#if defined MPISRC
  MPI_Status  status;
#elif defined NXSRC
  int len;
#endif


  error_msg_fatal("grop_hc_vvl() :: is not working!\n");

#ifdef SAFE
  /* ok ... should have some data, work, and operator(s) */
  if (!vals||!work||!oprs||!segs)
    {error_msg_fatal("grop_hc() :: vals=%d, work=%d, oprs=%d",vals,work,oprs);}

  /* non-uniform should have at least two entries */
  if ((oprs[0] == NON_UNIFORM)&&(n<2))
    {error_msg_fatal("grop_hc() :: non_uniform and n=0,1?");}

  /* check to make sure comm package has been initialized */
  if (!p_init)
    {comm_init();}
#endif

  /* if there's nothing to do return */
  if ((num_nodes<2)||(dim<=0))
    {return;}

  /* the error msg says it all!!! */
  if (modfl_num_nodes)
    {error_msg_fatal("grop_hc() :: num_nodes not a power of 2!?!");}

  /* can't do more dimensions then exist */
  dim = MIN(dim,i_log2_num_nodes);

  /* advance to list of n operations for custom */
  if ((type=oprs[0])==NON_UNIFORM)
    {oprs++;}

  if ((fp = (vfp) rvec_fct_addr(type)) == NULL)
    {
      error_msg_warning("grop_hc() :: hope you passed in a rbfp!\n");
      fp = (vfp) oprs;
    }

#if  defined NXSRC
  /* all msgs are *NOT* the same length */
  /* implement the mesh fan in/out exchange algorithm */
  for (mask=1,edge=0; edge<dim; edge++,mask<<=1)
    {
      n = segs[dim]-segs[edge];
      /*
      error_msg_warning("n=%d, segs[%d]=%d, segs[%d]=%d\n",n,dim,segs[dim],
      edge,segs[edge]);
      */
      len = n*REAL_LEN;
      dest = my_id^mask;
      if (my_id > dest)
  {csend(MSGTAG2+my_id,(char *)vals+segs[edge],len,dest,0); break;}
      else
  {crecv(MSGTAG2+dest,(char *)work,len); (*fp)(vals+segs[edge], work, n, oprs);}
    }

  if (edge==dim)
    {mask>>=1;}
  else
    {while (++edge<dim) {mask<<=1;}}

  for (edge=0; edge<dim; edge++,mask>>=1)
    {
      if (my_id%mask)
  {continue;}
      len = (segs[dim]-segs[dim-1-edge])*REAL_LEN;
      /*
      error_msg_warning("n=%d, segs[%d]=%d, segs[%d]=%d\n",n,dim,segs[dim],
                         dim-1-edge,segs[dim-1-edge]);
      */
      dest = my_id^mask;
      if (my_id < dest)
  {csend(MSGTAG4+my_id,(char *)vals+segs[dim-1-edge],len,dest,0);}
      else
  {crecv(MSGTAG4+dest,(char *)vals+segs[dim-1-edge],len);}
    }

#elif defined MPISRC
  for (mask=1,edge=0; edge<dim; edge++,mask<<=1)
    {
      n = segs[dim]-segs[edge];
      dest = my_id^mask;
      if (my_id > dest)
  {MPI_Send(vals+segs[edge],n,REAL_TYPE,dest,MSGTAG2+my_id,MPI_COMM_WORLD);}
      else
  {
    MPI_Recv(work,n,REAL_TYPE,MPI_ANY_SOURCE,MSGTAG2+dest,
       MPI_COMM_WORLD, &status);
    (*fp)(vals+segs[edge], work, n, oprs);
  }
    }

  if (edge==dim)
    {mask>>=1;}
  else
    {while (++edge<dim) {mask<<=1;}}

  for (edge=0; edge<dim; edge++,mask>>=1)
    {
      if (my_id%mask)
  {continue;}

      n = (segs[dim]-segs[dim-1-edge]);

      dest = my_id^mask;
      if (my_id < dest)
  {MPI_Send(vals+segs[dim-1-edge],n,REAL_TYPE,dest,MSGTAG4+my_id,
      MPI_COMM_WORLD);}
      else
  {
    MPI_Recv(vals+segs[dim-1-edge],n,REAL_TYPE,MPI_ANY_SOURCE,
       MSGTAG4+dest,MPI_COMM_WORLD, &status);
  }
    }
#else
  return;
#endif
}
