/* Copyright 1990-2007, Jsoftware Inc.  All rights reserved.               */
/* Licensed use only. Any other use is in violation of copyright.          */
/*                                                                         */
/* Symbol Table                                                            */

#include "j.h"


/* a symbol table aka locale is a type LX vector                          */
/* the length is odd after deducting the leading info entry                               */
/* zero elements mean unused entry                                         */
/* non-zero elements are indices in the global symbol pool and             */
/*     are head pointers to a linked list                                  */
/* the first element is symbol pool index for locale info                  */

/* the global symbol pool is a type INT matrix                             */
/* the number of columns is symcol=ceiling(sizeof(L)/sizeof(I))            */
/* elements are interpreted per type L (see jtype.h)                       */
/* a linked list of free entries is kept using the next pointer                  */
// JT(jt,symp):     symbol pool array fixed rank 0, holding L entries (each 3 Is (4 in 32-bit))
// LXAV0(JT(jt,symp))    symbol pool array base.  First ele is base of free chain
// AM(JT(jt,symp)) symbol table index (monotonically increasing) - not really needed


/* numbered locales:                                                       */
// JT(jt,stnum)   A block, data is hashtable holding symtab pointer or 0.  Fixed rank 0
// AN(JT(jt,stnum)) size of hashtable in entries (each 1 L*)
// AK(JT(jt,stnum)) next loc# to allocate
// AM(JT(jt,stnum)) number of entries in use in table

/* named locales:                                                          */
/* JT(jt,stloc):  locales symbol table                                        */


#define symcol ((sizeof(L)+SZI-1)/SZI)

B jtsymext(J jt,B b){A x,y;I j,m,n,*v,xn,yn;L*u;
 if(b){y=JT(jt,symp); j=allosize(y)+NORMAH*SZI; yn=AN(y); n=yn/symcol;}  // extract allo size from header (approx)
 else {            j=((I)1)<<12;                  yn=0; n=1;   }  // n is # rows in chain base + old values
 m=j<<1;                              /* new size in bytes           */
 m-=AKXR(0);                  /* less array overhead         */
 m/=symcol*SZI;                             /* new # rows                  */
 xn=m*symcol;          /* new pool array atoms        */
 GATV0(x,INT,xn,0); v=(I*)LAV0(x);                 /* new pool array              */
 if(b)ICPY(v,LAV0(y),yn);                     /* copy old data to new array  */
 memset(v+yn,C0,SZI*(xn-yn));               /* 0 unused area for safety    */
 u=n+(L*)v; j=1+n;
 DQ(m-n-1, u++->next=(LX)(j++););                 /* build free list extension, leave last chain 0   */
 if(b)u->next=LAV0(JT(jt,symp))[0].next;              /* push extension onto stack   */
 ((L*)v)[0].next=(LX)n;                           /* new base of free chain               */
 ACINITZAP(x); JT(jt,symp)=x;                           /* preserve new array          */
 if(b)fa(y);                                /* release old array           */
 R 1;
}    /* 0: initialize (no old array); 1: extend old array */

// hv->hashtable slot; allocate new symbol, install as head/tail of hash chain, with previous chain appended
// if SYMNEXT(tailx)==0, append at head (immediately after *hv); if SYMNEXT(tailx)!=0, append after tailx.  If queue is empty, tailx is always 0
// The stored chain pointer to the new record is given the non-PERMANENT status from the sign of tailx
// result is new symbol
L* jtsymnew(J jt,LX*hv, LX tailx){LX j;L*u,*v;
 while(!(j=SYMNEXT(LAV0(JT(jt,symp))[0].next)))RZ(symext(1));  /* extend pool if req'd        */
 LAV0(JT(jt,symp))[0].next=LAV0(JT(jt,symp))[j].next;       /* new top of stack            */
 u=j+LAV0(JT(jt,symp));  // the new symbol.  u points to it, j is its index
 if(SYMNEXT(tailx)) {L *t=SYMNEXT(tailx)+LAV0(JT(jt,symp));
  // appending to tail, must be a symbol.  Queue is known to be nonempty
  u->next=t->next;t->next=j|(tailx&SYMNONPERM);  // it's always the end: point to next & prev, and chain from prev.  Everything added here is non-PERMANENT
 }else{
  // appending to head.
  u->next=*hv;  // chain old queue to u
  *hv=j|(tailx);   // set new head, flagged as NONPERM unless suppressed
 }
 R u;
}    /* allocate a new pool entry and insert into hash table entry hv */

