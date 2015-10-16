

#include "network.h"
using namespace netNS;

//=============================================================================
// Constructor
//=============================================================================
Network::Network()
{
    bufferLength = THE_BUFFER_L;     // Length of send and receive buffers
    sockt = NULL;
    ret = 0;
    reAddSize = 0;
    netInitialized = false;
    within = false;
    mState = NotStarted;
    choice = NotConnected;
}

//=============================================================================
// Destructor
//=============================================================================
Network::~Network()
{
    closedSocket();                  // close connection, release memory
}

//=============================================================================
// Initialize network
// pChoice = optionUDP or optionTCP
// Called by netCreateServer and netCreatClient
// Pre:
//   portNo = Port number.
//   pChoice = optionUDP or optionTCP.
// Post:
//   Returns two part int code on theError.
//     The low 16 bits contains Status code as defined in net.h.
//     The high 16 bits contains "Windows Socket Error Code".
//=============================================================================
int Network::StartTheNetwork(int portNo, int pChoice)
{
    unsigned long ul = 1;
    int           nRet;
    int status;

    if(netInitialized)              // if network currently initialized
        closedSocket();              // close current network and start over

    mState = NotStarted;

    status = WSAStartup(0x0202, &wsaData);  // initiate the use of winsock 2.2
    if (status != 0)
        return ( (status << 16) + NetworkStartUpFailed);

    switch (pChoice)
    {
    case optionUDP:     // optionUDP
        // Create optionUDP socket and bind it to a local interface and portNo
        sockt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sockt == INVALID_SOCKET) {
            WSACleanup();
            status = WSAGetLastError();         // get detailed theError
            return ( (status << 16) + SocketFailed);
        }
        choice = optionUDP;
        break;
    case optionTCP:     // optionTCP
        // Create optionTCP socket and bind it to a local interface and portNo
        sockt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockt == INVALID_SOCKET) {
            WSACleanup();
            status = WSAGetLastError();         // get detailed theError
            return ( (status << 16) + SocketFailed);
        }
        choice = NotConnectedTCP;
        break;
    default:    // Invalid choice
        return (NetworkStartUpFailed);
    }

    // put socket in non-blocking mState
    nRet = ioctlsocket(sockt, FIONBIO, (unsigned long *) &ul);
    if (nRet == SOCKET_ERROR) {
        WSACleanup();
        status = WSAGetLastError();             // get detailed theError
        return ( (status << 16) + SocketFailed);
    }

    // set local family and portNo
    localAdd.sin_family = AF_INET;
    localAdd.sin_port = htons((u_short)portNo);    // portNo number

    // set remote family and portNo
    remAdd.sin_family = AF_INET;
    remAdd.sin_port = htons((u_short)portNo));   // portNo number

    netInitialized = true;
    return NetworkOk;
}

//=============================================================================
// Setup network for use as tServer
// May not be configured as Server and Client at the same time.
// Pre: 
//   portNo = Port number to listen on.
//     Port numbers 0-1023 are used for well-known services.
//     Port numbers 1024-65535 may be freely used.
//   pChoice = optionUDP or optionTCP
// Post:
//   Returns NetworkOk on success
//   Returns two part int code on theError.
//     The low 16 bits contains Status code as defined in net.h.
//     The high 16 bits contains "Windows Socket Error Code".
//=============================================================================
int Network::developServer(int portNo, int pChoice) 
{
    int status;

    // ----- Initialize network stuff -----
    status = StartTheNetwork(portNo, pChoice);
    if (status != NetworkOk)
        return status;

    localAdd.sin_addr.s_addr = htonl(INADDR_ANY);    // listen on all addresses

    // bind socket
    if (bind(sockt, (SOCKADDR *)&localAdd, sizeof(localAdd)) == SOCKET_ERROR)
    {
        status = WSAGetLastError();          // get detailed theError
        return ((status << 16) + BindFailed);
    }
    within = true;
    mState = tServer;

    return NetworkOk;
}

