#ifndef  MatriceCreuse_tpl_
#ifndef  MatriceCreuse_h_
#include "MatriceCreuse.hpp"
#include <set>
#include <list>
#include <map>
#endif

#ifndef __MWERKS__
// test blas 
//  on MacOS9 under MWERKS
//  cblas_ddot macos-9 is not 
#ifdef HAVE_CBLAS_H
extern "C" {
#include <cblas.h> 
}
#define WITHBLAS 1
#elif HAVE_VECLIB_CBLAS_H
#include <vecLib/cblas.h> 
#define WITHBLAS 1
#endif  
#endif  
#ifdef WITHBLAS 
template<class R> inline R blas_sdot(const int n,const R *sx,const int incx,const R *sy,const int  incy)
{
  R s=0;
  
  if(incx == 1 && incy == 1)
   for (int k = 0; k< n; k++)
    s += *sx++ * * sy++;
  else
   for (int k = 0; k< n; k++, sx += incx, sy += incy)
    s += *sx * *sy;   
   return  s;
}
template<> inline float blas_sdot(const int n,const  float *sx,const int incx,const float *sy,const int  incy)
{
  return cblas_sdot(n,sx,incx,sy,incy);
}
template<> inline double blas_sdot(const int n,const  double *sx,const int incx,const double *sy,const int  incy)
{
  return cblas_ddot(n,sx,incx,sy,incy);
}
template<> inline  complex<double> blas_sdot(const int n,const  complex<double> *sx,const int incx,const complex<double> *sy,const int  incy)
{
  complex<double> s;
   cblas_zdotu_sub(n,(const void *)sx,incx,(const void *)sy,incy,( void *)&s);
  return s;
}
template<> inline  complex<float> blas_sdot(const int n,const  complex<float> *sx,const int incx,const complex<float> *sy,const int  incy)
{
  complex<float> s;
   cblas_zdotu_sub(n,(const void *)sx,incx,(const void *)sy,incy,( void *)&s);
  return s;
}
#endif
//  end modif FH
using Fem2D::HeapSort;

//  -----------
inline int  BuildMEK_KK(const int l,int *p,int *pk,int *pkk,const FElement * pKE,const FElement*pKKE)
{
  // routine  build  les array p, pk,pkk 
  // which return number of df int 2 element pKE an pKKE
  // max l size of array p, pk, pkk
  // p[i] is the global number of freedom
  // pk[i] is is the local number in pKE ( -1 if not in pKE element)
  // pkk[i] is is the local number in pKKE ( -1 if not in pKKE element)
  //  remark, if pKKE = 0 => 
     const FElement (*pK[2])={pKE,pKKE};

     int ndf=0; // number of dl
     int * qk=pk, *qkk=pkk;
     for (int k=0;k<2;k++)
      if(pK[k]) 
       {
         if(k) Exchange(qk,qkk);
         const FElement& FEK=*pK[k];
         int nbdf =FEK.NbDoF();
         
         for (int ii=0;ii<nbdf;ii++)
          {
           p[ndf] = FEK(ii); // copy the numbering 
           qk[ndf] = ii;
           qkk[ndf++] = -1;
          } // end for ii
         } 
      assert(ndf <=l);
   // compression suppression des doublons
       Fem2D::HeapSort(p,pk,pkk,ndf);
       int k=0;  
        for(int ii=1;ii<ndf;++ii)
          if (p[k]==p[ii]) // doublons 
            { 
              if (pkk[ii]>=0) pkk[k]=pkk[ii];
              if (pk[ii]>=0) pk[k]=pk[ii];
              assert(pk[k] >=0 && pkk[k]>=0);
            }
           else { // copy 
              p[++k] =p[ii];
              pk[k]=pk[ii];
              pkk[k]=pkk[ii];
             }
        ndf=k+1; 
         
   return ndf;
} //  BuildMEK_KK

template<class R>
void MatriceElementairePleine<R>::call(int k,int ie,int label,void * stack) {
  for (int i=0;i<this->lga;i++) 
     this->a[i]=0;
  if(this->onFace)
    { 
     throwassert(faceelement);
     const Mesh &Th(this->Vh.Th);
     
     int iie=ie,kk=Th.TriangleAdj(k,iie);
     if(kk==k|| kk<0) kk=-1;
     if ( &this->Vh == &this->Uh)
      {
       FElement Kv(this->Vh[k]);
       if(kk<0)
        { // return ; // on saute ????  bof bof 
         this->n=this->m=BuildMEK_KK(this->lnk,this->ni,this->nik,this->nikk,&Kv,0);
         int n2 =this->m*this->n; 
         for (int i=0;i<n2;i++) this->a[i]=0;
         faceelement(*this,Kv,Kv,Kv,Kv,this->data,ie,iie,label,stack);
        }
        else
        {
         FElement KKv(this->Vh[kk]);
         this->n=this->m=BuildMEK_KK(this->lnk,this->ni,this->nik,this->nikk,&Kv,&KKv);
        
         
         faceelement(*this,Kv,KKv,Kv,KKv,this->data,ie,iie,label,stack);

        }
      }
     else 
      {
        ERREUR("A FAIRE/ TO DO  (see F. hecht) ", 0); 
        assert(0); // a faire F. Hecht desole 
       
      }
   }
  else {
  throwassert(element);
  const FElement&Kv(this->Vh[k]);
  int nbdf =Kv.NbDoF();
  for (int i=0;i<nbdf;i++)
     this->ni[i] = Kv(i); // copy the numbering 
  this->m=this->n=nbdf;  

  if(this->ni != this->nj) { // 
    const FElement&Ku(this->Uh[k]);
    int nbdf =Ku.NbDoF();
    for (int i=0;i<nbdf;i++)
      this->nj[i] = Ku(i); // copy the numbering 
     this->m=nbdf;
     int n2 =this->m*this->n; 
     for (int i=0;i<n2;i++) this->a[i]=0;
     element(*this,Ku,Kv,this->data,ie,label,stack);
  }
  else 
    {
     int n2 =this->m*this->n;
     for (int i=0;i<n2;i++) this->a[i]=0;
     element(*this,Kv,Kv,this->data,ie,label,stack);
   // call the elementary mat 
    }  
  }  
}

template<class R>
void MatriceElementaireSymetrique<R>::call(int k,int ie,int label,void * stack) {
  // mise a zero de la matrice elementaire, plus sur
  for (int i=0;i<this->lga;i++) 
     this->a[i]=0;
  if(this->onFace)
    { 
       assert(0); // a faire 
    }
  else {

 if (k< this->Uh.Th.nt)
  {
  throwassert(element);
  const FElement K(this->Uh[k]);
  int nbdf =K.NbDoF();
  for (int i=0;i<nbdf;i++)
     this->ni[i] = K(i); // copy the numbering 
  this->m=this->n = nbdf; 

  element(*this,K,this->data,ie,label,stack); 
  }// call the elementary mat 
  else
  {
  throwassert(mortar);
  {
  const FMortar K(&(this->Uh),k);
  int nbdf = K.NbDoF();
  for (int i=0;i<nbdf;i++)
     this->ni[i] = K(i); // copy the numbering 
  this->m=this->n = nbdf; 
  // mise a zero de la matrice elementaire, plus sur
  
   mortar(*this,K,stack);}
  }
  }
}

