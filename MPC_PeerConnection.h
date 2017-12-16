#ifndef MPC_PEERCONNECTION_H
#define MPC_PEERCONNECTION_H

// These classes are ported from the file btpeer.py and article here:
// http://cs.berry.edu/~nhamid/p2p/
// http://cs.berry.edu/~nhamid/p2p/framework-python.html

#include "MPC_PeerCommon.h"

//------------------------------------------------------------
//
//------------------------------------------------------------
class PeerConnection {
private:
    
    string ID;
    string host;
    int    port;
    int    sock;
    int    client_sock;
    int    socket_status;

public:
    // Constructor
    PeerConnection( string peerID, string host, int port, int client_sock ) :
    ID( peerID ), host( host ), port( port ), client_sock( client_sock )
    {
        socket_status = Connect();
    }

    int Connect();

    string MakeMessage( string msgType, string msgData );
    
    bool SendData( string msgType, string msgData );

    string ReceiveData();

    void Close();

};
#endif
