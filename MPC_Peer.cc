#include "MPC_Peer.h"

// Global mutex for Peers map in Peer classs
mutex peerLock;

// Constructor
// Initializes a peer server listening on the serverHost and serverPort.
Peer::Peer( string serverhost, int serverport,
            string name = "No Name", int maxpeers = 16, int timeout = 65 ) :
    // variable initialization
    serverHost( serverhost ), serverPort( serverport ), name( name ),
    maxPeers( maxpeers ), timeOut( timeout )
{
    if ( serverHost.size() < 3 ) {
        // serverHost should be "localhost" or xxx.x.x.x 
        // Attempt to get IP address for this peer's serverHost
        // by pinging an outside server like google.com
        serverHost = GetServerHost( serverPort );
    }

    // The Peer ID consists of the "serverHost:serverPort" string.
    // For a remote Peer, this is often referred to as peerID
    ID = serverHost + ":" + to_string( serverPort );

    routerFunc = NULL;
            
    Poly = MPC_PolyModule(); // Create local instance of PolyModule
}
    
// Destructor
Peer::~Peer() {
    if ( Share ) { delete Share; }
        
    if ( CollectedShares.size() ) {
        map< string, ShareInfo * >::iterator csi;
        for ( csi = CollectedShares.begin();
              csi != CollectedShares.end(); ++csi ) {
            delete csi->second;
        }
    }
    CollectedShares.clear();
}
    
// Encapsulation Accessor methods
void Peer::Shutdown() { shutdown = true; }

//------------------------------------------------------------
// 
//------------------------------------------------------------
void Peer::CreatePeerShare( string shareID, int numcoef, vector<int64> coef,
                            int64 x, int64 secret, int64 prime ) {

    PeerShare *share;
        
    if ( coef.empty() ) {
        // PeerShare constructor will create a random polynomial with
        // numcoef, evaluate it at x, and store the result in f_x
        share = new PeerShare( shareID, numcoef, x, secret, prime );
    }
    else {
        // PeerShare constructor will use specified polynomial coef
        // evaluate it at x, and store the result in f_x
        share = new PeerShare( coef, shareID, numcoef, x, secret, prime );
    }
        
    Share = share; // Assign to the Peer Share object, destructor deletes

    // Create and assign a ShareInfo struct to the CollectedShares
    // Note that f_x is 0
    ShareInfo *shareInfo = new ShareInfo( shareID, prime, x, share->f_x );

    CollectedShares[ shareID ] = shareInfo;
}
    
//------------------------------------------------------------
// Call the function pointer in the Handlers map based on
// the command key (processed as msgType in most functions).
//------------------------------------------------------------
void Peer::CallHandler( const string &command, PeerConnection *pc, string data ) {
    // Get function pointer from Handlers map
    HandlerFunc fp = Handlers[ command ];

    // Call function with PeerConnection (pc) and incoming socket data
    return (this->*fp)( pc, data );
}

//------------------------------------------------------------
// This function is run as a thread from the MainLoop() to
// handle a request that came in from another peer on the
// listening socket.  It is a wrapper for CallHandler() that
// actually calls the function in the Handlers map. 
//------------------------------------------------------------
void Peer::HandlePeer( int client_sock, string host, int port ) {
        
    thread::id threadID = this_thread::get_id();
    ostringstream ostrm;
    ostrm << "Peer::HandlePeer() New thread ID [" << threadID << "]";
    DebugMsg( ostrm.str() );
    DebugMsg( "Peer::HandlePeer() " + name + " Connecting to " + host +
              " port " + to_string( port ) );

    // Create a PeerConnection object 
    string None("");
    PeerConnection PC = PeerConnection( None, host, port, client_sock );

    // Receive message on the client_sock connection
    string peerMessage = PC.ReceiveData();

    DebugMsg( "Peer::HandlePeer() " + name + " Received: " + peerMessage );

    // The message is of the format msgType:msgData
    // where msgType corresponds to a key in the Handlers map
    // Extract the msgType and msgData
    size_t i       = peerMessage.find( ":" );
    string msgType = peerMessage.substr( 0, i );
    string msgData = peerMessage.substr( i + 1, string::npos );
        
    // Call the appropriate handler (function) in the Handlers map
    if ( Handlers.count( msgType ) == 0 ) {
        // msgType is not a valid key in the Handlers map
        ConsoleMsg( "ERROR: Peer::HandlePeer() " + name + " Failed to find"
                    " msgType " + msgType + " in Handlers map." );
    }
    else {
        DebugMsg( "Peer::HandlePeer() " + name +
                  " msgType " + msgType + " Call Handler... " );
            
        CallHandler( msgType, &PC, msgData );
    }

    close( client_sock );
    DebugMsg( "Peer::HandlePeer() " + name + " Disconnected" );
}