template<class R>
MatriceProfile<R>::~MatriceProfile() {
  if(!this->dummy) 
    { //cout << " del mat profile " << endl ;
    if (U && (U !=L))  delete [] U;
    if (D)  delete [] D;
    if (L)  delete [] L;
    if (pU && (pU != pL)) delete [] pU;
    if (pL) delete [] pL;
    //cout << " dl de MatriceProfile " << this << endl;
    }
}
template<class R>
int MatriceProfile<R>::size() const {
  int s = sizeof(MatriceProfile<R>);
  if (D) s += this->n*sizeof(R);
  if (pL) s += this->n*sizeof(int);
  if (pU && (pU != pL)) s += this->n*sizeof(int);
  if (L) s += pL[this->n]*sizeof(int);
  if (U && (U != L)) s += pU[this->n]*sizeof(int);
  return s;
}

template<class R>
  MatriceMorse<R> *MatriceProfile<R>::toMatriceMorse(bool transpose,bool copy) const 
  {
  // A FAIRE;
    assert(0); // TODO
   return 0;
  }
template<class R>
bool MatriceProfile<R>::addMatTo(R coef,map< pair<int,int>, R> &mij)
{
   double eps0=numeric_limits<double>::min();
 if( norm(coef)<eps0) return  L == U ;
 int i,j,kf,k;
  if(D)
   for( i=0;i<this->n;i++)
    if( norm(D[i])>eps0)
     mij[make_pair(i,i)] += coef*D[i];
   else
   for(int i=0;i<this->n;i++) // no dia => identity dai
     mij[make_pair(i,i)]=mij[make_pair(i,i)] += coef;
     
 if (L && pL )    
   for (kf=pL[0],i=0;  i<this->n;   i++  )  
     for ( k=kf,kf=pL[i+1], j=i-kf+k;   k<kf; j++,  k++  )
        if(norm(L[k])>eps0)
        mij[make_pair(i,j)]=L[k]*coef;
 if (U && pU)     
   for (kf=pU[0],j=0;  j<this->m;  j++)  
     for (k=kf,kf=pU[j+1], i=j-kf+k;   k<kf; i++,  k++  )
      if(norm(U[k])>eps0)
        mij[make_pair(i,j)]=U[k]*coef;
 return L == U ; // symetrique               
}
template<class R>
MatriceProfile<R>::MatriceProfile(const int nn,const R *a)
  :MatriceCreuse<R>(nn,nn,0),typefac(FactorizationNO)
{
  int *pf = new int [this->n+1];
  int i,j,k;
  k=0;
  for (i=0;i<=this->n;k+=i++)
    {
      pf[i]=k;
      //  cout << " pf " << i<< " = " << k  << endl;
    }
  throwassert( pf[n]*2 == n*(n-1));
  pU = pf; // pointeur profile U
  pL = pf; // pointeur profile L
  U = new R[pf[this->n]];
  L = new R[pf[this->n]];
  D = new R[this->n];  
  const R *aij=a;
  for (i=0;i<this->n;i++)
    for (j=0;j<this->n;j++)
      if      (j<i)   L[pL[i+1]-i+j] = *aij++;
      else if (j>i)   U[pU[j+1]-j+i] = *aij++;
      else            D[i] = *aij++;
}

template<class R>
MatriceProfile<R>::MatriceProfile(const FESpace & Vh,bool VF) 
  :MatriceCreuse<R>(Vh.NbOfDF,Vh.NbOfDF,0),typefac(FactorizationNO)
{
   // for galerkine discontinue ....
   // VF : true=> Finite Volume matrices (change the stencil) 
   // VF = false => Finite element 
   // F. Hecht nov 2003
   // -----
  this->dummy=0;
  this->n = this->m = Vh.NbOfDF;
  int i,j,k,ke,ie,mn,jl;
  int itab,tabk[5]; 
  int *pf = new int [this->n+1];
  for (i=0;i<this->n;i++)  pf[i]=0;
  for (ke=0;ke<Vh.NbOfElements;ke++)
    { 
      itab=0;
      tabk[itab++]=ke;
      if(VF) itab += Vh.Th.GetAllTriangleAdj(ke,tabk+itab);
      tabk[itab]=-1;    
      mn = this->n;
      for( k=tabk[ie=0]; ie <itab; k=tabk[++ie])
        for (j=Vh(k,jl=0);jl<(int) Vh(k);j=Vh(k,++jl)) 
	      mn = Min ( mn , Vh.FirstDFOfNode(j) ) ;

       //for( k=tabk[ie=0]; ie <itab; k=tabk[++ie])
        { k=ke; // bof bof a verifier finement .... FH
        for (j=Vh(k,jl=0);jl<(int) Vh(k);j=Vh(k,++jl)) 
	     {
	      int df1 = Vh.LastDFOfNode(j);
	      for (int df= Vh.FirstDFOfNode(j);  df < df1; df++  )
	       pf[df] = Max(pf[df],df-mn);
	     }
	     }
    }
  int l =0;
  for (i=0;i<this->n;i++)  {int tmp=l;l += pf[i]; pf[i]=tmp;}
  pf[this->n] = l;
  if(verbosity >3) 
    cout << "  -- SizeOfSkyline =" <<l << endl;

  pU = pf; // pointeur profile U
  pL = pf; // pointeur profile L
  D = 0; // diagonal
  U = 0; // upper part
  L = 0; // lower part 
}

template<class R>
void MatriceProfile<R>::addMatMul(const KN_<R> &x,KN_<R> &ax) const 
{if (x.n!= this->n ) ERREUR(MatriceProfile MatMut(xa,x) ," longueur incompatible x (in) ") ;
 if (ax.n!= this->n ) ERREUR(MatriceProfile MatMut(xa,x) ," longueur incompatible ax (out)") ;
 int i,j,k,kf;
 throwassert(n == m);
 if (D) 
   for (i=0;i<this->n;i++) 
     ax[i] += D[i]*x[i];
 else
   for (i=0;i<this->n;i++) // no dia => identyty dai
     ax[i] +=x[i];
      
 if (L && pL )    
   for (kf=pL[0],i=0;  i<this->n;   i++  )  
     for ( k=kf,kf=pL[i+1], j=i-kf+k;   k<kf; j++,  k++  )
       ax[i] += L[k]*x[j],throwassert(i>=0 && i <n && j >=0 && j < m && k>=0 && k < pL[n]);
       
 if (U && pU)     
   for (kf=pU[0],j=0;  j<this->m;  j++)  
     for (k=kf,kf=pU[j+1], i=j-kf+k;   k<kf; i++,  k++  )
       ax[i] += U[k]*x[j],throwassert(i>=0 && i <n && j >=0 && j < m &&  k>=0 && k < pU[n]);
 

}


