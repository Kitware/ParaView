#include "isoutils.h"

double SQ_PNT_TOL=1e-8;

// define == if point is within 1e-4
bool operator == (const Vertex& x, const Vertex& y)
{
  return ((x.m_x-y.m_x)*(x.m_x-y.m_x) + (x.m_y-y.m_y)*(x.m_y-y.m_y) +
    (x.m_z-y.m_z)*(x.m_z-y.m_z) < SQ_PNT_TOL)? 1:0;
}

// define != if point is outside 1e-4
bool operator != (const Vertex& x, const Vertex& y)
{
  return ((x.m_x-y.m_x)*(x.m_x-y.m_x) + (x.m_y-y.m_y)*(x.m_y-y.m_y) +
    (x.m_z-y.m_z)*(x.m_z-y.m_z) < SQ_PNT_TOL)? 0:1;
}

void set_SQ_PNT_TOL(double val)
{
    fprintf(stderr,"Resetting points separation to %lg\n",val);
    SQ_PNT_TOL = val;
}

void Iso::condense(){
  register int i,j,k,cnt;
  int nvi, id[3];
  Vertex v;
  vector<Vertex> vert;
  vector<Vertex>::iterator pt;

  if(m_condensed) return;
  m_condensed = 1;

  if(!m_ntris) return;

  vert.reserve(m_ntris/6);

  m_vid = imatrix(0,m_ntris-1,0,2);

  // fill first 3 points
  for(cnt =1, i = 0; i < 3; ++i){
    v.m_x = m_x[i];
    v.m_y = m_y[i];
    v.m_z = m_z[i];
    v.m_id = cnt;
    vert.push_back(v);
    m_vid[0][i] = v.m_id;

    ++cnt;
  }

  for(i = 1; i < m_ntris; ++i){
    for(j = 0; j < 3; ++j){
      v.m_x = m_x[3*i+j];
      v.m_y = m_y[3*i+j];
      v.m_z = m_z[3*i+j];

      pt = find(vert.begin(),vert.end(),v);
      if(pt != vert.end()){
  m_vid[i][j] = pt[0].m_id;
      }
      else{
  v.m_id = cnt;
  vert.push_back(v);

  m_vid[i][j] = v.m_id;
  ++cnt;
      }
    }
  }

  // remove multiple vertices
  for(i = 0,cnt=0; i < m_ntris;)
    if((m_vid[i][0]==m_vid[i][1])||(m_vid[i][0]==m_vid[i][2])||(m_vid[i][1]==m_vid[i][2])){
      cnt++;
      for(j =i; j < m_ntris-1; ++j)
  icopy(3,m_vid[j+1],1,m_vid[j],1);
      m_ntris--;
    }
    else
      ++i;

  // recast x,y.z;
  free(m_x); free(m_y); free(m_z);

  m_nvert  = vert.size();
  m_x  = dvector(0,m_nvert-1);
  m_y  = dvector(0,m_nvert-1);
  m_z  = dvector(0,m_nvert-1);

  for(i = 0; i < m_nvert; ++i){
    m_x[i] = vert[i].m_x;
    m_y[i] = vert[i].m_y;
    m_z[i] = vert[i].m_z;
  }
}

int same(double x1,double y1,double z1,double x2,double y2,double z2){
  int retval = 0;

  if((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2) < SQ_PNT_TOL)
    retval = 1;

  return retval;
}