// free all the symbols in symbol table w.  As long as the symbols are PERMANENT, delete the values but not the name.
// For non-PERMANENT, delete name and value.
// Reset the fields in the deleted blocks.
// This is used only for freeing local symbol tables, thus does not need to clear the name/path or worry about CACHED values
extern void jtsymfreeha(J jt, A w){I j,wn=AN(w); LX k,* RESTRICT wv=LXAV0(w);
 L *jtsympv=LAV0(JT(jt,symp));  // Move base of symbol block to a register.  Block 0 is the base of the free chain.  MUST NOT move the base of the free queue to a register,
  // because when we free a locale it frees its symbols here, and one of them might be a verb that contains a nested SYMB, giving recursion.  It is safe to move sympv to a register because
  // we know there will be no allocations during the free process.
 // loop through each hash chain, clearing the blocks in the chain.  Do not clear chain 0, which holds x/y bucket numbers
 for(j=SYMLINFOSIZE;j<wn;++j){
  LX *aprev=&wv[j];  // this points to the predecessor of the last block we processed
  // process the chain
  if(k=*aprev){
   // first, free the PERMANENT values (if any), but not the names
   do{
    if(!SYMNEXTISPERM(k))break;  // we are about to free k.  exit if it is not permanent
    aprev=&jtsympv[k].next;  // save last item we processed here
    if(jtsympv[k].val){
     // if the value was abandoned to an explicit definition, we took usecount 8..1  -> 1 ; revert that.  Can't change an ACPERMANENT!
     // otherwise decrement the usecount
     SYMVALFA(jtsympv[k]);
     jtsympv[k].val=0;  // clear value - don't clear name
    }
    k=jtsympv[k].next;
   }while(k);
   // clear chain in last PERMANENT block
   *aprev=0;  // only the PERMANENT survive
   // We are now pointing at the first non-permanent, if any.  Erase them all, deleting the name and value
   if(k){
    LX k1=SYMNEXT(k);  // remember first non-PERMANENT 
    do{
     k=SYMNEXT(k);  // remove address flagging
     aprev=&jtsympv[k].next;  // save last item we processed here
     fa(jtsympv[k].name);fa(jtsympv[k].val);jtsympv[k].name=0;jtsympv[k].val=0;jtsympv[k].sn=0;jtsympv[k].flag=0;
     k=jtsympv[k].next;
    }while(k);
    // make the non-PERMANENTs the base of the free pool & chain previous pool from them
    *aprev=jtsympv[0].next; jtsympv[0].next=k1;
   }
  }
 }
}

static SYMWALK(jtsympoola, I,INT,100,1, 1, *zv++=j;)

F1(jtsympool){A aa,q,x,y,*yv,z,*zv;I i,n,*u,*xv;L*pv;LX j,*v;
 ARGCHK1(w); 
 ASSERT(1==AR(w),EVRANK); 
 ASSERT(!AN(w),EVLENGTH);
 GAT0(z,BOX,3,1); zv=AAV(z);
 n=AN(JT(jt,symp))/symcol; pv=LAV0(JT(jt,symp));
 GATV0(x,INT,n*5,2); AS(x)[0]=n; AS(x)[1]=5; xv= AV(x); zv[0]=incorp(x);  // box 0: sym info
 GATV0(y,BOX,n,  1);                         yv=AAV(y); zv[1]=incorp(y);  // box 1: 
 for(i=0;i<n;++i,++pv){         /* per pool entry       */
  *xv++=i;   // sym number
  *xv++=(q=pv->val)?LOWESTBIT(AT(pv->val)):0;  // type: only the lowest bit.  Must allow SYMB through
  *xv++=pv->flag+(pv->name?LHASNAME:0)+(pv->val?LHASVALUE:0);  // flag
  *xv++=pv->sn;    
  *xv++=SYMNEXT(pv->next);
  RZ(*yv++=(q=pv->name)?incorp(sfn(SFNSIMPLEONLY,q)):mtv);  // simple name
 }
 // Allocate box 3: locale name
 GATV0(y,BOX,n,1); yv=AAV(y); zv[2]=incorp(y);
 DO(n, yv[i]=mtv;);
 n=AN(JT(jt,stloc)); v=LXAV0(JT(jt,stloc)); 
 for(i=0;i<n;++i){  // for each chain-base in locales pool
  for(j=v[i];j=SYMNEXT(j),j;j=LAV0(JT(jt,symp))[j].next){      // j is index to named local entry; process the chain
   x=LAV0(JT(jt,symp))[j].val;  // x->symbol table for locale
   RZ(yv[j]=yv[LXAV0(x)[0]]=aa=incorp(sfn(SFNSIMPLEONLY,LOCNAME(x))));  // install name in the entry for the locale
   RZ(q=sympoola(x)); u=AV(q); DO(AN(q), yv[u[i]]=aa;);
  }
 }
 n=jtcountnl(jt);
 for(i=0;i<n;++i)if(x=jtindexnl(jt,i)){   /* per numbered locales */
  RZ(      yv[LXAV0(x)[0]]=aa=incorp(sfn(SFNSIMPLEONLY,LOCNAME(x))));
  RZ(q=sympoola(x)); u=AV(q); DO(AN(q), yv[u[i]]=aa;);
 }
 if(AN(x=jt->locsyms)>1){               /* per local table      */
  RZ(aa=incorp(cstr("**local**")));
  RZ(q=sympoola(x)); u=AV(q); DO(AN(q), yv[u[i]]=aa;);
 }
 RETF(z);
}    /* 18!:31 symbol pool */