template<class R>
void MatriceProfile<R>::operator=(const R & v) {
  if(v!=0.0)
    { cerr << " Mise a zero d'une matrice MatriceProfile<R>::operator=(R v) uniquement v=" << v << endl;
    throw(ErrorExec("exit",1));
    }
  typefac = FactorizationNO;
  delete [] U;
  delete [] L;
  delete [] D;
  U=L=D=0;
}
template<class R>
MatriceCreuse<R>  & MatriceProfile<R>::operator +=(MatriceElementaire<R> & me) {
  int il,jl,i,j,k;
  int * mi=me.ni, *mj=me.nj;
  if (!D)  // matrice vide 
    { D  = new R[this->n];
    L  = pL[this->n] ? new R[pL[this->n]] :0 ;
    for (i =0;i<this->n;i++) D[i] =0;
    for (k =0;k<pL[this->n];k++) L[k] =0;
    switch (me.mtype) {
    case MatriceElementaire<R>::Full :     
      U  = pU[this->n] ? new R[pU[this->n]] : 0;
      for (k =0;k<pU[this->n];k++) U[k] =0;
      break;
    case MatriceElementaire<R>::Symetric :     
      U = L; 
      break;
    default:
      cerr << "Big bug type MatriceElementaire unknown" << (int) me.mtype << endl;
      throw(ErrorExec("exit",1));
      break; 
    }
    }
  R * al = me.a; 
  switch (me.mtype) {
  case MatriceElementaire<R>::Full : //throwassert(L !=U);
    for (i=mi[il=0]; il<me.n; i=mi[++il])  
      for ( j=mj[jl=0]; jl< me.m ; j=mj[++jl],al++)  
	if      (j<i)  L[ pL[i+1] - (i-j) ] += *al;
	else if (j>i)  U[ pU[j+1] - (j-i) ] += *al;
	else           D[i] += *al;
    break;
     
  case MatriceElementaire<R>::Symetric : //throwassert(L ==U);   
    for (i=mi[il=0]; il<me.n; i=mi[++il])  
      for (j=mj[jl=0];jl< il+1 ; j=mj[++jl])  
	if      (j<i)  L[ pL[i+1] - (i-j) ] += *al++;
	else if (j>i)  U[ pU[j+1] - (j-i) ] += *al++;
	else           D[i] += *al++;
    break;
  default:
    cerr << "Big bug type MatriceElementaire unknown" << (int) me.mtype << endl;
    exit(1);
    break; 
  }      
  return *this;
} 

template<class R>
ostream& MatriceProfile<R>::dump (ostream& f) const 
{f<< " matrice profile " << this->n << '\t' << this->m << '\t' ;
 f <<  "  this " << endl;
 f << " pL = " << pL << " L ="  << L << endl
   << " pU = " << pU << " U ="  << U << endl
   << " D = " << D << endl;
 if ( (pL == pU) &&  (U == L) )
   if (pL && L) 
     {f << " matrice profile symetrique " <<endl;
     int i,j,k;
     for (i = 0;i<this->n;i++) 
       { f << i << " {" << pL[i+1]-pL[i] << "}" <<'\t' ;
       for (k=pL[i];k<pL[i+1];k++)
	 { j=i-(pL[i+1]-k);
	 f << j << " " << L[k] << "; "; 
	 }
       f <<  i  << ":" << D[i] << endl  ;
       }
     }
   else f << " MatriceProfile: pointeur vide " <<endl; 
 else 
   { 
     f << " matrice profile non symetrique " << endl;
     int i,k;
     for (i = 0;i<this->n;i++) 
       {
	 f << i ;
	 if (pL && L) 
	   {
	     f << " jO=" << i-pL[i+1]+pL[i] << " L= " <<'\t' ;
	     for (k=pL[i];k<pL[i+1];k++)
	       { 
		 f <<  " " << L[k] ; 
	       }
	   }
	 if (D)
	   f  << " D= " << D[i]  << '\t' ;
	 else 
	   f  << " D=0 => 1 ; ";
	 if (pU && U) 
	   {
	     f << " i0=" << i-pU[i+1]+pU[i] << " U= " <<'\t' ;
	     for (k=pU[i];k<pU[i+1];k++)
	       f << " " << U[k] ; 

	   }
	 f << endl;  
       }
    
   }
 return f;
}
template<class R>
void MatriceProfile<R>::cholesky(double eps) const {
  R  *ij , *ii  , *ik , *jk , xii;
  int i,j,k;
  if (L != U) ERREUR(factorise,"matrice non symetrique");
  U = 0; // 
  typefac = FactorizationCholeski;
  D[0] = sqrt(D[0]); 
  ij = L ; // pointeur sur le terme ij de la matrice avec j<i 
  for (i=1;i<this->n;i++) // boucle sur les lignes 
    { ii = L+pL[i+1]; // pointeur sur le terme fin de la ligne +1 =>  ij < ii;
    xii = D[i] ; 
    for ( ; ij < ii ; ij++) // pour les j la ligne i
      { j = i -(ii - ij); 
      k = Max( j - (pL[j+1]-pL[j]) ,  i-(pL[i+1]-pL[i]) ); 
      ik =  ii - (i - k); 
      jk =  L + pL[j+1] -(j - k); 
      k = j - k ;
      R s= -*ij; 
#ifdef WITHBLAS
      s += blas_sdot(k,ik,1,jk,1);
#else
      while(k--) s += *ik++ * *jk++;  
#endif
      *ij =  -s/D[j] ;
      xii -= *ij * *ij ;
      }
    if ( norm(xii) < norm(eps*D[i])) 
      ERREUR(cholesky,"pivot (" << i << ")= " << xii << " < " << eps*abs(D[i]))
    D[i] = sqrt(xii);
    }
}
template<class R>
void MatriceProfile<R>::crout(double eps) const  {
  R  *ij , *ii  , *ik , *jk , xii, *dkk;
  int i,j,k;
  if (L != U) ERREUR(factorise,"matrice non symetrique");
  U = 0; // 
  typefac = FactorizationCrout;
   
  ij = L ; // pointeur sur le terme ij de la matrice avec j<i 
  for (i=1;i<this->n;i++) // boucle sur les lignes 
    { ii = L+pL[i+1]; // pointeur sur le terme fin de la ligne +1 =>  ij < ii;
    xii = D[i] ; 
    for ( ; ij < ii ; ij++) // pour les j la ligne i
      { j = i -(ii - ij); 
      k = Max( j - (pL[j+1]-pL[j]) ,  i-(pL[i+1]-pL[i]) ); 
      ik =  ii - (i - k); 
      jk =  L + pL[j+1] -(j - k); 
      dkk = D + k;
      k = j - k ; 
      R s=-*ij;
      while ( k-- ) s += *ik++ * *jk++ * *dkk++;  
      *ij = -s/ *dkk ; // k = j ici 

      xii -= *ij * *ij * *dkk;
      }
    if (norm(xii) <= Max(norm(eps*D[i]),1.0e-60))
      ERREUR(crout,"pivot (" << i << " )= " << abs(xii)<< " <= " << eps*abs(D[i]) << " eps = " << eps)
	D[i] = xii;
    }
}
template<class R>
void MatriceProfile<R>::LU(double eps) const  {
  R s,uii;
  int i,j,k;
  if (L == U && ( pL[this->n]  || pU[this->n] ) ) ERREUR(LU,"matrice  symetrique");
  if(verbosity>3)
  cout << " -- LU " << endl;
  typefac=FactorizationLU;

  for (i=1;i<this->n;i++) // boucle sur les sous matrice de rang i 
    { 
      // for L(i,j)  j=j0,i-1
      int j0 = i-(pL[i+1]-pL[i]);
      for ( j = j0; j<i;j++)
        {           
          int k0 = Max(j0,j-(pU[j+1]-pU[j]));
          R *Lik = L + pL[i+1]-i+k0; // lower
          R *Ukj = U + pU[j+1]-j+k0; // upper
          s =0;
#ifdef WITHBLAS
	  s = blas_sdot(j-k0,Lik,1,Ukj,1);
	  Lik += j-k0;
#else
          for (k=k0;k<j;k++) // k < j < i ;
	    s += *Lik++ * *Ukj++ ;     // a(i,k)*a(k,j);
#endif
	  *Lik -= s;
	  *Lik /= D[j]; //  k == j here
        }
      // for U(j,i) j=0,i-1        
      j0=i-pU[i+1]+pU[i];
      for (j=j0;j<i;j++) 
	{
	  s = 0;
	  int k0 = Max(j0,j-pL[j+1]+pL[j]);
	  R *Ljk = L + pL[j+1]-j+k0;   
	  R *Uki = U + pU[i+1]-i+k0;   
#ifdef WITHBLAS
	  s = blas_sdot(j-k0,Ljk,1,Uki,1);
          Uki += j-k0;
#else
	  for (k=k0  ;k<j;k++)    // 
	    s +=  *Ljk++ * *Uki++ ;
#endif
	  *Uki -= s;  // k = j here 
	}
      // for D (i,i) in last because we need L(i,k) and U(k,i) for k<j
      int k0 = i-Min(pL[i+1]-pL[i],pU[i+1]-pU[i]);
      R *Lik = L + pL[i+1]-i+k0; // lower
      R *Uki = U + pU[i+1]-i+k0; // upper
      s =0;
#ifdef WITHBLAS
       s = blas_sdot(i-k0,Lik,1,Uki,1);
#else
      for (k=k0;k<i;k++) // k < i < i ;
	s += *Lik++ * *Uki++ ;     // a(i,k)*a(k,i);
#endif
      // cout << " k0 " << k0 << " i = " << i << " " <<  s << endl;
      uii = D[i] -s;
      
      if (norm(uii) <= Max(norm(eps*D[i]),1.0e-30))
	ERREUR(LU,"pivot (" << i << " )= " << abs(uii) << " <= " << eps*abs(D[i]) << " eps = " << eps);     
      
      D[i] = uii;
      
    }
}


