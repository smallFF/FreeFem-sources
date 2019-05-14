// ORIG-DATE:     Dec 2007
// -*- Mode : c++ -*-
//
// SUMMARY  :  Model  mesh 3d
// USAGE    : LGPL
// ORG      : LJLL Universite Pierre et Marie Curi, Paris,  FRANCE
// AUTHOR   : Frederic Hecht
// E-MAIL   : frederic.hecht@ann.jussieu.fr
//

/*
  This file is part of Freefem++

  Freefem++ is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  Freefem++  is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Freefem++; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Thank to the ARN ()  FF2A3 grant
  ref:ANR-07-CIS7-002-01
*/


#include <fstream>
#include <iostream>
#include <cstring>
#include "libmesh5.h"
#include "ufunction.hpp"
#include "error.hpp"
#include "RNM.hpp"
namespace Fem2D
{
}
#include "Mesh2dn.hpp"
#include "Mesh3dn.hpp"
#include "rgraph.hpp"
#include "fem.hpp"
#include "PlotStream.hpp"

namespace Fem2D
{




  /*
    const short int v_tet_face[4][3]=  {{3,2,1},{0,2,3},{ 3,1,0},{ 0,1,2}};
    const short int a_tet_face[4][3]=  {{ 0,1,0},{ 0,2,0},{ 0,3,1},{ 1,2,1}};
    const bool  sig_tet_face[4][3]=  {{ 0,1,0},{ 1,0,1},{ 0,1,0},{ 1,0,1}};
    const short int v_tet_edge[6][2]= {{ 1,2},{1,3},{1,4},{2,3},{2,4},{3,4}};
    const short int fadj_tet_edge[6][2]= {{4,3},{2,4},{3,2},{4,1},{1,3},{2,1}};
    const short int op_tet_edge[6]={ 6, 5, 4, 3, 2, 1};
  */

  //  Attention  nvfaceTet  donnne les faces  les 4 faces de tet telle que la
  // tel que  le tet forme des trois sommet  + l'autre sommet soit positif.
  //  donc  le  produit vectoriel des 2 aretes  (0,1) (0,2)  donne une  normale interieur.
  //  Ok pour les gradients des $\lambda_i$

  // definition of the reference tetrahedron 0 1 2 3
  static const int  nvfaceTet[4][3]  ={{3,2,1}, {0,2,3},{ 3,1,0},{ 0,1,2}}  ;
  static const int  nvedgeTet[6][2] = { {0,1},{0,2},{0,3},{1,2},{1,3},{2,3} };
  // definition of the reference triangle 0 1 2
  static const int  nvfaceTria[1][3]  = { {0,1,2} };
  static const int  nvedgeTria[3][2] = { {1,2},{2,0},{0,1}};
  // definition of the reference segment 0 1
  static const int  nvfaceSeg[1][3]  = {{-1,-1,1}};
  static const int  nvedgeSeg[1][2] = { {0,1} };
  static const int  nvadjSeg[2][1] = { {0},{1} };

  // geometry element for Triangle ( triangle for boudary elements in volume mesh, Rd=3 RdHat=2 )
  template<>
  const int (* const GenericElement<DataTriangle3>::nvface)[3] = nvfaceTria ;
  template<>
  const int (* const GenericElement<DataTriangle3>::nvedge)[2] = nvedgeTria ;
  template<>
  const int (* const GenericElement<DataTriangle3>::nvadj)[2] = nvedgeTria ;
  template<> const int  GenericElement<DataTriangle3>::nitemdim[4] = {3,3,1,0 }  ;
  static const int onWhatIsEdge3[3][7] = {
    {0,1,3, 2,0,0, 0}, // edge 0
    {3,0,1, 0,2,0, 0},
    {1,3,0, 0,0,2, 0} };
  template<>
  const int (* const GenericElement<DataTriangle3>::onWhatBorder)[7] = onWhatIsEdge3 ;

  // geometry element for Tetrahedron ( volume elements in 3d mesh, Rd=3 RdHat=3 )
  template<>
  const int (* const GenericElement<DataTet>::nvface)[3] = nvfaceTet ;
  template<>
  const int (* const GenericElement<DataTet>::nvedge)[2] = nvedgeTet ;
  template<>
  const int (* const GenericElement<DataTet>::nvadj)[3] = nvfaceTet ;
  template<> const int  GenericElement<DataTet>::nitemdim[4] = {4,6,4,1 }  ;

  int onWhatIsFace[4][15] ;
  typedef const int   (*const PtrConst15int) [15]; //  a pointeur on  const arry of 15 int. (to help xcode)
  // static const int (* const SetonWhatIsFace(int  onWhatIsFace[4][15] ,const int  nvfaceTet[4][3],const int nvedgeTet[6][2]))[15];
  static PtrConst15int SetonWhatIsFace(int  onWhatIsFace[4][15] ,const int  nvfaceTet[4][3],const int nvedgeTet[6][2]);
  template<>
  const int (* const GenericElement<DataTet>::onWhatBorder)[15] = SetonWhatIsFace(onWhatIsFace,nvfaceTet,nvedgeTet) ;


  // geometry element for segment ( boundary elements in surface mesh, Rd=3 RdHat=1 )
  template<> const int (* const GenericElement<DataSeg3>::nvface)[3] = 0 ;
  template<> const int (* const GenericElement<DataSeg3>::nvedge)[2] = nvedgeSeg; //nvedgeTria ;
  template<> const int (* const GenericElement<DataSeg3>::nvadj)[1] = nvadjSeg ;


  // template for Mesh3 ( volume mesh )
  template<> int   GenericMesh<Tet,Triangle3,Vertex3>::kfind=0;
  template<> int   GenericMesh<Tet,Triangle3,Vertex3>::kthrough=0;

  //template for MeshS ( surface mesh )
  template<> int   GenericMesh<TriangleS,BoundaryEdgeS,Vertex3>::kfind=0;
  template<> int   GenericMesh<TriangleS,BoundaryEdgeS,Vertex3>::kthrough=0;


  //  const int (* const SetonWhatIsFace(int  onWhatIsFace[4][15] ,const int  nvfaceTet[4][3],const int nvedgeTet[6][2]))[15]
  PtrConst15int  SetonWhatIsFace(int  onWhatIsFace[4][15] ,const int  nvfaceTet[4][3],const int nvedgeTet[6][2])
  {
    for(int i=0;i<15;++i)
      for(int j=0;j<4;++j)
        onWhatIsFace[j][i]=0;
    for(int j=0;j<4;++j)
      for(int i=0;i<3;++i)
        onWhatIsFace[j][nvfaceTet[j][i]]=1;
    for(int j=0;j<4;++j)
      {
        onWhatIsFace[j][j+4+6]=3;
        int ff[]={0,0,0,0};
        int jo=1+2+3-nvfaceTet[j][0]-nvfaceTet[j][1]-nvfaceTet[j][2];
        ff[jo]=1;
        for(int i=0;i<6;++i)
          if(ff[nvedgeTet[i][0]]+ff[nvedgeTet[i][1]]==0)
            onWhatIsFace[j][i+4]=2;
      }
    if(0)
      for(int j=0;j<4;++j)
        {
          for(int i=0;i<15;++i)
            cout << onWhatIsFace[j][i] << " ";
          cout << endl;
        }

    return onWhatIsFace;
  }

  void Add(int *p,int n,int o)
  {
    for(int i=0;i<n;++i)
      p[i] += o;
  }

  // constructor of the class Mesh3

  Mesh3::Mesh3(const string  filename)
    :meshS(0)
  {
    int ok=load(filename);
    if(verbosity) {
      cout << "read mesh ok " << ok ;
      if (typeMesh3==0) cout << " ---- pure surface Mesh"<< endl;
      else if (typeMesh3==1) cout << " ---- pure volume Mesh"<< endl;
      else if (typeMesh3==2) cout << " ---- volume and surface Meshes"<< endl;
      if (typeMesh3!=0)
        cout << "volume Mesh, num Tetra:= " << nt << ", num Vertice:= " << nv << " num boundary Triangles:= " << nbe << endl;
      if (typeMesh3!=1)
        cout << "surface Mesh, num Triangles:= " << this->meshS->nt << ", num Vertice:= " << this->meshS->nv << " num boundary Edges:= " << this->meshS->nbe << endl;
    }

    if (ok) {
      ifstream f(filename.c_str());
      if(!f) {
	cerr << "  --  Mesh3::Mesh3 Erreur openning " << filename<<endl;ffassert(0);exit(1);}
      if(verbosity>2)
	cout << "  -- Mesh3:  Read On file \"" <<filename<<"\""<<  endl;
      if(filename.rfind(".msh")==filename.length()-4) {
	//readmsh(f,-1); // TODO with surface, detection based on number of vertice to build one elt
	cout << "caution, surface mesh isn't available with the format .msh" << endl;
	ffassert(0);
      }
      //   else
      read(f); // TODO with surface,
    }


    if(typeMesh3!=0 ) {
      BuildBound();
      if(nt > 0){
	BuildAdj();
	Buildbnormalv();
	BuildjElementConteningVertex();
      }
    }
    if(typeMesh3!=1 ) {
      meshS->BuildBound();
      if(meshS->nt > 0){
	meshS->BuildAdj();
	meshS->Buildbnormalv();
	meshS->BuildjElementConteningVertex();
      }
    }
    if(verbosity>2) {
      if(typeMesh3!=0 ) cout << "  -- End of read: Mesh3 mesure = " << mes << " border mesure " << mesb << endl;
      if(typeMesh3!=1 ) cout << "  -- End of read: MeshS mesure = " << meshS->mes << " border mesure " << meshS->mesb << endl;
    }
    if(verbosity) {
      if(typeMesh3!=0 ) cout << "  -- Mesh3 : "<<filename  << ", space dimension "<< 3  << ", num Tetrahedron elts " << nt << ", num Vertice "
			     << nv << " num Bordary elts " << nbe << endl;
      if(typeMesh3!=1 ) cout << "  -- Mesh3:MeshS : "<<filename  << ", space dimension "<< 3  << ", num Triangle elts " << meshS->nt << ", num Vertice "
			     << meshS->nv << " num Bordary elts " << meshS->nbe << endl;
    }
    if(typeMesh3!=0 ) ffassert(mes>=0); // add F. Hecht sep 2009.
    if(typeMesh3!=1 ) ffassert(meshS->mes>=0); // add F. Hecht sep 2009.

  }


