#include "MPC_PeerHandler.h"

extern mutex peerLock; // In MPC_Peer.cc

//------------------------------------------------------------
// Constructor
// Initializes the peer to support connections up to maxpeers number
// of peers, with its server listening on the specified port.
// Adds handlers to the Peer framework.
//------------------------------------------------------------
MPC_Peer::MPC_Peer( string serverhost, int serverport,
                    string name, int maxpeers, int timeout ) :
    Peer::Peer( serverhost, serverport, name, maxpeers, timeout ) {

    // Assign the Router function pointer to the Peer base class
    // routerFunc pointer.
    Peer::AddRouter( (RouterFunc)(&MPC_Peer::Router) );

    // Setup the Peer::Handlers map with the handler functions
    // in MPC_Peer
    Handlers[ "INSERTPEER" ] = (HandlerFunc)(&MPC_Peer::InsertPeer);
    Handlers[ "LISTPEERS"  ] = (HandlerFunc)(&MPC_Peer::ListPeers);
    Handlers[ "PEERDATA"   ] = (HandlerFunc)(&MPC_Peer::PeerData);
    Handlers[ "DISTRIBUTE" ] = (HandlerFunc)(&MPC_Peer::Distribute);
    Handlers[ "SHAREVALUE" ] = (HandlerFunc)(&MPC_Peer::ReceiveShareValue);
    Handlers[ "LISTSHARES" ] = (HandlerFunc)(&MPC_Peer::ListShares);
    Handlers[ "LI"         ] = (HandlerFunc)(&MPC_Peer::LagrangeInterp);
    Handlers[ "LIADD"      ] = (HandlerFunc)(&MPC_Peer::LagrangeInterpAdd);
    Handlers[ "REMOVE"     ] = (HandlerFunc)(&MPC_Peer::Remove);
    Handlers[ "PING"       ] = (HandlerFunc)(&MPC_Peer::Ping);
    Handlers[ "COMMANDS"   ] = (HandlerFunc)(&MPC_Peer::Commands);
    Handlers[ "EXIT"       ] = (HandlerFunc)(&MPC_Peer::Exit);
}

//------------------------------------------------------------
// The Router is used by Peer::SendToPeer() to decide where to
// send the message. It does this by updating the PeerRoute
// structure in the Peer object. 
//------------------------------------------------------------
void MPC_Peer::Router( string peerID ) {
        
    if ( Peers.count( peerID ) == 0 ) {
        // peerID not in Peers map
        peerRoute.peerID = "";
        peerRoute.host   = "";
        peerRoute.port   = 0;
    }
    else {
        // Populate the PeerRoute struct for the requested peerID
        PeerInfo *peerInfo = Peers[ peerID ];
        peerRoute.peerID = peerID;
        peerRoute.host   = peerInfo->host;
        peerRoute.port   = peerInfo->port;
    }

    return;
}

//------------------------------------------------------------
// INSERTPEER message handler.
// Message data should be a string of the form:
// "peerid  host  port shareID x", where peerid is the name of
// the peer that desires to be added to this peer's list of peers,
// host and port are the necessary data to connect to the peer,
// shareID, and x is the peers share base exponent
//------------------------------------------------------------
void MPC_Peer::InsertPeer( PeerConnection *pc, string data ) {

    DebugMsg( "MPC_Peer::InsertPeer " + name + " data  [" + data + "]" );

    vector<string> tokens = Tokenize( data );

    if ( tokens.size() < 5 ) {
        ConsoleMsg( "ERROR: MPC_Peer::InsertPeer " + name +
                    " Tokenize failed on data [" + data + "]" );
        return;
    }
        
    string peerID  = tokens[0];
    string host    = tokens[1];
    int    port    = stoi( tokens[2] );
    string shareID = tokens[3];
    int64  base_x  = stoll( tokens[4] );

    DebugMsg( "MPC_Peer::InsertPeer " + name +
              " Tokenize: peerID " + peerID + 
              "  host " + host + "  port " + to_string( port ) +
              " share " + shareID );
            
    if ( MaxPeersReached() ) {
        ConsoleMsg( "WARNING: MPC_Peer::InsertPeer " + name +
                    " Max Peers reached: " + to_string( maxPeers ) + 
                    " terminating connection" );

        pc->SendData( "ERROR", "InsertPeer max peers reached." );
        return;
    }
            
    peerLock.lock();   // Critical Section Lock <<<<<<<<<<<<<<

    if ( Peers.count( peerID ) == 0 and
         peerID.compare( ID ) != 0 ) {
        // peerID not in Peers map, and not this Peer ID
        AddPeer( peerID, host, port, shareID, base_x );

        DebugMsg( "MPC_Peer::InsertPeer " + name +
                  " Added peer with ID " + peerID );
            
        pc->SendData( "REPLY", "InsertPeer added peer " +  peerID );
    }
    else {
        pc->SendData( "ERROR", "InsertPeer Peer " + peerID +
                      " already inserted" );          
    }
        
    peerLock.unlock(); // Critical Section UnLock >>>>>>>>>>>>
}
    
