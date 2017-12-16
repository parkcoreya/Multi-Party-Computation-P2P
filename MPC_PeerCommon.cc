#include "MPC_PeerCommon.h"

//----------------------------------------------------------------
// Tokenizing function... Creates tokens based on whitespace only
//----------------------------------------------------------------
vector<string> Tokenize( string message ) {
    // Break message string into tokens
    stringstream strstrm( message ); // Insert message into a stringstream
    vector<string> tokens;           // Break by whitespace into tokens
    string tmp_string;
    while ( strstrm >> tmp_string ) {
	tokens.push_back( tmp_string );
    }
    return( tokens );
}

//------------------------------------------------------------
// Prints messsage to console with id of the current thread
//------------------------------------------------------------
void ConsoleMsg( string msg ) {
    cout << "[" << this_thread::get_id() << "] "
         << msg << endl;
}

//------------------------------------------------------------
// Prints messsage to console with id of the current thread
// if DEBUG is #defined
//------------------------------------------------------------
void DebugMsg( string msg ) {
#ifdef DEBUG
    cout << "[" << this_thread::get_id() << "] "
         << msg << endl;
#endif
}

//------------------------------------------------------------
// Attempts to open a TCP/IP socket on the specified port
//------------------------------------------------------------
int GetSocket( int port, bool bind_socket ) {

    int sock = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sock == -1 ) {
        cerr << "ERROR: GetSocket() socket failed "
             << " port " << port << strerror( errno ) << endl;
        return( sock );
    }

    if ( bind_socket ) {
        struct sockaddr_in addr;
        socklen_t          addr_size = sizeof(struct sockaddr_in);

        addr.sin_family      = AF_INET;
        addr.sin_port        = htons( port );
        addr.sin_addr.s_addr = INADDR_ANY;
        
        // bind the socket to the address
        int status = ::bind( sock, (struct sockaddr *) &addr, addr_size );
        
        if ( status == 0 ) {
            DebugMsg( "GetSocket() socket " + to_string( sock ) +
                        " is bound on port: " + to_string( port) );
        }
        else {
            ConsoleMsg( "ERROR: GetSocket() bind failed on socket " +
                        to_string( sock ) + "  port " + to_string( port) +
                        " " + strerror( errno ) );
            close( sock ); // Failed to bind
        }
    }
    
    return( sock );
}

//------------------------------------------------------------
// Attempts to open a TCP/IP socket on the specified port
//------------------------------------------------------------
int GetSocketByAddrInfo( int port, bool bind_socket ) {

    int status         = 0;
    int sock           = 0;
    int socket_options = 1;

    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *rp;

    // Configure hints addrinfo structure for getaddrinfo()
    // memset( &hints, 0, sizeof(struct addrinfo) );
    hints.ai_family    = AF_UNSPEC;   // Allow IPv4 or IPv6
    hints.ai_socktype  = SOCK_STREAM; // Stream socket
    hints.ai_flags     = AI_PASSIVE;  // For wildcard IP address
    hints.ai_protocol  = 0;           // Any protocol
    hints.ai_canonname = NULL;
    hints.ai_addr      = NULL;
    hints.ai_next      = NULL;

    // Get address info for the specified port
    status = getaddrinfo( NULL, to_string( port ).c_str(), &hints, &result );
    
    if ( status != 0 ) {
        cerr << "GetSocket() getaddrinfo failed: "
             << gai_strerror( errno ) << endl;
        return( 0 );
    }

    // getaddrinfo() returns a list of address structures into result.
    // Try each address until we successfully bind(2).
    // If socket(2) (or bind(2)) fails, close the socket and try next address.
    for ( rp = result; rp != NULL; rp = rp->ai_next ) {
        
        sock = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        
        if ( sock == -1 ) {
            continue; // Failed, look at next address in list
        }

        // Set socket to reuse connection
        status = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR,
                            (char *)&socket_options, sizeof(socket_options));
        if( status < 0 ) {
            cerr << "ERROR: GetSocket() setsockopt failed "
                 << strerror( errno ) << endl;
        }

        if ( bind_socket ) {
            // bind the socket to the address
            status = ::bind( sock, rp->ai_addr, rp->ai_addrlen );
            
            if ( status == 0 ) {
                DebugMsg( "GetSocket() socket " + to_string( sock ) +
                            " is bound on port: " + to_string( port) );
                break; // Success
            }
            else {
                ConsoleMsg( "ERROR: GetSocket() bind failed on socket " +
                            to_string( sock ) + "  port " + to_string( port) );
                close( sock ); // Failed to bind
            }
        }
    }

    if ( rp == NULL ) { // No address succeeded
        ConsoleMsg( "ERROR: GetSocket() failed on socket " + to_string( sock ) );
        sock = 0;
    }

    freeaddrinfo( result ); // result is no longer needed

    return( sock );
}

//------------------------------------------------------------
// Try to resolve this host name by connecting to remoteServer
// This code is taken from the getaddrinfo man page example
// (http://man7.org/linux/man-pages/man3/getaddrinfo.3.html)
//
// In the Python btpeer.py design, it replaces __initserverhost()
//------------------------------------------------------------
string GetServerHost( int port ) {
    
    int sock = GetSocket( port );

    if ( sock == -1 ) {
        ConsoleMsg( "ERROR: GetServerHost() GetSocket Failed on port " +
                    to_string(port) );
        return( "None" );
    }        

    DebugMsg( "Peer::GetServerHost() on port " + to_string( port ) +
              "  socket " + to_string( sock ) );

    // Try to connect to remoteServer on port 80
    string remoteServer( "216.58.209.110" );  // google.com 216.58.218.238
    struct sockaddr_in remoteaddr;
    remoteaddr.sin_family = AF_INET;
    remoteaddr.sin_port   = htons( 80 );
    inet_pton( AF_INET, remoteServer.c_str(), &(remoteaddr.sin_addr) );
    
    int err = connect( sock, (struct sockaddr *)&remoteaddr,
                       sizeof(remoteaddr) );
    if ( err != 0 ) {
        ConsoleMsg( "ERROR: GetServerHost() connect failed " +
                    string( strerror( errno ) ) );
    }
    else {
        ConsoleMsg( "GetServerHost() connected to " + remoteServer );
    }

    struct sockaddr_in in_address;
    socklen_t socklen = sizeof(in_address);
    err = getsockname( sock, (struct sockaddr *) &in_address, &socklen );
    if ( err != 0 ) {
        ConsoleMsg( "ERROR: GetServerHost() getsockname failed " +
                    string( strerror( errno ) ) );
    }

    close( sock );
    
    char hostAddress[128];
    inet_ntop( AF_INET, &in_address.sin_addr.s_addr, hostAddress, 128 );

    ConsoleMsg( "GetServerHost() Local address is " + string(hostAddress) );

    return( string( hostAddress ) );
}
