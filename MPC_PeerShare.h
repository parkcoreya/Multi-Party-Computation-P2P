#ifndef MPC_NETWORKSHARE_H
#define MPC_NETWORKSHARE_H

#include "MPC_PeerCommon.h"
#include "MPC_PolyModule.h"

class Peer;
class MPC_Peer;

//------------------------------------------------------------
// Class PeerShare
//------------------------------------------------------------
class PeerShare {

    friend Peer;
    friend MPC_Peer;

private:
protected:
    string shareID;     // Share public ID
    int    numCoef;     // Number of polynomial coefficients
    vector<int64> coef; // Polynomial coefficients
    int64 x;            // base of polynomial exponents
    int64 f_x;          // evaluated value of polynomial using x
    int64 secret;
    int64 prime;

    // Poly module for polynomial operations
    MPC_PolyModule Poly;
    
public:
    // Constructor #1 : No coef specified, call CreateSecretPolynomial()
    PeerShare( string shareID = "No Name", int numcoef = 0,
               int64 x = 0, int64 secret = 0, int64 prime = 0 );

    // Constructor #2 : coef specified
    PeerShare( vector<int64> _coef,
               string shareID = "No Name", int numcoef = 0,
               int64 x = 0, int64 secret = 0, int64 prime = 0 );

    string const ShareID();

    int64 const F_x();

    void CreateSecretPolynomial();

    void PrintSecretPolynomial();
    
};

#endif