  // Read a mesh with correct the mesh :
  // 1) delete multiple points defined
  // 2) delete points which is not in element or in border element
  Mesh3::Mesh3(const string  filename, const long change)
    :meshS(0)
  {

    int ok=load(filename);     // 1 fixed for the moment, just initialize a mesh3, meshS isn't used
    if(verbosity) {
      cout << "read mesh ok " << ok  << endl;
      cout << ", nt " << nt << ", nv " << nv << " nbe:  = " << nbe << endl;
    }
    if(ok)
      {
        ifstream f(filename.c_str());
        if(!f) {
          cerr << "  --  Mesh3::Mesh3 Erreur openning " << filename<<endl;ffassert(0);exit(1);}
        if(verbosity>2)
          cout << "  -- Mesh3:  Read On file \"" <<filename<<"\""<<  endl;
        if(filename.rfind(".msh")==filename.length()-4)
	  readmsh(f,-1);
	else
	  read(f);
      }

    if(change){
      // verification multiple points
      double hseuil=hmin();
      hseuil = hseuil/10;
      cout << " hseuil = " << hseuil << endl;
      KN<int> Numero_Som(this->nv);
      Vertex *vv=new Vertex[this->nv];
      int nv_t=0;
      {
        R3 Pinf(1e100,1e100,1e100),Psup(-1e100,-1e100,-1e100);
        for (int ii=0;ii< this->nv;ii++){
          R3 P( vertices[ii].x, vertices[ii].y, vertices[ii].z);
          Pinf=Minc(P,Pinf);
          Psup=Maxc(P,Psup);
        }
        EF23::GTree<Vertex3> *gtree = new EF23::GTree<Vertex3>(vv,Pinf,Psup,0);
        // creation of octree
        for (int ii=0;ii<this->nv;ii++){
          const R3 r3vi( this->vertices[ii].x, this->vertices[ii].y, this->vertices[ii].z );
          const Vertex3 &vi(r3vi);

          Vertex3 * pvi=gtree->ToClose(vi,hseuil);

          if(!pvi){
            vv[nv_t].x = vi.x;
            vv[nv_t].y = vi.y;
            vv[nv_t].z = vi.z;
            vv[nv_t].lab = this->vertices[ii].lab; // lab mis a zero par default
            Numero_Som[ii] = nv_t;
            gtree->Add( vv[nv_t] );
            nv_t=nv_t+1;
          }
          else{
            Numero_Som[ii] = pvi-vv;
          }
        }

        delete gtree;
      }

      // general case

      KN<int> takevertex(nv_t,0);
      for (int k=0; k<nbe; k++) {
        const BorderElement & K(this->borderelements[k]);
        for(int jj=0; jj<3; jj++){
          takevertex[ Numero_Som[this->operator()(K[jj])] ] = 1;
        }
      }
      for(int k=0; k< this->nt; k++){
        const Element & K(this->elements[k]);
        for(int jj=0; jj<4; jj++){
          takevertex[ Numero_Som[this->operator()(K[jj])] ] = 1;
        }
      }

      int newvertex=0;
      for(int iv=0; iv<nv_t; iv++){
        newvertex+=takevertex[iv];
      }

      if( newvertex != this->nv){

        // determination of vertex
        Vertex *vvv = new Vertex[ newvertex ];
        KN<int> newNumero_Som(nv_t);
        int iii=0;
        for(int iv=0;  iv< nv_t; iv++){
          if( takevertex[iv ] == 1  ){
	    vvv[iii].x = vv[iv].x;
	    vvv[iii].y = vv[iv].y;
	    vvv[iii].z = vv[iv].z;
	    vvv[iii].lab = vv[iv].lab; // lab mis a zero par default
	    newNumero_Som[iv] = iii;
	    iii++;
          }
        }
        ffassert( newvertex== iii );

        Element *tt={};
        if(this->nt !=0) tt=new Element[this->nt];
        BorderElement *bb = new BorderElement[this->nbe];

        Element *ttt=tt;
        BorderElement *bbb=bb;

        for (int k=0; k<this->nbe; k++) {
          const BorderElement & K(this->borderelements[k]);
          int iv[3];
          for(int jj=0; jj<3; jj++){
            iv[jj] = Numero_Som[this->operator()(K[jj])];
            iv[jj] = newNumero_Som[iv[jj]];
          }
          (bbb++)->set(vvv,iv,K.lab);
        }

        for(int k=0; k< this->nt; k++){
          const Element & K(this->elements[k]);
          int iv[4];
          for(int jj=0; jj<4; jj++){
            iv[jj] = Numero_Som[this->operator()(K[jj])];
            iv[jj] = newNumero_Som[iv[jj]];
          }
          (ttt++)->set(vvv,iv,K.lab);
        }
        cout << " delete vertices + autre " << endl;
        delete [] vertices;
        delete [] elements;
        delete [] borderelements;

        nv = newvertex;

	    vertices = vvv;
        elements = tt;
        borderelements = bb;

        delete [] newNumero_Som;
      }
      else{
        cout << " no need to change the mesh " << endl;
      }
      delete [] Numero_Som;
    }

    BuildBound();
    if(nt > 0){
      BuildAdj();
      Buildbnormalv();
      BuildjElementConteningVertex();
    }

    if(verbosity>2)
      cout << "  -- End of read: mesure = " << mes << " border mesure " << mesb << endl;
    if(verbosity)
      cout << "  -- Mesh3 : "<<filename  << ", d "<< 3  << ", n Tet " << nt << ", n Vtx "
           << nv << " n Bord " << nbe << endl;
    ffassert(mes>=0); // add F. Hecht sep 2009.
  }

  Mesh3::Mesh3(const  Serialize &serialized)
    :GenericMesh<Tet,Triangle3,Vertex3> (serialized),
    meshS(0),typeMesh3(1)
  {
    BuildBound();
    if(verbosity>1)
      cout << "  -- End of serialized: mesure = " << mes << " border mesure " << mesb << endl;

    if(nt > 0){
      BuildAdj();
      Buildbnormalv();
      BuildjElementConteningVertex();
    }

    if(verbosity>1)
      cout << "  -- Mesh3  (serialized), d "<< 3  << ", n Tet " << nt << ", n Vtx "
	   << nv << " n Bord " << nbe << endl;
    ffassert(mes>=0); // add F. Hecht sep 2009.

  }
  Mesh3::Mesh3(FILE *f,int offset)
    :meshS(0)
  {

    GRead(f,offset);// remove 1
    assert( (nt >= 0 || nbe>=0)  && nv>0) ;
    BuildBound();
    if(verbosity>2)
      cout << "  -- End of read: mesure = " << mes << " border mesure " << mesb << endl;

    if(nt > 0){
      BuildAdj();
      Buildbnormalv();
      BuildjElementConteningVertex();
    }
    if(verbosity>2)
      cout << "  -- End of read: mesure = " << mes << " border mesure " << mesb << endl;

    if(verbosity>1)
      cout << "  -- Mesh3  (File *), d "<< 3  << ", n Tet " << nt << ", n Vtx "
           << nv << " n Bord " << nbe << endl;
    ffassert(mes>=0); // add F. Hecht sep 2009.
  }

  // Add by J. Morice 11/10
  // compute the hmin in a 3d mesh
  // Remark: on peut le mettre dans generic mesh
  //         (attention aux boucles sur les arretes)
  double Mesh3::hmin() const{
    R3 Pinf(1e100,1e100,1e100),Psup(-1e100,-1e100,-1e100);   // Extremite de la boite englobante
    double hmin=1e10;

    for (int ii=0;ii< this->nv;ii++){
      R3 P( vertices[ii].x, vertices[ii].y, vertices[ii].z);
      Pinf=Minc(P,Pinf);
      Psup=Maxc(P,Psup);
    }

    for (int k=0;k<this->nt;k++){
      for (int e=0;e<6;e++){
        if( this->elements[k].lenEdge(e) < Norme2(Psup-Pinf)/1e9 )
          {
            const Tet & K(this->elements[k]);
            int iv[4];
            for(int jj=0; jj <4; jj++){
              iv[jj] = this->operator()(K[jj]);
            }
            for (int eh=0;eh<6;eh++){
              if(verbosity>2) cout << "tetrahedra: " << k << " edge : " << eh << " lenght "<<  this->elements[k].lenEdge(eh) << endl;
            }
            if(verbosity>2) cout << " A tetrahedra with a very small edge was created " << endl;
	    return 1;
          }
        hmin=min(hmin,this->elements[k].lenEdge(e));   // calcul de .lenEdge pour un Mesh3
      }
    }

    for (int k=0;k<this->nbe;k++){
      for (int e=0;e<3;e++){
        if( this->be(k).lenEdge(e) < Norme2(Psup-Pinf)/1e9 )
          {
            for (int eh=0;eh<3;e++){
              cout << "triangles: " << k << " edge : " << eh << " lenght "<<  this->be(k).lenEdge(e) << endl;
            }
	    cout << " A triangle with a very small edges was created " << endl;
	    return 1;
          }
        hmin=min(hmin,this->be(k).lenEdge(e));   // calcul de .lenEdge pour un Mesh3
      }
    }
    ffassert(hmin>Norme2(Psup-Pinf)/1e9);
    return hmin;
  }

  // Fin Add by J. Morice nov 2010
  // Add J. Morice 12/2010
  void Mesh3::TrueVertex()
  {
    // verification multiple points
    double hseuil=hmin();
    hseuil =hseuil/10;
    cout << " hseuil = " << hseuil << endl;
    KN<int> Numero_Som(this->nv);
    Vertex *vv=new Vertex[this->nv];
    int nv_t=0;
    {
      R3 Pinf(1e100,1e100,1e100),Psup(-1e100,-1e100,-1e100);
      for (int ii=0;ii< this->nv;ii++){
	R3 P( vertices[ii].x, vertices[ii].y, vertices[ii].z);
	Pinf=Minc(P,Pinf);
	Psup=Maxc(P,Psup);
      }
      EF23::GTree<Vertex3> *gtree = new EF23::GTree<Vertex3>(vv,Pinf,Psup,0);
      // creation of octree
      for (int ii=0;ii<this->nv;ii++){
	const R3 r3vi( this->vertices[ii].x, this->vertices[ii].y, this->vertices[ii].z );
	const Vertex3 &vi(r3vi);
	Vertex3 * pvi=gtree->ToClose(vi,hseuil);
	if(!pvi){
	  vv[nv_t].x = vi.x;
	  vv[nv_t].y = vi.y;
	  vv[nv_t].z = vi.z;
	  vv[nv_t].lab = this->vertices[ii].lab; // lab mis a zero par default
	  Numero_Som[ii] = nv_t;
	  gtree->Add( vv[nv_t] );
	  nv_t=nv_t+1;
	}
	else{
	  Numero_Som[ii] = pvi-vv;
	}
      }
      delete gtree;
    }
    // general case
    KN<int> takevertex(nv_t,0);
    for (int k=0; k<nbe; k++) {
      const BorderElement & K(this->borderelements[k]);
      for(int jj=0; jj<3; jj++){
	takevertex[ Numero_Som[this->operator()(K[jj])] ] = 1;
      }
    }
    for(int k=0; k< this->nt; k++){
      const Element & K(this->elements[k]);
      for(int jj=0; jj<4; jj++){
	takevertex[ Numero_Som[this->operator()(K[jj])] ] = 1;
      }
    }
    int newvertex=0;
    for(int iv=0; iv<nv_t; iv++){
      newvertex+=takevertex[iv];
    }
    if( newvertex != this->nv){
      // determination of vertex
      Vertex *vvv = new Vertex[ newvertex ];
      KN<int> newNumero_Som(nv_t);
      int iii=0;
      for(int iv=0;  iv< nv_t; iv++){
	if( takevertex[iv ] == 1  ){
	  vvv[iii].x = vv[iv].x;
	  vvv[iii].y = vv[iv].y;
	  vvv[iii].z = vv[iv].z;
	  vvv[iii].lab = vv[iv].lab; // lab mis a zero par default
	  newNumero_Som[iv] = iii;
	  iii++;
	}
      }
      ffassert( newvertex== iii );

      Element *tt={};
      if(this->nt !=0) tt=new Element[this->nt];
      BorderElement *bb = new BorderElement[this->nbe];
      Element *ttt=tt;
      BorderElement *bbb=bb;
      for (int k=0; k<this->nbe; k++) {
	const BorderElement & K(this->borderelements[k]);
	int iv[3];
	for(int jj=0; jj<3; jj++){
	  iv[jj] = Numero_Som[this->operator()(K[jj])];
	  iv[jj] = newNumero_Som[iv[jj]];
	}
	(bbb++)->set(vvv,iv,K.lab);
      }
      for(int k=0; k< this->nt; k++){
	const Element & K(this->elements[k]);
	int iv[4];
	for(int jj=0; jj<4; jj++){
	  iv[jj] = Numero_Som[this->operator()(K[jj])];
	  iv[jj] = newNumero_Som[iv[jj]];
	}
	(ttt++)->set(vvv,iv,K.lab);
      }
      cout << " delete vertices + autre " << endl;
      delete [] vertices;
      delete [] elements;
      delete [] borderelements;
      nv = newvertex;
      vertices = vvv;
      elements = tt;
      borderelements = bb;

      delete [] newNumero_Som;
    }
    else{
      cout << " no need to change the mesh " << endl;
    }
    delete [] Numero_Som;


    BuildBound();
    if(nt > 0){
      BuildAdj();
      Buildbnormalv();
      BuildjElementConteningVertex();
    }

    if(verbosity>2)
      cout << "  -- End of read: mesure = " << mes << " border mesure " << mesb << endl;
    if(verbosity)
      cout << "  -- Mesh3 :  d "<< 3  << ", n Tet " << nt << ", n Vtx "
	   << nv << " n Bord " << nbe << endl;
    ffassert(mes>=0); // add F. Hecht sep 2009.
  }