//------------------------------------------------------------
// LISTPEERS message handler.  Message data is not used.
// This is implemented differently than the Python app where
// individual messages are sent: one with the number of peers,
// then one for each peer.  We send all the information in
// one message. 
//------------------------------------------------------------
void MPC_Peer::ListPeers( PeerConnection *pc, string data ) {

    DebugMsg( "MPC_Peer::ListPeers " + name + " data [" + data + "]" );
        
    peerLock.lock();   // Critical Section Lock <<<<<<<<<<<<<<

    ostringstream ostrm;
    ostrm << "NUMPEERS=" << Peers.size();
    int iPeer = 1;
    map< string, PeerInfo * >::iterator pi;
    for( pi = Peers.begin(); pi != Peers.end(); ++pi ) {
        string peerID = pi->first;

        ostrm << " PEER" << iPeer << "=" << peerID;
            
        ++iPeer;
    }

    pc->SendData( "REPLY", ostrm.str() );
        
    peerLock.unlock(); // Critical Section UnLock >>>>>>>>>>>>
}
    
//------------------------------------------------------------
// PEERDATA message handler. Message data is not used.
// Replies with information about this Peer including the
// Peer ID, shareID and share base exponent. 
//------------------------------------------------------------
void MPC_Peer::PeerData( PeerConnection *pc, string data ) {

    DebugMsg( "MPC_Peer::PeerData " + name +
              " ID [" + ID + "]  data [" + data + "]" );

    // Sends: "ID shareID ShareBaseExponent"
    ostringstream ostrm;
    ostrm << ID << " " << Share->shareID << " " << Share->x;
        
    pc->SendData( "REPLY", ostrm.str() );
}
    
//------------------------------------------------------------
// COMMANDS message handler.  Message data is not used.
// Sends a string of available commands/message handlers
// This is just a convenience function for telnet interfacing
//------------------------------------------------------------
void MPC_Peer::Commands( PeerConnection *pc, string data ) {
        
    DebugMsg( "MPC_Peer::Commands " + name +
              " ID [" + ID + "]  data [" + data + "]" );

    ostringstream ostrm;
        
    map< string, HandlerFunc >::iterator mi;
    for ( mi = Handlers.begin(); mi != Handlers.end(); ++mi ) {
        ostrm << mi->first << " ";
    }
        
    pc->SendData( "REPLY", ostrm.str() + '\n' );
}

//------------------------------------------------------------
// LISTSHARES message handler.  Message data is not used.
// This is just a convenience function for telnet interfacing 
//------------------------------------------------------------
void MPC_Peer::ListShares( PeerConnection *pc, string data ) {

    DebugMsg( "MPC_Peer::ListShares " + name + " data [" + data + "]" );
        
    peerLock.lock();   // Critical Section Lock <<<<<<<<<<<<<<

    ostringstream ostrm;
    ostrm << name + " NUMSHARES=" << CollectedShares.size();
    int iShare = 1;

    // Iterate through the CollectedShares map of this Peer
    map< string, ShareInfo * >::iterator si;
    for( si = CollectedShares.begin(); si != CollectedShares.end(); ++si ) {
        string     shareID    = si->first;
        ShareInfo *pShareInfo = si->second;

        ostrm << " SHARE" << iShare << "=[" << shareID
              << " x=" << pShareInfo->x << " f_x=" << pShareInfo->f_x;

        // Iterate through the ShareInfo evaluatedShare map
        map< int64, int64 >::iterator ei;
        for ( ei =  pShareInfo->evaluatedShare.begin();
              ei != pShareInfo->evaluatedShare.end(); ++ei ) {
            ostrm<< " " << ei->first << " " << ei->second;
        }
        ostrm << "]  ";
            
        ++iShare;
    }

    pc->SendData( "REPLY", ostrm.str() );
        
    peerLock.unlock(); // Critical Section UnLock >>>>>>>>>>>>
}
    
