/* Copyright 1990-2008, Jsoftware Inc.  All rights reserved.               */
/* Licensed use only. Any other use is in violation of copyright.          */
/*                                                                         */
/* Conjunctions: Power Operator ^: and Associates                          */

#include "j.h"
#define ZZDEFN
#include "result.h"
#define STATEPOSITIVEPOWX 11
#define STATEPOSITIVEPOW (((I)1)<<STATEPOSITIVEPOWX)
#define STATEINFINITEPOWX 12
#define STATEINFINITEPOW (((I)1)<<STATEINFINITEPOWX)
#define STATENEEDNEWPOWX 13
#define STATENEEDNEWPOW (((I)1)<<STATENEEDNEWPOWX)
#define ZZFLAGWORD state


static DF1(jtpowseqlim){PROLOG(0039);A x,y,z,*zv;I i,n;
 RZ(w);
 RZ(z=exta(BOX,1L,1L,20L)); zv=AAV(z); *zv++=x=w;
 i=1; n=AN(z);
 while(1){
  if(n==i){RZ(z=ext(0,z)); zv=i+AAV(z); n=AN(z);}
  A z0; RZ(*zv++=x=df1(z0,y=x,self));
  if(equ(x,y)){AN(z)=*AS(z)=i; break;}
  ++i;
 }
 z=ope(z);
 EPILOG(z);
}    /* f^:(<_) w */

// AR(a) is 1 and AN(w)!=0
static F2(jttclosure){A z;I an,*av,c,d,i,wn,wr,wt,*wv,*zv,*zz;
 RZ(a&&w);
 wt=AT(w); wn=AN(w); wr=AR(w);
 if(B01&wt)RZ(w=cvt(INT,w)); wv=AV(w);
 av=AV(a); an=AN(a);
 RZ(z=exta(INT,1+wr,wn,20L));   // init the expanding result area
 zv=AV(z); zz=zv+AN(z);
 if(1==wn){   // just 1 result, which will be a list
  *zv++=c=*wv;  // store first selection
  do{
   if(zv==zz){i=zv-AV(z); ASSERT(i<=an,EVLIMIT) RZ(z=ext(0,z)); zv=AV(z)+i; zz=AV(z)+AN(z);}  // if we have more results than items in a, there must be a cycle - quit
   d=c; if((UI)c>=(UI)an){c+=an; ASSERT((UI)c<(UI)an,EVINDEX);} *zv++=c=av[c];  // d is prev selection, c is next.  Store c
  }while(c!=d);  // stop when we get a repeated value (normal exit)
  d=(zv-AV(z))-1;
 }else{  // multiple starting points.  Each cell of result has one successor for each starting point.
  ICPY(zv,wv,wn); zv+=wn;
  I resultdiff;
  do{
   if(zv==zz){i=zv-AV(z); ASSERT(i<=an*wn,EVLIMIT) RZ(z=ext(0,z)); zv=AV(z)+i; zz=AV(z)+AN(z);}  // break if there is a cycle
   resultdiff=0; DQ(wn, d=c=zv[-wn]; if((UI)c>=(UI)an){c+=an; ASSERT((UI)c<(UI)an,EVINDEX);} *zv++=c=av[c]; resultdiff|=c^d;);  // set diff if not a steady state
  }while(resultdiff);
  d=(zv-AV(z))/wn-1;
 }
 AS(z)[0]=d; AN(z)=d*wn; MCISH(1+AS(z),AS(w),wr); 
 RETF(z);
}    /* {&a^:(<_) w */

static DF1(jtindexseqlim1){A fs;
 RZ(w); 
 fs=FAV(self)->fgh[0];  // {&x
 R AN(w)&&AT(w)&B01+INT?tclosure(FAV(fs)->fgh[1],w):powseqlim(w,fs);
}    /* {&x^:(<_) w */

static DF2(jtindexseqlim2){
 RZ(a&&w);
 R 1==AR(a)&&AT(a)&INT&&AN(w)&&AT(w)&B01+INT?tclosure(a,w):powseqlim(w,amp(ds(CFROM),a));
}    /* a {~^:(<_) w */

