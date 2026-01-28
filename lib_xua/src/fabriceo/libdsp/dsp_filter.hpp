#ifndef _DSP_FILTER_H_
#define _DSP_FILTER_H_

#include <stdint.h>
//#include <math.h>
#include "dsp_filter_calc.hpp"
#include "dsp_list.hpp"

//prototypes
struct dspFilter;
struct dspFilterState;

//create a list of objects, to chain all the elementary filter declared
dspChain< dspFilter > filtersChain;

//create a list of objects, to chain all the elementary filterStates declared
dspChain< dspFilterState > fStatesChain;

//used to declare a filter with its specific type and parameters (F,Q,G)
struct dspFilter {

    //9 words size per filter
    dspFilter * prevItem;       //all fiters are chained upon construction
    struct {
        char ftype;             //filter type
        char ordermax;          //maximum order accepted for this filter (impact the futur size of coefficients)
        char changed;           //set to 1 when user is changing a value F Q G, requiring to compute coefficients
                                //set to 2 when bypass flag is modified, requiring to set coefs b0 to 1.0
                                //set to 4 when ftype has changed, requiring coef recalc and reset of states
                                //set to 16 when filter is bypassed
        char spare;
    } s;                        //status record in 1 word size
    float F, Q, G;              //filter characteristics
    float Fp, Qp;               //TODO create another filter object ? to extend the default one

    // list of constructor to accomodate all possible types of filter with specific parameters
    //each filter object is added to the filtersChain list

    dspFilter() : s{FNONE,2,18,0} {  //by defualt this is a 2nd order "bypassed" filter
        filtersChain.append(*this);
    }

    //standard copy constructor to copy the given filter
    dspFilter(dspFilter& f) { 
        s = f.s; F = f.F; Q = f.Q; G = f.G; Fp = f.Fp; Qp = f.Qp;
        //this copy is independent and belongs to the list of filter
        filtersChain.append(*this);
    }

    dspFilter( dspFilterOrder_t omax) : 
        s{(char)FNONE,(char)omax,1,0}, 
        F(0),Q(0),G(0),Fp(0),Qp(0) { 
            filtersChain.append(*this);
        }

    dspFilter(dspFilters_F_t t1, float f, float g) : 
        s{(char)t1,0,1,0},
        F(f),Q(0),G(g),Fp(0),Qp(0) { 
            s.ordermax=dspFilterGetOrder(t1); 
            filtersChain.append(*this);
        }

    dspFilter(dspFilters_F_t t1, float f) : dspFilter(t1,f,1.0) {  }

    dspFilter(dspFilters_FQ_t t2, float f, float q, float g) : 
        s{(char)t2,0,1,0},
        F(f),Q(q),G(g),Fp(0),Qp(0) { 
            s.ordermax = dspFilterGetOrder(t2); 
            filtersChain.append(*this);
        }
        
    dspFilter(dspFilters_FQ_t t2, float f, float q) : dspFilter(t2,f,q,1.0) {  }

    dspFilter(dspFilters_FQG_t t3, float f, float q, float g) : 
        s{(char)t3,0,1,0 },
        F(f),Q(q),G(g),Fp(0),Qp(0) { 
            s.ordermax=dspFilterGetOrder(t3); 
            filtersChain.append(*this);
        }

    dspFilter(dspFilters_FQ_FpQp_t t4, float f, float q, float f2, float q2) : 
        s{(char)t4,0,1,0},
        F(f),Q(q),Fp(f2),Qp(q2) { 
            s.ordermax=dspFilterGetOrder(t4); 
            filtersChain.append(*this);
        }

    ~dspFilter() {
        filtersChain.remove(*this);
    }

    void changeF(float f) {
        if (s.ftype && (F != f)) { 
            F=f; s.changed |=1; }
    }
    void changeQ(float q) {
        if (s.ftype && (Q != q)) { 
            Q=q; s.changed |=1; }
    }
    void changeG(float g) {
        if (s.ftype && (G != g)) { 
            G=g; s.changed |=1; }
    }
    void changeFQG(float f, float q, float g) {
        if (s.ftype && ((F != f)||(Q != q)||(G != g))) { 
            F=f; Q=q; G=g; s.changed |=1; }
    }

    void changeBypass(char mode) {
        char bp = ((s.changed & 16) != 0);
        if (s.ftype && (bp  != mode)) { 
            if (mode) s.changed |= (16+2);
            else {
                s.changed |= 2;
                s.changed &= ~16;
            }
        }
    }

    void changeType(char t) {
        if (t != s.ftype) {
            int32_t newOrder = dspFilterGetOrder(t);
            if (newOrder <= s.ordermax) {
                s.ftype = t;
                s.changed |= 5;
            }
        }
    }

    //specific for L.T.
    void changeF2Q2(float fp, float qp) {
        if ((s.ftype == FLT)  && ((Fp != fp)||(Qp != qp))) { 
            Fp = fp; Qp = qp; s.changed |= 1; }
    }

