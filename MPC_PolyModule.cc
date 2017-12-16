#include "MPC_PolyModule.h"

//------------------------------------------------------------
// C++ modulo operator % returns the remainder with the same
// sign as the dividend (a): (a/b)*b + a%b == a; for b!=0
// Define a modulus function that always returns a positive value
int64 MPC_PolyModule::Modulus( int64 a, int64 b )
{
    return ( a % b + b ) % b;
}

//------------------------------------------------------------
// Evaluate polynomial in mod P
// a is the polynomial coefficients
// x is the value of the variable (exponent base)
// P is the prime number
//------------------------------------------------------------
int64 MPC_PolyModule::Polynomial ( vector <int64> a, int64 x, int64 P )
{
    int64 f_x = 0;

    // f_x = ( Σ a[i] * x^i ) % P;
    for( vector<int64>::size_type i = 0; i < a.size(); i++ ) {
        f_x = f_x + a[i] * (int64)pow(x,i);
    }
    f_x = Modulus( f_x, P );

    return( f_x );
}

//------------------------------------------------------------
// Add polynomial a and polynomial b
// Returns vector c that is the sum polynomial of a and b
//------------------------------------------------------------
vector <int64> MPC_PolyModule::AddPoly( vector <int64> a, vector <int64> b )
{
    if ( a.size() != b.size() ) {
        throw( runtime_error( "AddPoly() "
                              "polynomials must be of equal degree" ));
    }
	
    vector <int64> c( a.size() );
	
    for( vector<int64>::size_type i = 0; i < b.size(); i++ ) {
        c[i] = (a[i]+b[i]);
    }
	
    return c;
}
    
//------------------------------------------------------------
// Multiply polynomial a and polynomial b
// Returns vector c that is the product of a and b
//------------------------------------------------------------
vector <int64> MPC_PolyModule::MultPoly( vector <int64> a, vector <int64> b )
{
    if ( a.size() != b.size() ) {
        throw( runtime_error( "MultPoly() "
                              "polynomials must be of equal degree"));
    }

    vector <int64> c((a.size() + b.size())-1);

    // Take every term of first polynomial
    for( vector<int64>::size_type i = 0; i < a.size(); i++ ) {

        // Multiply the current term of the first polynomial
        // with every term of second polynomial
        for (vector<int64>::size_type j = 0; j < b.size(); j++ ){
            c[i+j] += a[i]*b[j];
        }
    }
    return c;
}

//------------------------------------------------------------
// Takes a vector and prints to the standard output the
// polynomial representation
//------------------------------------------------------------
void MPC_PolyModule::PrintPoly(vector <int64> a)
{
    for( vector<int64>::size_type i = 0; i < a.size(); i++ ) {
	
        cout << a[i] << "x^" << i;
	
        if( i != a.size() - 1 ){
            cout << " + ";
        }
    }
    cout << endl;
}

//------------------------------------------------------------
// Decomposition of the GCD of a and b.
// Returns [x,y,z] such that x = GCD(a,b) and y*a + z*b = x
//------------------------------------------------------------
vector<int64> MPC_PolyModule::GCD( int64 a, int64 b )
{
    int64 G_[] = { 0, 0, 0 }; // local array for [x,y,z]
	
    if ( b == 0 ) {
        G_[0] = a;
        G_[1] = 1;
    }
    else {
        int64 n = floor( a / b );
        int64 c = Modulus( a, b );
        vector<int64> r = GCD( b, c );
	    
        G_[0] = r[0];
        G_[1] = r[2];
        G_[2] = r[1] - r[2] * n;
    }
    // Copy int[] array G_ into vector<int> G
    vector<int64> G( G_, G_ + sizeof(G_) / sizeof(int64) );
    return G;
}

//------------------------------------------------------------
// Returns the multiplicative inverse of k mod prime.  
// (k * modInverse(k)) % prime = 1 for all prime > k >= 1
//------------------------------------------------------------
int64 MPC_PolyModule::ModInverse( int64 k, int64 prime )
{
    int64 r = 0;

    k = Modulus( k, prime );

    if ( k < 0 ) {
        r = -1 * GCD( prime, -k )[2];
    }
    else {
        r = GCD( prime, k )[2];
    }

    return( Modulus( ( prime + r ), prime ) );
}