// u^:(<n) If n negative, take inverse of u; if v infinite, go to routine that checks for no change.  Otherwise convert to u^:(i.|n) and restart
static DF1(jtpowseq){A fs,gs,x;I n=IMAX;V*sv;
 RZ(w);
 sv=FAV(self); fs=sv->fgh[0]; gs=sv->fgh[1];
 ASSERT(!AR(gs),EVRANK);
 ASSERT(BOX&AT(gs),EVDOMAIN);
 x=*AAV(gs); if(!AR(x))RE(n=i0(vib(x)));
 if(0>n){RZ(fs=inv(fs)); n=-n;}
 if(n==IMAX||1==AR(x)&&!AN(x))R powseqlim(w,fs);
 R df1(gs,w,powop(fs,IX(n),0));
}    /* f^:(<n) w */

// u^:n w where n is nonnegative finite integer atom (but never 0 or 1, which are handled as special cases)
static DF1(jtfpown){A fs,z;AF f1;I n;V*sv;A *old;
 RZ(w);
 F1PREFIP;
 sv=FAV(self);
 n=AV(sv->fgh[2])[0];
 fs=sv->fgh[0]; f1=FAV(fs)->valencefns[0];
 z=w; 
 old=jt->tnextpushp; 
 DQ(n, JBREAK0; RZ(z=CALL1IP(f1,z,fs)); z=gc(z,old);); // could force inplace after the first?
 RETF(z);
// }
}