void Iso::globalcondense(int n_iso, Iso **iso, Element_List *U){
  register int i,j;
  int  id,eid,nvert;
  int nel=U->nel,nelmt;
  Vertex v;
  vector <Vertex> vert;
  vector <Vertex>::iterator pt;
  int id1,id2, pt1,pt2,loc;
  int  **vidmap,*elmtid;
  Element *E;
  Edge *ed;

  if(m_condensed) return;
  m_condensed = 1;

  vidmap = (int **) calloc(nel,sizeof(int *));
  elmtid = ivector(0,nel-1);

  m_ntris = 0;
  for(i = 0; i < n_iso; ++i)
    if(iso[i]->m_ntris) m_ntris += iso[i]->m_ntris;

  m_vid = imatrix(0,m_ntris-1,0,2);

  m_nvert = 0;
  for(i = 0; i < n_iso; ++i)
    if(iso[i]->m_ntris) m_nvert += iso[i]->m_nvert;

  vert.reserve(m_nvert/4);
  id = 0; // find first non-zero iso
  while(!iso[id]->m_ntris) id++;

  //load all vertices from first iso
  icopy(3*iso[id]->m_ntris,iso[id]->m_vid[0],1,m_vid[0],1);

  vidmap[id] = ivector(0,iso[id]->m_nvert-1);

  for(i = 0; i < iso[id]->m_nvert; ++i){
    v.m_x = iso[id]->m_x[i];
    v.m_y = iso[id]->m_y[i];
    v.m_z = iso[id]->m_z[i];
    v.m_id = i;
    vert.push_back(v);

    vidmap[id][i] = i+1;
  }

  nvert = iso[id]->m_nvert;
  nelmt = iso[id]->m_ntris;

  // work out an elmtent mapping
  int *elmt_map = ivector(0,nel-1);

  if(iso[0]->m_eid)
  {
      ifill(nel,-1,elmt_map,1);
      for(i = 0; i < n_iso; ++i)
      {
    elmt_map[iso[i]->get_eid()-1] = i;
      }
  }
  else
  {
      for(i = 0; i < nel; ++i)
      {
    elmt_map[i] = i;
      }
  }


  for(id = 1; id < n_iso; ++id){
      E = U->flist[iso[id]->get_eid()-1];
      if(iso[id]->m_ntris){
    vidmap[id] = ivector(0,iso[id]->m_nvert-1);
    ifill(iso[id]->m_nvert,-1,vidmap[id],1);

    // idenitfy elements surrounding edges
    izero(nel,elmtid,1);
    for(i = 0; i < E->Nedges; ++i)
    {
        for(ed = E->edge[i].base; ed; ed = ed->link)
        {
      if(elmt_map[eid = ed->eid] < id)
      {
          elmtid[eid] = 1;
      }
        }
    }

    for(i = 0; i < nel; ++i)
        if(elmtid[i])
        {
      if((loc = elmt_map[i])+1)
      {

          for(id1 = 0; id1 < iso[id]->m_nvert;++id1)
          {
        for(id2 = 0; id2 < iso[loc]->m_nvert;++id2)
        {
            pt1 = id1;
            pt2 = id2;
            if(same(iso[id]->m_x[pt1],iso[id]->m_y[pt1],
              iso[id]->m_z[pt1],iso[loc ]->m_x[pt2],
              iso[loc ]->m_y[pt2],iso[loc ]->m_z[pt2]))
            {
          vidmap[id][pt1] = vidmap[loc][pt2];
            }
        }
          }
      }
        }

    for(i = 0; i < iso[id]->m_nvert; ++i){
        if(!(vidmap[id][i]+1)){
      v.m_x = iso[id]->m_x[i];
      v.m_y = iso[id]->m_y[i];
      v.m_z = iso[id]->m_z[i];
      v.m_id = ++nvert ;
      vert.push_back(v);
      vidmap[id][i] = v.m_id;
        }
    }

    for(i = 0; i < iso[id]->m_ntris; ++i,nelmt++){
        for(j=0; j < 3;++j)
      m_vid[nelmt][j] = vidmap[id][iso[id]->m_vid[i][j]-1];
    }
      }
  }

  if(nelmt != m_ntris) cerr << "error in globalconsense" << endl;
  if(nvert != vert.size()) cerr << "error in globalconsense" << endl;

  m_nvert = vert.size();

  m_x = dvector(0,m_nvert-1);
  m_y = dvector(0,m_nvert-1);
  m_z = dvector(0,m_nvert-1);

  for(i = 0; i < m_nvert; ++i){
    m_x[i] = vert[i].m_x;
    m_y[i] = vert[i].m_y;
    m_z[i] = vert[i].m_z;
  }

  free(elmt_map);
  free(elmtid);
  for(i = 0; i < n_iso; ++i)
    if(vidmap[i]) free(vidmap[i]);
  free(vidmap);
}

