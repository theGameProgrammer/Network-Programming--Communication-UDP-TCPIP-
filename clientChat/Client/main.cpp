// Programming 2D Games, Copyright (c) 2011 by: Charles Kelly 
// Chat Client v2.0

#include <iostream>
#include <conio.h>
#include "network.h"
using namespace std;

int main()
{
    const int BUFSIZE = 256;
    int port = 48161;
    int newPort = 0;
    USHORT remotePort = port;
    int protocol;           // TCP or UDP
    char remoteIP[BUFSIZE]; // Remote IP address or domain name
    char localIP[16];       // Local IP address as dotted quad; nnn.nnn.nnn.nnn
    char netbuf[BUFSIZE];   // network receive
    char keybuf[BUFSIZE];   // keyboard input
    int error     = netNS::NET_OK; 
    int lastError = netNS::NET_OK; 
    int sizeXmit=0;         // transmit size
    int sizeRecv=0;         // receive size
    int size=0;
    Network net;                // Network communication object

    // ----- create client -----
    do
    {
        // display protocol menu
        cout << "----- Chat Client -----\n"
             << "\nSelect Protocol\n"
             << "    0 = UDP\n"
             << "    1 = TCP\n\n"
             << "  Choice: ";
        cin >> protocol;    // get character
    }while(protocol != 0 && protocol != 1);
    cin.ignore();

    // prompt for IP address or name of server
    cout << "Enter IP address or name of server: ";
    cin.getline(remoteIP, BUFSIZE);     // read IP address or name

    // get port number
    //     Port numbers 0-1023 are used for well-known services.
    //     Port numbers 1024-65535 may be freely used.
    do
    {
        cout << "Enter port number (Use 0 for default 48161): ";
        cin >> newPort;
    }while(newPort < 0 || newPort > 65535);
    if(newPort != 0)
        port = newPort;
    cin.ignore();

    // create client
    error = net.createClient(remoteIP, port, protocol);
    if(error != netNS::NET_OK)          // if error
    {
        cout << net.getError(error) << endl;
        system("pause");
        return 1;
    }

    // display serverIP
    cout << "Server IP is: " << remoteIP << endl;

    // display clientIP
    net.getLocalIP(localIP);
    cout << "Client IP is: " << localIP << endl;

    // send messages to server and display what is sent back
    while(true)     // ***** INFINITE LOOP *****
    {
        // if keyboard input pending
        if (_kbhit())           
        {
            cin.getline(keybuf,BUFSIZE);    // get input
            sizeXmit += (int)cin.gcount() + 1;   // get size of input + null
            if(cin.fail())      // if failbit
            {
                cin.clear();    // clear error
                cin.ignore(INT_MAX,'\n');   // flush input
            }
        }

        // if data ready to send
        if(sizeXmit > 0)
        {
            size = sizeXmit;
            error = net.sendData(keybuf,size,remoteIP,port); // send data
            if(error != netNS::NET_OK)
                cout << net.getError(error) << endl;
            sizeXmit -= size;   // - number of characters sent
            if(sizeXmit < 0)
                sizeXmit = 0;
        }

        // check for receive
        sizeRecv = BUFSIZE;     // max chars to read
        error = net.readData(netbuf,sizeRecv,remoteIP,remotePort);
        if(error != netNS::NET_OK)  // if error
        {
            if(error != lastError)  // if new error
            {
                lastError = error;
                cout << net.getError(error) << endl;
            }
        }
        else
            if(sizeRecv > 0)        // if characters received
                // display incomming message
                cout << netbuf << endl;
        // insert short delay to reduce CPU workload
        Sleep(10);
    }

    return 0;
}