template<class R>
KN_<R> & operator/=(KN_<R> & x ,const MatriceProfile<R> & a) 
{
  // --------------------------------------------------------------------
  //   si La diagonal D n'existe pas alors on suppose 1 dessus (cf crout)
  // --------------------------------------------------------------------
  R * v = &x[0];
  int n = a.n;  
  if (x.n != n ) 
    ERREUR (KN_<R> operator/(MatriceProfile<R>),"  matrice et KN_<R> incompatible");
  const R *ij ,*ii, *ik, *ki;
  R *xk,*xi;
  int i;
  switch (a.typefac) {
  case FactorizationNO:
    if (a.U && a.L) {cerr << "APROGRAMMER (KN_<R><R>::operator/MatriceProfile)";throw(ErrorExec("exit",2));}
   
    if ( a.U && !a.L ) 
      { // matrice triangulaire superieure
	// cout << " remonter " << (a.D ? "DU" : "U") << endl;
	ki = a.U + a.pU[n]; 
	i = n;
	while ( i-- )
	  { ii = a.U + a.pU[i];
          xi= xk  = v +  i ;
          if (a.D) *xi /= a.D[i];// pour crout ou LU
          while ( ki > ii) 
	    *--xk  -=  *--ki *  *xi ; 
	  }
      }
    else if  ( !a.U && a.L ) 
      { // matrice triangulaire inferieure
	// cout << " descente "  <<( a.D ? "LD" : "L" ) <<endl;
	ii = a.L;
	for (i=0; i<n; i++)
	  { ij = ik = (a.L + a.pL[i+1]) ;  // ii =debut,ij=fin+1 de la ligne 
          xk = v + i;
          R ss = v[i]; 
          while ( ik > ii) 
	    ss -= *--ik * *--xk ; 
          if ( a.D) ss /= a.D[i];// pour crout ou LU
          v[i] = ss ;
          ii = ij;
	  }
      }
    else if (a.D) 
      { // matrice diagonale
	// cout << " diagonal D" <<endl;
	for (i=0;i<n;i++) 
	  v[i]=v[i]/a.D[i];
      }
    break;
  case FactorizationCholeski:  
    //     cout << " FactorizationChosleski" << endl;
    x /= a.ld();
    x /= a.ldt();   
    break;
  case FactorizationCrout:
    //   cout << " FactorizationCrout" << endl;
    x /= a.l();
    x /= a.d();
    x /= a.lt();
    break;
  case FactorizationLU:
    //  cout << " FactorizationLU" << endl;
    x  /= a.l();
    x  /= a.du();
    break;
    /*   default:
	 ERREUR  (operator /=(MatriceProfile," Error unkwon type of Factorization  =" << typefac);
    */
  }
  return x;
}

template <class R> 
 MatriceMorse<R>::MatriceMorse(KNM_<R> & A,double tol)
    :MatriceCreuse<R>(A.N(),A.M(),false),solver(0) 
      {
  double tol2=tol*tol;    
  symetrique = false;
  this->dummy=false;
  int nbcoeff=0;
  for(int i=0;i<this->n;i++)
    for(int j=0;j<this->m;j++)
      if(norm(A(i,j))>tol2) nbcoeff++;

  nbcoef=nbcoeff;
  a=new R[nbcoef] ;
  lg=new int [this->n+1];
  cl=new int [nbcoef];
  nbcoeff=0;
  R aij;
  for(int i=0;i<this->n;i++)
   { 
    lg[i]=nbcoeff;
    for(int j=0;j<this->m;j++)
     
      if(norm(aij=A(i,j))>tol2)
       {
         cl[nbcoeff]=j;
         a[nbcoeff]=aij;
         nbcoeff++;
       }
    }
   lg[this->n]=nbcoeff;

  
}
template <class R> 
 MatriceMorse<R>::MatriceMorse(const int  n,const R *aa)
    :MatriceCreuse<R>(n),solver(0) 
      {
  symetrique = true;
  this->dummy=false;
  nbcoef=n;
  a=new R[n] ;
  lg=new int [n+1];
  cl=new int [n];
  for(int i=0;i<n;i++)
   {
    lg[i]=i;
    cl[i]=i;
    a[i]=aa[i];      
      }
lg[n]=n;
}

template<class R>
template<class K>
 MatriceMorse<R>::MatriceMorse(const MatriceMorse<K> & A)
     : MatriceCreuse<R>(A.n,A.m,A.dummy),solver(0),nbcoef(A.nbcoef),
      a(new R[nbcoef]),lg(new int [n+1]), cl(new int[nbcoef]),symetrique(false)
{
  assert(a && lg &&  cl);
  for (int i=0;i<=n;i++)
    lg[i]=A.lg[i];
  for (int k=0;k<=nbcoef;k++)
   {
    cl[k]=A.cl[k];
    a[k]=A.a[k];
    }
    
}