//------------------------------------------------------------
// LI message handler.  data is a shareID
// 
// CollectedShares is this Peers map of ShareInfo structs
// Each ShareInfo struct corresponds to a remote Peer
//
// ShareInfo contains:
//    int64  x;
//    int64  f_x;
//    map< int64, int64 > evaluatedShare;
//
// x   is the Peers own base exponent
// f_x is the Peers polynomial evaluated at x
//
// evaluatedShare is a map of { x, f_x } pairs (key, value)
// of the other Peers base exponent (x) evaluated at this Peers
// polynomial into f_x.
//
// A complete share in terms of x, f_x pairs is then:
// [ x, f_x, x.i, f_x.i, x.j, f_x.j, x.k, f_x.k, ... ]
// where x.i, f_x.i... are the remote Peer values from evaluatedShare
//------------------------------------------------------------
void MPC_Peer::LagrangeInterp( PeerConnection *pc, string data ) {

    DebugMsg( "MPC_Peer::LagrangeInterp " + name + " data [" + data + "]" );

    vector<string> tokens = Tokenize( data );

    if ( tokens.size() < 1 ) {
        ConsoleMsg( "ERROR: MPC_Peer::LagrangeInterp " + name +
                    " Tokenize failed on data [" + data + "]" );
        return;
    }
    string shareID = tokens[0];
        
    // Get the ShareInfo for this shareID
    if ( CollectedShares.count( shareID ) == 0 ) {
        ConsoleMsg( "ERROR::MPC_Peer LagrangeInterp " + name +
                    " failed to find share " + shareID +
                    " in CollectedShares" );
        return;
    }
        
    // Is the mutex lock needed?  CP
    peerLock.lock();   // Critical Section Lock <<<<<<<<<<<<<<

    ShareInfo *pShareInfo = CollectedShares[ shareID ];
        
    // Get the x,f_x pairs into vectors for Peer::LagrangeInterpolate
    vector< int64 > x_vec;
    vector< int64 > f_x_vec;

    // Insert the x, f_x pairs from the ShareInfo evaluatedShare map
    map< int64, int64 >::iterator ei;
    for ( ei  = pShareInfo->evaluatedShare.begin();
          ei != pShareInfo->evaluatedShare.end(); ++ ei ) {
            
        x_vec.push_back  ( ei->first  );
        f_x_vec.push_back( ei->second );
    }
        
    Peer::LagrangeInterpolate( x_vec, f_x_vec, pShareInfo->prime );

    peerLock.unlock(); // Critical Section UnLock >>>>>>>>>>>>

    ConsoleMsg( "MPC_Peer::LagrangeInterp " + name + " Recovered value " +
                to_string( recoveredSecret ) + " from " + shareID );
}