//------------------------------------------------------------
// Registers a routing function with this peer. The setup of routing
// is as follows: This peer maintains a list of other known peers
// in Peers map. The routing function should take the name of
// a peer (which may not necessarily be present in Peers map)
// and decide which of the known peers a message should be routed
// to next in order to (hopefully) reach the desired peer. The router
// function should return a struct three values: (next-peer-id, host,
// port). If the message cannot be routed, the next-peer-id should be
// 0.
//------------------------------------------------------------
void Peer::AddRouter( RouterFunc router ) {
        
    routerFunc = router;
}

//------------------------------------------------------------
// Adds a peer name and host:port mapping to the known list of peers.
//------------------------------------------------------------
bool Peer::AddPeer( string peerID, string host, int port,
                    string shareID, int64 x ) {
        
    bool addedPeer = false;
        
    if ( ( Peers.count( peerID ) == 0 ) and // peerID not in Peers map
         ( Peers.size() < maxPeers ) ) {

        PeerInfo *peerInfo = new PeerInfo{ host, port, shareID, x };
        Peers[ peerID ] = peerInfo;
        addedPeer = true;

        ConsoleMsg( "Peer:AddPeer " + name + " Added peerID [" +
                    peerID + "]  host " + host +
                    " port " + to_string( port ) + " " + shareID );
    }
    return( addedPeer );
}

//------------------------------------------------------------
// Removes peer information from the known list of peers
//------------------------------------------------------------
void Peer::RemovePeer( string peerID ) {
    if ( Peers.count( peerID ) == 1 ) { // peerID in Peers map
        PeerInfo *peerInfo = Peers[ peerID ];
        Peers.erase( peerID );  // erase reference from map
        delete peerInfo;        // free the allocated struct
    }
}

    
//------------------------------------------------------------
// Returns whether the maximum limit of names has been added to the
// list of known peers.
//------------------------------------------------------------
bool Peer::MaxPeersReached() {
    if ( Peers.size() >= maxPeers ) { return true; }
    return false;
}
    
//------------------------------------------------------------
// Constructs and prepares a peer server socket listening
// on the given port.
//------------------------------------------------------------
int Peer::MakeServerSocket( int port, int backlog = 10 ) {

    // Get a socket connection for this Peer to listen on
    int sock = GetSocket( port, true );
        
    if ( sock == -1 ) {
        ConsoleMsg( "ERROR: Peer::MakeServerSocket() " + name +
                    " GetSocket Failed on port " +
                    to_string( port ) );
        return( -1 );
    }

    DebugMsg( "Peer::MakeServerSocket() " + name +
              " new listening socket " +
              to_string( sock ) + " on port " + to_string( port ) );
        
    int status = listen( sock, backlog );
        
    if ( status < 0 ) {
        cerr << "Peer::MakeServerSocket listen "
             << strerror( errno ) << endl;
        return( -1 );
    }
        
    return( sock );
}
//------------------------------------------------------------
// Send a message to the identified peer. In order to decide how to
// send the message, the router handler for this peer will be called.
// If no router function has been registered, it will not work. The
// router function should provide the next immediate peer to whom the 
// message should be forwarded. The peer's reply, if it is expected, 
// will be returned.
//
// Returns "Send Failed" if the message could not be routed.
//------------------------------------------------------------
vector<string> Peer::SendToPeer( string peerID,
                                 string msgType,
                                 string msgData,
                                 bool   waitReply = true ) {

    DebugMsg( "Peer::SendToPeer() " + name + " to " +
              peerID + " " + msgType + ":" + msgData );
        
    vector<string> replyMessages;
        
    if ( routerFunc ) {
        (this->*routerFunc)( peerID );
    }
    else {
        ConsoleMsg( "ERROR: Peer::SendToPeer() " + name +
                    " Router Function not available for [" +
                    msgType + "] to " + peerID );
        replyMessages.push_back( "Send Failed" );
        return replyMessages;
    }

    if ( not peerRoute.peerID.size() ) {
        ConsoleMsg( "ERROR: Peer::SendToPeer() " + name +
                    "routerFunc() peerID is empty "
                    "for msgType [" + msgType + "] to " + peerID );
        replyMessages.push_back( "Send Failed" );
        return replyMessages;
    }
        
    // The peerRoute structure is valid, send to that peer
    replyMessages = ConnectAndSend( peerRoute.host,
                                    peerRoute.port,
                                    msgType, msgData,
                                    peerRoute.peerID,
                                    waitReply );
    return replyMessages;
}
   