  // Fin Add J. Morice 12/2010

  void  Mesh3::read(istream &f)
  { // read the mesh
    int i;
    string str;
    int err=0;
    while(!f.eof())
      {
        f >> str;
        if( str== "Vertices")
          {
            f >> nv;
            assert(!this->vertices );
            if(verbosity>2)
              cout << "  -- Nb of Vertex " << nv << endl;
            this->vertices = new Vertex[nv];
            for (i=0;i<nv;i++)
              {
		f >> this->vertices[i];
		assert(f.good());
              }
          }
        else if (str=="Tetrahedra")
          {
            f >> nt;
            assert(this->vertices && !this->elements);
            this->elements = new Element [nt];
            mes=0;
            assert(this->elements);
            if(verbosity>2)
              cout <<   "  -- Nb of Elements " << nt << endl;
            for (int i=0;i<nt;i++)
              {
		this->t(i).Read1(f,this->vertices,this->nv);
		if(this->t(i).mesure()<=0) err++; // Modif FH nov 2014
		mes += this->t(i).mesure();
	      }
          }
        else if (str=="Triangles")
          {
            mesb=0;
            int kmv=0,ij;
            f >> nbe;
            assert(vertices);
            this->borderelements = new BorderElement[nbe];
            if(verbosity>2)
              cout <<   "  -- Nb of border Triangles " << nbe << endl;
            for (i=0;i<nbe;i++)
              {
		this->be(i).Read1(f,this->vertices,this->nv);
		mesb += this->be(i).mesure();
		for(int j=0;j<3;++j)
		  if(!vertices[ij=this->be(i,j)].lab)
		    {
		      vertices[ij].lab=1;
		      kmv++;
		    }
              }
          }
        else if(str[0]=='#')
          {// on mange la ligne
            int c;
            while ( (c=f.get()) != '\n' &&  c != EOF)
              ;
          }
      }
    assert( (nt >= 0 || nbe>=0)  && nv>0) ;
    if(err!=0)
      {
	cerr << " Mesh3::read: sorry bad mesh. Number of negative Tet " << err << endl;
	this->~Mesh3();
	ffassert(0);
      }
  }