//------------------------------------------------------------
// LIADD message handler.  data are two shareID's
// 
//------------------------------------------------------------
void MPC_Peer::LagrangeInterpAdd( PeerConnection *pc, string data ) {

    DebugMsg( "MPC_Peer::LagrangeInterpAdd " + name + " data ["+data+"]");

    vector<string> tokens = Tokenize( data );

    if ( tokens.size() < 2 ) {
        ConsoleMsg( "ERROR: MPC_Peer::LagrangeInterpAdd " + name +
                    " Tokenize failed on data [" + data + "]" );
        return;
    }
    string shareID_1 = tokens[0];
    string shareID_2 = tokens[1];
        
    // Check that the shareID's are in the Peers CollectedShares map
    if ( CollectedShares.count( shareID_1 ) == 0 ) {
        ConsoleMsg( "ERROR::MPC_Peer LagrangeInterpAdd " + name +
                    " failed to find share " + shareID_1 +
                    " in CollectedShares" );
        return;
    }
    if ( CollectedShares.count( shareID_2 ) == 0 ) {
        ConsoleMsg( "ERROR::MPC_Peer LagrangeInterpAdd " + name +
                    " failed to find share " + shareID_2 +
                    " in CollectedShares" );
        return;
    }
        
    // Is the mutex lock needed?  CP
    peerLock.lock();   // Critical Section Lock <<<<<<<<<<<<<<

    // Get the ShareInfo for both shareID's
    ShareInfo *pShareInfo_1 = CollectedShares[ shareID_1 ];
    ShareInfo *pShareInfo_2 = CollectedShares[ shareID_2 ];

    if ( pShareInfo_1->evaluatedShare.size() !=
         pShareInfo_2->evaluatedShare.size() ) {
            
        ConsoleMsg( "ERROR::MPC_Peer LagrangeInterpAdd " + name +
                    " share " + shareID_1 + " and " + shareID_2 +
                    " do not have the same length evaluatedShare maps" );
        return;
    }
        
    // Get the x,f_x pairs into vectors for Peer::LagrangeInterpolate
    vector< int64 > x_vec;
    vector< int64 > f_x_vec;

    // Iterate through both ShareID ShareInfo evaluatedShare maps
    // simultaneously and sum the f_x values into f_x_vec
    map<int64, int64>::iterator ei1;
    map<int64, int64>::iterator ei2;
    for( ei1 =  pShareInfo_1->evaluatedShare.begin(),
             ei2 =  pShareInfo_2->evaluatedShare.begin();
         ei1 != pShareInfo_1->evaluatedShare.end() or
             ei2 != pShareInfo_2->evaluatedShare.end();
         ++ei1, ++ei2 ) {

        if ( ei1->first != ei2->first ) {
            ConsoleMsg( "ERROR: MPC_Peer LagrangeInterpAdd "
                        " evaluatedShare maps have different x keys" );
            return;
        }
                            
        x_vec.push_back( ei1->first ); // Same x for shareID_1 shareID_2
        f_x_vec.push_back( ei1->second + ei2->second );
    }

    Peer::LagrangeInterpolate( x_vec, f_x_vec, pShareInfo_1->prime );

    peerLock.unlock(); // Critical Section UnLock >>>>>>>>>>>>

    ConsoleMsg( "MPC_Peer::LagrangeInterpAdd " + name + " Recovered value "+
                to_string( recoveredSecret ) + " from the sum of " +
                shareID_1 + " and " + shareID_2 );
}