//------------------------------------------------------------
// Connects and sends a message to the specified host:port.
// The host's reply, if expected, will be returned as a list.
//------------------------------------------------------------
vector<string> Peer::ConnectAndSend( string host, int port,
                                     string msgType, string msgData,
                                     string peerID,
                                     bool waitReply ) {
    string reply;
    vector<string> replies;

    // Create a PeerConnection object
    // client_sock is 0, requesting a new socket for the connection
    PeerConnection PC = PeerConnection( peerID, host, port, 0 );

    bool status = PC.SendData( msgType, msgData );
    if ( status ) {
        DebugMsg( "Peer::ConnectAndSend() " + name +
                  " sent " + msgType + " to " + peerID );
    }
    else {
        ConsoleMsg( "ERROR: Peer::ConnectAndSend() " + name +
                    " Failed to send " + msgType + " to " + peerID );
    }

    if ( waitReply ) {
        reply = PC.ReceiveData();

        while ( reply != "None" ) {
            replies.push_back( reply );
                
            DebugMsg( "Peer::ConnectAndSend() " + name +
                      " received from [" + peerID +
                      "]  reply [" + reply + "]");

            size_t i = reply.find( ":" );
            string replyMsg  = reply.substr( 0, i );
            string replyHost = reply.substr( i + 1, string::npos );

            if ( replyHost.size() < 6 ) {
                // This can't be a valid host string (host:port)
                ConsoleMsg( "ERROR: Peer::ConnectAndSend() " + name +
                            " replyMsg [" + replyMsg +
                            "]  replyHost [" + replyHost + "]  size:" +
                            to_string( replyHost.size() ) +
                            " is to small to be valid host:port" );
                break;
            }

            reply = PC.ReceiveData();
        }
        PC.Close();
    }

    return( replies );
}

//------------------------------------------------------------
// Registers and starts a stabilizer function with this peer. 
// The function will be activated every <delay> seconds. 
//------------------------------------------------------------
void Peer::StartStabilizer( int delay = 3 ) {
    DebugMsg( "Peer::StartStabilizer " + name + " delay " +
              to_string( delay ) );
        
    thread stableThread( &Peer::RunStabilizer, this, delay );
    stableThread.join();
}

//------------------------------------------------------------
// CheckLivePeers() should be run every few seconds....
// 
//------------------------------------------------------------
void Peer::RunStabilizer( int delay ) {
    DebugMsg( "Peer::RunStabilizer " + name + " delay " +
              to_string( delay ) );
        
    while ( not shutdown ) {
        CheckLivePeers();
        sleep( delay );
    }
}
    
//------------------------------------------------------------
// Attempts to ping all currently known peers to ensure that
// they are still active. Removes any from the peer list that do
// not reply. This function can be used as a simple stabilizer.
//------------------------------------------------------------
void Peer::CheckLivePeers() {

    DebugMsg( "Peer::CheckLivePeers " + name );

    vector<string> toDelete;
        
    map< string, PeerInfo * >::iterator pi; // Peers map iterator

    for( pi = Peers.begin(); pi != Peers.end(); ++pi ) {
        string peerID      = pi->first;
        PeerInfo *peerInfo = pi->second;

        ConsoleMsg( "Peer::CheckLivePeers " + name +
                    " checking " + peerID );

        // Create a PeerConnection object
        // client_sock is 0, requesting a new socket for the connection
        PeerConnection PC = PeerConnection( peerID,
                                            peerInfo->host,
                                            peerInfo->port, 0 );

        if ( not PC.SendData( "PING", "" ) ) {
            ConsoleMsg( "ERROR: Peer::CheckLivePeers() " + name +
                        " PING SendData Failed to " + peerInfo->host + ":" +
                        to_string( peerInfo->port ) );
            toDelete.push_back( peerID );
        }
        else {
            string reply = PC.ReceiveData();
            DebugMsg( "Peer::CheckLivePeers() " + name +
                      " Received: " + reply );
            PC.Close();
        }
    }
        
    ConsoleMsg( "Peer::CheckLivePeers " + name + " There are " +
                to_string( toDelete.size() ) + " Peers to delete" );
        
    // Now delete all Peers that didn't respond from the Peers map
    peerLock.lock();   // Critical Section Lock <<<<<<<<<<<<<<<<<<<
    vector<string>::iterator vi;
    for( vi = toDelete.begin(); vi != toDelete.end(); ++vi ) {
        RemovePeer( *vi );
    }
    peerLock.unlock(); // Critical Section Unlock >>>>>>>>>>>>>>>>>
}    
    
//------------------------------------------------------------
// 
//------------------------------------------------------------
void Peer::StartMainLoop() {
    thread mainLoop( &Peer::MainLoop, this );
    mainLoop.detach();
}
    