// general u^:n w where n is any array or finite atom.  If atom, it will be negative
#if 0  // obsolete
static DF1(jtply1){PROLOG(0040);DECLFG;A b,hs,j,*xv,y,z;B*bv,q;I i,k,m,n,*nv,p=0;AD * RESTRICT x;A *old;  // RESTRICT on x fails in VS2013
 hs=sv->fgh[2]; m=AN(hs); 
 RZ(y=ravel(hs)); RZ(y=from(j=grade1(y),y)); nv=AV(y);  // j is grading permutation of y; y is sorted powers
 GATV0(x,BOX,m,1); xv=AAV(x);  // cannot be virtual
 while(p<m&&0>nv[p])p++;  // find first positive power
 if(p<m){  // if there is a positive power...
  z=w;
  n=nv[m-1]; k=p;
  while(k<m&&!nv[k]){INSTALLBOX(x,xv,k,z); ++k;}  // install the input as the result for any 0 powers
  RZ(b=eq(ainf,from(j,ravel(gs)))); bv=BAV(b); q=k<m?bv[k]:0;
  old=jt->tnextpushp;  // should move this up
  for(i=1;i<=n;++i){
   RZ(z=CALL1(f1,y=z,fs));  // z=next power, y=previous power
   if(q&&equ(y,z)){DQ(m-k, INSTALLBOX(x,xv,k,z); ++k;); break;}  // if there is an infinity, check for repetition; if any, use it for all higher powers, & be done
   while(k<m&&i==nv[k]){INSTALLBOX(x,xv,k,z); ++k; q=k<m?bv[k]:0;}  // otherwise use result for all equal powers
   if(!(i&7)){JBREAK0; if(!gc3((A*)&x,&z,0L,old))R0;}
 }}
 if(0<p){  // if there was a negative power...
  RZ(fs=inv(fs)); f1=FAV(fs)->valencefns[0];
  z=w;
  n=nv[0]; k=p-1;
  RZ(b=eq(scf(-inf),from(j,ravel(gs)))); bv=BAV(b); q=bv[k];
  old=jt->tnextpushp;
  for(i=-1;i>=n;--i){
   RZ(z=CALL1(f1,y=z,fs));
   if(q&&equ(y,z)){DQ(1+k, INSTALLBOX(x,xv,k,z); --k;); break;}
   while(0<=k&&i==nv[k]){INSTALLBOX(x,xv,k,z); --k; q=0<=k?bv[k]:0;}
   if(!(i&7)){JBREAK0; if(!gc3((A*)&x,&z,0L,old))R0;}
 }}
 z=ope(reshape(shape(hs),from(grade1(j),x))); EPILOG(z);
}
#else
static DF1(jtply1){PROLOG(0040);DECLFG;A zz=0;
#define ZZWILLBEOPENEDNEVER 1  // can't honor willbeopened because the results are recycled as inputs
#define ZZPOPNEVER 1  // can't pop the inputs - 
#define ZZDECL
#include "result.h"
 I state=0;  // flags for result.h
 // p =. ~. sn=.(gn=./:,n) { ,n   which gives the list of distinct powers
 A n=sv->fgh[2]; A rn; RZ(rn=ravel(n));  // n is powers, rn is ravel of n
 A gn; RZ(gn=grade1(rn)); A p; RZ(p=nub(from(gn,rn)));  // gn is grade of power, p is sorted list of unique powers we want
 // find index of first nonneg power, remember, set scan pointer, set direction forward.  Set current power to 0.  Indic read of power needed
 I *pv=IAV(p); I np=AN(p);  // base of array of powers, and the number of them
 A z=w;  // the next input/previous result
 I zpow=0;  // The power corresponding to the value in z
 I pscan; for(pscan=0;pscan<np&&pv[pscan]<0;++pscan);  // scan pointer through powers, initialized to point to first nonnegative
 I pscan0=pscan;  // remember where the positive scan started
 state|=STATEPOSITIVEPOW|STATENEEDNEWPOW;  // start in positive direction
 // remember if there is an infinite power in this direction
 state|=(pv[np-1]==IMAX)<<STATEINFINITEPOWX;   // infinities are now +/- IMAX
 ZZPARMS(1,np,1)
#define ZZINSTALLFRAME(optr) *optr++=np;
 // loop for each result power:
 I neededpow;  // the next power we are looking for
 A invfs; if(pv[0]<0)RZ(invfs=inv(fs));   // if there is an inverse, look it up outside the loop so we don't free it
 A *stkfreept=jt->tnextpushp;  // stack-free pointer: we will free non-result values back to this point
 while(1){
  // pscan points to the next power to read - could be out of bounds.  But we might not be reading yet
  // If read of power needed:
  if(state&STATENEEDNEWPOW){
   if(!BETWEENO(pscan,0,np)){
    // No new power this direction; switch direction if currently moving positive
    if(state&STATEPOSITIVEPOW){
     if((pscan=pscan0-1)>=0){  // start on first negative power.  If there is one.
      state&=~(STATEPOSITIVEPOW|STATEINFINITEPOW);  // start in negative direction
      state|=(pv[0]==-IMAX)<<STATEINFINITEPOWX;  // set if there is a __ power
      z=w;  // start back on the initial arg
      zpow=0;  // reset the power corresponding to z
      fs=invfs; f1=FAV(fs)->valencefns[0];  // switch over to the inverse function
     }
    }
    if(!BETWEENO(pscan,0,np))break;  // when we exhaust the powers, quit the loop
   }
   neededpow=pv[pscan]; pscan+=state&STATEPOSITIVEPOW?1:-1; state&=~STATENEEDNEWPOW;  // take next power; advance to following; set no power needed till this consumed
  }
  if((neededpow^REPSGN(neededpow))<=(zpow^REPSGN(neededpow))){  // check for closer to 0, based on direction
   // desired power<=current power, store result and indic power needed.  z has the value to use as result
#define ZZBODY
#include "result.h"
   state|=STATENEEDNEWPOW;   // once we store a power we need the next one
  }else{  // don't execute the verb willy-nilly because the store we just made may be the last  (we could check pscan and execute if we know another power is needed - but don't execute if zpow is infinite)
   // else execute verb, increment current power.  If there is an infinite power, see if result matches input, set current power to infinite if so
   A oldz=z; RZ(z=CALL1(f1,z,fs));  // run the verb
   zpow+=state&STATEPOSITIVEPOW?1:-1;  // increment the power corresponding to z
   if(state&STATEINFINITEPOW){if(equ(oldz,z))zpow=state&STATEPOSITIVEPOW?IMAX:-IMAX;}  // if no change, make z apply to everything.  Don't check unless _ power given
   if(!(zpow&7)){
    // Since we are not popping inside the loop (because each result must be reused even after its values have been moved), we
    // have to pop explicitly.  We protect the components of the overall result and the most recent result
    // We do this only occasionally
    JBREAK0;  // while we're waiting, check for attention interrupt
    if(!gc3(&z,zz?&zz:0,zzbox?&zzbox:0,stkfreept))R0; // free old unused blocks
    if(zzbox){
     // zzbox is normally NONrecursive and we add boxes to it as they come in.  Protecting it has made it recursive, which will
     // cause a double-free if we add another box to it.  So we have to go through it and make it nonrecursive again.  We don't have to do it recursively.
     // This is regrettable, but rare.  If we cared, we could save the whole gc3() call any time the result is going to be stored in zzbox, since there's
     // nothing else to free; but that's not worth it.
     AFLAG(zzbox)&=~BOX; DQ(AN(zzbox), if(AAV(zzbox)[i])tpush(AAV(zzbox)[i]);)  // mark zzbox nonrecursive; for each child, replace the implied free with an explicit one on the stack
    }
   } 
  }
 }
 // collect result
#define ZZEXIT
#include "result.h"
 // Now zz has the result
 // if (there is a negative power) p =. (nnegs }. p) , |. nnegs {. p to match the order in which results were stored
 if(pscan0){A sneg; RZ(sneg=sc(pscan0)); RZ(p=apip(drop(sneg,p),reverse(take(sneg,p))));}
 // result is ($n) $ (p i. ,n) { result - avoid the reshape if n is a list, and avoid the from if (p i. ,n) is an index vector
 RZ(p=indexof(p,rn));  // for each input power, the position of its executed result
 if(!equ(IX(np),p))RZ(zz=from(p,zz));  // order result-cells in order of the input powers
 if(AR(n)!=1)zz=reshape(shape(n),zz);  // if n is an arry, use its shape
 EPILOG(zz);
}
#endif