//------------------------------------------------------------
// DISTRIBUTE message handler.
// For each Peer in the Peers map, evaluate this peers polynomial
// at the base exponent of remote Peers and send the updated
// values in a SHAREVALUE command to the remote Peers with SendToPeer() 
//------------------------------------------------------------
void MPC_Peer::Distribute( PeerConnection *pc, string data ) {

    DebugMsg( "MPC_Peer::Distribute " + name + " data [" + data + "]" );
        
    // Acknowledge the DISTRIBUTE
    string reply = "DISTRIBUTE ACK: " + name;
    pc->SendData( "REPLY", reply );

    // First evaluate this Peers own share at x and update the
    // Peer Share object f_x .... This may not be needed?
    Share->f_x = Poly.Polynomial( Share->coef,   // This Peers Share coef
                                  Share->x,      // This Peers Share x 
                                  Share->prime );// This Peers Share prime

    // Determine what base exponents are needed from the
    // current network of Peers, store in peerBaseExponents vector.
    // We will then iterate through this vector to compute f_x for
    // each base exponent x and store the pairs in evaluatedShare map
    vector< int64 > peerBaseExponents; // List of all peer base exponents
    vector< int64 >::iterator bi;
        
    peerLock.lock();  // Critical Section Lock <<<<<<<<<<<<<<

    map< string, PeerInfo * >::iterator pi;
    for( pi = Peers.begin(); pi != Peers.end(); ++pi ) {
        string    peerID = pi->first;
        PeerInfo *pInfo  = pi->second;

        if ( find( peerBaseExponents.begin(),
                   peerBaseExponents.end(),
                   pInfo->x ) == peerBaseExponents.end() ) {
            // pInfo->x is not in peerBaseExponents, insert it
            peerBaseExponents.push_back( pInfo->x );
        }
    }
        
    // Evaluate polynomial at each base exponent of the Peers
    // and insert in localEvaluatedShare map
    map< int64, int64 > localEvaluatedShare;
    map< int64, int64 >::iterator ei;
    for ( bi  = peerBaseExponents.begin();
          bi != peerBaseExponents.end(); ++bi ) {

        localEvaluatedShare[ *bi ] =
            Poly.Polynomial( Share->coef,   // This Peers Share coef
                             *bi,           // Remote Peer x
                             Share->prime );// This Peers Share prime
    }
    // Also insert the Peers own x, f_x, this will be redundant with the
    // Peers x, f_x sent as tokens 3 and 4 in SHAREVALUE, but will make
    // it easier when multiple shares are added, multiplied etc...
    localEvaluatedShare[ Share->x ] = Share->f_x;

    // Create a SHAREVALUE message to send to each Peer
    // SHAREVALUE msgData string: "ShareID prime x f_x xi f_xi xj f_xj..."
    ostringstream ostrm;
    // Start the message with "ShareID prime" of this Peer
    ostrm << Share->shareID << " " << Share->prime << " " ;
    // **** !!!! CP
    //     The Peers own base exponent (x) and evaluated polynomial
    //     (f_x) are listed before the x, f_x pairs for other Peers
    //     This is redundant, since they are also listed in the
    //     evaluatedShares map, but makes it easier to support
    //     multiple shares. 
    // **** !!!!
    ostrm << Share->x << " " << Share->f_x;
        
    for ( ei  = localEvaluatedShare.begin();
          ei != localEvaluatedShare.end(); ++ei ) {
        // Add list of all x, f_x pairs
        ostrm << " " << ei->first << " " << ei->second;
    }

    DebugMsg( "MPC_Peer::Distribute " + name +
              " SHAREVALUE: " + ostrm.str() );
        
    // Send the evaluated x:f_x pairs to each Peer using a SHAREVALUE msg.
    for( pi = Peers.begin(); pi != Peers.end(); ++pi ) {
        string    peerID = pi->first;
        PeerInfo *pInfo  = pi->second;

        vector<string> replies;
        replies = SendToPeer( peerID, "SHAREVALUE", ostrm.str(), false );

        if ( replies.size() ) {
            if ( replies[0].find( "Send Failed" ) != string::npos ) {
                ConsoleMsg( "ERROR: MPC_Peer::Distribute " + name +
                            " SendToPeer() Failed to " + peerID );
            }
        }
        // ConsoleMsg success message?
    }
        
    peerLock.unlock(); // Critical Section UnLock >>>>>>>>>>>>
        
}
    