template <class R> 
int MatriceMorse<R>::size() const 
{
  return nbcoef*(sizeof(int)+sizeof(R))+ sizeof(int)*(this->n+1);
}
template <class R> 
ostream& MatriceMorse<R>::dump(ostream & f) const 
{
  f << " Nb line = " << this->n << " Nb Colonne " << this->m << " symetrique " << symetrique << " nbcoef = " << nbcoef <<endl;
  int k=lg[0];
  for (int i=0;i<this->n;i++)
   { 
    
    f << i << " : " << lg[i] <<","<< lg[i+1]-1 << " : " ;
    int ke=lg[i+1];
    for (;k<ke;k++)
      if (norm(a[k])) f  << cl[k] << " " << a[k]<< ", ";
      else f  << cl[k] << " 0., " ;
    f << endl;    
   }
  return f;
}
template <class R> 
inline R*  MatriceMorse<R>::pij(int i,int j) const 
 {
   if (! (i<this->n && j< this->m)) 
   throwassert(i<this->n && j< this->m);
   int i0=lg[i];
   int i1=lg[i+1]-1;
   while (i0<=i1) // dichotomie
    { 
      int im=(i0+i1)/2;
      if (j<cl[im]) i1=im-1;
      else if (j>cl[im]) i0=im+1;
      else return a+im;      
    }
   return 0;     
 }
template <class R> 
void MatriceMorse<R>::Build(const FESpace & Uh,const FESpace & Vh,bool sym,bool VF)
{
   // for galerkine discontinue ....
   // VF : true=> Finite Volume matrices (change the stencil) 
   // VF = false => Finite element 
   // F. Hecht nov 2003
   // -----
  symetrique = sym;
  this->dummy=false;
  a=0;
  lg=0;
  cl=0;
  bool same  = &Uh == & Vh;
  throwassert( &Uh.Th == &Vh.Th);  // same Mesh
  const Fem2D::Mesh & Th(Uh.Th);
  int nbt = Th.nt;
  int nbv = Th.nv;
  int nbm = Th.NbMortars;
  int nbe = Uh.NbOfElements;
  int nbn_u = Uh.NbOfNodes;
  int nbn_v = Vh.NbOfNodes;

  KN<int> mark(nbn_v);
  KN<int> pe_u(nbn_u+1+Uh.SizeToStoreAllNodeofElement());
  //  les element du node i 
  // sont dans pe_u[k] pour k \in [ pe_u[i] , pe_u[i+1] [
  pe_u=0;
  for (int k=0;k<nbe;k++)
   { 
      int nbne=Uh(k);
      for (int in=0;in<nbne;in++)
        pe_u[(Uh(k,in)+1)]++;
   }
  int kk= nbn_u+1,kkk=kk;
  pe_u[0]=kk;
  for (int in1=1;in1<=nbn_u;in1++)
   { // in1 = in + 1
      kk += pe_u[in1];
      pe_u[in1] = kkk; // store the last of in 
      kkk=kk;
   } 
  if(verbosity>4) 
  cout <<" -- MatriceMorse<R>::Build " << kk << " " << nbn_u << " " << Uh.SizeToStoreAllNodeofElement() 
       << " " <<  nbn_u+1+Uh.SizeToStoreAllNodeofElement() << endl;
  throwassert(kk== nbn_u+1+Uh.SizeToStoreAllNodeofElement());
  for (int k=0;k<nbe;k++)
   { 
      int nbne=Uh(k);
      for (int in=0;in<nbne;in++)
        pe_u[pe_u[(Uh(k,in)+1)]++] = k;
   }
    
  
  int color=0;
  mark=color++;
  lg = new int [this->n+1];
  throwassert(lg);
  for (int step=0;step<2;step++) 
   { 
    int ilg=0;
    lg[0]=ilg;
    int kij=0;
    for (int in=0;in<nbn_u;in++)
     {
     int nbj=0; // number of j
     int kijs=kij;
     // for all triangle contening node in
     for (int kk= pe_u[in];kk<pe_u[in+1];kk++)
      {
         int ke=pe_u[kk];// element of 
         int tabk[5];
         int ltab=0;
         tabk[ltab++]=ke;
         if( VF) // if Finite volume then add Triangle adj in stencil ...
           ltab+= Th.GetAllTriangleAdj(ke,tabk+ltab);
         tabk[ltab]=-1;
         for(int ik=0,k=tabk[ik];ik<ltab;k=tabk[++ik])
          {
            throwassert(k>=0 && k < nbe);
            int njloc = Vh(k);
            for (int jloc=0;jloc<njloc;jloc++)
             { 
             int  jn = Vh(k,jloc);
             if (mark[jn] != color && (!sym ||  jn < in) ) 
                {
                 mark[jn] = color;
                 int fdf=Vh.FirstDFOfNode(jn);
                 int ldf=Vh.LastDFOfNode(jn);
                 if (step)
                  for (int j=fdf;j<ldf;j++)
                    cl[kij++] = j;
                  nbj += ldf-fdf;
                }            
           }} 
       }
      int fdf=Uh.FirstDFOfNode(in);
      int ldf=Uh.LastDFOfNode(in);
      int kijl=kij;
      if (step)
       {
       HeapSort(cl+kijs,kij-kijs);
       for (int i=fdf;i<ldf;i++)
        { 
         if (i!=fdf) //  copy the ligne if not the first 
           for (int k=kijs;k<kijl;k++)
            cl[kij++]=cl[k]; 
         if (sym) // add block diag
           for(int j=fdf;j<=i;j++)
            cl[kij++]=j;            
         throwassert(kij==lg[i+1]);// verif           
        }
        }
      else
       for (int i=fdf;i<ldf;i++)
        { 
         if (sym) ilg += ++nbj; // for the diag block
         else ilg += nbj;             
         lg[i+1]=ilg;
        }
       color++; // change the color
    }
    if (step==0) { // do allocation 
      nbcoef=ilg;
      if (verbosity >3)
        cout << "  -- MatriceMorse: Nb coef !=0 " << nbcoef << endl;
      a = new R[nbcoef];
      cl = new int [nbcoef];}
      throwassert( a && cl);
      for (int i=0;i<nbcoef;i++) 
        a[i]=0;
    
   }
  
}
template<class R> inline void ConjArray( R  *v, int n) 
{
  for (int i=0;i<n;++i)
    v[i] = conj(v[i]);
}
template<> inline void ConjArray<double>(double *v, int n) {}
template<> inline void ConjArray<float>(float *v, int n) {}

