#include "MPC_PeerShare.h"

// Constructor #1 : No coef specified, call CreateSecretPolynomial()
PeerShare::PeerShare( string shareID, int numcoef,
                      int64 x, int64 secret, int64 prime ) :
    shareID( shareID ), numCoef( numcoef), x( x ), f_x( 0 ),
    secret( secret ), prime( prime )
{
    Poly = MPC_PolyModule();  // Create local instance of PolyModule
        
    CreateSecretPolynomial();
        
    // Evaluate the Share polynomial at x and store in f_x
    f_x = Poly.Polynomial( coef, x, prime );

//#ifdef DEBUG
    PrintSecretPolynomial();
//#endif
}
    
// Constructor #2 : coef specified
PeerShare::PeerShare( vector<int64> _coef,
                      string shareID, int numcoef,
                      int64 x, int64 secret, int64 prime ) :
    coef( _coef ), shareID( shareID ), numCoef( numcoef), x( x ), f_x( 0 ),
    secret( secret ), prime( prime )
{
    Poly = MPC_PolyModule();  // Create local instance of PolyModule
        
    // Evaluate the Share polynomial at x and store in f_x
    f_x = Poly.Polynomial( coef, x, prime );

//#ifdef DEBUG
    PrintSecretPolynomial();
//#endif
}
    
// Method to return shareID
string const PeerShare::ShareID() { return shareID; }

// Method to return f_x, the value of the share
int64 const PeerShare::F_x() { return f_x; }


//------------------------------------------------------------    
// Create list of random polynomial coefficients:
// coef[0] = secret, coef[1]...coef[k-1] = random numbers less than Z
//------------------------------------------------------------
void PeerShare::CreateSecretPolynomial() {
    if ( numCoef < 2 ) {
        ConsoleMsg( "ERROR: PeerShare::CreateSecretPolynomial " + shareID +
                    " number coefficients less than 2" );
        return;
    }
        
    srand( time(NULL) + rand() ); // seed random number generator
	
    coef.resize( numCoef ); // call resize to allocate elements
    coef[0] = Poly.Modulus( secret, prime );// Secret value coef is modulo P

    for( vector<int64>::size_type i = 1; i < coef.size(); i++ ) {
        // Generate random coef, ensure they are less than prime
        coef[i] = ( floor( Poly.Modulus( rand(), prime ) ) );
    }
}
    
//------------------------------------------------------------
//------------------------------------------------------------
void PeerShare::PrintSecretPolynomial() {
    ostringstream ostrm;

    ostrm << "PeerShare::PrintSecretPolynomial " << shareID << " ";
        
    for( vector<int64>::size_type i = 0; i < coef.size(); i++ ) {
            
        ostrm << coef[i] << "x^" << i;
            
        if( i != coef.size() - 1 ) {
            ostrm << " + ";
        }
    }
    ostrm << endl;

    ConsoleMsg( ostrm.str() );
}