// l/string are length/addr of name, hash is hash of the name, g is symbol table
// the symbol is deleted if found.  Return address of deleted symbol if it was cached - caller must then take responsibility for the name
// if the symbol is PERMANENT, it is not deleted but its value is removed
// if the symbol is CACHED, it is removed from the chain but otherwise untouched, leaving the symbol abandoned.  It is the caller's responsibility to handle the name
L* jtprobedel(J jt,I l,C*string,UI4 hash,A g){
 RZ(g);
 LX *asymx=LXAV0(g)+SYMHASH(hash,AN(g)-SYMLINFOSIZE);  // get pointer to index of start of chain; address of previous symbol in chain
 while(1){
  LX delblockx=SYMNEXT(*asymx);
  if(!delblockx)R 0;  // if chain empty or ended, not found
  L *sym=LAV0(JT(jt,symp))+delblockx;  // address of next in chain, before we delete it
  IFCMPNAME(NAV(sym->name),string,l,     // (1) exact match - if there is a value, use this slot, else say not found
    {
     if(unlikely(sym->flag&LCACHED)){
      *asymx=sym->next;   // cached: just unhook from chain.  can't be permanent
      R sym;
     }else{
      SYMVALFA(*sym); sym->val=0;   // decr usecount in value; remove value from symbol
      if(!(sym->flag&LPERMANENT)){*asymx=sym->next; fa(sym->name); sym->name=0; sym->flag=0; sym->sn=0; sym->next=LAV0(JT(jt,symp))[0].next; LAV0(JT(jt,symp))[0].next=delblockx;}  // add to symbol free list
     }
     R 0;
    }
   // if match, bend predecessor around deleted block, return address of match (now deleted but still points to value)
  )
  asymx=&sym->next;   // mismatch - step to next
 }
}

// l/string are length/addr of name, hash is hash of the name, g is symbol table
// result is L* address of the symbol-table entry for the name, or 0 if not found
L*jtprobe(J jt,I l,C*string,UI4 hash,A g){
 RZ(g);
 LX symx=LXAV0(g)[SYMHASH(hash,AN(g)-SYMLINFOSIZE)];  // get index of start of chain
 while(1){
  if(!symx)R 0;  // if chain empty or ended, not found
  symx=SYMNEXT(symx);
  L *sym=LAV0(JT(jt,symp))+symx;
  IFCMPNAME(NAV(sym->name),string,l,R sym->val?sym:0;)     // (1) exact match - if there is a value, use this slot, else say not found
  symx=sym->next;   // mismatch - step to next
 }
}

// a is A for name; result is L* address of the symbol-table entry in the local symbol table, if there is one
// If the value is empty, return 0 for not found
// This code is copied in p.c
L *jtprobelocal(J jt,A a,A locsyms){NM*u;I b,bx;
 // There is always a local symbol table, but it may be empty
 ARGCHK1(a);u=NAV(a);  // u->NM block
 if(likely((b = u->bucket)!=0)){
  if(0 > (bx = ~u->bucketx)){
   // positive bucketx (now negative); that means skip that many items and then do name search.  This is set for words that were recognized as names but were not detected as assigned-to in the definition
   // If no new names have been assigned since the table was created, we can skip this search, since it must fail (this is the path for words in z eg)
   if(likely(!(AR(locsyms)&LNAMEADDED)))R 0;
   LX lx = LXAV0(locsyms)[b];  // index of first block if any
   I m=u->m; C* s=u->s;  // length/addr of name from name block
   while(0>++bx){lx = LAV0(JT(jt,symp))[lx].next;}  // all PERMANENT
   // Now lx is the index of the first name that might match.  Do the compares
   while(lx=SYMNEXT(lx)) {L* l = lx+LAV0(JT(jt,symp));  // symbol entry
    IFCMPNAME(NAV(l->name),s,m,R l->val?l : 0;)
    lx = l->next;
   }
   R 0;  // no match.
  } else {
   LX lx = LXAV0(locsyms)[b];  // index of first block if any
   L* l = lx+LAV0(JT(jt,symp));  // fetch hashchain headptr, point to L for first symbol
   // negative bucketx (now positive); skip that many items, and then you're at the right place
   while(bx--){l = l->next+LAV0(JT(jt,symp));}  // all permanent
   R l->val?l:0;
  }
 } else {
  // No bucket information, do full search.  This includes names that don't come straight from words in an explicit definition
  R probe(NAV(a)->m,NAV(a)->s,NAV(a)->hash,locsyms);
 }
}

// a is A for name; result is L* address of the symbol-table entry in the local symbol table (which must exist)
// If not found, one is created
L *jtprobeislocal(J jt,A a){NM*u;I b,bx;L *sympv=LAV0(JT(jt,symp));
 // If there is bucket information, there must be a local symbol table, so search it
 ARGCHK1(a);u=NAV(a);  // u->NM block
 if(b = u->bucket){
  LX lx = LXAV0(jt->locsyms)[b];  // index of first block if any
  if(0 > (bx = ~u->bucketx)){
   // positive bucketx (now negative); that means skip that many items and then do name search
   // Even if we know there have been no names assigned we have to spin to the end of the chain
   I m=u->m; C* s=u->s;  // length/addr of name from name block
   LX tx = lx;  // tx will hold the address of the last item in the chain, in case we have to add a new symbol
   L* l;

   while(0>++bx){tx = lx; lx = sympv[lx].next;}  // all permanent
   // Now lx is the index of the first name that might match.  Do the compares
   while(lx=SYMNEXT(lx)) {
    l = lx+sympv;  // symbol entry
    IFCMPNAME(NAV(l->name),s,m,R l;)
    tx = lx; lx = l->next;
   }
   // not found, create new symbol.  If tx is 0, the queue is empty, so adding at the head is OK; otherwise add after tx.  Make it non-PERMANENT
   RZ(l=symnew(&LXAV0(jt->locsyms)[b],tx|SYMNONPERM)); 
   ra(a); l->name=a;  // point symbol table to the name block, and increment its use count accordingly
   AR(jt->locsyms)|=LNAMEADDED;  // Mark that a name has been added beyond what was known at preprocessing time
   R l;
  } else {L* l = lx+sympv;  // fetch hashchain headptr, point to L for first symbol
   // negative bucketx (now positive); skip that many items, and then you're at the right place
   while(bx--){l = l->next+sympv;}  // all permanent
   R l;  // return 
  }
 } else {
  // No bucket information, do full search
  L *l=probeis(a,jt->locsyms);
  RZ(l);
  AR(jt->locsyms)|=(~l->flag)&LPERMANENT;  // Mark that a name has been added beyond what was known at preprocessing time, if the added name is not PERMANENT
  R l;
 }
}



