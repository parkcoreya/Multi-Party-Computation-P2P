#ifndef MPC_COMMON_H
#define MPC_COMMON_H

#include <vector>
#include <sstream>

using namespace std;

// Common definitions for MPC modules

#define MAX_CLIENT_SOCKETS 32    // Not used in P2P
#define BUFFER_LENGTH      4096  // Not used in P2P

// 64 bit integers
typedef long long int64;
typedef unsigned long long uint64;

// Array of int64 to hold share values [ID, x, f_x] for binary socket comms
// in the ClientServer application. Not used in P2P
typedef struct sharebin { int64 share[3]; } sharebin; // sizeof() = 24

// Tokenize function in MPC_PeerCommon.cc
vector<string> Tokenize( string message );

#endif