template<class R>
 void  MatriceMorse<R>::dotransposition()
 {
   if(symetrique) return; 
   
   assert(dummy==false);  
   int *llg= new int[nbcoef];
   int *clg= new int[this->m+1];
   
   for (int i=0;i<this->n;i++)
     for (int k=lg[i];k<lg[i+1];k++)
        llg[k]=i;
 
  HeapSort(cl,llg,a,nbcoef);
  for(int k=0;k<this->m;k++)
    clg[k]=-1;

  // build new line end (old column)
  for(int k=0;k<nbcoef;k++)
    clg[cl[k]+1]=k+1;
      
   for(int kk=0, k=0;k<=this->m;k++)
   if (clg[k]==-1)
      clg[k]=kk;
    else kk=clg[k];
    
  clg[this->m]=nbcoef;
  // sort the new column (old line)
  for(int i=0;i<this->m;i++)  
    HeapSort(llg+clg[i],cl+clg[i],a+clg[i],clg[i+1]-clg[i]); 
  delete[] cl;
  delete[] lg;
  Exchange(this->n,this->m);       
  cl=llg;
  lg=clg;
  ConjArray(a,nbcoef);    
 }

template<class R>
  MatriceMorse<R> * BuildCombMat(const list<pair<R,MatriceCreuse<R> *> >  &lM)
  {
    typedef typename list<pair<R,MatriceCreuse<R> *> >::const_iterator lconst_iterator;
    
    lconst_iterator begin=lM.begin();
    lconst_iterator end=lM.end();
    lconst_iterator i;
    
    map< pair<int,int>, R> mij;
    
    int n=0,m=0;
    bool sym=true;
    for(i=begin;i!=end;i++++)
     {
       MatriceCreuse<R> & M=*i->second;
       assert( &M);
       R coef=i->first;
       if (n==0) { n=M.n; m=M.m;}
       else { assert(n== M.n && m==M.m);}
       sym = M.addMatTo(coef,mij) && sym;              
     } 
    int nbcoef=mij.size();
    if(sym) nbcoef = (nbcoef+n)/2;
/*    MatriceMorse<R> * r=new  MatriceMorse<R>();
    
    r->n=n;
    r->m=m;
    int *lg, *cl;
    R *a;
    r->lg=lg=new int[n+1];
     r->cl=cl=new int [nbcoef];
     r->a=a=new R [nbcoef];        
     r->nbcoef=nbcoef;
     r->symetrique=sym;
     r->dummy=false;
     lg[0]=0;
     int k=0;
     bool nosym=!sym;
     for (typename map< pair<int,int>, R>::iterator iter=mij.begin();iter!=mij.end();++iter)
      { 
        int i=iter->first.first;
        int j=iter->first.second;
        R aij=iter->second;
       if(j<=i || nosym)
        {
        cl[k]=j;
        a[k]=aij;
        lg[i+1]=++k;
        }
       }
   
     return r;
 */
   return new   MatriceMorse<R>(n,m,mij,sym);   
     
  }
template<class R>
bool MatriceMorse<R>::addMatTo(R coef,map< pair<int,int>, R> &mij)
{
  double eps0=numeric_limits<double>::min();
  int i,j,k;
  if (symetrique)
   {
     for ( i=0;i<this->n;i++)
       for ( k=lg[i];k<lg[i+1];k++)
         {
           j=cl[k];
           if(norm(coef*a[k])>eps0)
           {
           mij[make_pair(i,j)] += coef*a[k];
           if (i!=j)
             mij[make_pair(j,i)] += coef*a[k];
           }
         }
           
   }
  else
   {
     for ( i=0;i<this->n;i++)
       for ( k=lg[i];k<lg[i+1];k++)
         {
           j=cl[k];
           if(norm(coef*a[k])>eps0)
           mij[make_pair(i,j)] = coef*a[k];
         }
   }

return symetrique;
}
 
template<class R> 
 template<class K>
  MatriceMorse<R>::MatriceMorse(int nn,int mm, map< pair<int,int>, K> & m, bool sym):
   MatriceCreuse<R>(nn,mm,0),solver(0),nbcoef(m.size()),symetrique(sym),  
   a(new R[nbcoef]),
   lg(new int[n+1]),
   cl(new int[nbcoef])     
  {
     lg[0]=0;
     int k=0;
     bool nosym=!sym;
     typename map< pair<int,int>, R>::iterator iter=m.begin(), mend=m.end();
     for (;iter!=mend;++iter)
      { 
        int i=iter->first.first;
        int j=iter->first.second;
        K aij=iter->second;
        if(j<=i || nosym)
        {
         cl[k]=j;
         a[k]=aij;
         lg[i+1]=++k;
        }
       }
  
  
  }

template<class RA>
 template<class RB,class RAB>
 void  MatriceMorse<RA>::prod(const MatriceMorse<RB> & B, MatriceMorse<RAB> & AB)
 {
   //  compute the s
  bool sym=this == & B &&symetrique;
  int *blg=B.lg;
  int *bcl=B.cl;
  assert(m==B.n); 
  bool delbl= B.symetrique;
  if (delbl)
    {
     int nn=B.n;
      blg = new int[nn+1];
     for (int i=0;i<B.n;i++)
         blg[i]=B.lg[i+1]-B.lg[i];
      blg[nn]=0;   
      
      for (int i=0;i<nn;i++)
        for (int k= B.lg[i];k<B.lg[i+1];k++)
          {  int j=B.cl[k];
              assert(j <= i);
             if (j!=i)  
               blg[j]++;
             }
             
      for (int i=1;i<=nn;i++)
       blg[i]+=blg[i-1];
      int nbnz = blg[nn];
      bcl= new int[nbnz];
      
      for (int i=0;i<B.n;i++)
        for (int k= B.lg[i];k<B.lg[i+1];k++)
          {  int j=B.cl[k];
             assert(j <= i);
             bcl[--blg[i] ]=j;
             if(i !=j)
               bcl[--blg[j]]=i;
          }
    }
   
   set<pair<int,int> > sij;
   double eps0=numeric_limits<double>::min();

     for (int i=0;i<this->n;i++)
       for (int k=lg[i];k<lg[i+1];k++)
         {    
           int j=cl[k];
           if(norm(a[k])<eps0) continue;
           int ii[2],jj[2];
           ii[0]=i;ii[1]=j;
           jj[0]=j;jj[1]=i;
           int kk=1;
           if(symetrique && i != j) kk=2;
           for (int ll=0;ll<kk;ll++)
            {
                int i=ii[ll];
                int j=jj[ll];
                for (int kkb=blg[j];kkb<blg[j+1];kkb++)
                  { 
                   int kz= bcl[kkb];
                   RB bjk;
                   if (B.symetrique && kz > j)
                     bjk=B(kz,j);
                   else
                      bjk=B(j,kz);
                   if( norm(bjk)>eps0 && (!sym || kz<=i))
                     sij.insert(make_pair(i,kz));
                  }
            }
           
         }
    int nn=this->n;
    int mm=B.m;
    int * llg=new int[nn+1];
    int * lcl=new int[sij.size()];  
    RAB * aa = new RAB[sij.size()];
    for(int i=0;i<=nn;i++)
        llg[i]=0;
        
    for (set<pair<int,int> >::iterator iter=sij.begin();iter!=sij.end();++iter)
      { 
        int i=iter->first;
        int j=iter->second;
        llg[i]++;
       }
     for (int i=1;i<=nn;i++)
       llg[i]+=llg[i-1];
     assert(llg[n]==sij.size());
     for (set<pair<int,int> >::iterator iter=sij.begin();iter!=sij.end();++iter)
      { 
        int i=iter->first;
        int j=iter->second;
       // cout << i << " , " << j << endl;
        lcl[--llg[i]]=j;
       }
     for(int i=0;i<nn;i++)  
       HeapSort(lcl+llg[i],llg[i+1]-llg[i]); 
       
     AB.n=nn;
     AB.m=mm;
     AB.lg=llg;
     AB.cl=lcl;
     AB.a=aa;        
     AB.nbcoef=sij.size();
     AB.symetrique=sym;
     AB.dummy=false;
     AB = RAB();
     for (int i=0;i<this->n;i++)
       for (int k=lg[i];k<lg[i+1];k++)
         {    
           int j=cl[k];
           RAB aij = a[k];
           if(norm(aij) <eps0 ) continue;
           int ii[2],jj[2];
           ii[0]=i;ii[1]=j;
           jj[0]=j;jj[1]=i;
           int kk=1;
           if(symetrique && i != j) kk=2;
           for (int ll=0;ll<kk;ll++)
            {
                int i=ii[ll];
                int j=jj[ll];
                for (int kb=blg[j];kb<blg[j+1];kb++)
                  { 
                   int k= bcl[kb];
                   RB bjk;
                   if (B.symetrique && k > j)
                     bjk=B(k,j);
                   else
                      bjk=B(j,k);
                //   cout << i << "," << "," << j << "," << k << " " << aij << " " << bjk << endl;
                   if( norm( bjk)> eps0  && (!sym || k<=i))
                       AB(i,k) += aij*bjk;
                  }
            }
           
         }

    if (delbl) {
      delete [] blg;
      delete [] bcl;
    }
     
     
 }