// u^:_ w  Bivalent, called as w,fs or a,w,fs
static DF2(jtpinf12){PROLOG(0340);A z;  // no reason to inplace, since w must be preserved for comparison, & a for reuse
 I i=0;
 I ismonad=(AT(w)>>VERBX)&1; self=ismonad?w:self;  // the call shows the dyad; if it's really a monad, w->self, leave arg in a
 A fs=FAV(self)->fgh[0]; w=ismonad?fs:w;   // for monad, fs->w
 AF f=FAV(fs)->valencefns[1-ismonad];
 while(1){
  RZ(z=CALL2(f,a,w,fs));  // call the fn, either monadic or dyadic
  A oldw=w; oldw=ismonad?a:oldw; w=ismonad?w:z; a=ismonad?z:a;  // oldw=input w to f; save result for next loop overwriting a(monad) or w(dyad)
  I isend=equ(z,oldw);  // remember if result is same an input
  if(!((isend-1)&++i&7)) {  // every so often, but always when we leave...
   JBREAK0;   // check for user interrupt, in case the function doesn't allocate memory
   RZ(z=EPILOGNORET(z));  // free up allocated blocks, but keep z.  If z is virtual it will be realized
   if(isend)RETF(z);  // return at end
  }
    // make the new result the starting value for next loop, replacing w wherever it is
 }
}

#if 0 // obsolete 
#define DIST(i,x)  if(i==e){v=CAV(x); \
                     while(k<m&&i==(e=nv[jv[k]])){MC(zv+c*jv[k],v,c); ++k;}}