//------------------------------------------------------------
// SHAREVALUE message handler.
// data from the DISTRIBUTE command.
// This function is to populate the ShareInfo evaluatedShare map.
// A remote Peer has sent this peer their share value evaluated
// at the base exponent of this peer using the DISTRIBUTE
// command.  Save the ShareInfo in this peers CollectedShares map. 
//------------------------------------------------------------
void MPC_Peer::ReceiveShareValue( PeerConnection *pc, string data ) {

    DebugMsg( "MPC_Peer::ReceiveShareValue " + name +
              " data  [" + data + "]" );

    vector<string> tokens = Tokenize( data );

    if ( tokens.size() < 4 ) {
        ConsoleMsg( "ERROR: MPC_Peer::ReceiveShareValue " + name +
                    " Tokenize failed on data [" + data + "]" );
        return;
    }
        
    // data is: "ShareID prime x f_x xi f_xi xj f_xj..."
    //     The Peers own base exponent (x) and evaluated polynomial
    //     (f_x) are listed before the xi, f_xi pairs for other Peers
    string shareID  = tokens[0];
    int64  prime    = stoll( tokens[1] );
    int64  peer_x   = stoll( tokens[2] );  // The Peers own x
    int64  peer_f_x = stoll( tokens[3] );  // Peers own f_x

    // Extract the x1 f_x1, x f_x pairs into local evaluatedShare map
    map< int64, int64 > localEvaluatedShare;
    map< int64, int64 >::iterator ei;
        
    for ( size_t i = 4; i < tokens.size(); i = i + 2 ) {
        int64 x_in   = stoll( tokens[ i ]   );
        int64 f_x_in = stoll( tokens[ i+1 ] );
            
        localEvaluatedShare[ x_in ] = f_x_in;
    }
        
    DebugMsg( "MPC_Peer::ReceiveShareValue " + name +
              " Tokenize: shareID " + shareID +
              "  prime " + to_string( prime ) );

    // Create a new ShareInfo if one for shareID doesn't exist,
    // or update the existing one with the data sent from DISTRIBUTE
        
    peerLock.lock();   // Critical Section Lock <<<<<<<<<<<<<<

    ShareInfo *pShareInfo = 0;
        
    if ( CollectedShares.count( shareID ) == 0 ) {
        // This share is not in the CollectedShares map
        // Create a new ShareInfo and insert in the map
        pShareInfo = new ShareInfo();
        pShareInfo->shareID = shareID;
        pShareInfo->x       = peer_x;
        pShareInfo->f_x     = peer_f_x;
        pShareInfo->prime   = prime;

        // Now update the xi f_xi, xj f_xj pairs in the
        // ShareInfo evaluatedShare map
        for ( ei  = localEvaluatedShare.begin();
              ei != localEvaluatedShare.end(); ++ei ) {

            pShareInfo->evaluatedShare[ ei->first ] = ei->second;
        }

        // Insert the new ShareInfo struct in the CollectedShares map
        CollectedShares[ shareID ] = pShareInfo;
    }
    else {
        // This share is already in the CollectedShares map
        pShareInfo = CollectedShares[ shareID ];
            
        // Update this peer f_x in the CollectedShares ShareInfo
        pShareInfo->x   = peer_x;
        pShareInfo->f_x = peer_f_x;
            
        // Now update the x1 f_x1, x2 f_x2 pairs in the
        // ShareInfo evaluatedShare map
        pShareInfo->evaluatedShare.clear();
        for ( ei  = localEvaluatedShare.begin();
              ei != localEvaluatedShare.end(); ++ei ) {

            pShareInfo->evaluatedShare[ ei->first ] = ei->second;
        }
    }

    // Insert the Peers own share in evaluatedShare so that
    // LIADD or other multiple share MPC can operate on this
    // Peer with another Peer.
    if ( CollectedShares.count( Share->shareID ) == 0 ) {
        // A ShareInfo for this Peer is not in CollectedShares
        pShareInfo = new ShareInfo();
        pShareInfo->shareID = Share->shareID;
        pShareInfo->x       = Share->x;
        pShareInfo->f_x     = Share->f_x;
        pShareInfo->prime   = Share->prime;

        // Now update the xi f_xi, xj f_xj pairs in the
        // ShareInfo evaluatedShare map from the local evaluatedShare
        // by evaluating the Peers Polynomial at each xi
        for ( ei =  localEvaluatedShare.begin();
              ei != localEvaluatedShare.end(); ++ei ) {

            int64 f_x = 
                Poly.Polynomial( Share->coef,   // This Peers Share coef
                                 ei->first,     // value of x
                                 Share->prime );// This Peers Share prime
                
            pShareInfo->evaluatedShare[ ei->first ] = f_x;
        }

        // Insert the new ShareInfo struct in the CollectedShares map
        CollectedShares[ Share->shareID ] = pShareInfo;
    }
    else {
        // The Peer already has it's own ShareInfo in CollectedShares
        pShareInfo = CollectedShares[ Share->shareID ];
            
        // Update the x1 f_x1, x2 f_x2 pairs in the
        // ShareInfo evaluatedShare map
        pShareInfo->evaluatedShare.clear();
            
        for ( ei  = localEvaluatedShare.begin();
              ei != localEvaluatedShare.end(); ++ei ) {

            int64 f_x = 
                Poly.Polynomial( Share->coef,   // This Peers Share coef
                                 ei->first,     // value of x
                                 Share->prime );// This Peers Share prime
                
            pShareInfo->evaluatedShare[ ei->first ] = f_x;
        }
    }

    ConsoleMsg( "MPC_Peer::ReceiveShareValue " + name +
                " received from " + shareID );

    peerLock.unlock(); // Critical Section UnLock >>>>>>>>>>>>
}

