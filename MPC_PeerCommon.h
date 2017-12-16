#ifndef MPC_PEERCOMMON_H
#define MPC_PEERCOMMON_H

#include <string.h>     // strerror
#include <arpa/inet.h>  // inet_ntoa
#include <netdb.h>      // addrinfo
#include <unistd.h>     // recv, close

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <algorithm>
#include <map>

#include "MPC_Common.h"

using namespace std;

//#define DEBUG // Comment out to disable DebugMsg output

// Declarations MPC_Common.cc
void   ConsoleMsg( string msg );
void   DebugMsg  ( string msg );
int    GetSocket ( int port, bool bind_socket = false );
int    GetSocketByAddrInfo( int port, bool bind_socket = false );
string GetServerHost( int port );

//------------------------------------------------------------
// Each Peer object has a pointer to a routerFunc() that
// populates it's PeerRoute structure telling the peer which
// remote peer to send a message to via SendToPeer(). 
//------------------------------------------------------------
struct PeerRoute {
    string peerID;
    string host;
    int    port;

    PeerRoute( string peer = "", string host = "", int port = 0 ) :
               peerID( peer ), host( host ), port( port ) {}
};

//------------------------------------------------------------
// Container for remote Peer information
// Saved in the Peer Peers map. 
//------------------------------------------------------------
struct PeerInfo {
    string host;
    int    port;
    int64  x;        // base exponent of the share
    string shareID;

    PeerInfo( string host = "", int port = 0,
              string shareid = "", int64 x = 0 ) :
              host( host ), port( port ), x( x ), shareID( shareid ) {}
};

//------------------------------------------------------------
// Container for remote Share information
// Saved in the Peer CollectedShares map. 
//------------------------------------------------------------
struct ShareInfo {
    string shareID;
    int64  prime;
    int64  x;      // base of polynomial exponents
    int64  f_x;    // evaluated value of polynomial using x

    // Map with keys that are the polynomial coefficient base
    // exponent (x), and values the evaluated polynomial using
    // the base exponent (f_x).  [ x ] : f_x
    map< int64, int64 > evaluatedShare;

    ShareInfo( string shareid = "", int64 prime = 0,
               int64 x = 0, int64 f_x = 0 ) :
        shareID( shareid ), prime( prime ), x( x ), f_x( f_x ) {}
};

#endif