//=============================================================================
// Setup network for use as a Client
// Pre: 
//   *tServer = IP address of tServer to connect to as null terminated
//     string (e.g. "192.168.1.100") or null terminated domain name
//     (e.g. "www.programming2dgames.com").
//   portNo = Port number. Port numbers 0-1023 are used for well-known services.
//     Port numbers 1024-65535 may be freely used.
//   pChoice = optionUDP or optionTCP
// Post:
//   Returns NetworkOk on success
//   Returns two part int code on theError.
//     The low 16 bits contains Status code as defined in net.h.
//     The high 16 bits contains "Windows Socket Error Code".
//   *tServer = IP address connected to as null terminated string.
//=============================================================================
int Network::developClient(char *tServer, int portNo, int pChoice) 
{
    int status;
    char theLocalIP[IPSize];  // IP as string (e.g. "192.168.1.100");
    ADDRINFOA host;
    ADDRINFOA *result = NULL;

    // ----- Initialize network stuff -----
    status = StartTheNetwork(portNo, pChoice);
    if (status != NetworkOk)
        return status;

    // if tServer does not contain a dotted quad IP address nnn.nnn.nnn.nnn
    if ((remAdd.sin_addr.s_addr = inet_addr(tServer)) == INADDR_NONE)
    {
        // setup host structure for use in getaddrinfo() function
        ZeroMemory(&host, sizeof(host));
        host.ai_family = AF_INET;
        host.ai_socktype = SOCK_STREAM;
        host.ai_protocol = IPPROTO_TCP;

        // get address information
        status = getaddrinfo(tServer,NULL,&host,&result);
        if(status != 0)                 // if getaddrinfo failed
        {
            status = WSAGetLastError();
            return ((status << 16) + DomainNFound);
        }
        // get IP address of tServer
        remAdd.sin_addr = ((SOCKADDR_IN *) result->ai_addr)->sin_addr;
        strncpy_s(tServer, IPSize, inet_ntoa(remAdd.sin_addr), IPSize);
    }

    // set local IP address
    retLocalIP(theLocalIP);          // get local IP
    localAdd.sin_addr.s_addr = inet_addr(theLocalIP);   // local IP

    mState = tClient;
    return NetworkOk;
}

//=============================================================================
// Send theData to remote IP and portNo
// Pre:
//   *theData = Data to send.
//   size = Number of bytes to send.
//   *senderIP = Destination IP address as null terminated char array.
//   portNo = Destination portNo number.
// Post: 
//   Returns NetworkOk on success. Success does not indicate theData was sent.
//   Returns two part int code on theError.
//     The low 16 bits contains Status code as defined in net.h.
//     The high 16 bits contains "Windows Socket Error Code".
//   size = Number of bytes sent, 0 if no theData sent.
//=============================================================================
int Network::dataSent(const char *theData, int &size, const char *senderIP, const USHORT portNo) 
{
    int status;
    int sendSize = size;

    size = 0;       // assume 0 bytes sent, changed if send successful

	if (mState == tServer)
	{
        remAdd.sin_addr.s_addr = inet_addr(senderIP);
		remAdd.sin_port = portNo;
	}

    if(mState == tClient && choice == NotConnectedTCP) 
    {
        ret = connect(sockt,(SOCKADDR*)(&remAdd),sizeof(remAdd));
        if (ret == SOCKET_ERROR) {
            status = WSAGetLastError();
            if (status == WSAEISCONN)   // if connected
            {
                ret = 0;          // clear SOCKET_ERROR
                choice = ConnectedTCP;
            } 
            else 
            {
                if ( status == WSAEWOULDBLOCK || status == WSAEALREADY) 
                    return NetworkOk;  // no connection yet
                else 
                    return ((status << 16) + ErrorOnNetwork);
            }
        }
    }

    ret = sendto(sockt, theData, sendSize, 0, (SOCKADDR *)&remAdd, sizeof(remAdd));
    if (ret == SOCKET_ERROR) 
    {
        status = WSAGetLastError();
        return ((status << 16) + ErrorOnNetwork);
    }
    within = true;         // automatic binding by sendto if unbound
    size = ret;           // number of bytes sent, may be 0
    return NetworkOk;
}