template<class R>
  void  MatriceMorse<R>::addMatMul(const KN_<R> &  x, KN_<R> & ax) const   
{
  int i,j,k;
  throwassert(n==ax.N());
  throwassert(m==x.N());  
  if (symetrique)
   {
     for (i=0;i<this->n;i++)
       for (k=lg[i];k<lg[i+1];k++)
         {
           j=cl[k];
           ax[i] += a[k]*x[j];
           if (i!=j)
             ax[j] += a[k]*x[i];
         }
           
   }
  else
   {
     for (i=0;i<this->n;i++)
       for (k=lg[i];k<lg[i+1];k++)
         {
           j=cl[k];
           ax[i] += a[k]*x[j];
         }
   }
}

template<class R>
  void  MatriceMorse<R>::addMatTransMul(const KN_<R> &  x, KN_<R> & ax) const   
{
  int i,j,k;
  throwassert(m==ax.N());
  throwassert(n==x.N());  
  if (symetrique)
   {
     for (i=0;i<this->n;i++)
       for (k=lg[i];k<lg[i+1];k++)
         {
           j=cl[k];
           ax[j] += a[k]*x[i];
           if (i!=j)
             ax[i] += a[k]*x[j];
         }
           
   }
  else
   {
     for (i=0;i<this->n;i++)
       for (k=lg[i];k<lg[i+1];k++)
         {
           j=cl[k];
           ax[j] += a[k]*x[i];
         }
   }
}

template<class R>
MatriceMorse<R>  & MatriceMorse<R>::operator +=(MatriceElementaire<R> & me) {
  int il,jl,i,j;
  int * mi=me.ni, *mj=me.nj;
  if ((this->n==0) && (this->m==0))
   {
   
    this->n=me.Uh.NbOfDF;
    this->m=me.Vh.NbOfDF;
    if(verbosity>3)
    cout << " -- Matrice morse  vide on la construit" << endl;
    switch (me.mtype) {
     case MatriceElementaire<R>::Full : 
      Build(me.Uh,me.Vh,false);    
      break;
     case MatriceElementaire<R>::Symetric :     
      Build(me.Uh,me.Vh,true);    
      break;
     default:
      cerr << "Big bug type MatriceElementaire unknown" << (int) me.mtype << endl;
      throw(ErrorExec("exit",1));
      break; }     
   }
  R * al = me.a; 
  R * aij;
  switch (me.mtype) {
  case MatriceElementaire<R>::Full : throwassert(!symetrique);
    for (i=mi[il=0]; il<me.n; i=mi[++il])  
      for ( j=mj[jl=0]; jl< me.m ; j=mj[++jl],al++)  {
        aij = pij(i,j);
        throwassert(aij);
	*aij += *al;}
    break;
     
  case MatriceElementaire<R>::Symetric : throwassert(symetrique);   
    for (i=mi[il=0]; il<me.n; i=mi[++il])  
      for (j=mj[jl=0];jl< il+1 ; j=mj[++jl]) { 
	 aij =    (j<i) ? pij(i,j) : pij(j,i);
         throwassert(aij);
         *aij += *al++;}
    break;
  default:
    cerr << "Big bug type MatriceElementaire unknown" << (int) me.mtype << endl;
    exit(1);
    break; 
  }      
  return *this;
} 

template<class R>
  void MatriceMorse<R>::Solve(KN_<R> &x,const KN_<R> &b) const{
    if (solver)    
      solver->Solver(*this,x,b);
    else
  {  cerr << "Pas de Solver d�fine  pour cette matrice morse " << endl;
    throw(ErrorExec("exit",1));}
  }