// u^:n w where n is not empty and is either an array or a negative atom (which will abort to ply1)
static DF1(jtply1s){DECLFG;A hs,j,y,y1,z;C*v,*zv;I c,e,i,*jv,k,m,n,*nv,r,*s,t,zn;
 RZ(w);
 hs=sv->fgh[2]; m=AN(hs); nv=AV(hs); 
 RZ(j=grade1(ravel(hs))); jv=AV(j); e=nv[*jv];  // e=lowest power
 if(!e&&!nv[jv[m-1]])R reshape(over(shape(hs),shape(w)),w);  // all powers 0
 RZ(y=y1=CALL1(f1,w,fs)); t=AT(y); r=AR(y);
 if(0>e||t&BOX)R ply1(w,self);  // negative or boxed power - go to general case
 if(!e){
  if(HOMO(t,AT(w)))RZ(w=pcvt(t,w));
  if(!(TYPESEQ(t,AT(w))&&AN(y)==AN(w)&&(r==AR(w)||1>=r&&1>=AR(w))))R ply1(w,self);
 }
 k=AR(hs); RE(zn=mult(m,AN(y)));
 GA(z,AT(y),zn,k+AR(y),0); zv=CAV(z);
 s=AS(z); MCISH(s,AS(hs),k); MCISH(s+k,AS(y),r);
 n=nv[jv[m-1]]; c=AN(y)<<bplg(t); s=AS(y);
 k=0; DIST(0,w); DIST(1,y);
 for(i=2;i<=n;++i){
  RZ(y=CALL1(f1,y,fs));
  if(TYPESNE(t,AT(y))||r!=AR(y)||ICMP(AS(y),s,r))R ply1(w,self);
  DIST(i,y);
 }
 RETF(z);
}    /* f^:n w, non-negative finite n, well-behaved f */
#endif

static DF1(jtinv1){F1PREFIP;DECLFG;A z; RZ(w);A i; RZ(i=inv((fs))); FDEPINC(1);  z=(FAV(i)->valencefns[0])(FAV(i)->flag&VJTFLGOK1?jtinplace:jt,w,i);       FDEPDEC(1); RETF(z);}  // was invrecur(fix(fs))
static DF1(jtinvh1){F1PREFIP;DECLFGH;A z; RZ(w);    FDEPINC(1); z=(FAV(hs)->valencefns[0])(jtinplace,w,hs);        FDEPDEC(1); RETF(z);}
static DF2(jtinv2){DECLFG;A z; RZ(a&&w); FDEPINC(1); df1(z,w,inv(amp(a,fs))); FDEPDEC(1); STACKCHKOFL RETF(z);}  // the CHKOFL is to avoid tail recursion, which prevents a recursion loop from being broken
static DF1(jtinverr){F1PREFIP;ASSERT(0,EVDOMAIN);}  // used for uninvertible monads

// old static CS2(jtply2, df1(z,w,powop(amp(a,fs),gs,0)),0107)  // dyad adds x to make x&u, and then reinterpret the compound.  We could interpret u differently now that it has been changed (x {~^:a: y)
DF2(jtply2){PROLOG(107);DECLFG;A z, zz; PREF2(jtply2); z=(df1(zz,w,powop(amp(a,fs),gs,0))); EPILOG(z);}

static DF1(jtpowg1){A z,h=FAV(self)->fgh[2]; R df1(z,  w,*AAV(h));}
static DF2(jtpowg2){A z,h=FAV(self)->fgh[2]; R df2(z,a,w,*AAV(h));}

// When u^:v is encountered, we replace it with a verb that comes to one of these.
// This creates a verb, jtpowxx, which calls jtdf1 within a PROLOG/EPILOG pair, after creating several names:
// sv->self data; fs=sv->fgh[0] (the A block for the f operand); f1=f1 in sv->fgh[0] (0 if sv->fgh[0]==0); f2=f2 in sv->fgh[0] (0 if sv->fgh[0]==0);
//                gs=sv->fgh[1] (the A block for the g operand); g1=f1 in sv->fgh[1] (0 if sv->fgh[1]==0); g2=f2 in sv->fgh[1] (0 if sv->fgh[1]==0)
// Here, f1 is the original u and g1 is the original v
// We call g1 (=original v), passing in y (and gs as self).  This returns v y
// We then call powop(original u,result of v y), which is the VN case for u^:(v y) and creates a derived verb to perform that function 
// Finally df1 treats the powop result as self, calling self/powop->valencefns[0] (the appropriate power case based on v y)
//   with the y arg as the w operand (and self/powop included to provide access to the original u)
// We allow v to create a gerund, but we do not allow a gerund to create a gerund.

