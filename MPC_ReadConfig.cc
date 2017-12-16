
#include <fstream>
#include <iostream>
#include "MPC_Common.h"
#include "MPC_ReadConfig.h"

//--------------------------------------------------------------
//--------------------------------------------------------------
int ReadConfig( PeerParams *peerParams, ShareParams *shareParams,
                string configFile, string path = "./config/" ) {

    // Create input file stream and open file for input
    ifstream configStrm( path + configFile );

    if ( not configStrm.is_open() ) {
        cerr << "ERROR: ReadConfig() file " << path + configFile
             << " is not open for reading." << endl;
        return -1;
    }
    if ( not configStrm.good() ) {
        cerr << "ERROR: ReadConfig() file " << path + configFile
             << " is not ready for reading." << endl;
        return -1;
    }

    // Read configFile into a vector of strings, one line per string
    vector< string > configLines;
    string tmp;

    while( getline( configStrm, tmp ) ) {
        configLines.push_back( tmp );
    }
    configStrm.close();

#ifdef DEBUG
    cout << "------- ReadConfig() Contents of file "
         << configFile << "-------" << endl;
    for( vector< string >::iterator ci = configLines.begin();
         ci != configLines.end(); ++ci ) {
        cout << *ci << endl;
    }
#endif

    // Extract the configFile param values into the structures
    vector< string >::iterator ci;
    for( ci = configLines.begin(); ci != configLines.end(); ++ci ) {

        vector<string> words = Tokenize( *ci );

        if ( words.size() ) {
            if ( words[0].find_first_of( '#' ) == 0 ) {
                // Ignore comment lines
                continue;
            }
            else if( words[0] == "name" ) {
                peerParams->name = words[1];
            }
            else if( words[0] == "serverPort" ) {
                peerParams->serverPort = stoi( words[1] );
            }
            else if( words[0] == "serverHost" ) {
                peerParams->serverHost = words[1];
            }
            else if( words[0] == "peerID" ) {
                peerParams->peerID = words[1];
            }
            else if( words[0] == "maxPeers" ) {
                peerParams->maxPeers = stoi( words[1] );
            }
            else if( words[0] == "timeOut" ) {
                peerParams->timeOut = stoi( words[1] );
            }
            else if( words[0] == "stabilize" ) {
                peerParams->stabilize = stoi( words[1] );
            }
            else if( words[0] == "hops" ) {
                peerParams->hops = stoi( words[1] );
            }
            else if( words[0] == "shareName" ) {
                shareParams->name = words[1];
            }
            else if( words[0] == "numCoef" ) {
                shareParams->numCoef = stoi( words[1] );
            }
            else if( words[0] == "coef" ) {
                // Assume that shareParams->numCoef is already processed
                if ( shareParams->numCoef < 1 ) {
                    cerr << "ERROR: ReadConfig() numCoef "
                         << shareParams->numCoef
                         << " must be provided for coef " << endl;
                    return -1;
                }
                // words[1] ... words[numCoef-1] have the coef values
                vector<int64> _coef;
                for ( size_t i = 1; i <= shareParams->numCoef; i++ ) {
                    _coef.push_back( stoll( words[ i ] ) );
                }
                shareParams->coef = _coef;
            }
            else if( words[0] == "x" ) {
                shareParams->x = stoll( words[1] );
            }
            else if( words[0] == "secret" ) {
                shareParams->secret = stoll( words[1] );
            }
            else if( words[0] == "prime" ) {
                shareParams->prime = stoll( words[1] );
            }
            else {
                cerr << "ERROR: ReadConfig() Invalid token "
                     << words[0] << endl;
                return -1;
            }
        }
        // cout << *ci << endl;
    }
    
    return 0;
}

//--------------------------------------------------------------
//--------------------------------------------------------------
void PrintConfig( PeerParams *peerParams, ShareParams *shareParams ) {

    cout << "Peer: name      : " << peerParams->name       << endl;
    cout << "Peer: serverPort: " << peerParams->serverPort << endl;
    cout << "Peer: serverHost: " << peerParams->serverHost << endl;
    cout << "Peer: peerID    : " << peerParams->peerID     << endl;
    cout << "Peer: maxPeers  : " << peerParams->maxPeers   << endl;
    cout << "Peer: timeOut   : " << peerParams->timeOut    << endl;
    cout << "Peer: stabilize : " << peerParams->stabilize  << endl;
    cout << "Peer: hops      : " << peerParams->hops       << endl;
    
    cout << "Share: shareName: " << shareParams->name    << endl;
    cout << "Share: numCoef  : " << shareParams->numCoef << endl;
    if ( shareParams->coef.size() ) {
        cout << "Share: coef     : ";
        for ( size_t i = 0; i < shareParams->coef.size(); i++ ) {
            cout << shareParams->coef[i] << " ";
        } cout << endl;
    }
    cout << "Share: x        : " << shareParams->x       << endl;
    cout << "Share: secret   : " << shareParams->secret  << endl;
    cout << "Share: prime    : " << shareParams->prime   << endl;
    
    return;
}