    //calc all coefficients according to filter type and filter parameters
    //if their value is flagged as "changed". force flag can be used to overload it
    int32_t calcCoefs(float fs, float * pcoefs, char force = 0)  {
        int32_t res = 0;
        char chg = s.changed | force;

        if (chg & 7) {  //value (or type) changed or bypass changed

            if ((s.changed & 16) || (s.ftype==FNONE))  
                res = dspFilterCaclCoefs(FNONE, fs, F, Q, G, pcoefs);
            
            else if (s.ftype >= FLP1) 
                res = dspFilterCaclCoefs(s.ftype, fs, F, Q, G, pcoefs);

            //linkwitz transform
            else if (s.ftype == FLT) 
                res =  dspFilterCaclCoefsLT(s.ftype, fs, F, Q, G, Fp, Qp, pcoefs);

            //TODO hilbert transform
            else if (s.ftype == FHILB) res = 0;


            //other combined filters
            else if (s.ftype < FLP1) 
                res = dspFilterCaclCoefsMultiple(s.ftype, fs, F, G, pcoefs);

            s.changed &= ~3;            //clear changed
        }
        return res;
    }

};


//helper macro combining a filterbank reference and its size as a second parameter
#define filtersArray(fb) fb,sizeof(fb)/sizeof(fb[0])


struct dspFilterState { 
/*
each filterState has a dynamic record used by the biquad routine.
containg pointers on states, active coefficient, target coefficients (when slew enable)
structure of one object filterState in DSP_MALLOC:
    state area x N sections
    coef area : (5+1) filter coefs x N sections ( x 2 if slew)
*/
    dspFilterState *    prevItem;       //chaining all filterState for dynamic update                                  
    dspFilter *         pfarray;        // point on the original array used in this state
    uint32_t            Nfilters;       // number of filters in this array
    uint32_t            mode;           // 0 = floatingpoint (5coefs)
                                        // 1 = int32_t       (6coefs)
                                        // 2 = vpu           (8coefs)
                                        // or 4 = duplicated for slew management
    uint32_t            format;         // value 32 for fp32 or mantissa for the local coefs/states 
    float *             pstate;         // point on the whole state area, all in a raw
    float *             pcoefs;         // point on all the active coefficients, all in a raw or null if no locals
    float *             pcoefsTarget;   // point on all the target coefficients. nullptr if no slew required
    uint32_t            Nsections;      // computed during init based on each filter.s.ordermax
//    uint32_t            Nstates;        // number of states words for a single biquad (4, 6 or 8).
    uint32_t            slewIdx;        // go trough each filters of the array to detect slew

    //constructor for a single filter (corner case)
    dspFilterState( dspFilter& filter, uint32_t mode_ = 1) {
        Nfilters = 1;
        pfarray  = &filter;
        mode = mode_;
        pcoefs = nullptr;
        fStatesChain.append(*this);
        init( );
    }

    //constructor expecting a filter array and number of bins
    dspFilterState(dspFilter farray[], uint32_t fnum, uint32_t mode_= 1) {
        Nfilters = fnum;
        pfarray  = farray;
        mode = mode_;
        pcoefs = nullptr;
        fStatesChain.append(*this);
        init( );
    }
    //constructor expecting reference to a parent dspFilterState, containing master coefficients
    dspFilterState( dspFilterState& master) {
        Nfilters = master.Nfilters;
        pfarray  = master.pfarray;
        pcoefs   = master.pcoefs;
        mode     = master.mode;
        fStatesChain.append(*this); // ????? sure ????
        // prevItem = nullptr; //alternative
        init( );
    }
    //destructor
    ~dspFilterState() {
        fStatesChain.remove(*this);
    }

    //initialize the dynamic record based on the given filter table. done only once.
    void init() {
        //compute memory area needed
        uint32_t memneed = 0;
        //point on the filter table given as parameter
        dspFilter * ptr = pfarray; 
        uint32_t n = Nfilters;
        //number of second order section in total for the whole array
        Nsections = 0;
        //compute size of states area and coefs area depending on mode
        uint32_t sizeCoefs = 0;
        uint32_t sizeStates = 0;
        //go through all filter array
        while (n && ptr && (ptr->s.ftype != FLAST)) {
            //compute individual filter sections
            uint32_t sections = (ptr->s.ordermax+1)>>1;
            Nsections += sections;
            if ((mode & 3) == 1) sizeStates +=  4 * sections + 2; //cpu
            else sizeStates +=  3 * (sections + 1); //vpu or float
            //test if pcoefs is already initialized due to a link with a master filterState
            if (pcoefs == nullptr) sizeCoefs += ((mode & 3) == 2) ? 8 : 6;
            ptr++; n--;
        }
        memneed += (sizeStates + sizeCoefs * ((mode & 4)?2:1)) * sizeof(float) ;
        Nfilters -= n; //just in case the total number of filters is lower than expected
        uint32_t * alloc = DSP_MALLOC.get(memneed);
        //now populate the record with information we have
        if (alloc) {
            memset(alloc, 0, memneed );
            pstate = (float*)alloc;
            if (pcoefs == nullptr) {
                alloc += sizeStates;
                pcoefs = (float*)alloc;
                if (mode & 4) pcoefsTarget = &pcoefs[sizeCoefs];
                else pcoefsTarget = pcoefs;
            } else pcoefsTarget = nullptr;
        } else {
            pstate = nullptr;
        }
        slewIdx = 0;
    }

