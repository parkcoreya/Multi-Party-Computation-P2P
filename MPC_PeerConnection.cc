#include "MPC_PeerConnection.h"


//------------------------------------------------------------
// Set socket_status to -1 or errno on failure, 0 on success
//------------------------------------------------------------
int PeerConnection::Connect() {
        
    if ( client_sock == 0 ) {

        DebugMsg( "PeerConnection::Connect GetSocket() port " +
                  to_string( port ) );
            
        sock = GetSocket( port );

        if ( sock == -1 ) {
            return( -1 );
        }
    }
    else {
        sock = client_sock;
    }

    // Try to connect to other peer on port
    struct sockaddr_in remoteaddr;
    remoteaddr.sin_family = AF_INET;
    remoteaddr.sin_port   = htons( port );
    inet_pton( AF_INET, host.c_str(), &(remoteaddr.sin_addr) );
        
    int err = connect( sock, (struct sockaddr *)&remoteaddr,
                       sizeof(remoteaddr) );

    if ( err != 0 ) {
        // On Ubuntu, socket error codes were in
        // /usr/include/asm-generic/errno.h
        if ( errno == EISCONN ) {
            // No problem, already connected
            err = 0;
        }
        else {
            ConsoleMsg( "ERROR: PeerConnection::Connect() "
                        " failed on host " + host +
                        " port "  + to_string( port ) +
                        " sock "  + to_string( sock ) +
                        " errno " + to_string( errno ) + " " +
                        strerror( errno ) );
        }
        return( err );
    }
    else {
        DebugMsg( "PeerConnection::Connect() connected to " + host );
        return( 0 );
    }
}
    
//------------------------------------------------------------
//
//------------------------------------------------------------
string PeerConnection::MakeMessage( string msgType, string msgData ) {
        
    string message = msgType + ":" + msgData;
        
    return( message );
}

//------------------------------------------------------------
// Send a message through a peer connection.
// Returns True on success or False if there was an error.
//------------------------------------------------------------
bool PeerConnection::SendData( string msgType, string msgData ) {

    string message = MakeMessage( msgType, msgData );

    DebugMsg( "PeerConnection::SendData to " +
              host + " port: " + to_string( port ) + " " +
              msgType + ":" + msgData + " socket_status is " +
              to_string( socket_status ) );

    if ( socket_status != 0 ) {
        ConsoleMsg( "ERROR: PeerConnection::SendData socket_status error " +
                    host + " port: " + to_string( port ) );
        return( false );
    }

    int status = send( sock, message.c_str(), message.size(), 0 );
        
    if( status != message.size() ) {
        ConsoleMsg( "ERROR: PeerConnection::SendData send error to " +
                    host + " port: " + to_string( port ) + " " +
                    strerror( errno ) );
        return( false );
    }
        
    ConsoleMsg( "PeerConnection::SendData message [" + message +
                " ]  sent to: " + host + " port: " + to_string( port ) );
        
    return( true );
}
    
//------------------------------------------------------------
// Receive a message from a peer connection. Returns "None"
// if there was any error.
//------------------------------------------------------------
string PeerConnection::ReceiveData() {
        
    // Incoming message read buffer
    vector<char> buffer( BUFFER_LENGTH, 0 );

    // Read the incoming message
    int valread = recv( sock, buffer.data(), BUFFER_LENGTH, 0 );

    string message("None");
    if ( valread > 0 ) {
        message.clear();
        vector<char>::iterator bi;
        bi = find( buffer.begin(), buffer.end(), 0 );
        message.append( buffer.begin(), bi );

        ConsoleMsg( "PeerConnection::ReceiveData message from: " +
                    host + " port: " + to_string( port ) +
                    " [" + message + "]" );
    }
        
    return( message );
}

//------------------------------------------------------------
//
//------------------------------------------------------------
void PeerConnection::Close() {
    close( sock );
}