//------------------------------------------------------------
// REMOVE message handler. The message data should be in the
// format of a string, "peer-id", where peer-id is the ID of the
// peer that wishes to be removed from this peer's directory.
//------------------------------------------------------------
void MPC_Peer::Remove( PeerConnection *pc, string data ) {

    DebugMsg( "MPC_Peer::Remove " + name + " data [" + data + "]" );

    vector<string> tokens = Tokenize( data );

    if ( tokens.empty() ) {
        ConsoleMsg( "ERROR: MPC_Peer::Remove " + name +
                    " No ID [" + data + "]" );
        return;
    }
        
    string peerID = tokens[0];
        
    DebugMsg( "MPC_Peer::Remove " + name + " Tokenize: peerID " + peerID );
        
    peerLock.lock();   // Critical Section Lock <<<<<<<<<<<<<<

    if ( Peers.count( peerID ) == 1 ) {
        // peerID is in Peers map
        string message = name + " REMOVE: Peer " + peerID + " removed.";
        ConsoleMsg( message );

        pc->SendData( "REPLY", message );

        RemovePeer( peerID );
    }
    else {
        string message = name + " REMOVE: Peer " + peerID + " not found.";
        ConsoleMsg( message );

        pc->SendData( "ERROR:", message );
    }
        
    peerLock.unlock(); // Critical Section UnLock >>>>>>>>>>>>
}

//------------------------------------------------------------
// PING message handler.  Message data is not used.
//------------------------------------------------------------
void MPC_Peer::Ping( PeerConnection *pc, string data ) {

    DebugMsg( "MPC_Peer::Ping " + name +
              " ID [" + ID + "]  data [" + data + "]" );

    pc->SendData( "REPLY", ID );
}

//------------------------------------------------------------
// EXIT message handler. 
// Exit sets shutdown true to exit server main loop
//------------------------------------------------------------
void MPC_Peer::Exit( PeerConnection *pc, string data ) {

    DebugMsg( "MPC_Peer::Exit " + name + " data [" + data + "]" );
    shutdown = true;
}

