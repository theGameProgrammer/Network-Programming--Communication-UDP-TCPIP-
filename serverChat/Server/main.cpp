

#include <iostream>
#include <conio.h>
#include "network.h"
using namespace std;

const int BUFSIZE = 256;
const USHORT MaxPort = 48161;
int main()
{
    USHORT portNo = MaxPort;
    USHORT remotePort = MaxPort;  // Remote portNo number
    int newPort = MaxPort;
    int pChoice;           // optionTCP or optionUDP
    char senderIP[16];      // Remote IP address as dotted quad; nnn.nnn.nnn.nnn
    char theLocalIP[16];       // Local IP address as dotted quad; nnn.nnn.nnn.nnn
    char netbuf[BUFSIZE];   // network receive
    char keybuf[BUFSIZE];   // keyboard input
    int theError     = netNS::NetworkOk; 
    int lastError = netNS::NetworkOk; 
    int sizeXmit=0;         // transmit size
    int sizeRecv=0;         // receive size
    int size=0;
    Network net;                // Network communication object
    netbuf[0] = '\0';

    // ----- create tServer -----
    do
    {
        // display pChoice menu
        cout << "----- Chat Server -----\n"
             << "\nSelect Protocol\n"
             << "    0 = optionUDP\n"
             << "    1 = optionTCP\n\n"
             << "  Choice: ";
        cin >> pChoice;    // get character
    }while(pChoice != 0 && pChoice != 1);
    cin.ignore();

    // get portNo number
    //     Port numbers 0-1023 are used for well-known services.
    //     Port numbers 1024-65535 may be freely used.
    do
    {
        cout << "Enter portNo number (Use 0 for default 48161): ";
        cin >> newPort;
    }while(newPort < 0 || newPort > 65535);
    if(newPort != 0)
        portNo = newPort;
    cin.ignore();

    // create tServer
    theError = net.developServer(portNo, pChoice);
    if(theError != netNS::NetworkOk)          // if theError
    {
        cout << net.retError(theError) << endl;
        system("pause");
        return 1;
    }

    // display serverIP
    net.retLocalIP(theLocalIP);
    cout << "Server IP is: " << theLocalIP << endl;
    cout << "Server portNo is: " << portNo << endl;

    // display incoming text and send back response
    while(true)     // ***** INFINITE LOOP *****
    {
        // check for receive
        sizeRecv = BUFSIZE;     // max bytes to receive
        theError = net.dataRead(netbuf,sizeRecv,senderIP, remotePort);
        if(theError != netNS::NetworkOk)
        {
            cout << net.retError(theError) << endl;
            theError = net.closedSocket();     // close connection
            // re-create tServer
            theError = net.developServer(portNo, pChoice);
            if(theError != netNS::NetworkOk)          // if theError
            {
                cout << net.retError(theError) << endl;
                return 1;
            }
        }
        if(sizeRecv > 0)        // if characters received
        {
            // display incomming msg
            cout << netbuf << endl;
        }

        // if keyboard input pending
        if (_kbhit())           
        {
            cin.getline(keybuf, BUFSIZE);    // get input
            sizeXmit += (int)cin.gcount() + 1;   // get size of input + null
            if(cin.fail())      // if failbit
            {
                cin.clear();    // clear theError
                cin.ignore(INT_MAX, '\n');   // flush input
            }
        }

        // if theData ready to send
        if(sizeXmit > 0)
        {
            size = sizeXmit;
            theError = net.dataSent(keybuf,size,senderIP,remotePort); // send theData
            if(theError != netNS::NetworkOk)
                cout << net.retError(theError) << endl;
            sizeXmit -= size;   // - number of characters sent
            if(sizeXmit < 0)
                sizeXmit = 0;
        }

        // insert 10 millisecond delay to reduce CPU workload
        Sleep(10);
    }

    return 0;
}