// We catch the special cases 0  & 1 here, mostly for branch-prediction purposes.  All results of g1/g2 will be nouns, while
// most instances of u^:v (run through powop) have v as verb

// here for u^:v y
CS1IP(static,jtpowv1, \
A u; A v; RZ(u=CALL1(g1,  w,gs));  /* execute v */ \
if(!AR(u) && (v=vib(u)) && !(IAV(v)[0]&~1)){z=IAV(v)[0]?(FAV(fs)->valencefns[0])(FAV(fs)->flag&VJTFLGOK1?jtinplace:jt,w,fs):w;} \
else{RESETERR; RZ(u = powop(fs,u,(A)1));  \
z=(FAV(u)->valencefns[0])(FAV(u)->flag&VJTFLGOK1?jtinplace:jt,w,u);} \
,0108)
// here for x u^:v y 
CS2IP(static,static,jtpowv2, \
A u; A v; RZ(u=CALL2(g2,a,w,gs));  /* execute v */ \
if(!AR(u) && (v=vib(u)) && !(IAV(v)[0]&~1)){z=IAV(v)[0]?(FAV(fs)->valencefns[1])(FAV(fs)->flag&VJTFLGOK2?jtinplace:jt,a,w,fs):w;} \
else{RESETERR; RZ(u = powop(fs,u,(A)1));  \
z=(FAV(u)->valencefns[1])(FAV(u)->flag&VJTFLGOK2?jtinplace:jt,a,w,u);} \
,0109)
// here for x u@:]^:v y and x u@]^:v y
CS2IP(static,static,jtpowv2a, \
jtinplace=(J)((I)jtinplace&~JTINPLACEA); /* monads always have IP2 clear */ \
A u; A v; fs=FAV(fs)->fgh[0]; RZ(u=CALL2(g2,a,w,gs));  /* execute v */ \
if(!AR(u) && (v=vib(u)) && !(IAV(v)[0]&~1)){z=IAV(v)[0]?(FAV(fs)->valencefns[0])(FAV(fs)->flag&VJTFLGOK1?jtinplace:jt,w,fs):w;} \
else{RESETERR; RZ(u = powop(fs,u,(A)1));  \
z=(FAV(u)->valencefns[0])(FAV(u)->flag&VJTFLGOK1?jtinplace:jt,w,u);} \
,0110)