//------------------------------------------------------------
// Attempt to build the local peer list up to the limit stored by
// maxpeers.  First, send a PEERDATA request to the remote peer.
// If there is a valid response, then send an INSERTPEER request
// to insert this local peer into the remote peers Peers map.
// Then call AddPeer() to insert the remote peers ID into the
// Peers map of this local peer.  Finally, send a LISTPEERS request
// to the remote peer to get a list of their known-peers, and 
// using a simple depth-first search given an initial host 
// and port as starting point search the network for more peers.
// The depth of the search is limited by the hops parameter.
//------------------------------------------------------------
void MPC_Peer::BuildPeers( string host, int port, int hops ) {
        
    ConsoleMsg( "MPC_Peer::BuildPeers " + name +
                " to host " + host + "  port " +
                to_string( port ) + "  hops " + to_string( hops ) );

    // Why does this return if MaxPeersReached?  
    if ( MaxPeersReached() or hops < 1 ) {
        ConsoleMsg( "WARNING: MPC_Peer::BuildPeers " + name +
                    " max peers or zero hops " );
        return;
    }
        
    string remotePeerID;
    string msgType;
    string msgData;
    string reply;
    vector<string> replies;
    size_t i;  // index for string.find
        
    // Request PEERDATA from the remote peer 
    replies = ConnectAndSend( host, port, "PEERDATA", "" );
    if ( replies.size() == 0 ) {
        ConsoleMsg( "ERROR: MPC_Peer::BuildPeers " + name +
                    " PEERDATA no response" );
        return;
    }
    reply = replies[0];

    // PEERDATA returns ID shareID ShareBaseExponent as
    // REPLY:127.0.0.1:7777 Bob_Share 1
    vector<string> tokens = Tokenize( reply );
        
    if ( tokens.empty() ) {
        ConsoleMsg( "ERROR: MPC_Peer::BuildPeers" + name +
                    " PEERDATA reply was empty" );
        return;
    }
        
    // If there is no peerID of the form host:peer, ignore
    i            = tokens[0].find( ":" );
    msgType      = tokens[0].substr( 0, i );
    remotePeerID = tokens[0].substr( i + 1, string::npos );
    if ( remotePeerID.size() < 6 ) {
        // peerID should be XXX.XXX.XXX.XXX:YYYY
        ConsoleMsg( "ERROR: MPC_Peer::BuildPeers " + name +
                    " Invalid PEERDATA ID " + remotePeerID );
        return;
    }

    string remoteShareID = "";
    int64  remoteBase_x    = 0;
    if ( tokens.size() > 1 ) {
        remoteShareID = tokens[1];
    }
    if ( tokens.size() > 2 ) {
        remoteBase_x = stoll( tokens[2] );
    }

    DebugMsg( "MPC_Peer::BuildPeers " + name + " PEERDATA reply from peer ["
              + remotePeerID + "]  reply [" + reply + "]" );

    //------------------------------------------------------------
    // INSERTPEER this peer ID to the remote peer
    //------------------------------------------------------------
    msgData = ID + " " + serverHost + " " + to_string( serverPort ) +
        " " + Share->shareID + " " + to_string( Share->x );
        
    replies.clear();
    replies = ConnectAndSend( host, port, "INSERTPEER", msgData );
    reply   = replies[0];
    i       = reply.find( ":" );
    msgType = reply.substr( 0, i );
        
    DebugMsg( "MPC_Peer::BuildPeers " + name +
              " INSERTPEER response " + replies[0] );

    if ( msgType != "REPLY" ) {
        // INSERTPEER should reply: REPLY:InsertPeer added peer peerID
        ConsoleMsg( "ERROR: MPC_Peer::BuildPeers " + name +
                    " INSERTPEER response is not REPLY " + msgType );
        return;
    }

    if ( Peers.count( remotePeerID ) == 1 ) {
        ConsoleMsg( "WARNING: MPC_Peer::BuildPeers " + name +
                    " remotePeerID " + remotePeerID +
                    " is already in Peers" );
        return;
    }

    //------------------------------------------------------------
    // Add the remote peerID to this peer Peers map
    //------------------------------------------------------------
    AddPeer( remotePeerID, host, port, remoteShareID, remoteBase_x );
        
    //------------------------------------------------------------
    // Now a recursive depth-first search to add more Peers
    //------------------------------------------------------------
    replies.clear();
    replies = ConnectAndSend( host, port, "LISTPEERS", "", remotePeerID );

    // The REPLY from LISTPEERS will be a message of the form:
    // [REPLY:NUMPEERS=2 PEER1=127.0.0.1:7733 PEER2=127.0.0.1:7755]
    tokens = Tokenize( replies[0] );
        
    if ( tokens.empty() ) {
        ConsoleMsg( "MPC_Peer::BuildPeers " + name +
                    " LISTPEERS reply was empty" );
        return;
    }

#ifdef DEBUG
    // Print a message to the console 
    ostringstream ostrm;
    ostrm << "MPC_Peer::BuildPeers Tokenize LISTPEERS response: [";
    for ( vector<string>::iterator ri = tokens.begin();
          ri != tokens.end(); ++ri ) {
        ostrm << *ri << ", ";
    } ostrm << "]";
    DebugMsg( ostrm.str() );
#endif
        
    // Get the number of peers
    string numPeersStr = tokens[0]; // REPLY:NUMPEERS=2
    i = numPeersStr.find("=");
    int numPeers = stoi( numPeersStr.substr( i + 1, string::npos ) );

    if ( numPeers < 1 ) {
        ConsoleMsg( "MPC_Peer::BuildPeers " + name +
                    " LISTPEERS number of peers is " +
                    to_string( numPeers) );
        return;
    }
        
    if ( tokens.size() > 1 ) {
        // Start at i = 1 to skip the number of peers from LISTPEERS

        //----------------------------------------------------------
        // Loop for each PEER returned from LISTPEERS 
        //----------------------------------------------------------
        for( size_t j = 1; j < tokens.size(); j++ ) {
            string token = tokens[ j ];

            // token is: PEER1=127.0.0.1:7733
            i = token.find( "=" );
            string nextPeerID = token.substr( i + 1, string::npos );
                
            if ( nextPeerID != ID ) {
                i = nextPeerID.find( ":" );
                
                string host = nextPeerID.substr( 0, i );
                int    port = stoi(nextPeerID.substr( i + 1, string::npos));
                
                ConsoleMsg( "MPC_Peer::BuildPeers " + name +
                            " Recursive call to BuildPeers for host " +
                            host + " port " + to_string( port ) );
                //------------------------------
                // Recursive call to BuildPeers
                //------------------------------
                BuildPeers( host, port, hops-1 );
            }
        }
    }

    // If ERROR then remove peer?
    // RemovePeer( peerID );
}