//------------------------------------------------------------
//
//------------------------------------------------------------
void Peer::MainLoop() {
        
    DebugMsg( "Peer::MainLoop" + name );
        
    int listen_sock = MakeServerSocket( serverPort );
        
    //-------------------------------------------------------
    // Set a non-blocking timeout of timeOut seconds
    struct timeval timeout;      
    timeout.tv_sec  = timeOut;
    timeout.tv_usec = 0;
        
    int status = setsockopt( listen_sock,
                             SOL_SOCKET,
                             SO_RCVTIMEO,
                             (char *)&timeout,
                             sizeof(timeout) );
    if ( status < 0) {
        cerr << "Peer::MainLoop setsockopt SO_RCVTIMEO failed "
             << strerror( errno ) << endl;
        shutdown = true;
    }
    // Do we need to set a timeout on send? -> SO_SNDTIMEO
        
    //-------------------------------------------------------
    // Server listening main loop for this peer
        
    while ( not shutdown ) {
        DebugMsg( "Peer::MainLoop " + name +
                  " Listening for connections..." );
            
        struct sockaddr_in in_address;
        int addrlen = sizeof( in_address );

        // Wait for incoming connections
        // The accept() system call is used with connection-based socket
        // types (SOCK_STREAM, SOCK_SEQPACKET).  It extracts the first
        // connection request on the queue of pending connections for
        // the listening socket, creates a new connected socket, and
        // returns a new file descriptor referring to that socket.
        // The newly created socket is not in the listening state.
        int client_sock = accept( listen_sock,
                                  (struct sockaddr *)&in_address,
                                  (socklen_t *)&addrlen);
        if ( client_sock == -1 ) {
            // Timed out... Loop back and call accept again
            // This is intended to allow shutdown to be activated.
            // But if CheckLivePeers() is running and there are other
            // peers on the network, then the above accept will be
            // returning at the interval of the stabilizer
            continue;
        }
        else if ( client_sock < 0 ) {
            cerr << "Peer::MainLoop accept error "
                 << strerror( errno ) << endl;
            shutdown = true;
        }

        string client_host = inet_ntoa( in_address.sin_addr );
        int    client_port = ntohs    ( in_address.sin_port );
            
        // Inform user of socket number used in send and receive commands
        ConsoleMsg( "Peer::MainLoop " + name +
                    " New client connection on socket " +
                    to_string( client_sock ) + " host: " + client_host +
                    " port: " + to_string( client_port ) );

        // Create handler thread for the client request via lambda func
        // encapsulating the HandlePeer() function
        thread handler_thread( [ this,
                                 client_sock,
                                 client_host,
                                 client_port ]() {
                                   HandlePeer( client_sock,
                                               client_host,
                                               client_port );
                               } );

        handler_thread.detach();
            
    } // while ( not shutdown )

    // Should implment a SIGINT hander for ctrl-c to set shutdown
    // and exit MainLoop... but can't do it from within class
    // since signal handler requires a static function, but a
    // static member function doesn't have this-> to call the shutdown
    // accessor function...
    // https://www.tutorialspoint.com/cplusplus/cpp_signal_handling.htm
    // Instead, an EXIT message will set shutdown true
    //
    // This should work:
    // "https://stackoverflow.com/questions/4250013/"
    // "is-destructor-called-if-sigint-or-sigstp-issued"
        
    close( listen_sock );
}  

//------------------------------------------------------------
// 
//------------------------------------------------------------
void Peer::LagrangeInterpolate( vector<int64> x_vec,
                                vector<int64> f_x_vec,
                                int64 prime ) {
        
    int64 recoveredValue = 0;
    vector<int64>::size_type i;
    vector<int64>::size_type j;
	
    for( i = 0; i < x_vec.size(); i++ ){
        // Multiply numerators across the top and denominators
        // across the bottom for Lagrange interpolation
        // Π[ (x_i % Z) / (( x_i - x_j ) % Z) ]
        int64 numerator   = 1;
        int64 denominator = 1;
	    
        for( j = 0; j < x_vec.size(); j++ ) {
            if( i == j ) {
                continue; // The same share
            }

            // Get values of x from the i,j share pair
            int64 x_i = x_vec[ i ];
            int64 x_j = x_vec[ j ];
		
            numerator   = Poly.Modulus( numerator * -x_j, prime );
            denominator = Poly.Modulus( denominator * (x_i - x_j), prime );
        }

        // Sum values of { Z + f(x) * Π[ x_i / ( x_i - x_j ) ] } % Z
        int64 f_x_i  = f_x_vec[ i ];
        int64 modInv = Poly.ModInverse( denominator, prime );

        recoveredValue = Poly.Modulus(
            ( prime + recoveredValue + f_x_i * numerator * modInv ),
            prime );
    } 

    recoveredSecret = recoveredValue;

    DebugMsg( "Peer::LagrangeInterp() " + name + " Recovered secret: " +
              to_string( recoveredValue ) );
}
