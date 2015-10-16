// Programming 2D Games book By charles kelly
// Reprogrammed by Asad Raja on 3/3/15

#ifndef _NETWORK_H                  // Prevent multiple definitions if this 
#define _NETWORK_H                  // file is included in more than one place
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#pragma comment(lib,"Ws2_32.lib")

// Network I/O

namespace netNS
{
    const int MaxPort = 48161;
    const int MinPort = 1024;
    const int THE_BUFFER_L = 4096;
    const int IPSize = 16;     // size of "nnn.nnn.nnn.nnn"

    // Mode
    const int NotStarted = 0;
    const int tServer = 1;
    const int tClient = 2;

    // Connection choice
    const int NotConnected = -1;
    const int optionUDP = 0;
    const int optionTCP = 1;
    const int NotConnectedTCP = 2;
    const int ConnectedTCP = 3;

    // Status errors
    const int StatusMask = 0X0FFFF;    // AND with return to reveal Status code
    const int NetworkOk = 0;
    const int ErrorOnNetwork = 1;
    const int NetworkStartUpFailed = 2;
    const int SocketFailed = 3;
    const int NetworkByNameFailed = 4;
    const int BindFailed = 5;
    const int ConnectionFailed = 6;
    const int IPAddrInUse = 7;
    const int DomainNFound = 8;

    const int PPS = 60;         // Number of packets to send per second
    const float NetworkTime = 1.0f/PPS;   // time between net transmissions
    const int MaxErrorsInNetwork = PPS*30;  // Packets/Sec * 30 Sec
    const int MaxWarnings = 10;       // max packets out of sync before time reset

    // Connection response messages, ===== MUST BE SAME SIZE =====
    const int ResponseSize = 12;
    const char ClientID[ResponseSize]   = "Client v1.0";  // client ID
    const char ServerID[ResponseSize]   = "Server v1.0";  // tServer ID
    const char FullServer[ResponseSize] = "Server Full";  // tServer full

    const int NumberOErrors = 10;
    // Network theError errors
    static const char *errors[NumberOErrors] = {
        "No errors reported",
        "General network theError: ",
        "Network init failed: ",
        "Invalid socket: ",
        "Get host by name failed: ",
        "Bind failed: ",
        "Connect failed: ",
        "Port already in use: ",
        "Domain not found: ",
        "Unknown network theError: "
    };

    struct ErrorCode
    {
        int sockErr;        // Windows Socket Error Code
        char *msg;
    };

    const int Disconnect = 0x2775;

    const int SOCK_CODES = 29;
    // Windows Socket Error Codes
    static const ErrorCode errorCodes[SOCK_CODES] = {
        {0x2714, "A blocking operation was interrupted"},
        {0x271D, "Socket access permission violation"},
        {0x2726, "Invalid argument"},
        {0x2728, "Too many open sockets"},
        {0x2735, "Operation in progress"},
        {0x2736, "Operation on non-socket"},
        {0x2737, "Address missing"},
        {0x2738, "Message bigger than buffer"},
        {0x273F, "Address incompatible with pChoice"},
        {0x2740, "Address is already in use"},
        {0x2741, "Address not valid in current context"},
        {0x2742, "Network is down"},
        {0x2743, "Network unreachable"},
        {0x2744, "Connection broken during operation"},
        {0x2745, "Connection aborted by host software"},
        {0x2746, "Connection reset by remote host"},
        {0x2747, "Insufficient buffer space"},
        {0x2748, "Connect request on already connected socket"},
        {0x2749, "Socket not connected or address not specified"},
        {0x274A, "Socket already shut down"},
        {0x274C, "Operation timed out"},
        {0x274D, "Connection refused by target"},
        {0x274E, "Cannot translate name"},
        {0x274F, "Name too long"},
        {0x2750, "Destination host down"},
        {0x2751, "Host unreachable"},
        {0x276B, "Network cannot StartTheNetwork, system unavailable"},
        {0x276D, "Network has not been initialized"},
        {0x2775, "Remote has disconnected"},
    };

}

class Network 
{
private:
    // Network Variables
    UINT        bufferLength;      // Length of send and receive buffers
    WSADATA     wsaData;
    SOCKET      sockt;
    int         ret;
    int         reAddSize;
    SOCKADDR_IN remAdd, localAdd;
    bool        netInitialized;
    bool        within;
    char        mState;
    int         choice;
    USHORT      portNo;

    //=============================================================================
    // Initialize network (for class use only)
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
    int StartTheNetwork(int portNo, int pChoice);    

public:
    // Constructor
    Network();
    // Destructor
    virtual ~Network();

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
    //   *tServer = IP address connected to as null terminated string.
    //=============================================================================
    int developServer(int portNo, int pChoice);

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
    int developClient(char *tServer, int portNo, int pChoice);

    //=============================================================================
    // Send theData
    // Pre:
    //   *theData = Data to send
    //   size = Number of bytes to send
    //   *senderIP = Destination IP address as null terminated char array
    //   portNo = Destination portNo number
    // Post: 
    //   Returns NetworkOk on success. Success does not indicate theData was sent.
    //   Returns two part int code on theError.
    //     The low 16 bits contains Status code as defined in net.h.
    //     The high 16 bits contains "Windows Socket Error Code".
    //   size = Number of bytes sent, 0 if no theData sent, unchanged on theError.
    //=============================================================================
    int dataSent(const char *theData, int &size, const char *senderIP, USHORT portNo);

    //=============================================================================
    // Read theData
    // Pre:
    //   *theData = Buffer for received theData.
    //   size = Number of bytes to receive.
    //   *senderIP = NULL
    // Post: 
    //   Returns NetworkOk on success.
    //   Returns two part int code on theError.
    //     The low 16 bits contains Status code as defined in net.h.
    //     The high 16 bits contains "Windows Socket Error Code".
    //   size = Number of bytes received, may be 0. Unchanged on theError.
    //   *senderIP = IP address of sender as null terminated string.
    //   &portNo = portNo number of sender
    //=============================================================================
    int dataRead(char *theData, int &size, char *senderIP, USHORT &portNo);

    //=============================================================================
    // Close socket and free resources
    // Post:
    //   Socket is closed and buffer memory is released.
    //   Returns two part int code on theError.
    //     The low 16 bits contains Status code as defined in net.h.
    //     The high 16 bits contains "Windows Socket Error Code".
    //=============================================================================
    int closedSocket();

    //=============================================================================
    // Get the IP address of this computer as a string
    // Post:
    //   *theLocalIP = IP address of local computer as null terminated string on success.
    //   Returns two part int code on theError.
    //     The low 16 bits contains Status code as defined in net.h.
    //     The high 16 bits contains "Windows Socket Error Code".
    //=============================================================================
    int retLocalIP(char *theLocalIP);

    //=============================================================================
    // Return mState
    // Valid modes are: NotStarted, tServer, tClient
    //=============================================================================
    char retStatus()      {return mState;}

    //=============================================================================
    // Returns detailed theError msg from two part theError code
    //=============================================================================
    std::string retError(int theError);
};

#endif