  int Mesh3::load(const string & filename)
  {
    int bin;
    int ver,inm,dim;
    int lf=filename.size()+20;
    KN<char>  fileb(lf),filef(lf);
    char *data = new char[filename.size()+1];
    size_t ssize = filename.size()+1;
    char *ptr;
    char *pfile=data;
    strncpy( data, filename.c_str(),ssize);
    ptr = strstr(data,".mesh");
    if( !ptr ){
      strcpy(filef,filename.c_str());
      strcpy(fileb,filef);
      strcat(filef,".mesh");
      strcat(fileb,".meshb");
      if( (inm=GmfOpenMesh(pfile=fileb, GmfRead,&ver,&dim)) )
        bin=true;
      else if( (inm=GmfOpenMesh(pfile=filef, GmfRead,&ver,&dim)) )
        bin=false;
      else
        if(verbosity>5){
          cerr << " Erreur ouverture file " << (char *) fileb  << " " << (char *) filef  <<endl;
          return   1;
        }
    }
    else{
      if( !(inm=GmfOpenMesh(data, GmfRead,&ver,&dim)) ){
        if(verbosity>5)
          cerr << " Erreur ouverture file " << (char *) data  << endl;
        return   1;
      }
    }
    // data file is readed and the meshes are initilized
    int nv=-1,nTet=-1,nTri=-1,nSeg=-1;
    nv = GmfStatKwd(inm,GmfVertices);  // vertice
    nTet= GmfStatKwd(inm,GmfTetrahedra); // tetrahedron elements only present in volume mesh
    nTri= GmfStatKwd(inm,GmfTriangles); // triangles in case of volume mesh -> boundary element / in case of surface mesh -> element
    nSeg=GmfStatKwd(inm,GmfEdges); // segment elements only present in surface mesh

    //define the type of meshes present in the data file .mesh

    if (nTet==0 && nTri>0 && nSeg>0) {  // Mesh3 null, MeshS (real surface with boundaries)
        if(verbosity) cout << "data file "<< pfile <<  " contains only a surface MeshS" <<endl;
        typeMesh3=0;}
     if (nTet==0 && nTri>0 && nSeg==0) { // Mesh3(old surface), MeshS null
        if(verbosity) cout << "data file "<< pfile <<  " contains only a old surface Mesh3" <<endl;
          typeMesh3=1;}
    if (nTet>0 && nTri>0 && nSeg==0) { // Mesh3 (volume), MeshS null
        if(verbosity) cout << "data file "<< pfile <<  " contains only a volume Mesh"<<endl;
        typeMesh3=1; }
    if (nTet==0 && nTri>0 && nSeg>0) { // Mesh3(old surface), MeshS(real surface with boundaries)
        if(verbosity) cout << "data file "<< pfile << " contains only an old surface mesh3 surface and meshS"<<endl;
        typeMesh3=2; }
    if (nTet>0 && nTri>0 && nSeg>0) { // Mesh3(volume with boundaries), MeshS(real surface with boundaries)
        if(verbosity) cout << "data file "<< pfile << " contains a volume and surface Meshes"<<endl;
        typeMesh3=2; }

    // By default, there is a mesh3 volume mesh. If pure surface mesh, mesh3 is empty except to the pointer on meshS
    // Initialize num of vertice, tetra, triang in the volume mesh
    this->set(nv,nTet,nTri);
    if(verbosity>1 && (typeMesh3!=0) )
      cout << "  -- Mesh3(load): "<< (char *) data <<  ", MeshVersionFormatted:= " << ver << ", space dimension:= "<< dim
           << ", num Tetrahedron elts:= " << nTet << ", num vertice:= " << nv << " num Triangle boundaries:= " << nTri << endl;

    if(dim  != 3) {
      cerr << "Err dim == " << dim << " !=3 " <<endl;
      return 2; }
    if( nv<=0 && ((nTet <0 || nTri <=0) || (nTri <=0 || nSeg<=0)) ) {
      cerr << " missing data "<< endl;
      return 3;
    }
    int iv[4],lab;
    float cr[3];
    memset(cr, 0, sizeof(float) * 3);
    // caution for the vertice 3 cases :
    // pure volume mesh -> in the .mesh, the vertice are true
    // pure surface mesh -> in the .mesh, the vertice are true
    // volume and surface meshes -> in the .mesh, vertice are true for the volume and the surface vertice
    // must be searched - *liste_v_num_surf is the mapping bewteen surface and volume vertice in mesh

      int mxlab=0, mnlab=0;
    // 1st case, the .mesh contains tetrahedrons -> volume mesh -> mesh3 is iniatiazed
    if (typeMesh3!=0) { // ! Mesh3 null
      // read vertices
      GmfGotoKwd(inm,GmfVertices);
      for(int i=0;i<nv;++i) {
	     if(ver<2) {
	        GmfGetLin(inm,GmfVertices,&cr[0],&cr[1],&cr[2],&lab);
	        vertices[i].x=cr[0];
            vertices[i].y=cr[1];
	        vertices[i].z=cr[2];
	     }
	     else
	       GmfGetLin(inm,GmfVertices,&vertices[i].x,&vertices[i].y,&vertices[i].z,&lab);
	       vertices[i].lab=lab;
           mxlab= max(mxlab,lab);
	       mnlab= min(mnlab,lab);
	  }

      // read tetrahedrons (element mesh3)
       GmfGotoKwd(inm,GmfTetrahedra);
       mes=0;
       for(int i=0;i<nTet;++i) {
	     GmfGetLin(inm,GmfTetrahedra,&iv[0],&iv[1],&iv[2],&iv[3],&lab);
	     assert( iv[0]>0 && iv[0]<=nv && iv[1]>0 && iv[1]<=nv && iv[2]>0 && iv[2]<=nv && iv[3]>0 && iv[3]<=nv);
	     for (int j=0;j<4;j++) iv[j]--;
         this->elements[i].set(vertices,iv,lab);
	     mes += this->elements[i].mesure();
	   }
      // read triangles (boundary element mesh3)
      if(mnlab==0 && mxlab==0 ) {
	    int kmv=0;
	    mesb=0;
	    GmfGotoKwd(inm,GmfTriangles);
	    for(int i=0;i<nbe;++i) {
	      GmfGetLin(inm,GmfTriangles,&iv[0],&iv[1],&iv[2],&lab);
	      assert( iv[0]>0 && iv[0]<=nv && iv[1]>0 && iv[1]<=nv && iv[2]>0 && iv[2]<=nv);
	      for(int j=0;j<3;++j)
		    if(!vertices[iv[j]-1].lab) {
		      vertices[iv[j]-1].lab=1;
		      kmv++;
            }
	        for (int j=0;j<3;++j) iv[j]--;
	        this->be(i).set(this->vertices,iv,lab);
	        mesb += this->be(i).mesure();
	    }
	    if(kmv&& verbosity>1) cout << "    Aucun label Hack (FH)  ??? => 1 sur les triangle frontiere "<<endl;
	  }
      else {
	    mesb=0;
	    GmfGotoKwd(inm,GmfTriangles);
	    for(int i=0;i<nbe;++i) {
	      GmfGetLin(inm,GmfTriangles,&iv[0],&iv[1],&iv[2],&lab);
	      assert( iv[0]>0 && iv[0]<=nv && iv[1]>0 && iv[1]<=nv && iv[2]>0 && iv[2]<=nv);
	      for (int j=0;j<3;++j) iv[j]--;
	      this->be(i).set(this->vertices,iv,lab);
	      mesb += this->be(i).mesure();
	     }
	  }
   } // end typeMesh3!=0

   // 2nd case, the .mesh don't contains tetrahedrons, that means it's only a surface mesh
   // the vertices given are the surface mesh vertices
   int mxlabSurf=0, mnlabSurf=0;
   if (typeMesh3==0) { // Mesh3 null
       meshS = new MeshS();
       this->meshS->set(nv,nTri,nSeg);
       // read vertices
       GmfGotoKwd(inm,GmfVertices);
       for(int i=0;i<this->meshS->nv;++i) {
         if(ver<2) {
           GmfGetLin(inm,GmfVertices,&cr[0],&cr[1],&cr[2],&lab);
           meshS->vertices[i].x=cr[0];
           meshS->vertices[i].y=cr[1];
           meshS->vertices[i].z=cr[2];}
         else
           GmfGetLin(inm,GmfVertices,&meshS->vertices[i].x,&meshS->vertices[i].y,&meshS->vertices[i].z,&lab);
           meshS->vertices[i].lab=lab;
           mxlabSurf= max(mxlabSurf,lab);
           mnlabSurf= min(mnlabSurf,lab);
        }
      // read triangles (element meshS)
      if(mnlab==0 && mxlab==0 ) {
	    int kmv=0;
	    meshS->mes=0;
	    GmfGotoKwd(inm,GmfTriangles);
	    for(int i=0;i<nTri;++i) {
	      GmfGetLin(inm,GmfTriangles,&iv[0],&iv[1],&iv[2],&lab);
	      assert( iv[0]>0 && iv[0]<=nv && iv[1]>0 && iv[1]<=nv && iv[2]>0 && iv[2]<=nv);
	      for(int j=0;j<3;++j)
		    if(!meshS->vertices[iv[j]-1].lab) {
		      meshS->vertices[iv[j]-1].lab=1;
		      kmv++;
		    }
	        for (int j=0;j<3;++j) iv[j]--;
	        meshS->elements[i].set(meshS->vertices,iv,lab);
	        meshS->mes += meshS->elements[i].mesure();
	     }
	     if(kmv&& verbosity>1) cout << "    Aucun label Hack (FH)  ??? => 1 sur les triangle frontiere "<<endl;
      }
      else {
	    meshS->mes=0;
	    GmfGotoKwd(inm,GmfTriangles);
	    for(int i=0;i<nTri;++i) {
	      GmfGetLin(inm,GmfTriangles,&iv[0],&iv[1],&iv[2],&lab);
	      assert( iv[0]>0 && iv[0]<=nv && iv[1]>0 && iv[1]<=nv && iv[2]>0 && iv[2]<=nv);
	      for (int j=0;j<3;++j) iv[j]--;
	      meshS->elements[i].set(this->meshS->vertices,iv,lab);
	      meshS->mes += meshS->elements[i].mesure();
	    }
	  }
      // read edges (boundary elements meshS)
      meshS->mesb=0;
      GmfGotoKwd(inm,GmfEdges);
      for(int i=0;i<nSeg;++i) {
	    GmfGetLin(inm,GmfEdges,&iv[0],&iv[1],&lab);
	    assert( iv[0]>0 && iv[0]<=nv && iv[1]>0 && iv[1]<=nv);
	    for (int j=0;j<2;j++) iv[j]--;
        meshS->borderelements[i].set(this->meshS->vertices,iv,lab);   //element
        meshS->mesb += this->meshS->borderelements[i].mesure();
      }
    } // end typeMesh==0

    // 3rd case, the .mesh contains tetrahedrons, triangles and edges
    // we are able to build a volume and surface mesh
    // for this, surface vertices must be extract of the original vertice list and a mapping must be created between surface and volume vertices

      if (typeMesh3==2) {  // ! MeshS null and Mesh3 null
      meshS = new MeshS();
      // Number of Vertex in the surface
      meshS->v_num_surf=new int[nv];
      meshS->liste_v_num_surf=new int[nv]; // mapping to surface/volume vertices
      for (int k=0; k<nv; k++) {
	    meshS->v_num_surf[k]=-1;
        meshS->liste_v_num_surf[k]=0;
      }
      // search Vertex on the surface
      int nbv_surf=0;
      for (int k=0; k<nbe; k++) {
	    const BorderElement & K(this->borderelements[k]);
	    for(int jj=0; jj<3; jj++) {
	      int i0=this->operator()(K[jj]);
	      if( meshS->v_num_surf[i0] == -1 ) {
            // the mapping v_num_surf -> new numbering /  liste_v_num_surf[nbv_surf] -> the global num
            meshS->v_num_surf[i0] = nbv_surf;
	        meshS->liste_v_num_surf[nbv_surf]= i0;
	        nbv_surf++;
	      }
	    }
      }
      this->meshS->set(nbv_surf,nTri,nSeg);
      // save the surface vertices
      for (int k=0; k<nbv_surf; k++) {
        int k0 = meshS->liste_v_num_surf[k];
	    const  Vertex & P = this->vertices[k0];
        meshS->vertices[k].lab=P.lab;
	    meshS->vertices[k].x=P.x;
	    meshS->vertices[k].y=P.y;
	    meshS->vertices[k].z=P.z;
      }
      // read triangles and change with the surface numbering
      int kmv=0;
      meshS->mes=0;
      GmfGotoKwd(inm,GmfTriangles);
      for(int i=0;i<nTri;++i) {
	    GmfGetLin(inm,GmfTriangles,&iv[0],&iv[1],&iv[2],&lab);
	    for (int j=0;j<3;++j) iv[j]=meshS->v_num_surf[iv[j]-1];
	    for(int j=0;j<3;++j)
	      if(!meshS->vertices[iv[j]].lab) {
		    meshS->vertices[iv[j]].lab=1;
		    kmv++;
	      }
        meshS->elements[i].set(meshS->vertices,iv,lab);
	    meshS->mes += meshS->elements[i].mesure();
	  }
      if(kmv&& verbosity>1) cout << "    Aucun label Hack (FH)  ??? => 1 sur les triangle frontiere "<<endl;
      // reading edges with the surface numbering
      meshS->mesb=0;
      GmfGotoKwd(inm,GmfEdges);
      for(int i=0;i<nSeg;++i) {
	    GmfGetLin(inm,GmfEdges,&iv[0],&iv[1],&lab);
	    for (int j=0;j<2;++j) iv[j]=meshS->v_num_surf[iv[j]-1];
        meshS->borderelements[i].set(meshS->vertices,iv,lab);
	    meshS->mesb += meshS->borderelements[i].mesure();
	  }
    } // end typeMesh3==2

    if(verbosity>1 && (meshS) )
      cout << "  -- MeshS(load): "<< (char *) data <<  ", MeshVersionFormatted:= " << ver << ", space dimension:= "<< dim
	   << ", Triangle elts:= " << meshS->nt << ", num vertice:= " << meshS->nv << ", num edges boundaries:= " << meshS->nbe << endl;

    GmfCloseMesh(inm);
    delete[] data;
    return 0; // OK
  }

  const     string Gsbegin="Mesh3::GSave v0",Gsend="end";
  void Mesh3::GSave(FILE * ff,int offset) const
  {
    PlotStream f(ff);

    f <<  Gsbegin ;
    f << nv << nt << nbe;
    for (int k=0; k<nv; k++) {
      const  Vertex & P = this->vertices[k];
      f << P.x <<P.y << P.z << P.lab ;
    }

    if(nt != 0){

      for (int k=0; k<nt; k++) {
        const Element & K(this->elements[k]);
        int i0=this->operator()(K[0])+offset;
        int i1=this->operator()(K[1])+offset;
        int i2=this->operator()(K[2])+offset;
        int i3=this->operator()(K[3])+offset;

        int lab=K.lab;
        f << i0 << i1 << i2 << i3 << lab;
      }
    }

    for (int k=0; k<nbe; k++) {
      const BorderElement & K(this->borderelements[k]);
      int i0=this->operator()(K[0])+offset;
      int i1=this->operator()(K[1])+offset;
      int i2=this->operator()(K[2])+offset;
      int lab=K.lab;
      f << i0 << i1 << i2  << lab;
    }
    f << Gsend;
  }



  void Mesh3::GRead(FILE * ff,int offset)
  {
    PlotStream f(ff);
    string s;
    f >> s;
    ffassert( s== Gsbegin);
    f >> nv >> nt >> nbe;
    if(verbosity>2)
      cout << " GRead : nv " << nv << " " << nt << " " << nbe << endl;
    this->vertices = new Vertex[nv];
    this->elements = new Element [nt];
    this->borderelements = new BorderElement[nbe];
    for (int k=0; k<nv; k++) {
      Vertex & P = this->vertices[k];
      f >> P.x >>P.y >> P.z >> P.lab ;
    }
    mes=0.;
    mesb=0.;

    if(nt != 0)
      {

        for (int k=0; k<nt; k++) {
          int i[4],lab;
          Element & K(this->elements[k]);
          f >> i[0] >> i[1] >> i[2] >> i[3] >> lab;
          Add(i,4,offset);
          K.set(this->vertices,i,lab);
          mes += K.mesure();

        }
      }
    for (int k=0; k<nbe; k++) {
      int i[3],lab;
      BorderElement & K(this->borderelements[k]);
      f >> i[0] >> i[1] >> i[2]  >> lab;
      Add(i,3,offset);
      K.set(this->vertices,i,lab);
      mesb += K.mesure();

    }
    f >> s;
    ffassert( s== Gsend);
  }
  void Mesh3::readmsh(ifstream & f,int offset)
  {
    int err=0;
    f >> nv >> nt >> nbe;
    if(verbosity>2)
      cout << " GRead : nv " << nv << " " << nt << " " << nbe << endl;
    this->vertices = new Vertex[nv];
    this->elements = new Element [nt];
    this->borderelements = new BorderElement[nbe];
    for (int k=0; k<nv; k++) {
      Vertex & P = this->vertices[k];
      f >> P.x >>P.y >> P.z >> P.lab ;
    }
    mes=0.;
    mesb=0.;

    if(nt != 0)
      {

	for (int k=0; k<nt; k++) {
	  int i[4],lab;
	  Element & K(this->elements[k]);
	  f >> i[0] >> i[1] >> i[2] >> i[3] >> lab;
	  Add(i,4,offset);
	  K.set(this->vertices,i,lab);
	  mes += K.mesure();
	  err += K.mesure() <0;

	}
      }


    for (int k=0; k<nbe; k++) {
      int i[4],lab;
      BorderElement & K(this->borderelements[k]);
      f >> i[0] >> i[1] >> i[2]  >> lab;
      Add(i,3,offset);
      K.set(this->vertices,i,lab);
      mesb += K.mesure();

    }
    if(err!=0)
      {
	cerr << " Mesh3::readmsh : sorry bad mesh. Number of negative Tet " << err << endl;
	this->~Mesh3();
	ffassert(0);
      }

  }