    void calcAllCoefs(float fs, int32_t force = 0) {
        //return if we depend on a master filterState
        if (pcoefsTarget == nullptr) return;
        //point on original table of filters
        dspFilter * pf = pfarray;
        float * pfc = pcoefsTarget;
        //for each filter in the table 
        for (int i=0; i<Nfilters; i++) {
            pf->calcCoefs(fs, pfc, force);
            if (format != fp32) {
                int32_t * pfci = (int32_t*)pfc;
                for (int i=0;i<6;i++) {
                    if (i != 3) pfci[i] = q32_float(pfc[i],format);
                }
            }
            if (mode == 2) {
                pfc[6] = pfc[7] = 0.0f;
                pfc += 8;
            } else pfc += 6;
            pf++;
        }
    }

    void clearFilterChanges() {
        dspFilter * pf = pfarray;
        for (int i=0; i<Nfilters; i++) {
            pf->s.changed &= ~15;   //keep bypass status
            pf++;
        }
    }

    void letsSlew() {
        if (format > 1) {
            dspFilter *pf = pfarray;
            dspFilterCoefs * pactive = (dspFilterCoefs*)pcoefs;
            dspFilterCoefs * ptarget = (dspFilterCoefs*)&pcoefs[Nsections];
            for (int i=0; i<Nfilters; i++) {
                uint32_t sections = (pf->s.ordermax+1)>>1;
                if (pf->s.changed & 8) {
                    //calc new active
                }
                pactive = &pactive[sections];
                ptarget = &ptarget[sections];
                pf++;
            }
        }
    }
    
};

//used to update the biquad coefficient of each filterState recoder declared
//after some filter have bee changed by user interface.
void dspFiterStateChainUpdateAll() {
    dspFilterState * pfs = (dspFilterState *)fStatesChain.last;
    while (pfs) {
        //pfs->updateCoefs();
        pfs = pfs->prevItem;
    }
    pfs = (dspFilterState *)fStatesChain.last;
    while (pfs) {
        pfs->clearFilterChanges();
        pfs = pfs->prevItem;
    }
}

void DSP_FILTER_TEST() {

dspFilter ftest(FHPLR4, 1000, 2.0), forder4(F_ORDER_4), flt(FLT,80,0.7,120,0.5);

dspFilter f1(FHP2, 200, 0.7), f2(FPEAK, 2000, 0.7, 1.0) ;

//dspFilterLT fLT { FLT, 100.0f,1.0f, 120.0f, 1.2f };

dspFilter fbank1[] = {
        f1, forder4,
        { FLPLR4, 1000, 1.0 },
        f2,
        { FHPLR4, 4000 },
        { FLAST }
};

//declare a filter state variable to be used by biquadcpu object
dspFilterState fbank1_test( fbank1, sizeof(fbank1)/sizeof(fbank1[0])-1 );
dspFilterState test2(fbank1_test);
dspFilterState test3(f1);

fbank1[2].changeF(1000);
//fbank1[2].calcCoefs(96000);
dspFiterStateChainUpdateAll();

}

static inline int32_t dspExtract(int64_t accu, const int32_t mant){
    int32_t hi = (accu >> 32); unsigned lo = accu & 0xFFFFFFFF;
    int32_t res;
    asm("lextract %0,%1,%2,%3,32":"=r"(res):"r"(hi),"r"(lo),"r"(mant));
    return res;
}

static inline int32_t dspSatExtract(int64_t accu, const int32_t mant_){
    int32_t hi = (accu >> 32); unsigned lo = accu & 0xFFFFFFFF;
    register int32_t mant asm("r11") = mant_;   //r11 will contain the mantissa
    asm("lsat %0,%1,%2 ; lextract %0,%0,%1,%2,32":"=r"(hi),"=r"(lo):"r"(mant),"0"(hi),"1"(lo));
    return hi;
}

extern int64_t dspBiquad_q64(int64_t sample, dspFilterCoefs * pfilter, int32_t * pstate); //mantissa in r11
extern int32_t dspBiquad_q32(int32_t sample, int32_t mant, dspFilterCoefs * pfilter, int32_t * pstate);
extern int32_t dspBiquad_vpu(int32_t sample, dspFilterCoefs * pfilter, int32_t * pstate);
extern float   dspBiquad_float(float sample, dspFilterCoefs * pfilter, float   * pstate);

//this class is to be used only with dspbase which inherit it
template< typename CRTP, int32_t L > struct dspfilter {
    dspfilter(){}
    friend CRTP;

    void filter(dspFilterState& fstate) {
        CRTP& derived = static_cast<CRTP&>(*this);



        derived.reg = 0; //TODO
    };

};



#endif // _DSP_FILTER_H_