/*
void  Element_Op(MatriceElementairePleine & mat,const int k)
{
  
  //  T_RN_ DWi(dWi),DWj(dWi);
  const FESpace & Uh = mat.Uh;
  const FESpace & Vh = mat.Vh;
  const int N = Uh.N;
  const int M = Vh.n;
  const Triangle & T  = Vh.Th[k];
  throwassert(&t == &Uh.Th[k]);  
  const int n = TVhk.NbOfNodes;
  const TypeElement &ke = TVhk;// type de element 
  const QuadratureFormular & FI = QuadratureFormular_T_5;
  int npi;
  R *a=mat.a;
  R *pa=a;
  int i,j;
  throwassert(mat.n==nn);
  throwassert(mat.m==mm);
  throwassert(mat.Op);

  const Opera &Op(*mat.Op);
  if(k==0) cout << " Operator Non symetric: " << Op;
  for (i=0;i< nx;i++) 
    *pa++ = 0.;    
  for (npi=0;npi<FI.n;npi++) // loop on the integration point
    {
      Coor P = FI[npi].c;
      CoorxCoor J;
      R coef;
      
      //      cout << " k = " << k ;
      TVhk.vDFb(P,Vh,k,dWi,J,coef);
      coef *= FI[npi].a;
      pa =a;
      
      for ( i=0;  i<nn;   i++ )  
	{ 
	  T_RNM_ dWii(dWi(i));
	  T_RN_ Wii(Wi(i));
	  for ( j=0;  j<nn;  j++,pa++ ) // 
	    {
	      T_RNM_ dWjj(dWi(j));
	      T_RN_ Wjj(Wj(j));
	      for (int l=0;l<Op.k;l++) // for all the terms of the bilinear form 
		{
		  int id = Op.i[l]%3; // 0: func : 1 dx 2: dy
		  int jd = Op.j[l]%3;
		  int ii =  Op.i[l]/3; // 
		  int jj = Op.j[l]/3;
		  throwassert(ii < N && jj < N); 
		  R wi = id ? dWii(id-1,ii) : Wii[ii];
		  R wj = jd ? dWjj(jd-1,jj) : Wjj[jj];
		  // cout << l << " " << wi << " " << wj << " "  <<  Op.v[l] << endl;
		  *pa += coef * Op.v[l] * wi * wj;

		}
	    }
	}

    }   
 }
/*
void  Element_Op(MatriceElementaireSymetrique & mat,const int k,T_RNM & W,T_RNMK & dW)
{
  //  T_RN_ DW(dW);
  const Interpolation & Vh = mat.Vh;
  int N(Vh.N),N2(2*N);
  const Element & e = Vh.Th[k]; // l'element 
  const TypeInterpolation & TVhk = Vh(k); // type Interpolation de element 
  const int n = TVhk.NbOfNodes;
  const TypeElement &ke = TVhk;// type de element 
  const QuadratureFormular & FI = QuadratureFormular_T_5;
  int npi;
  int i,j;
  R *a=mat.a;
  R *pa=a;
  int nn = N*n;
  int nx = nn*(nn+1)/2;
  throwassert(mat.Op);
  const Opera &Op(*mat.Op);
  if(k==0) cout << " Operator symetric: " << Op;

  for (i=0;i< nx ;i++) 
    *pa++ = 0.;  
  for (npi=0;npi<FI.n;npi++) // loop on the integration point
    {
      Coor P = FI[npi].c;
      TVhk.vFb(P,Vh,k,W);
      CoorxCoor J;
      R coef;
      TVhk.vDFb(P,Vh,k,dW,J,coef);
      coef *= FI[npi].a;
      //   cout << "DW = " << DW << endl;
      pa =a;
      for ( i=0;  i<nn;   i++ )  
	{ 
	  T_RNM_ dWi(dW(i));
	  T_RN_ Wi(W(i));
	  for ( j=0;  j<=i;  j++,pa++ ) // 
	    {
	      T_RNM_ dWj(dW(j));
	      T_RN_ Wj(W(j));
	      for (int l=0;l<Op.k;l++) // for all the terms of the bilinear form 
		{
		  int id = Op.i[l]%3; // 0: func : 1 dx 2: dy
		  int jd = Op.j[l]%3;
		  int ii =  Op.i[l]/3; // 
		  int jj = Op.j[l]/3;
		  throwassert(ii < N && jj < N); 
		  R wi = id ? dWi(id-1,ii) : Wi[ii];
		  R wj = jd ? dWj(jd-1,jj) : Wj[jj];
		  // cout << l << " " << wi << " " << wj << " "  <<  Op.v[l] << endl;
		  *pa += coef * Op.v[l] * wi * wj;

		}
	    }
	}

      
    }    	   	
}



void Int_L(const Interpolation &Uh,Vecteur &f,const Interpolation &Vh, Vecteur &b, const LOpera & LU,const LOpera & LV)
{  
  int k, i,ii,iii,ii1,j,jj,jjj,jj1;
  throwassert(f.N == Uh.NbOfDF);
  throwassert(b.N == Vh.NbOfDF);
  throwassert(&Vh.Th == &Uh.Th);
  //  throwassert(Uh.N == Vh.N);
  int NU = LU.imax();
  int NV = LV.imax();
  // to stock the value of the basic function
  T_RNM   U(Uh.N,Uh.MaxNbOfDFByElement);
  T_RNMK dU(2,Uh.N,Uh.MaxNbOfDFByElement);
  T_RNM V(Vh.N,Vh.MaxNbOfDFByElement);
  T_RNMK dV(2,Vh.N,Vh.MaxNbOfDFByElement);
 
  T_RN Lu(NV);
  

  for( k=0;k<Uh.NbOfElements;k++)
    { 
      const ElementFini & eu = Uh[k]; 
      const ElementFini & ev = Vh[k]; 
      const  TypeInterpolation & ku = eu; // type Interpolation de element 
      const TypeInterpolation & kv = ev; // type Interpolation de element 
      const QuadratureFormular & FI = QuadratureFormular_T_5;
      const TypeElement &ke = ku;// type de element 
      int n = Uh[k];
      int m = Vh[k];
      const Element & e = Uh.Th[k]; // l'element 
      for ( int npi=0; npi<FI.n; npi++ ) // loop on the integration point
	{
	  Coor P = FI[npi];
	  CoorxCoor J;
	  Lu =0;
	  kv.vFb(P,Vh,k,V);
	  ku.vFb(P,Uh,k,U);
	  R coef;
	  ku.vDFb(P,Uh,k,dU,J,coef); // coef = area of e
	  ku.vDFb(P,Vh,k,dV,J,coef); // coef = area of e
	  coef *=  FI[npi].a;
	  // calcule de LU * f * coef = Lu
	  int nn;
	  for ( i=nn=0; i<n; i++ )
	    for (iii=eu[i],ii=Uh.FirstDFOfNode(iii),ii1=Uh.LastDFOfNode(iii);ii<ii1;ii++,nn++)
	      { 
		T_RN_ Unn(U(nn));
		T_RNM_ dUnn(dU(nn));
		R fii = f[ii];
		for (int l=0;l<LU.k;l++) // for all the linear form 
		  {
		    int ii= LU.i[l];
		    int jj= LU.j[l]; 
		    throwassert(jj==0); 
		    const DOpera &f = LU.v[l];
		    // for all the term of the linear form f
		    for (int ll=0;ll<f.k;ll++)
		      {
			R  v = f.v[ll]*coef*fii;
			int iii = f.i[ll];
			int jjj = f.j[ll];
			if(ii<NV) //all the term  ii>= NV are not used 
			  {
			    if (jjj==0)
			      Lu[ii] += Unn [iii]*v;
			    else
			      Lu[ii] += dUnn(jjj-1,iii)*v;
			  }
		      }
		  }
	      }
	  //  cout << Lu << endl;
	  for (nn=j=0;j<m;j++)
	    for (jjj=ev[j], jj=Vh.FirstDFOfNode(jjj),jj1=Vh.LastDFOfNode(jjj);jj<jj1;jj++,nn++)
	      {
		T_RN_ Vnn(V(nn));
		T_RNM_ dVnn(dV(nn));
		R bb=0;
		//  bb = Lu . Lv 
		for (int l=0;l<LV.k;l++) // for all the term of LV
		  {
		    int ii= LV.i[l];
		    int jj= LV.j[l];
		    throwassert(jj==0);
		    const DOpera &f =LV.v[l];
		     // for all the term of the linear form f
		    for (int ll=0;ll<f.k;ll++)
		      {
			R  v = f.v[ll]*Lu[ii];
			int iii = f.i[ll];
			int jjj = f.j[ll];
			if(ii<NV) //all the term  ii>= NV are not used 
			  {
			    if (jjj==0)
			      bb += Vnn [iii]*v;
			    else
			      bb += dVnn(jjj-1,iii)*v;
			  }
			
		      }
		  }
		b[nn] += bb;
	      }
	}
    }
  
}


*/
#endif

