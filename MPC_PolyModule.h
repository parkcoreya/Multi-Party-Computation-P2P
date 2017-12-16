#ifndef MPC_POLYMODULE_H
#define MPC_POLYMODULE_H

#include <iostream>
#include <vector>
#include <cmath>

#include "MPC_Common.h"

using namespace std;

//#define DEBUG

//------------------------------------------------------------
// Class MPC_PolyModule
//------------------------------------------------------------
class MPC_PolyModule {
    
public:
    // Constructor
    MPC_PolyModule() {
    }

    int64 Modulus( int64 a, int64 b );

    int64 Polynomial( vector <int64> a, int64 x, int64 P );

    vector <int64> AddPoly( vector <int64> a, vector <int64> b );

    vector <int64> MultPoly( vector <int64> a, vector <int64> b );
    
    void PrintPoly( vector <int64> a );
    
    vector<int64> GCD( int64 a, int64 b );

    int64 ModInverse( int64 k, int64 prime );
    
};
#endif