  int Mesh3::Save(const string & filename) const
  {
      int ver = GmfFloat, outm;
    if ( !(outm = GmfOpenMesh(filename.c_str(),GmfWrite,ver,3)) ) {
      cerr <<"  -- Mesh3::Save  UNABLE TO OPEN  :"<< filename << endl;
      return(1);
    }
    float fx,fy,fz;
    // save the volume mesh
    if (typeMesh3!=0) {
      // write vertices (mesh3)
      GmfSetKwd(outm,GmfVertices,this->nv);
      for (int k=0; k<nv; k++) {
        const  Vertex & P = this->vertices[k];
        GmfSetLin(outm,GmfVertices,fx=P.x,fy=P.y,fz=P.z,P.lab);
      }
      // write tetrahedrons (mesh3)
        if(nt != 0) {
        GmfSetKwd(outm,GmfTetrahedra,nt);
        for (int k=0; k<nt; k++) {
          const Element & K(this->elements[k]);
          int i0=this->operator()(K[0])+1;
          int i1=this->operator()(K[1])+1;
          int i2=this->operator()(K[2])+1;
          int i3=this->operator()(K[3])+1;
          int lab=K.lab;
          GmfSetLin(outm,GmfTetrahedra,i0,i1,i2,i3,lab);
        }
      }
      // write triangles (boundary elements mesh3 = element meshS (mesh3)
      GmfSetKwd(outm,GmfTriangles,nbe);
      for (int k=0; k<nbe; k++) {
          const BorderElement & K(this->borderelements[k]);
        int i0=this->operator()(K[0])+1;
        int i1=this->operator()(K[1])+1;
        int i2=this->operator()(K[2])+1;
        int lab=K.lab;
        GmfSetLin(outm,GmfTriangles,i0,i1,i2,lab);
      }
    } // end typeMesh3 !=0
    // write edges, in cas of volume + surface mesh (meshS)
      if (typeMesh3==2) {
        int nbeS = meshS->nbe;
        if(nbeS != 0) {
      GmfSetKwd(outm,GmfEdges,nbeS);
      for (int k=0; k<nbeS; k++) {
        const MeshS::BorderElement & K(meshS->borderelements[k]);
        int i0= meshS->liste_v_num_surf[meshS->operator()(K[0])]+1;
        int i1= meshS->liste_v_num_surf[meshS->operator()(K[1])]+1;
        int lab=K.lab;
        GmfSetLin(outm,GmfEdges,i0,i1,lab);
        }
        }
    } //end typeMesh=2
    if (typeMesh3==0) {
      MeshS *ThS = this->meshS;
      // write vertice (meshS)
      GmfSetKwd(outm,GmfVertices,ThS->nv);
      for (int k=0; k<ThS->nv; k++) {
        const  Vertex & P = ThS->vertices[k];
        GmfSetLin(outm,GmfVertices,fx=P.x,fy=P.y,fz=P.z,P.lab);
      }
      // write triangles (meshS)
      GmfSetKwd(outm,GmfTriangles,ThS->nt);
      for (int k=0; k<ThS->nt; k++) {
        const MeshS::Element & K(ThS->elements[k]);
        int i0=ThS->operator()(K[0])+1;
        int i1=ThS->operator()(K[1])+1;
        int i2=ThS->operator()(K[2])+1;
        int lab=K.lab;
        GmfSetLin(outm,GmfTriangles,i0,i1,i2,lab);
      }
      // write edges (meshS)
      GmfSetKwd(outm,GmfEdges,ThS->nbe);
      for (int k=0; k<ThS->nbe; k++) {
        const MeshS::BorderElement & K(ThS->borderelements[k]);
        int i0=ThS->operator()(K[0])+1;
        int i1=ThS->operator()(K[1])+1;
        int lab=K.lab;
        GmfSetLin(outm,GmfEdges,i0,i1,lab);
      }
    } // end typeMesh==0
   GmfCloseMesh(outm);
   return (0);

  }


  int Mesh3::SaveSurface(const string & filename) const
  {
    int ver = GmfFloat, outm;
    if ( !(outm = GmfOpenMesh(filename.c_str(),GmfWrite,ver,3)) )
    {
        cerr <<"  -- Mesh3::Save  UNABLE TO OPEN  :"<< filename << endl;
        return(1);
    }
    // the mesh contains a volume mesh. Must extract the surface and the triangle on the surface ie the boundary mesh
    // the surface is the boundary elements, that means the triangle

    if (typeMesh3==1) {
        //extraction vertice surface must be make when pur volume
        // it's made in the mesh3::load function
       int *v_num_surf, *map_v_num_surf;
      // Extraction of Vertex  belongs to the surface
      v_num_surf=new int[nv];
      map_v_num_surf=new int[nv];
      for (int k=0; k<nv; k++){
	     v_num_surf[k]=-1;
	     map_v_num_surf[k]=0;
      }
      // Search Vertex on the surface
      int nbv_surf=0;
      for (int k=0; k<nbe; k++) {
         const BorderElement & K(this->borderelements[k]);
	     for(int jj=0; jj<3; jj++){
	        int i0=this->operator()(K[jj]);
	        if( v_num_surf[i0] == -1 ){
	           v_num_surf[i0] = nbv_surf;
               map_v_num_surf[nbv_surf]= i0;
	           nbv_surf++;
            }
	     }
      }
      // print in file the surface vertex
      float fx,fy,fz;
      GmfSetKwd(outm,GmfVertices,nbv_surf);
      for (int k=0; k<nbv_surf; k++) {
	     int k0 = map_v_num_surf[k];
	     const  Vertex & P = this->vertices[k0];
	     GmfSetLin(outm,GmfVertices,fx=P.x,fy=P.y,fz=P.z,P.lab);
      }
      // print in file the triangle (could be the boundary of volume mesh or the element of surface mesh)
      GmfSetKwd(outm,GmfTriangles,nbe);
      for (int k=0; k<nbe; k++) {
	     const BorderElement & K(this->borderelements[k]);
	     int i0=v_num_surf[this->operator()(K[0])]+1;
	     int i1=v_num_surf[this->operator()(K[1])]+1;
	     int i2=v_num_surf[this->operator()(K[2])]+1;
	     int lab=K.lab;
         assert(0<i0 && i0-1 < nbv_surf && 0<i1 && i1-1 < nbv_surf &&  0<i2 && i2-1 < nbv_surf );
         GmfSetLin(outm,GmfTriangles,i0,i1,i2,lab);
      }
      delete [ ] v_num_surf;
    }

    // the mesh contains a surface mesh. Mesh could be a pur surface mesh or volume+surface mesh
    // the surface is meshS

    if (typeMesh3!=1) {
    int nvS = meshS->nv;
    int ntS = meshS->nt;
    int nbeS = meshS->nbe;
    // print in file the surface vertex
    float fx,fy,fz;
    GmfSetKwd(outm,GmfVertices,nvS);
    for (int k=0; k<nvS; k++) {
	  const  Vertex & P = meshS->vertices[k];
	  GmfSetLin(outm,GmfVertices,fx=P.x,fy=P.y,fz=P.z,P.lab);
    }
    // print in file the triangles
    GmfSetKwd(outm,GmfTriangles,ntS);
    for (int k=0; k<ntS; k++) {
        const MeshS::Element & K(meshS->elements[k]);
         int i0=meshS->operator()(K[0])+1;
         int i1=meshS->operator()(K[1])+1;
         int i2=meshS->operator()(K[2])+1;
         int lab=K.lab;
         assert( 0<i0 && i0-1 < nvS &&  0<i1 && i1-1 < nvS && 0<i2 && i2-1 < nvS);
         GmfSetLin(outm,GmfTriangles,i0,i1,i2,lab);
      }
      // print in the edges
      GmfSetKwd(outm,GmfEdges,nbeS);
      for (int k=0; k<nbeS; k++) {
	     const MeshS::BorderElement & K(meshS->borderelements[k]);
	     int i0=meshS->operator()(K[0])+1;
	     int i1=meshS->operator()(K[1])+1;
	     int lab=K.lab;
         assert( 0<i0 && i0-1 < nvS &&  0<i1 && i1-1 < nvS );
	     GmfSetLin(outm,GmfEdges,i0,i1,lab);
      }
    }
    GmfCloseMesh(outm);
    return (0);
  }


