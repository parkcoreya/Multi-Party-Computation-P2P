#ifndef MPC_PEER_H
#define MPC_PEER_H

// These classes are ported from the file btpeer.py and article here:
// http://cs.berry.edu/~nhamid/p2p/
// http://cs.berry.edu/~nhamid/p2p/framework-python.html

#include "MPC_PeerCommon.h"
#include "MPC_PeerConnection.h"
#include "MPC_PeerShare.h"

using namespace std;

// Function pointer typedef for handler functions set into
// Peer::Hanlders map
class Peer;
typedef void (Peer::*HandlerFunc)( PeerConnection *, string );

// Function pointer typedef for Router function that
// populates a PeerRoute struct for where to send a message
class MPC_Peer;
typedef void (Peer::*RouterFunc)( string peerID );

//------------------------------------------------------------
//
//------------------------------------------------------------
class Peer {

    friend PeerShare;
    
protected:
    string name;
    string serverHost; // IP address
    int    serverPort; // Listener thread socket port
    string ID;         // "serverHost:serverPort" assigned in constructor
    int    maxPeers;
    
    bool shutdown = false; // Control variable for MainLoop
    int  timeOut;          // timeout for MainLoop listening socket

    // Map of Peers that this peer is currently able to connect
    // [ ID ] : PeerInfo struct pointer
    map< string, PeerInfo * > Peers;

    // The Share of this Peer
    PeerShare *Share;

    // Map of ShareInfo structs that this Peer has collected from others
    map< string, ShareInfo * > CollectedShares;

    int64 recoveredSecret;
    
    // Map of function pointers to Handler functions defined in MPC_Peer.
    // [ COMMAND ] : handler function pointer
    // When a request is serviced by this Peer, a new thread is
    // created to run the handler function pointed to by HandlerFunc
    map< string, HandlerFunc > Handlers;

    // A Router is a function that populates the PeerRoute struct
    RouterFunc routerFunc;

    // PeerRoute structure that is populated by the routerFunc
    // { ID, host, port }
    PeerRoute peerRoute;

    // Poly module for polynomial operations
    MPC_PolyModule Poly;
    
public:
    Peer( string, int, string, int, int );
    ~Peer();

    void Shutdown();

    void CreatePeerShare( string, int, vector<int64>,
                          int64, int64, int64 );

    void CallHandler( const string &, PeerConnection *, string );

    void HandlePeer( int, string, int );

    void AddRouter( RouterFunc );

    bool AddPeer( string, string, int, string, int64 );

    void RemovePeer( string );

    bool MaxPeersReached();

    int MakeServerSocket( int, int );

    vector<string> SendToPeer( string, string, string, bool );

    vector<string> ConnectAndSend( string host, int port,
                                   string msgType, string msgData,
                                   string peerID = "",
                                   bool waitReply = true );

    void StartStabilizer( int );

    void RunStabilizer( int );

    void CheckLivePeers();

    void StartMainLoop();

    void MainLoop();

    void LagrangeInterpolate( vector<int64>, vector<int64>, int64 );
};

#endif