void Iso::separate_regions(void){
  int i,j,k,id;
  vector<int> *vertcon;
  vector<int>::iterator ipt;
  list<int> tocheck;
  list<int>::iterator cid;
  int *viddone = ivector(0,m_nvert-1);

  vertcon = new vector<int>[m_nvert];

  for(i = 0; i < m_ntris; ++i)
    for(j = 0; j < 3; ++j)
      vertcon[m_vid[i][j]-1].push_back(i);

  m_vidregion = ivector(0,m_nvert-1);


  // since it is possible to have extra zeros, just zero out used vertices
  ifill(m_nvert,-1,m_vidregion,1);
  for(i =0 ; i < m_ntris; ++i)
    for(j = 0; j < 3; ++j)
      m_vidregion[m_vid[i][j]-1] = 0;

  izero(m_nvert,viddone,1);

  m_nregions = 0;

  // check all points are assigned;
  for(k = 0; k < m_nvert; ++k)
    if(m_vidregion[k] == 0){

      m_vidregion[k] = ++m_nregions;

      // find all elmts around this.. vertex  that need to be checked
      for(ipt = vertcon[k].begin(); ipt != vertcon[k].end(); ++ipt)
  for(i = 0; i < 3; ++i)
    if(m_vidregion[id = (m_vid[ipt[0]][i]-1)] == 0){
       tocheck.push_back(id);
       m_vidregion[id] = m_nregions;
    }
      viddone[k] = 1;

      // check all other neighbouring vertices
      while(tocheck.size()){

  cid = tocheck.begin();
  while(cid != tocheck.end()){
    if(!viddone[*cid]){
      for(ipt = vertcon[*cid].begin(); ipt != vertcon[*cid].end(); ++ipt){
        for(i = 0; i < 3; ++i){
    if(m_vidregion[id = (m_vid[ipt[0]][i]-1)] == 0){
      tocheck.push_back(id);
      m_vidregion[id] = m_nregions;
    }
        }
      }
      viddone[*cid] = 1;
      ++cid;
      tocheck.pop_front();
    }
    }
      }
    }
  free(viddone);
  delete[] vertcon;
}


void Iso::write(std::ostream &out, double val, int mincells){
  int i,j,nreg;

  if(m_nregions){
    int *vidmap = ivector(0,m_nvert-1);

    for(nreg = 1; nreg <= m_nregions; ++nreg){
      int cnt;
      int cnt_nv = 0;
      int cnt_elmt = 0;

      // count vertices;
      for(i = 0; i < m_nvert; ++i)
  if(m_vidregion[i] == nreg)
    ++cnt_nv;

      // count elmt;
      for(i = 0; i < m_ntris; ++i)
  if(m_vidregion[m_vid[i][0]-1] == nreg)
      ++cnt_elmt;

  if(cnt_elmt > mincells){
    out << "ZONE T=\"Countour ("<<val <<") Elmt=" << m_eid << "\""
        << endl;
    out <<  "N="<< cnt_nv << ", E="<< cnt_elmt <<
      ", F=FEPOINT, ET=TRIANGLE" <<endl;

    cnt = 1;
    for(i = 0; i < m_nvert; ++i){
      if(m_vidregion[i] == nreg){
        out.width(10);
        out  << m_x[i] << " ";
        out.width(10);
        out  << m_y[i] << " ";
        out.width(10);
        out  << m_z[i] << " ";
        out  << " " << val <<  endl;

        vidmap[i] = cnt++;
      }
    }

    for (i=0; i < m_ntris; ++i)
      if(m_vidregion[m_vid[i][0]-1] == nreg){
        for(j = 0; j < 3; ++j){
    out.width(6);
    out << vidmap[m_vid[i][j]-1] << " ";
        }
        out << endl;
      }
  }
    }
    free(vidmap);
  }
  else{
    if(m_ntris > mincells){
      out << "ZONE T=\"Countour ("<<val <<") Elmt=" << m_eid << "\""
    << endl;
      out <<  "N="<< m_nvert << ", E="<< m_ntris <<
  ", F=FEPOINT, ET=TRIANGLE" <<endl;

      for(i = 0; i < m_nvert; ++i){
  out.width(10);
  out  << m_x[i] << " ";
  out.width(10);
  out  << m_y[i] << " ";
  out.width(10);
  out  << m_z[i] << " ";
  out  << " " << val <<  endl;
      }

      for (i=0; i < m_ntris; ++i){
  for(j = 0; j < 3; ++j){
    out.width(6);
    out << m_vid[i][j] << " ";
  }
  out << endl;
      }
    }
  }
}