  int Mesh3::SaveSurface(const string & filename1,const string & filename2) const
  {
      // the mesh contains a volume mesh. Must extract the surface and the triangle on the surface ie the boundary mesh
      // the surface is the boundary elements, that means the triangle
      if (typeMesh3==1) {
          //extraction vertice surface must be make when pur volume
          // it's made in the mesh3::load function
          int *v_num_surf, *map_v_num_surf;
          // Extraction of Vertex  belongs to the surface
          v_num_surf=new int[nv];
          map_v_num_surf=new int[nv];
          for (int k=0; k<nv; k++){
              v_num_surf[k]=-1;
              map_v_num_surf[k]=0;
          }
          // Search Vertex on the surface
          int nbv_surf=0;
          for (int k=0; k<nbe; k++) {
              const BorderElement & K(this->borderelements[k]);
              for(int jj=0; jj<3; jj++){
                  int i0=this->operator()(K[jj]);
                  if( v_num_surf[i0] == -1 ){
                      v_num_surf[i0] = nbv_surf;
                      map_v_num_surf[nbv_surf]= i0;
                      nbv_surf++;
                  }
              }
          }
          FILE *fpoints = fopen(filename1.c_str(),"w");
          fprintf(fpoints,"%i\n",nbv_surf);
          // print in filename1 the surface vertex
          float fx,fy,fz;
          for (int k=0; k<nbv_surf; k++) {
              int k0 = map_v_num_surf[k];
              const  Vertex & P = this->vertices[k0];
              fprintf(fpoints,"%f %f %f %i\n",P.x,P.y,P.z,P.lab);

          }
          fclose(fpoints);
          // print in file in filename2 the triangle (could be the boundary of volume mesh or the element of surface mesh)
          FILE *ffaces = fopen(filename2.c_str(),"w");
          fprintf(ffaces,"%i\n",nbe);

          for (int k=0; k<nbe; k++) {
              const BorderElement & K(this->borderelements[k]);
              int i0=this->operator()(K[0]);
              int i1=this->operator()(K[1]);
              int i2=this->operator()(K[2]);
              int lab=K.lab;
              int label0= this->vertices[i0].lab;
              int label1= this->vertices[i1].lab;
              int label2= this->vertices[i2].lab;
              int nature=3;
              int i0v=v_num_surf[i0]+1;
              int i1v=v_num_surf[i1]+1;
              int i2v=v_num_surf[i2]+1;
              assert(0<i0 && i0-1 < nbv_surf && 0<i1 && i1-1 < nbv_surf &&  0<i2 && i2-1 < nbv_surf );
              fprintf(ffaces,"%i %i %i %i %i %i %i %i\n", nature, i0v, i1v, i2v, lab, label0, label1, label2);
          }
          fclose(ffaces);
          delete [ ] v_num_surf;
      }

      // the mesh contains a surface mesh. Mesh could be a pur surface mesh or volume+surface mesh
      // the surface is meshS

      if (typeMesh3!=0) {
          int nvS = meshS->nv;
          int ntS = meshS->nt;
          int nbeS = meshS->nbe;
          // print in file the surface vertex
          float fx,fy,fz;
          FILE *fpoints = fopen(filename1.c_str(),"w");
          fprintf(fpoints,"%i\n",nvS);
          for (int k=0; k<nvS; k++) {
              const  Vertex & P = meshS->vertices[k];
              fprintf(fpoints,"%f %f %f %i\n",P.x,P.y,P.z,P.lab);
          }
          fclose(fpoints);
          // print in file the triangles
          FILE *ffaces = fopen(filename2.c_str(),"w");
          fprintf(ffaces,"%i\n",ntS);
          for (int k=0; k<ntS; k++) {
              const MeshS::Element & K(meshS->elements[k]);
              int i0=meshS->operator()(K[0])+1;
              int i1=meshS->operator()(K[1])+1;
              int i2=meshS->operator()(K[2])+1;
              int lab=K.lab;
              int label0= meshS->vertices[i0].lab;
              int label1= meshS->vertices[i1].lab;
              int label2= meshS->vertices[i2].lab;
              int nature=3;
              assert( 0<i0 && i0-1 < nvS &&  0<i1 && i1-1 < nvS && 0<i2 && i2-1 < nvS);
              fprintf(ffaces,"%i %i %i %i %i %i %i %i\n", nature, i0, i1, i2, lab, label0, label1, label2);
          }
          // print in the edges
          fprintf(ffaces,"%i\n",nbeS);
          for (int k=0; k<nbeS; k++) {
              const MeshS::BorderElement & K(meshS->borderelements[k]);
              int i0=meshS->operator()(K[0])+1;
              int i1=meshS->operator()(K[1])+1;
              int lab=K.lab;
              int label0= meshS->vertices[i0].lab;
              int label1= meshS->vertices[i1].lab;
              int nature=2;
              assert( 0<i0 && i0-1 < nvS &&  0<i1 && i1-1 < nvS );
              fprintf(ffaces,"%i %i %i %i %i %i\n", nature, i0, i1, lab, label0, label1);
          }
        fclose(ffaces);
      }
      return (0);
  }

  Mesh3::Mesh3(int nnv, int nnt, int nnbe, Vertex3 *vv, Tet *tt, Triangle3 *bb)
    :meshS(0)
  {

    nv = nnv;
    nt = nnt;
    nbe =nnbe;
    vertices = vv;
    elements = tt;
    borderelements = bb;

    mes=0.;
    mesb=0.;

    for (int i=0;i<nt;i++)
      mes += this->elements[i].mesure();

    for (int i=0;i<nbe;i++)
      mesb += this->be(i).mesure();

    //  Add FH to be consitant we all constructor ...  July 09
    BuildBound();
    if(nt > 0){
      BuildAdj();
      Buildbnormalv();
      BuildjElementConteningVertex();
    }
    //  end add
    if(verbosity>1)
      cout << "  -- End of read: mesure = " << mes << " border mesure " << mesb << endl;

    assert(mes>=0.);
  }

  Mesh3::Mesh3(int nnv, int nnbe, Vertex3 *vv, Triangle3 *bb)
    :meshS(0)
  {

    nv = nnv;
    nbe =nnbe;

    vertices = vv;
    borderelements = bb;

    mes=0.;
    mesb=0.;

    for (int i=0;i<nbe;i++)
      mesb += this->be(i).mesure();

    //  Add FH to be consitant we all constructor ...  July 09
    BuildBound();
    if(nt > 0){
      BuildAdj();
      Buildbnormalv();
      BuildjElementConteningVertex();
    }
    //  end add

    if(verbosity>1)
      cout << "  -- End of Construct  mesh3: mesure = " << mes << " border mesure " << mesb <<  endl;
    ffassert(mes>=0); // add F. Hecht sep 2009.
  }

void Mesh3::flipSurfaceMesh3(int surface_orientation)
{
    /* inverse the orientation of the surface if necessary*/
    /* and control that all surfaces are oriented in the same way*/
    int nbflip=0;
    for (int i=0;i<this->nbe;i++)
      {
        double mes_triangle3= this->be(i).mesure();

        if( surface_orientation*mes_triangle3 < 0){
          const Triangle3 &K( this->be(i) );
          int iv[3];

          iv[0] = this->operator()(K[0]);
          iv[1] = this->operator()(K[1]);
          iv[2] = this->operator()(K[2]);

          int iv_temp=iv[1];
          iv[1]=iv[2];
          iv[2]=iv_temp;
          this->be(i).set( this->vertices, iv, K.lab ) ;
          nbflip++;
        }
      }
    assert(nbflip==0 || nbflip== this->nbe);
  }


  int  signe_permutation(int i0,int i1,int i2,int i3)
  {
    int p=1;
    if(i0>i1) Exchange(i0,i1), p = -p;
    if(i0>i2) Exchange(i0,i2), p = -p;
    if(i0>i3) Exchange(i0,i3), p = -p;
    if(i1>i2) Exchange(i1,i2), p = -p;
    if(i1>i3) Exchange(i1,i3), p = -p;
    if(i2>i3) Exchange(i2,i3), p = -p;
    return p;
}

  // new version ...
  int  WalkInTetn(const Mesh3 & Th,int it, R3 & Phat,const R3 & U, R & dt, R3 & offset)
  {
    int ncas=0 ;
    const R eps = 1e-12;
    const R epsb = 1e-10;
    const R epsedge = 1e-9;

    // corrected by F.H 23 juin 2015
    bool ddd=verbosity>2000;
    bool ddd9=verbosity>9;
    bool nomove=true;
    R lambda[4],dl[4],cl[4];
    Phat.toBary(lambda);
    if(ddd) cout << "\n\t\t\tWT: "  << Phat << " :  "  << lambda[0] << " " <<lambda[1] <<" " <<lambda[2] << " " <<lambda[3] << " u = "<< U << " dt " << dt <<endl;
#ifndef NDEBUG
    for(int i=0;i<4;++i)
      assert(lambda[i]<1.000001 && lambda[i]>-0.0000001);
#endif
    typedef R3 Rd;
    const Mesh3::Element & T(Th[it]);
    const int nve = 4;
    const Rd  Q[nve]={(const R3) T[0],(const R3) T[1],(const R3) T[2],(const R3) T[3]};

    Rd P  =T(Phat);

    Rd PF = P + U*dt;
    Rd PFK= PF;

    //  couleur(15);MoveTo( P); LineTo( PF);
    R l[nve];
    double Det=T.mesure()*6;
    l[0] = det(PF  ,Q[1],Q[2],Q[3]);
    l[1] = det(Q[0],PF  ,Q[2],Q[3]);
    l[2] = det(Q[0],Q[1],PF  ,Q[3]);
    l[3] = Det - l[0]-l[1]-l[2];
    l[0] /= Det;
    l[1] /= Det;
    l[2] /= Det;
    l[3] /= Det;

    if (l[0]>-eps && l[1]>-eps && l[2]>-eps && l[3]>-eps)
      { // a fini  .... ouf ...
	dt =0;
	Phat=R3(l+1);
	nomove=false;
	return -1;
      }

    // l(Q) = lambda + dl  *coef  avec Q= P+ U*dt*coef
    // coef > 0 et segement [PQ] dans K .. et Q \in \partial K
    // recherche de la face de sortie ..

    dl[0]= l[0]-lambda[0];
    dl[1]= l[1]-lambda[1];
    dl[2]= l[2]-lambda[2];
    dl[3]= l[3]-lambda[3];
    // attention a inf et NaN possible ci FH..
    cl[0]= -lambda[0]/dl[0];
    cl[1]= -lambda[1]/dl[1];
    cl[2]= -lambda[2]/dl[2];
    cl[3]= -lambda[3]/dl[3];

    //  min  positif
    int kf=-1;
    double cf=1;// 1 min  positif  < 1.
    if(cl[0]>0. && cl[0] < cf) cf=cl[kf=0]; // OK NaN or Inf test are wrong in any case ..
    if(cl[1]>0. && cl[1] < cf) cf=cl[kf=1];
    if(cl[2]>0. && cl[2] < cf) cf=cl[kf=2];
    if(cl[3]>0. && cl[3] < cf) cf=cl[kf=3];

    if(kf>=0)
      {
	double cf1 = 1-cf;
	// point de sortie en temps dt*cf .. ??
	l[0]-= dl[0]*cf1;
	l[1]-= dl[1]*cf1;
	l[2]-= dl[2]*cf1;
	l[3]-= dl[3]*cf1;
	PFK = P +  U*(dt*cf); // final  point in tet.

      }
    // on sort en temp  cf*dt



    if(ddd)  cout << "\t\t\t WT " << it << ", " << Phat << ",  PFK=" << PFK
		  << " :  "  << l[0] << " " <<l[1] <<" " <<l[2] << " " <<l[3]
		  << " == " << det(Q[0],Q[1],Q[2],PF)/Det
		  << " : l (in) "  << lambda[0] << " " <<lambda[1] <<" " <<lambda[2] << " " <<lambda[3]
		  << " PF= K(l) = " << Th[it](R3(l+1)) << " kf = " << kf << " " << cf << "/ " << PF
		  <<endl ;

    int neg[nve],k=0,nng[4],kn=0;// Bug missing init of kn Thank of Axel mars 2019..
    int kk=-1;

    if(kf>=0)   // sortie positive ..
      {
	// on regarde de les reelement negatif
	// on ne veut pas des points sur les faces.
	// car sinon il va y avoir un probleme quand on va projete sur la face
	//  et remettre le point dans le tetraedre.
	int ko=0,j=0;
	if (l[0]<=-eps ) neg[k++]=0,ko+=0+j, j+=4;else nng[kn++]=0;
	if (l[1]<=-eps ) neg[k++]=1,ko+=1+j, j+=4;else nng[kn++]=1;
	if (l[2]<=-eps ) neg[k++]=2,ko+=2+j, j+=4;else nng[kn++]=2;
	if (l[3]<=-eps ) neg[k++]=3,ko+=3+j, j+=4;else nng[kn++]=3;

	if(ddd)  cout << "\t\t\t k= " << k << endl;

	if(k>0)
	  {
	    {
	      R coef1 = 1-cf;
	      nomove= (cf<1e-10);
	      dt        = dt*coef1;// temps restant
	    }
	    // on met les points
	    for(int i=0; i<k; ++i)
	      l[neg[i]]=0;
	    int p[4]={0,1,2,3};
	    if(l[p[0]] <l[p[2]]) swap(p[0],p[2]);
	    if(l[p[1]] <l[p[3]]) swap(p[1],p[3]);
	    if(l[p[0]] <l[p[1]]) swap(p[0],p[1]);// 0 ok
	    if(l[p[2]] <l[p[3]]) swap(p[2],p[3]);// 3 ok
	    if(l[p[1]] <l[p[2]]) swap(p[1],p[2]);// 1,2 ok

	    ffassert((l[p[0]]  >= l[p[1]]) && (l[p[1]]  >= l[p[2]]) && (l[p[2]]  >= l[p[3]]) );
	    if(ddd) cout << "\t\t\t   \t\t -> kk=" << kk << " , l= "<< l[0]  << " "
			 <<l[1] << " " <<l[2] << " " << l[3] << " PF =" << PF <<  " " << &PF <<endl;
	    if( k==1) kk=p[3];
	    else if(k>1) { // on bouge la sortie un sortie a pb un peu...
	      if( l[p[1]] > epsedge )
		{ //sort sur un arete .. varaimant ...
		  // on met le point sur unz face ...
		  l[p[0]]-=epsb;
		  l[p[1]]-=epsb;
		  l[p[2]]+=epsb+epsb;
		  l[p[3]]=0;
		  kk  =p[3];
		  if(ddd9 )
		    cout << "  *** WalkInTetn: on bouge convect le point d'une arete  sur un face .... \n";
		}
	      else
		{ // sort sur un sommet
		  l[p[0]]-=epsb+epsb;
		  l[p[2]]+=epsb;
		  l[p[3]]=0;
		  kk=p[3];
		  if(ddd9 )
		    cout << "  *** WalkInTetn: on bouge convect le point d'un sommet sur un face .... \n";

		}
	      l[p[0]]=0;
	      l[p[0]]=1. - l[0] - l[1] - l[2] - l[3];
	    }

	  }
      }
    Phat=R3(l+1);
    if(ddd) cout  << "\t\t\t -> "<< dt << " : "  << Phat << " K(Phat) ="<< Th[it](Phat)
		  <<  ", " << kk << endl;
    return kk;
  }

