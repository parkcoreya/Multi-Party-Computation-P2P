#ifndef MPC_PEERHANDLER_H
#define MPC_PEERHANDLER_H

// These classes are ported from the file btfiler.py and article here:
// http://cs.berry.edu/~nhamid/p2p/
// http://cs.berry.edu/~nhamid/p2p/framework-python.html

#include "MPC_Peer.h"

using namespace std;

//------------------------------------------------------------
// MPC_Peer class inheritied from Peer
//------------------------------------------------------------
class MPC_Peer : public Peer {

public:
    MPC_Peer( string serverhost, int serverport, string name = "No Name",
              int maxpeers = 16, int timeout = 5 );

    ~MPC_Peer() {}

    void Router( string peerID );
    
    void InsertPeer( PeerConnection *pc, string data );

    void ListPeers( PeerConnection *pc, string data );

    void PeerData( PeerConnection *pc, string data );

    void Commands( PeerConnection *pc, string data );

    void ListShares( PeerConnection *pc, string data );

    void LagrangeInterp( PeerConnection *pc, string data );

    void LagrangeInterpAdd( PeerConnection *pc, string data );

    void Distribute( PeerConnection *pc, string data );

    void ReceiveShareValue( PeerConnection *pc, string data );

    void Remove( PeerConnection *pc, string data );

    void Ping( PeerConnection *pc, string data );

    void Exit( PeerConnection *pc, string data );

    void BuildPeers( string host, int port, int hops = 1 );

};

#endif