//=============================================================================
// Read theData, return sender's IP and portNo
// Pre:
//   *theData = Buffer for received theData.
//   size = Number of bytes to receive.
//   *senderIP = NULL
// Post: 
//   Returns NetworkOk on success.
//   Returns two part int code on theError.
//     The low 16 bits contains Status code as defined in net.h.
//     The high 16 bits contains "Windows Socket Error Code".
//   size = Number of bytes received, may be 0.
//   *senderIP = IP address of sender as null terminated string.
//   portNo = portNo number of sender.
//=============================================================================
int Network::dataRead(char *theData, int &size, char *senderIP, USHORT &portNo) 
{
    int status;
    int readSize = size;

    size = 0;           // assume 0 bytes read, changed if read successful
    if(within == false)  // no receive from unbound socket
        return NetworkOk;

    if(mState == tServer && choice == NotConnectedTCP) 
    {
        ret = listen(sockt,1);
        if (ret == SOCKET_ERROR) 
        {
            status = WSAGetLastError();
            return ((status << 16) + ErrorOnNetwork);
        }
        SOCKET tempSock;
        tempSock = accept(sockt,NULL,NULL);
        if (tempSock == INVALID_SOCKET) 
        {
            status = WSAGetLastError();
            if ( status != WSAEWOULDBLOCK)  // don't report WOULDBLOCK theError
                return ((status << 16) + ErrorOnNetwork);
            return NetworkOk;      // no connection yet
        }
        closedSocket(sockt);      // don't need old socket
        sockt = tempSock;        // optionTCP client connected
        choice = ConnectedTCP;
    }

    if(mState == tClient && choice == NotConnectedTCP) 
        return NetworkOk;  // no connection yet

    if(sockt != NULL)
    {
        reAddSize = sizeof(remAdd);
        ret = recvfrom(sockt, theData, readSize, 0, (SOCKADDR *)&remAdd,
                       &reAddSize);
        if (ret == SOCKET_ERROR) {
            status = WSAGetLastError();
            if ( status != WSAEWOULDBLOCK)  // don't report WOULDBLOCK theError
                return ((status << 16) + ErrorOnNetwork);
            ret = 0;            // clear SOCKET_ERROR
        // if optionTCP connection did graceful close
        } else if(ret == 0 && choice == ConnectedTCP)
            // return Remote Disconnect theError
            return ((Disconnect << 16) + ErrorOnNetwork);
		if (ret)
		{
			//IP of sender
			strncpy_s(senderIP, IPSize, inet_ntoa(remAdd.sin_addr), IPSize);
			portNo = remAdd.sin_port;		// portNo number of sender
		}
        size = ret;           // number of bytes read, may be 0
    }
    return NetworkOk;
}

//=============================================================================
// Close socket and free resources.
// Post:
//   Socket is closed
//   Returns two part int code on theError.
//     The low 16 bits contains Status code as defined in net.h.
//     The high 16 bits contains "Windows Socket Error Code".
//=============================================================================
int Network::closedSocket() 
{
    int status;

    choice = NotConnected;
    within = false;
    netInitialized = false;

    // closesocket() implicitly causes a shutdown sequence to occur
    if (closesocket(sockt) == SOCKET_ERROR) 
    {
        status = WSAGetLastError();
        if ( status != WSAEWOULDBLOCK)  // don't report WOULDBLOCK theError
            return ((status << 16) + ErrorOnNetwork);
    }

    if (WSACleanup())
        return ErrorOnNetwork;
    return NetworkOk;
}

//=============================================================================
// Get the IP address of this computer as a string
// Post:
//   *theLocalIP = IP address of local computer as null terminated string on success.
//   Returns two part int code on theError.
//     The low 16 bits contains Status code as defined in net.h.
//     The high 16 bits contains "Windows Socket Error Code".
//=============================================================================
int Network::retLocalIP(char *theLocalIP) 
{
    char hostName[40];
    ADDRINFOA host;
    ADDRINFOA *result = NULL;
    int status;

    gethostname (hostName,40);

    // setup host structure for use in getaddrinfo() function
    ZeroMemory(&host, sizeof(host));
    host.ai_family = AF_INET;
    host.ai_socktype = SOCK_STREAM;
    host.ai_protocol = IPPROTO_TCP;

    // get address information
    status = getaddrinfo(hostName,NULL,&host,&result);
    if(status != 0)                 // if getaddrinfo failed
    {
        status = WSAGetLastError();         // get detailed theError
        return ( (status << 16) + ErrorOnNetwork);
    }

    // get IP address of tServer
    IN_ADDR in_addr = ((SOCKADDR_IN *) result->ai_addr)->sin_addr;
    strncpy_s(theLocalIP, IPSize, inet_ntoa(in_addr), IPSize);

    return NetworkOk;
}

//=============================================================================
// Returns detailed theError msg from two part theError code
//=============================================================================
std::string Network::retError(int theError)
{
    int t = theError >> 16;  // upper 16 bits is sockErr
    std::string errorStr;

    theError &= StatusMask;       // remove extended theError code
    if(theError > NumberOErrors-2)   // if unknown theError code
        theError = NumberOErrors-1;
    errorStr = errors[theError];
    for (int i=0; i< SOCK_CODES; i++)
    {
        if(errorCodes[i].sockErr == sockErr)
        {
            errorStr += errorCodes[i].msg;
            break;
        }
    }
    return errorStr;
}