// a is A for name
// g is symbol table to use
// result is L* symbol-table entry to use
// if not found, one is created
L*jtprobeis(J jt,A a,A g){C*s;LX *hv,tx;I m;L*v;NM*u;L *sympv=LAV0(JT(jt,symp));
 u=NAV(a); m=u->m; s=u->s; hv=LXAV0(g)+SYMHASH(u->hash,AN(g)-SYMLINFOSIZE);  // get bucket number among the hash tables
 if(tx=SYMNEXT(*hv)){                                 /* !*hv means (0) empty slot    */
  v=tx+sympv;
  while(1){                               
   u=NAV(v->name);
   IFCMPNAME(u,s,m,R v;)    // (1) exact match - may or may not have value
   if(!v->next)break;                                /* (2) link list end */
   v=(tx=SYMNEXT(v->next))+sympv;
  }
 }
 // not found, create new symbol.  If tx is 0, the queue is empty, so adding at the head is OK; otherwise add after tx
 RZ(v=symnew(hv,tx|SYMNONPERM));   // symbol is non-PERMANENT
 ra(a); v->name=a;  // point symbol table to the name block, and increment its use count accordingly
 R v;
}    /* probe for assignment */

// look up a non-locative name using the locale path
// g is the current locale, l/string=length/name, hash is the hash for it
// result is L* symbol-table slot for the name, or 0 if none
// Bit 0 of the result is set iff the name was found in a named locale
L*jtsyrd1(J jt,I l,C *string,UI4 hash,A g){A*v,x,y;L*e;
// if(b&&jt->local&&(e=probe(NAV(a)->m,NAV(a)->s,NAV(a)->hash,jt->local))){av=NAV(a); R e;}  // return if found local
 RZ(g);  // make sure there is a locale...
 if(e=probe(l,string,hash,g)){e=(L*)((I)e+AR(g)); R e;}  // and if the name is defined there, use it
 RZ(y = LOCPATH(g));   // Not found in locale.  We must use the path
 v=AAV(y); 
 // in LOCPATH the 'shape' of each string is used to store the bucketx
 DO(AN(y), x=v[i]; if(e=probe(l,string,hash,g=stfindcre(AN(x),CAV(x),AS(x)[0]))){e=(L*)((I)e+AR(g)); break;});  // return when name found.  Create path locale if it does not exist
 R e;  // fall through: not found
}    /* find name a where the current locale is g */ 
// same, but return the locale in which the name is found.  We know the name will be found somewhere
A jtsyrd1forlocale(J jt,I l,C *string,UI4 hash,A g){A*v,x,y;L*e;
// if(b&&jt->local&&(e=probe(NAV(a)->m,NAV(a)->s,NAV(a)->hash,jt->local))){av=NAV(a); R e;}  // return if found local
 RZ(g);  // make sure there is a locale...
 if(e=probe(l,string,hash,g))R g;  // and if the name is defined there, use it
 RZ(y = LOCPATH(g));   // Not found in locale.  We must use the path
 v=AAV(y); 
 // in LOCPATH the 'shape' of each string is used to store the bucketx
 DO(AN(y), x=v[i]; if(e=probe(l,string,hash,g=stfindcre(AN(x),CAV(x),AS(x)[0])))break;);  // return when name found.  Create path locale if it does not exist
 R g;
}