void Iso::readzone(std::ifstream &in , double &val){
  int i,j;
  char buf[256];

  in.getline(buf,256) ;

  sscanf(buf,"N=%d, E=%d, F=FEPOINT, ET=TRIANGLE\n",  &m_nvert, &m_ntris);

  m_x   = dvector(0,m_nvert-1);
  m_y   = dvector(0,m_nvert-1);
  m_z   = dvector(0,m_nvert-1);

  m_vid = imatrix(0,m_ntris-1,0,2);

  for(i = 0; i < m_nvert; ++i){
    in >> m_x[i];
    in >> m_y[i];
    in >> m_z[i];
    in >> val;
  }

  for (i=0; i < m_ntris; ++i)
    for(j = 0; j < 3; ++j)
      in >> m_vid[i][j];
}


void Iso::smooth(int n_iter, double lambda, double mu){
  int   iter,i,j;
  double v[3],del_v[3];
  double w;
  double *xtemp, *ytemp, *ztemp;
  vector<int> *adj;
  vector<int>::iterator iad;
  vector<int> *vertcon;
  vector<int>::iterator ipt;

  // determine elements around each vertex
  vertcon = new vector<int>[m_nvert];
  for(i = 0; i < m_ntris; ++i)
    for(j = 0; j < 3; ++j)
      vertcon[m_vid[i][j]-1].push_back(i);

  // determine vertices around each vertex
  adj = new vector<int>[m_nvert];
  for(i =0; i < m_nvert; ++i){
    for(ipt = vertcon[i].begin(); ipt != vertcon[i].end(); ++ipt)
      for(j = 0; j < 3; ++j)
      {
    // make sure not adding own vertex
    if(m_vid[*ipt][j] != i+1)
    {
        // check to see if vertex has already been added
        for(iad = adj[i].begin(); iad != adj[i].end(); ++iad)
      if(*iad == (m_vid[*ipt][j]-1)) break;
        if(iad == adj[i].end())
      adj[i].push_back(m_vid[*ipt][j]-1);
    }
      }
  }

  xtemp = new double [m_nvert];
  ytemp = new double [m_nvert];
  ztemp = new double [m_nvert];

  // smooth each point

  for (iter=0;iter<n_iter;iter++) {

    // compute first weighted average

    for(i=0;i< m_nvert;++i) {
      w = 1.0/(double)(adj[i].size());

      del_v[0] = del_v[1] = del_v[2] = 0.0;

      for(iad = adj[i].begin(); iad != adj[i].end(); ++iad){
  del_v[0] =  del_v[0] + (m_x[*iad]-m_x[i])*w;
  del_v[1] =  del_v[1] + (m_y[*iad]-m_y[i])*w;
  del_v[2] =  del_v[2] + (m_z[*iad]-m_z[i])*w;
      }

      xtemp[i] = m_x[i] + del_v[0] * lambda;
      ytemp[i] = m_y[i] + del_v[1] * lambda;
      ztemp[i] = m_z[i] + del_v[2] * lambda;
    }

    // compute second weighted average

    for(i=0;i< m_nvert;++i) {

  w = 1.0/(double)(adj[i].size());
  del_v[0] = del_v[1] = del_v[2] = 0;

  for(iad = adj[i].begin(); iad != adj[i].end(); ++iad){
    del_v[0] =  del_v[0] + (xtemp[*iad]-xtemp[i])*w;
    del_v[1] =  del_v[1] + (ytemp[*iad]-ytemp[i])*w;
    del_v[2] =  del_v[2] + (ztemp[*iad]-ztemp[i])*w;
  }

  m_x[i] = xtemp[i] + del_v[0] * mu;
  m_y[i] = ytemp[i] + del_v[1] * mu;
  m_z[i] = ztemp[i] + del_v[2] * mu;
    }
  }

  delete[] xtemp;
  delete[] ytemp;
  delete[] ztemp;

  delete[] vertcon;
  delete[] adj;
}