  int  WalkInTet(const Mesh3 & Th,int it, R3 & Phat,const R3 & U, R & dt)
  {
    const R eps = 1e-8;

    // corrected by F.H 23 juin 2015
    bool ddd=verbosity>2000;
    bool nomove=true;
    R lambda[4],dl[4],cl[4];
    Phat.toBary(lambda);
    if(ddd) cout << "\n\t\t\tWT: "  << Phat << " :  "  << lambda[0] << " " <<lambda[1] <<" " <<lambda[2] << " " <<lambda[3] << " u = "<< U << " dt " << dt <<endl;
#ifndef NDEBUG
    for(int i=0;i<4;++i)
      assert(lambda[i]<1.000001 && lambda[i]>-0.0000001);
#endif
    typedef R3 Rd;
    const Mesh3::Element & T(Th[it]);
    const int nve = 4;
    const Rd  Q[nve]={(const R3) T[0],(const R3) T[1],(const R3) T[2],(const R3) T[3]};

    Rd P  =T(Phat);

    Rd PF = P + U*dt;
    Rd PFK= PF;

    //  couleur(15);MoveTo( P); LineTo( PF);
    R l[nve];
    double Det=T.mesure()*6;
    l[0] = det(PF  ,Q[1],Q[2],Q[3]);
    l[1] = det(Q[0],PF  ,Q[2],Q[3]);
    l[2] = det(Q[0],Q[1],PF  ,Q[3]);
    l[3] = Det - l[0]-l[1]-l[2];
    l[0] /= Det;
    l[1] /= Det;
    l[2] /= Det;
    l[3] /= Det;

    if (l[0]>-eps && l[1]>-eps && l[2]>-eps && l[3]>-eps)
      { // a fini  .... ouf ...
	dt =0;
	Phat=R3(l+1);
	nomove=false;
	return -1;
      }

    // l(Q) = lambda + dl  *coef  avec Q= P+ U*dt*coef
    // coef > 0 et segement [PQ] dans K .. et Q \in \partial K
    // recherche de la face de sortie ..

    dl[0]= l[0]-lambda[0];
    dl[1]= l[1]-lambda[1];
    dl[2]= l[2]-lambda[2];
    dl[3]= l[3]-lambda[3];
    // attention a inf et NaN possible ci FH..
    cl[0]= -lambda[0]/dl[0];
    cl[1]= -lambda[1]/dl[1];
    cl[2]= -lambda[2]/dl[2];
    cl[3]= -lambda[3]/dl[3];

    //  min  positif
    int kf=-1;
    double cf=1;// 1 min  positif  < 1.
    if(cl[0]>0. && cl[0] < cf) cf=cl[kf=0]; // OK NaN or Inf test are wrong in any case ..
    if(cl[1]>0. && cl[1] < cf) cf=cl[kf=1];
    if(cl[2]>0. && cl[2] < cf) cf=cl[kf=2];
    if(cl[3]>0. && cl[3] < cf) cf=cl[kf=3];

    if(kf>=0)
      {
	double cf1 = 1-cf;
	// point de sortie en temps dt*cf .. ??
	l[0]-= dl[0]*cf1;
	l[1]-= dl[1]*cf1;
	l[2]-= dl[2]*cf1;
	l[3]-= dl[3]*cf1;
	PFK = P +  U*(dt*cf); // final  point in tet.

      }
    // on sort en temp  cf*dt



    if(ddd)  cout << "\t\t\t WT " << it << ", " << Phat << ",  PFK=" << PFK
		  << " :  "  << l[0] << " " <<l[1] <<" " <<l[2] << " " <<l[3]
		  << " == " << det(Q[0],Q[1],Q[2],PF)/Det
		  << " : l (in) "  << lambda[0] << " " <<lambda[1] <<" " <<lambda[2] << " " <<lambda[3]
		  << " PF= K(l) = " << Th[it](R3(l+1)) << " kf = " << kf << " " << cf << "/ " << PF
		  <<endl ;

    int neg[nve]={},k=0;
    int kk=-1;

    if(kf>=0)   // sortie positive ..
      {
        // on regarde de les reelement negatif
	// on ne veut pas des points sur les faces.
	// car sinon il va y avoir un probleme ans on va projete sur la face
        //  et remettre le point dans le tetraedre.

        if (l[0]<=-eps ) neg[k++]=0;
        if (l[1]<=-eps ) neg[k++]=1;
        if (l[2]<=-eps ) neg[k++]=2;
        if (l[3]<=-eps ) neg[k++]=3;

	if(ddd)  cout << "\t\t\t k= " << k << endl;

        if (k>1) //  2 .. 3 face de sortie possible
          {
	    // regarde  sorti interne ..
	    int pos[4]={};
	    int kp =0;
	    if (l[0]<0 ) pos[kp++]=0;
	    if (l[1]<0 ) pos[kp++]=1;
	    if (l[2]<0 ) pos[kp++]=2;
	    if (l[3]<0 ) pos[kp++]=3;
	    if(kp>0) kk=pos[randwalk(kp)];//
	    else     kk=neg[randwalk(k)];

          }
        else if (k==1) //  une face possible de sortie (cas simple)
          kk = neg[0];

        if(kk>=0)
          {
	    if ( l[kk] ) // on bouge et on arete avant la fin ...
	      {
		R coef1 = 1-cf;
		nomove= (cf<1e-6);
		dt        = dt*coef1;// temps restant
	      }
	    if(ddd) cout << "\t\t\t   \t\t -> kk=" << kk << " , l= "<< lambda[0]  << " "
			 <<lambda[1] << " " <<lambda[2] << " " << lambda[3] << " PF =" << PF <<  " " << &PF <<endl;
            lambda[kk] =0;

          }

      }
    if(nomove )
      // on ne bouge pas on utilse Find ...
      {
	if(ddd) cout << "\t\t\t PF = " << PF << " dt =  " <<  dt  << " " << it << " " << &PF<<endl;
	R dx2= (U,U)*dt*dt;
	R ddt=dt, dc=1;
	// if Udt < h/2 => recherche un point final  (environ)
	if(dx2*dx2*dx2 > Det*Det/4)
	  dt=0;
	else
	  {
	    dc=0.25;
	    ddt=dt*dc;
	    PF= P + U*ddt; // on avance que d'un 1/4
	    dt -= ddt;
	  }
	if(ddd) cout << "\t\t\t PF = " << PF << " " <<  dt << " ddt = " << ddt << " " << it << " " << &PF<<endl;
	bool outside;
	const Mesh3::Element  *K=Th.Find(PF, Phat,outside,&Th[it]);
	if(outside) dt=0; // on a fini
	if(ddd) cout << "\t\t\t ***** WT :  Lock -> Find P+U*ddt*c "<< it<< " " << " -> "<< Th(K)
		     << " dt = " << dt << " c = " << dc << " outside: "<< outside <<" , PF " << PF << endl;
	return 4+Th(K);
      }

    //  on remet le point dans le tet.
    int jj=0;
    R lmx=lambda[0];
    if (lmx<lambda[1])  jj=1, lmx=lambda[1];
    if (lmx<lambda[2])  jj=2, lmx=lambda[2];
    if (lmx<lambda[3])  jj=3, lmx=lambda[3];
    if(lambda[0]<0) lambda[jj] += lambda[0],lambda[0]=0;
    if(lambda[1]<0) lambda[jj] += lambda[1],lambda[1]=0;
    if(lambda[2]<0) lambda[jj] += lambda[2],lambda[2]=0;
    if(lambda[3]<0) lambda[jj] += lambda[3],lambda[3]=0;
    Phat=R3(lambda+1);
    if(ddd) cout  << "\t\t\t -> "<< dt << " : "  << Phat << " K(Phat) ="<< Th[it](Phat) <<  ", " << kk << " jj= "<< jj << " "<< lmx << endl;
    assert(kk<0 || lambda[kk]==0);
    return kk;
  }

  inline int   cleanlambda(double *lambda)
  {
    const R eps=1e-15;
    //  on remet le point dans le tet.
    int jj=0,kk=0;
    R lmx=lambda[0];
    if (lmx<lambda[1])  jj=1, lmx=lambda[1];
    if (lmx<lambda[2])  jj=2, lmx=lambda[2];
    if (lmx<lambda[3])  jj=3, lmx=lambda[3];
    if(lambda[0]<eps) lambda[jj] += lambda[0],lambda[0]=0,kk+=1;
    if(lambda[1]<eps) lambda[jj] += lambda[1],lambda[1]=0,kk+=2;
    if(lambda[2]<eps) lambda[jj] += lambda[2],lambda[2]=0,kk+=4;
    if(lambda[3]<eps) lambda[jj] += lambda[3],lambda[3]=0,kk+=8;
    return kk;
  }