// u is address of indirect locative: in a__b__c, it points to the b
// n is the length of the entire locative (4 in this example)
// result is address of symbol table to use for name lookup (if not found, it is created)
static A jtlocindirect(J jt,I n,C*u,UI4 hash){A x,y;C*s,*v,*xv;I k,xn;
 A g;  // current locale.
 L *e=0;  // value looked up
 s=n+u;   // s->end+1 of name
 while(u<s){
  v=s; while('_'!=*--v); ++v;  // v->start of last indirect locative
  k=s-v; s=v-2;    // k=length of indirect locative; s->end+1 of next name if any
  if(!e){  // first time through
   e=probe(k,v,hash,jt->locsyms);  // look up local first
   if(!e)e=(L*)((I)syrd1(k,v,hash,jt->global)&~1);  // if not local, try global, and remove cachable flag
  }else e=(L*)((I)syrd1(k,v,(UI4)nmhash(k,v),g)&~1);   // look up later indirect locatives, yielding an A block for a locative; remove cachable flag
  ASSERTN(e,EVVALUE,nfs(k,v));  // verify found
  y=e->val;    // y->A block for locale
  ASSERTN(!AR(y),EVRANK,nfs(k,v));   // verify atomic
  if(AT(y)&(INT|B01)){g=findnl(BIV0(y)); ASSERT(g!=0,EVLOCALE);  // if atomic integer, look it up
  }else{
   ASSERTN(BOX&AT(y),EVDOMAIN,nfs(k,v));  // verify box
   x=AAV(y)[0]; if((((I)AR(x)-1)&-(AT(x)&(INT|B01)))<0) {
    // Boxed integer - use that as bucketx
    g=findnl(BIV0(x)); ASSERT(g!=0,EVLOCALE);  // boxed integer, look it up
   }else{
    xn=AN(x); xv=CAV(x);   // x->boxed contents, xn=length, xv->string
    ASSERTN(1>=AR(x),EVRANK,nfs(k,v));   // verify list (or atom)
    ASSERTN(xn,EVLENGTH,nfs(k,v));   // verify not empty
    ASSERTN(LIT&AT(x),EVDOMAIN,nfs(k,v));  // verify string
    ASSERTN(vlocnm(xn,xv),EVILNAME,nfs(k,v));  // verify legal name
    I bucketx=BUCKETXLOC(xn,xv);
    RZ(g=stfindcre(xn,xv,bucketx));  // find st for the name
   }
  }
 }
 R g;
}

// look up known locative name; return starting locale of locative, or 0 if error
A jtsybaseloc(J jt,A a) {I m,n;NM*v;
 n=AN(a); v=NAV(a); m=v->m;
 // Locative: find the indirect locale to start on, or the named locale, creating the locale if not found
 if(likely(!(NMILOC&v->flag)))R stfindcre(n-m-2,1+m+v->s,v->bucketx);
 R locindirect(n-m-2,2+m+v->s,(UI4)v->bucketx);
}

// look up a name (either simple or locative) using the full name resolution
// result is symbol-table slot for the name if found, or 0 if not found
// This code is copied in p.c
L*jtsyrd(J jt,A a,A locsyms){A g;
 ARGCHK1(a);
 if(!(NAV(a)->flag&(NMLOC|NMILOC))){L *e;
  // If there is a local symbol table, search it first
  if(e = probelocal(a,locsyms)){R e;}  // return flagging the result if local
  g=jt->global;  // Continue with the current locale
 } else RZ(g=sybaseloc(a));
 R (L*)((I)syrd1(NAV(a)->m,NAV(a)->s,NAV(a)->hash,g)&~1);  // Not local: look up the name starting in locale g
}
// same, but return locale in which found
A jtsyrdforlocale(J jt,A a){A g;
 ARGCHK1(a);
 if(!(NAV(a)->flag&(NMLOC|NMILOC))){L *e;
  // If there is a local symbol table, search it first
  if(e = probelocal(a,jt->locsyms)){R jt->locsyms;}  // return flagging the result if local
  g=jt->global;  // Start with the current locale
 } else RZ(g=sybaseloc(a));
 R syrd1forlocale(NAV(a)->m,NAV(a)->s,NAV(a)->hash,g);  // Not local: look up the name starting in locale g
}
// same as syrd, but we have already checked for buckets
// look up a name (either simple or locative) using the full name resolution
// result is symbol-table slot for the name if found, or 0 if not found
// This code is copied in p.c
L*jtsyrdnobuckets(J jt,A a){A g;
 ARGCHK1(a);
 if(!(NAV(a)->flag&(NMLOC|NMILOC))){L *e;
  // If there is a local symbol table, search it first
  if(!NAV(a)->bucket && (e = probe(NAV(a)->m,NAV(a)->s,NAV(a)->hash,jt->locsyms))){R e;}  // return if found locally from name
  g=jt->global;  // Start with the current locale
 } else RZ(g=sybaseloc(a));  // if locative, start in locative locale
 R (L*)((I)syrd1(NAV(a)->m,NAV(a)->s,NAV(a)->hash,g)&~1);  // Not local: look up the name starting in locale g
}


static A jtdllsymaddr(J jt,A w,C flag){A*wv,x,y,z;I i,n,*zv;L*v;
 ARGCHK1(w);
 n=AN(w); wv=AAV(w); 
 ASSERT(!n||BOX&AT(w),EVDOMAIN);
 GATV(z,INT,n,AR(w),AS(w)); zv=AV(z); 
 for(i=0;i<n;++i){
  x=wv[i]; v=syrd(nfs(AN(x),CAV(x)),jt->locsyms); 
  ASSERT(v!=0,EVVALUE);
  y=v->val;
  ASSERT(NOUN&AT(y),EVDOMAIN);
  zv[i]=flag?(I)AV(y):(I)v;
 }
 RETF(z);
}    /* 15!:6 (0=flag) or 15!:14 (1=flag) */

F1(jtdllsymget){R dllsymaddr(w,0);}
F1(jtdllsymdat){R dllsymaddr(w,1);}

// look up the name w using full name resolution.  Return the value if found, abort if not found or invalid name
F1(jtsymbrd){L*v; ARGCHK1(w); ASSERTN(v=syrd(w,jt->locsyms),EVVALUE,w); R v->val;}

