
// g++ MPC_PeerTest.cc -o netPeer -lstdc++ -std=c++11 -lpthread -g -Wno-pmf-conversions

//------------------------------------------------------------------------
// Main program for a MPC P2P network agent (peer). 
// 
// The MPC_Peer class is the main object. It holds a Share object, a map
// of PeerInfo structures of other connected Peers, and a list of Shares 
// from other peers. 
//
// The Peer and Share parameters are initialized from a config file. 
//
// The classes and P2P implementation are based on a file transfer
// application here: http://cs.berry.edu/~nhamid/p2p/
//------------------------------------------------------------------------

#include "MPC_Peer.h"
#include "MPC_PeerHandler.h"
#include "MPC_ReadConfig.h"

//-------------------------------------------------------------------
int main( int argc, char *argv[] ) {

    //------------------------------------------------------
    // Parse command line with getopt()
    extern char *optarg; // defined by getopt
    char     parse_char;
    string   path       = "./config/";
    string   configFile = "Bob.config";
    
    while ( ( parse_char = getopt( argc, argv, "c:" ) ) != -1 ) {
	switch ( parse_char ) {
	case 'p':
	    path = optarg;
	    break;
	case 'c':
	    configFile = optarg;
	    break;
	default :
	    cerr << "Invalid command line argument" << endl;
            cerr << "Usage: " << argv[0]
                 << " -p path " 
                 << " -c config file"
                 << endl;
            return -1;
	}
    }

    // Structures to hold parsed inputs from the peer config file
    struct PeerParams  peerParams;
    struct ShareParams shareParams;
    
    // Parse the config file into the parameter structures
    int status = ReadConfig( &peerParams, &shareParams, configFile, path );
    if ( status != 0 ) {
        cerr << "ERROR: Failed to parse " << configFile << endl;
        return status;
    }

    PrintConfig( &peerParams, &shareParams );

    // Instantiate a MPC_Peer object based on the config file parameters
    MPC_Peer P = MPC_Peer( peerParams.serverHost, peerParams.serverPort,
                           peerParams.name,       peerParams.maxPeers,
                           peerParams.timeOut );

    // Create a secret share for the Peer based on the config file
    // Do this prior to calling BuildPeers if you want share info
    P.CreatePeerShare( shareParams.name,   shareParams.numCoef,
                       shareParams.coef,   shareParams.x,
                       shareParams.secret, shareParams.prime );
    
    // Break the peerID host:port into separate host and port values.
    // The ID of a peer is made of a "host:port" string, where host
    // is a host name or IP address, and port the socket interface
    // on the host.  A peerID refers to a remote peer that this peer
    // communicates with.  Here we extract the host and port information
    // from the initial remote peer that this this peer will try to
    // communicate with to build up a list of peers on the network.
    size_t i    = peerParams.peerID.find( ":" );
    string host = peerParams.peerID.substr( 0, i );
    int    port = stoi( peerParams.peerID.substr( i + 1, string::npos ) );

    // Try to connect to the remote peerID to add each other to Peers map
    P.BuildPeers( host, port, peerParams.hops );

    // Start the Peer listener main loop in a detached thread
    P.StartMainLoop();

    // Make sure this is the last function since it's the only thread
    // that is joined and has a recursive timer that will prevent
    // the app from exiting
    P.StartStabilizer( peerParams.stabilize );
    
    return 0;
}