  int  WalkInTetv2(const Mesh3 & Th,int it, R3 & Phat,const R3 & U, R & dt)
  {

    const R eps = 1e-8;

    // corrected by F.H 23 juin 2015
    bool ddd=verbosity>2000;
    R lambda[4],dl[4],cl[4];
    Phat.toBary(lambda);
    if(ddd) cout << "\n\t\t\tWT: "  << Phat << " :  "  << lambda[0] << " " <<lambda[1] <<" " <<lambda[2] << " " <<lambda[3] << " u = "<< U << " dt " << dt <<endl;
#ifndef NDEBUG
    for(int i=0;i<4;++i)
      assert(lambda[i]<1.000001 && lambda[i]>-0.0000001);
#endif
    typedef R3 Rd;
    const Mesh3::Element & T(Th[it]);
    const int nve = 4;
    const Rd  Q[nve]={(const R3) T[0],(const R3) T[1],(const R3) T[2],(const R3) T[3]};

    Rd P  =T(Phat);

    Rd PF = P + U*dt;
    Rd PFK= PF;

    //  couleur(15);MoveTo( P); LineTo( PF);
    R l[nve];
    double Det=T.mesure()*6;
    l[0] = det(PF  ,Q[1],Q[2],Q[3]);
    l[1] = det(Q[0],PF  ,Q[2],Q[3]);
    l[2] = det(Q[0],Q[1],PF  ,Q[3]);
    l[3] = Det - l[0]-l[1]-l[2];
    l[0] /= Det;
    l[1] /= Det;
    l[2] /= Det;
    l[3] /= Det;

    if (l[0]>-eps && l[1]>-eps && l[2]>-eps && l[3]>-eps)
      { // a fini  .... ouf ...
	dt =0;
	Phat=R3(l+1);
	return -1;
      }

    // l(Q) = lambda + dl  *coef  avec Q= P+ U*dt*coef
    // coef > 0 et segement [PQ] dans K .. et Q \in \partial K
    // recherche de la face de sortie ..

    dl[0]= l[0]-lambda[0];
    dl[1]= l[1]-lambda[1];
    dl[2]= l[2]-lambda[2];
    dl[3]= l[3]-lambda[3];
    // attention a inf et NaN possible ci FH..
    cl[0]= -lambda[0]/dl[0];
    cl[1]= -lambda[1]/dl[1];
    cl[2]= -lambda[2]/dl[2];
    cl[3]= -lambda[3]/dl[3];

    //  min  positif
    int kf=-1;
    double cf=1;// 1 min  positif  < 1.
    if(cl[0]>0. && cl[0] < cf) cf=cl[kf=0]; // OK NaN or Inf test are wrong in any case ..
    if(cl[1]>0. && cl[1] < cf) cf=cl[kf=1];
    if(cl[2]>0. && cl[2] < cf) cf=cl[kf=2];
    if(cl[3]>0. && cl[3] < cf) cf=cl[kf=3];

    if(kf>=0)
      {
	double cf1 = 1-cf;
	// point de sortie en temps dt*cf .. ??
	l[0]-= dl[0]*cf1;
	l[1]-= dl[1]*cf1;
	l[2]-= dl[2]*cf1;
	l[3]-= dl[3]*cf1;
	// PFK = P +  U*(dt*cf); // final  point in tet.

      }
    // on sort en temp  cf*dt
    dt*=cf;
    int kk=cleanlambda(lambda);
    Phat=R3(lambda+1);
    if(ddd) cout  << "\t\t\t -> "<< dt << " : "  << Phat << " K(Phat) ="<< Th[it](Phat) <<  ", " << kk << endl;
    assert(kk<0 || lambda[kk]==0);
    return kk;
  }

  const     string GsbeginS="MeshS::GSave v0",GsendS="end";
  void MeshS::GSave(FILE * ff,int offset) const
  {
    PlotStream f(ff);

    f <<  GsbeginS ;
    f << nv << nt << nbe;
    for (int k=0; k<nv; k++) {
      const  Vertex & P = this->vertices[k];
      f << P.x <<P.y << P.z << P.lab ;
    }

    if(nt != 0){

      for (int k=0; k<nt; k++) {
	const Element & K(this->elements[k]);
	int i0=this->operator()(K[0])+offset;
	int i1=this->operator()(K[1])+offset;
	int i2=this->operator()(K[2])+offset;

	int lab=K.lab;
	f << i0 << i1 << i2  << lab;
      }
    }

    for (int k=0; k<nbe; k++) {
      const BorderElement & K(this->borderelements[k]);
      int i0=this->operator()(K[0])+offset;
      int i1=this->operator()(K[1])+offset;
      int lab=K.lab;
      f << i0 << i1  << lab;
    }
    f << GsendS;
  }


  MeshS::MeshS(FILE *f,int offset)
    :liste_v_num_surf(0),v_num_surf(0)
  {
    GRead(f,offset);// remove 1
    assert( (nt >= 0 || nbe>=0)  && nv>0) ;
    BuildBound();
    if(verbosity>2)
      cout << "  -- End of read: mesure = " << mes << " border mesure " << mesb << endl;

    if(nt > 0){
      BuildAdj();
      Buildbnormalv();
      BuildjElementConteningVertex();
    }
    if(verbosity>2)
      cout << "  -- End of read: mesure = " << mes << " border mesure " << mesb << endl;

    if(verbosity>1)
      cout << "  -- MeshS  (File *), d "<< 3  << ", n Tri " << nt << ", n Vtx "
	   << nv << " n Bord " << nbe << endl;
    ffassert(mes>=0); // add F. Hecht sep 2009.
  }


  double MeshS::hmin() const
  {
    R3 Pinf(1e100,1e100,1e100),Psup(-1e100,-1e100,-1e100);   // Extremite de la boite englobante
    double hmin=1e10;

    for (int ii=0;ii< this->nv;ii++) {
      R3 P( vertices[ii].x, vertices[ii].y, vertices[ii].z);
      Pinf=Minc(P,Pinf);
      Psup=Maxc(P,Psup);
    }

    for (int k=0;k<this->nt;k++) {
      for (int e=0;e<3;e++){
        if( this->elements[k].lenEdge(e) < Norme2(Psup-Pinf)/1e9 ){
          const TriangleS & K(this->elements[k]);
          int iv[3];
          for(int jj=0; jj <3; jj++)
            iv[jj] = this->operator()(K[jj]);
              if(verbosity>2) for (int eh=0;eh<3;eh++)
                cout << "TriangleS: " << k << " edge : " << eh << " lenght "<<  this->elements[k].lenEdge(eh) << endl;
              if(verbosity>2) cout << " A triangleS with a very small edge was created " << endl;
                return 1;
          }
          hmin=min(hmin,this->elements[k].lenEdge(e));   // calcul de .lenEdge pour un Mesh3
        }
      }
      ffassert(hmin>Norme2(Psup-Pinf)/1e9);
      return hmin;
  }


  // brute force method
  void MeshS::GRead(FILE * ff,int offset)
  {
    PlotStream f(ff);
    string s;
    f >> s;
    ffassert( s== GsbeginS);
    f >> nv >> nt >> nbe;
    /*if(verbosity>2)*/
    cout << " GRead : nv " << nv << " " << nt << " " << nbe << endl;
    this->vertices = new Vertex[nv];
    this->elements = new Element [nt];
    this->borderelements = new BorderElement[nbe];
    for (int k=0; k<nv; k++) {
      Vertex & P = this->vertices[k];
      f >> P.x >>P.y >> P.z >> P.lab ;
    }
    mes=0.;
    mesb=0.;

    if(nt != 0)
      {

	for (int k=0; k<nt; k++) {
	  int i[3],lab;
	  Element & K(this->elements[k]);
	  f >> i[0] >> i[1] >> i[2]  >> lab;
	  Add(i,3,offset);
	  K.set(this->vertices,i,lab);
	  mes += K.mesure();

	}
      }
    for (int k=0; k<nbe; k++) {
      int i[2],lab;
      BorderElement & K(this->borderelements[k]);
      f >> i[0] >> i[1] >> lab;
        Add(i,2,offset);
      K.set(this->vertices,i,lab);
      mesb += K.mesure();

    }
    f >> s;
    ffassert( s== GsendS);
  }

  const MeshS::Element * MeshS::Find( Rd P, R2 & Phat,bool & outside,const Element * tstart) const //;

  {

  for (int i=0;i<nt;i++)
  {
  kthrough++;
  const TriangleS & K(this->elements[i]);
  R3 A(K[0]),B(K[1]),C(K[2]);
  // the normal n
  R3 n = (B-A)^(C-A);
  // to build the vectorial area
  R a=(Area3(P,B,C),n);
  R b=(Area3(A,P,C),n);
  R c=(Area3(A,B,P),n);
  R s=a+b+c;
  R eps=s*1e-6;
  if (a>-eps && b >-eps && c >-eps && abs((P-A,n)) < eps) {
    Phat=R2(b/s,c/s);
    return this->elements + i;
  }
  }
  return 0; // outside
  }


  MeshS::MeshS(int nnv, int nnt, int nnbe, Vertex3 *vv, TriangleS *tt, BoundaryEdgeS *bb)
    :v_num_surf(0),liste_v_num_surf(0)
 {
    nv = nnv;
    nt = nnt;
    nbe =nnbe;
    vertices = vv;
    elements = tt;
    borderelements = bb;
    mes=0.;
    mesb=0.;

    for (int i=0;i<nt;i++)
      mes += this->elements[i].mesure();
    for (int i=0;i<nbe;i++)
      mesb += this->be(i).mesure();

    BuildBound();
    if(nt > 0){
      BuildAdj();
      Buildbnormalv();
      BuildjElementConteningVertex();
    }
    if(verbosity>1)
      cout << "  -- End of read meshS: mesure = " << mes << " border mesure " << mesb << endl;

    assert(mes>=0.);
  }


  int MeshS::Save(const string & filename) const
  {
    int ver = GmfFloat, outm;
    if ( !(outm = GmfOpenMesh(filename.c_str(),GmfWrite,ver,3)) ) {
      cerr <<"  -- MeshS**::Save  UNABLE TO OPEN  :"<< filename << endl;
      return(1);
    }
    float fx,fy,fz;
    // write vertice (meshS)
    GmfSetKwd(outm,GmfVertices,nv);
    for (int k=0; k<nv; k++) {
      const  Vertex & P = vertices[k];
      GmfSetLin(outm,GmfVertices,fx=P.x,fy=P.y,fz=P.z,P.lab);
    }
    // write triangles (meshS)
    GmfSetKwd(outm,GmfTriangles,nt);
    for (int k=0; k<nt; k++) {
      const MeshS::Element & K(elements[k]);
      int i0=this->operator()(K[0])+1;
      int i1=this->operator()(K[1])+1;
      int i2=this->operator()(K[2])+1;
      int lab=K.lab;
      GmfSetLin(outm,GmfTriangles,i0,i1,i2,lab);
    }
    // write edges (meshS)
    GmfSetKwd(outm,GmfEdges,nbe);
    for (int k=0; k<nbe; k++) {
      const BorderElement & K(borderelements[k]);
      int i0=this->operator()(K[0])+1;
      int i1=this->operator()(K[1])+1;
      int lab=K.lab;
      GmfSetLin(outm,GmfEdges,i0,i1,lab);
    }
    GmfCloseMesh(outm);
    return (0);
  }
} //   End  namespace Fem2D