// look up name w, return value unless locked or undefined; then return just the name
F1(jtsymbrdlocknovalerr){A y;L *v;
 if(!(v=syrd(w,jt->locsyms))){
  // no value.  Could be undefined name (no error) or some other error including value error, which means error looking up an indirect locative
  // If error, abort with it; if undefined, return a reference to the undefined name
  RE(0);   // if not simple undefined, error
  R nameref(w,jt->locsyms);  // return reference to undefined name
 }
 // no error.  Return the value unless locked function
 y=v->val;
 R FUNC&AT(y)&&(jt->glock||VLOCK&FAV(y)->flag)?nameref(w,jt->locsyms):y;
}

// Same, but value error if name not defined
F1(jtsymbrdlock){A y;
 RZ(y=symbrd(w));
 R FUNC&AT(y)&&(jt->glock||VLOCK&FAV(y)->flag)?nameref(w,jt->locsyms):y;
}


// w is a value, v is the symbol-table entry about to be assigned
// Called only in debug mode.  If we redefine an executing name, it would invalidate
// the debug stack.  Fail if any redefinition would change part of speech or the id of the executing function.
// If the currently-executing definition is reloaded, mark the stack entry: xdefn will pick it up when debug is on
// as modified - xdefn will try to hot-swap to the new definition between lines
// If the modified name is executing higher on the stack, fail
// returns nonzero for OK to allow the assignment to proceed
B jtredef(J jt,A w,L*v){A f;DC c,d;
 // find the most recent DCCALL, exit if none
 d=jt->sitop; while(d&&!(DCCALL==d->dctype&&d->dcj))d=d->dclnk; if(!(d&&DCCALL==d->dctype&&d->dcj))R 1;
 if(v==(L*)d->dcn){  // if the saved stabent (from the unquote lookup) matches the name being assigned...
  // attempted reassignment of the executing name
  // insist that the redefinition have the same type, and the same explicit character
  f=d->dcf;
  ASSERTN(TYPESEQ(AT(f),AT(w))&&(CCOLON==FAV(f)->id)==(CCOLON==FAV(w)->id),EVSTACK,d->dca);
  d->dcf=w;
  // If we are redefining the executing explicit definition during debug, remember that.
  // debug will switch over to the new definition before the next line is executed.
  // Reassignment outside of debug continues executing the old definition
  if(CCOLON==FAV(w)->id){d->dcredef=1;}
  // Erase any stack entries after the redefined call
  c=jt->sitop; while(c&&DCCALL!=c->dctype){c->dctype=DCJUNK; c=c->dclnk;}
 }
 // Don't allow redefinition of a name that is suspended higher up on the stack
 c=d; while(c=c->dclnk){ ASSERTN(!(DCCALL==c->dctype&&v==(L*)c->dcn),EVSTACK,c->dca);}
 R 1;
}    /* check for changes to stack */

// find symbol entry for name a in symbol table g; this is known to be global assignment
// the name a may require lookup through the path; once we find the locale, we search only in it
// Result is &symbol-table entry, if one exists
L* jtprobeisquiet(J jt,A a,A locsyms){A g;  // locsyms is used in the call, but is not needed in the function
 I n=AN(a); NM* v=NAV(a); I m=v->m;  // n is length of name, v points to string value of name, m is length of non-locale part of name
 if(n==m){g=jt->global;}   // if not locative, define in default locale
 else{C* s=1+m+v->s; if(!(g=NMILOC&v->flag?locindirect(n-m-2,1+s,(UI4)v->bucketx):stfindcre(n-m-2,s,v->bucketx))){RESETERR; R 0;}}  // if locative, find the locale for the assignment; error is not fatal
 R probeis(a, g);  // return pointer to slot, creating one if not found
}