// This executes the conjunction u^:v to produce a derived verb.  If the derived verb
// contains verb v or gerund v, it executes v on the xy arguments and then calls jtpowop
// to recreate a new derived verb with the function specified by the result of v.
// The result of v could be a gerund, which would cause the process to continue, so we put the
// kibosh on it by setting self (otherwise unused, and set to nonzero in the initial invocation
// from parse) to 0 in all calls resulting from execution of gerund v.  Then we fail any gerund
// if self is 0.
DF2(jtpowop){A hs;B b;V*v;
 RZ(a&&w);
 ASSERT(AT(a)&VERB,EVDOMAIN);  // u must be a verb
 if(AT(w)&VERB){
  // u^:v.  Create derived verb to handle it.
  v=FAV(a); b=((v->id&~1)==CATCO)&&ID(v->fgh[1])==CRIGHT;  // detect u@]^:v  (or @:)
  // The action routines are inplaceable; take ASGSAFE from u and v, inplaceability from u
  R CDERIV(CPOWOP,jtpowv1cell,b?jtpowv2acell:jtpowv2cell,(v->flag&FAV(w)->flag&VASGSAFE)+(v->flag&(VJTFLGOK1|VJTFLGOK2)), RMAX,RMAX,RMAX);
 }
 // u^:n.  Check for special types.
 if(BOX&AT(w)){A x,y;AF f1,f2;
  // Boxed v.  It could be <n or [v0`]v1`v2 or <''.
  if(!AR(w)&&(x=*AAV(w),!AR(x)&&NUMERIC&AT(x)||1==AR(x)&&!AN(x))){
   // here for <n or <''.  That will be handled by special code.
   f1=jtpowseq; f2=jtply2; v=FAV(a);
   // if u is {&n or {~, and n is <_ or <'', do the tclosure trick
   if((!AN(x)||FL&AT(x)&&inf==*DAV(x))){
    if(CAMP==v->id&&(CFROM==ID(v->fgh[0])&&(y=v->fgh[1],INT&AT(y)&&1==AR(y)))){f1=jtindexseqlim1;}  // {&b^:_ y
    else if(CTILDE==v->id&&CFROM==ID(v->fgh[0])){f2=jtindexseqlim2;}   // x {~^:_ y
   }
   R CDERIV(CPOWOP,f1,f2,VFLAGNONE, RMAX,RMAX,RMAX);  // create the derived verb for <n
  }
//    ASSERT(self,EVDOMAIN);  // If gerund returns gerund, error.  This check is removed pending further design
  R gconj(a,w,CPOWOP);  // create the derived verb for [v0`]v1`v2
 }
 // fall through for unboxed n.
 RZ(hs=vib(w));   // hs=n coerced to integer
 AF f1=jtply1;  // default routine for general array.  no reason to inplace this, since it has to keep the old value to check for changes
 I flag=0;  // flags for the verb we build
 if(!AR(w)){  // input is an atom
  // Handle the 4 important cases: atomic _1 (inverse), 0 (nop), 1 (execute u), and _ (converge/do while)
  if(!(IAV(hs)[0]&~1))R a=IAV(hs)[0]?a:ds(CRIGHT);  //  u^:0 is like ],  u^:1 is like u 
  if((IAV(hs)[0]<<1)==-2){  //  u^:_1 or u^:_
   if(IAV(hs)[0]<0){  // u^:_1
    // if there are no names, calculate the monadic inverse and save it in h.  Inverse of the dyad, or the monad if there are names,
    // must wait until we get arguments
    A h=0; f1=jtinv1; if(nameless(a)){if(h=inv(a)){f1=jtinvh1;}else{f1=jtinverr; RESETERR}} // h must be valid for free.  If no names in w, take the inverse.  If it doesn't exist, fail the monad but keep the dyad going
    flag = (FAV(a)->flag&VASGSAFE) + (h?FAV(h)->flag&VJTFLGOK1:VJTFLGOK1);  // inv1 inplaces and calculates ip for next step; invh has ip from inverse
    R fdef(0,CPOWOP,VERB,(AF)(f1),jtinv2,a,w,h,flag,RMAX,RMAX,RMAX);
   }else{  // u^:_
    R fdef(0,CPOWOP,VERB,jtpinf12,jtpinf12,a,w,0,VFLAGNONE,RMAX,RMAX,RMAX);
   }
  }
  if(IAV(hs)[0]>=0){f1=jtfpown; flag=FAV(a)->flag&VJTFLGOK1;}  // if nonneg atom, go to special routine for that, which supports inplace
 }
 // If not special case, fall through to handle general case
 I m=AN(hs); // m=#atoms of n; n=1st atom; r=n has rank>0
 ASSERT(m,EVDOMAIN);  // empty power is error
// obsolete  b=0; if(m&&AT(w)&FL+CMPX)RE(b=!all0(eps(w,over(ainf,scf(infm)))));   // set b if n is nonempty FL or CMPX array containing _ or __ kludge should just use hs
// obsolete  b|=!m; B nonnegatom=!AR(w)&&0<=IAV(hs)[0]; I flag=FAV(a)->flag&((~b&nonnegatom)<<VJTFLGOK1X);  // b is (empty or contains _/__), (scalar n>=0); if the latter, keep the inplace flag
// obsolete  R fdef(0,CPOWOP,VERB, b?jtply1:nonnegatom?jtfpown:jtply1/*obsolete s*/,jtply2, a,w,hs,   // scaf Create derived verb: special cases pown (nonneg noninfinite atom), ply1s (nonempty not containing infinity)
// obsolete     flag|VFLAGNONE, RMAX,RMAX,RMAX);
 R fdef(0,CPOWOP,VERB, f1,jtply2, a,w,hs,flag, RMAX,RMAX,RMAX);   // Create derived verb: pass in integer powers as h
}
