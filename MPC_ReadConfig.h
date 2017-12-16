#ifndef MPC_READCONFIG_H
#define MPC_READCONFIG_H

struct PeerParams;
struct ShareParams;

int ReadConfig( PeerParams *peerParams, ShareParams *shareParams,
                string configFile, string path );

void PrintConfig( PeerParams *peerParams, ShareParams *shareParams );

//--------------------------------------------------------------
// The Peer class ID will be created in the Peer constructor
// from the serverHost:serverPort.  peerID is the ID of the
// remote peer that this peer will attempt to connect to in
// BuildPeers().
//--------------------------------------------------------------
struct PeerParams {
    string name;         // public name
    string peerID;       // "serverHost:serverPort" of remote peer
    string serverHost;   // IP address of this peer
    int    serverPort;   // socket port of this peer
    int    maxPeers;
    int    timeOut;      // timeout for MainLoop listening socket
    int    stabilize;    // timeout for CheckLivePeers()
    int    hops;
};

//--------------------------------------------------------------
struct ShareParams {
    string name;        // Share public ID
    int    numCoef;     // Number of polynomial coefficients
    vector<int64> coef; // User-specified polynomial coefficients
    int64  x;           // base of polynomial exponents
    int64  secret;
    int64  prime;
};    

#endif