// assign symbol: assign name a in symbol table g to the value w (but g is special if jt->assignsym is nonnull)
// Result points to the symbol-table block for the assignment
L* jtsymbis(J jt,A a,A w,A g){F2PREFIP;A x;I wn,wr;L*e;
 ARGCHK2(a,w);
 // If we have an assignsym, we have looked this name up already, so just use the symbol-table entry found then
 // in this case g is the type field of the name being assigned; and jt->locsyms must exist, since it comes from
 // an explicit definition
 I anmf=NAV(a)->flag; RZ(g)
 if(jt->assignsym) {
// obsolete   ASSERT(((I)g&ASGNLOCAL||NAV(a)->flag&(NMLOC|NMILOC)||!probelocal(a,jt->locsyms)),EVDOMAIN)  //  if global assignment not to locative, verify non locally defined
  if(unlikely((((I)g&ASGNLOCAL)|(anmf&(NMLOC|NMILOC)))==0))ASSERT(!probelocal(a,jt->locsyms),EVDOMAIN)  // if non-locative, give error if there is a local
// obsolete   ASSERT(((I)g&ASGNLOCAL||NAV(a)->flag&(NMLOC|NMILOC)||!probelocal(a,jt->locsyms)),EVDOMAIN)  //  if global assignment not to locative, verify non locally defined
  e = jt->assignsym;   // point to the symbol-table entry being assigned
  CLEARZOMBIE   // clear until next use.
 } else {A jtlocal=jt->locsyms;
// obsolete   if(likely(n==m))ASSERT(!(g==jt->global&&probelocal(a,jtlocal)),EVDOMAIN)  // if non-locative, give error if there is a local
  if(likely(!(anmf&(NMLOC|NMILOC)))){if(unlikely(g==jt->global))ASSERT(!probelocal(a,jtlocal),EVDOMAIN)  // if non-locative, give error if there is a local
    // symbol table, and we are assigning to the global symbol table, and the name is defined in the local table
  }else{I n=AN(a); I m=NAV(a)->m;    // locative: n is length of name, v points to string value of name, m is length of non-locale part of name
// obsolete    C*s=1+m+NAV(a)->s; RZ(g=anmf&NMILOC?locindirect(n-m-2,1+s,(UI4)NAV(a)->bucketx):stfindcre(n-m-2,s,NAV(a)->bucketx));
   C*s=1+m+NAV(a)->s; if(unlikely(anmf&NMILOC))g=locindirect(n-m-2,1+s,(UI4)NAV(a)->bucketx);else g=stfindcre(n-m-2,s,NAV(a)->bucketx); RZ(g);
  }
    // locative: s is the length of name_.  Find the symbol table to use, creating one if none found
  // Now g has the symbol table to store into
  RZ(e=g==jtlocal?probeislocal(a) : probeis(a,g));   // set e to symbol-table slot to use
  if(unlikely(AT(w)&FUNC))if(likely(FAV(w)->fgh[0]!=0)){if(FAV(w)->id==CCOLON)FAV(w)->flag|=VNAMED; if(jt->glock)FAV(w)->flag|=VLOCK;}
   // If the new value is a function created by n : m, this becomes a named function; if running a locked function, this is locked too.
   // kludge  these flags are modified in the input area (w), which means they will be improperly set in the result of the
   // assignment (ex: (nm =: 3 : '...') y).  There seems to be no ill effect, because VNAMED isn't used much.
 }
 if(unlikely(jt->uflags.us.cx.cx_c.db))RZ(redef(w,e));  // if debug, check for changes to stack
 if(unlikely(e->flag&(LCACHED|LREADONLY))){  // exception cases
  ASSERT(!(e->flag&LREADONLY),EVRO)  // if writing read-only value (xxx_index), fail
  // LCACHED: We are reassigning a value that is cached somewhere.  We must protect the old value.  We will create a new symbol after e, transfer ownership of
  // the name to the new symbol, and then delete e, which will actually just make it a value-only unmoored symbol
  L *newe; RZ(newe=symnew(0,(e-LAV0(JT(jt,symp)))|SYMNONPERM)) probedel(NAV(e->name)->m,NAV(e->name)->s,NAV(e->name)->hash,g); newe->name=e->name; e->name=0; e=newe;
 }
 x=e->val;   // if x is 0, this name has not been assigned yet; if nonzero, x points to the incumbent value
 I xaf;  // holder for nvr/free flags
 I xt;  // If not assigned, use empty type
 if(x){
   xaf=AFLAG(x); xt=AT(x); // if assigned, get the actual flags, from the name and the old value
   if(unlikely((e->flag&LWASABANDONED)!=0)){
     // Reassigning an x/y that was abandoned into this execution.  We did not increment the value when we started, so we'd better not decrement now.
     // However, we did change 8..1 to 1, and if the 1 is still there, we set it back to 8..1 so that the caller can see that the value is unincorporated.
     // The case where x==w is of interest (it comes up in x =. x , 5).  In that case we will not change the usecount of x/w below, so we have to keep the ABANDONED
     // status in sync with the usecount.  The best thing is to keep both unchanged, so that we can continue to inplace x
     ACOR(x,ACINPLACE&(AC(x)-1-(x!=w)))  // apply ABANDONED: 1 -> 8..1 but only if we are going to replace x; we don't want 8..1 in an active name
     e->flag&=~((x!=w)<<LWASABANDONEDX);  // turn off abandoned flag after it has been applied, but only if we replace x
     xaf = AFNVRUNFREED; // ignore other flags; set xaf as if x==0 to avoid any other usecount changes
   }
 } else {xaf = AFNVRUNFREED; xt=0;}   // If name is not assigned, indicate that it is not read-only or memory-mapped.  Also set 'impossible' code of unfreed+not NVR
 if(likely(!(AFNJA&xaf))){
  I wt=AT(w);
  // Normal case of non-memory-mapped assignment.
  // If we are assigning the same data block that's already there, don't bother with changing use counts or anything else (assignment-in-place)
  if(likely(x!=w)){
   if(unlikely(((xt|wt)&(VERB|CONJ|ADV))!=0)){
    // If we are assigning a adverb value, check to see if it is nameless, and mark the value if it is
    if(unlikely((wt&ADV)!=0))AT(w)=wt|(I)nameless(w)<<NAMELESSMODX;
   }
   // Increment the use count of the value being assigned, to reflect the fact that the assigned name will refer to it.
   // This realizes any virtual value, and makes the usecount recursive if the type is recursible
   // If the value is abandoned inplaceable, we can just zap it, set its usecount to 1, and make it recursive if not already
   // We do this only for final assignment, because we are creating a name that would then need to be put onto the NVR stack for protection if the sentence continued
   rifv(w); // must realize any virtual
   if(likely((SGNIF((I)jtinplace,JTFINALASGNX)&AC(w)&(-(wt&NOUN)))<0)){  // if final assignment to abandoned noun
    *AZAPLOC(w)=0; ACRESET(w,ACUC1) if(unlikely(((wt^AFLAG(w))&RECURSIBLE)!=0)){AFLAGORLOCAL(w,wt&RECURSIBLE) jtra(w,wt);}  // zap it, make it non-abandoned, make it recursive (incr children if was nonrecursive).  This is like raczap(1)
      // NOTE: NJA can't zap either, but it never has AC<0
   }else ra(w);  // if zap not allowed, just ra() the whole thing
   // If this is a reassignment, we need to decrement the use count in the old name, since that value is no longer used.
   // But if the value of the name is 'out there' in the sentence (coming from an earlier reference), we'd better not delete
   // that value until its last use.
   // For simplicity, we defer ALL deletions till the end of the sentence.  We put the to-be-deleted value onto the NVR stack if it isn't there already,
   // and free it.  If the value is already on the NVR stack and has been deferred-freed, we decrement the usecount here to mark the current free, knowing that the whole block
   // won't be freed till later.  By deferring all deletions we don't have to worry about whether local values are on the stack; and that allows us to avoid putting local values
   // on the NVR stack at all.
   // ABANDONED values can never be NVR (which are never inplaceable), so they will be flagged as !NVR,!UNFREED,ABANDONED
   if(likely((xaf&AFNVRUNFREED)!=0)){  // x is 0, or unfreed on the NVR stack, or abandoned.  Do not fa().  0 is probably the normal case (assignment to unassigned name)
    if(unlikely((xaf&AFNVR)!=0)){AFLAGSET(x,xaf&=~AFNVRUNFREED)} // If unfreed on the NVR stack, mark as to-be-freed on the stack.  This defers the deletion
    // x=0 case, and LABANDONED case, go through quietly making no change to the usecount of x
   }else{  // x is non0 and either already marked as freed on the NVR stack or must be put there now, or VIRTUAL
    if(likely(!(xaf&(AFNVR|AFVIRTUAL)))){
     // (1) the value in x is not on the NVR stack.  But it may still be at large in the sentence, because we don't push local names
     // onto the NVR stack.  So, we defer the deletion until the end of the sentence, by adding the name to the NVR stack.  If we are already at the end of the sentence
     // we can avoid this, and just fa(), since the local name cannot be in use higher uup
     // (2) the value is not VIRTUAL.  The only way for an assigned value to be VIRTUAL is for it to be an initial assignment to x/y.  And to get here
     // the value must not have been adandoned.  So the usecount was raised on assignment (otherwise we would have gone through the no-fa special case).  So it is safe to fa() immediately then
     if(likely((I)jtinplace&JTFINALASGN)){
      fa(x);  // sentence is ending, no need to defer free
     }else{
      // must push onto NVR stack for the first time
      A nvra=jt->nvra;
      if(unlikely((I)(jt->parserstackframe.nvrtop+1U) > AN(nvra)))RZ(nvra=extnvr());  // Extend nvr stack if necessary.  copied from parser
      AAV1(nvra)[jt->parserstackframe.nvrtop++] = x;   // record the place where the value was protected (i. e. this sentence); it will be freed when this sentence finishes
      AFLAGOR(x,AFNVR)  // mark the value as protected in NVR stack
      AM(x)=1;  // When NVR is set, AM must contain count of # times value has been pushed onto the stack
     }
    }else{
     // already NVR+FREED or VIRTUAL: 'free' this time, knowing the real free will happen later,  We know usecount>1, but it may be PERMANENT; decrement it if not
     fadecr(x)
    }
   }
   e->val=w;   // install the new value
  } 
 } else {  // x exists, and is either read-only or memory-mapped
  ASSERT(!(AFRO&xaf),EVRO);   // error if read-only value
  if(x!=w){  // replacing name with different mapped data.  If data is the same, just leave it alone
   realizeifvirtual(w);  // realize if virtual.  The copy stored in the mapped array must be real
   I wt=AT(w); wn=AN(w); wr=AR(w); I m=wn<<bplg(wt);
   ASSERT(wt&DIRECT,EVDOMAIN);  // boxed, extended, etc can't be assigned to memory-mapped array
   ASSERT(allosize(x)>=m,EVALLOC);  // ensure the file area can hold the data
   AT(x)=wt; AN(x)=wn; AR(x)=(RANKT)wr; MCISH(AS(x),AS(w),wr); MC(AV(x),AV(w),m);  // copy in the data
  }
 }
 e->sn=jt->currslistx;  // Save the script in which this name was defined
 if(unlikely(JT(jt,stch)!=0))e->flag|=LCH;  // update 'changed' flag if enabled - no harm in marking locals too
 R e;   // return the block for the assignment
}    /* a: name; w: value; g: symbol table */

// assign symbol and free values immediately
// assignment within a sentence requires that values linger on a bit: till the end of the sentence or sometimes till printing is complete
// Values awaiting deletion are accumulated within the NVR stack till the sentence ends.  If there is an assignment not in a sentence, such as for for_x. or from sockets or DLLs,
// we have to finish the deletion immediately so that the NVR stack doesn't overflow
L* jtsymbisdel(J jt,A a,A w,A g){
 // All we have to do is mark the assignment as final.  That prevents any additions to the NVR stack.
 R jtsymbis((J)((I)jt|JTFINALASGN),a,w,g);
}
