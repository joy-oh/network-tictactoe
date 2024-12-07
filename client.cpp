/**
 * -----------------------------client.cpp--------------------------------------
 * Joy Hyunjung Oh, CSS432, assignment 1
 * Created: 09/30/2024
 * Last edited: 10/04/2024
 * -----------------------------------------------------------------------------
 * Summary: establish a connection to a server, sends data or requests,
 *          and close the connection
 * -----------------------------------------------------------------------------
 */
#include<iostream>
#include <sys/types.h>    // socket, bind
#include <sys/socket.h>   // socket, bind, listen, inet_ntoa
#include <netinet/in.h>   // htonl, htons, inet_ntoa
#include <arpa/inet.h>    // inet_ntoa
#include <netdb.h>     // gethostbyname
#include <unistd.h>    // read, write, close
#include <strings.h>     // bzero
#include <netinet/tcp.h>  // SO_REUSEADDR
#include <sys/uio.h>      // writev
#include <string.h>
#include <stdio.h>
#include <stdexcept>
#include <sys/time.h>
#include <sstream>
/**
 * ---------------------------------------- printBoard ---------------------------------
 */
void printBoard(char* databuf) {
    for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                std::cout << databuf[i * 3 + j] << "|";
            }
            std::cout << databuf[i * 3 + 2] << '\n';
            std::cout << "------" << '\n';
        }
        for (int j = 0; j < 2; j++) {
                std::cout << databuf[2 * 3 + j] << "|";
        }
        std::cout << databuf[2 * 3 + 2] << '\n';
}
void getPosition(unsigned int* row, unsigned int* column) {
    std::string r, c;
    bool correct = false;
    while(!correct){
        std::cout << "Choose your spot between 0-2 for row and column.\nEnter in a the order of row column";
        std::cin >> r >> c;
        try{
            *row = stoi(r);
            *column = stoi(c);
        }
        catch(std::invalid_argument const &arg) {
            std::cout << "invalid argument" << arg.what() << '\n';
            continue;
        }
        if (*row > 2 || *column > 2) {
            std::cout << "invalid choice\n";
            continue;
        }
        else {
            break;
        }
    }
}

bool positionValid(unsigned int row, unsigned int col, char* board) {
    int pos = row * 3 + col;
    if (board[pos] == 'X' || board[pos] == 'Y'){
        return false;
    }
    return true;
}

int main (int argc, char* argv[]) {

	char* serverIp; //a server ip name
    char* port; //a server IP port 2462
    struct addrinfo hints;
    struct addrinfo* result, *rp;
    int clientSd = -1;

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << "missing arguments" << std::endl;
        return -1;
    }
    serverIp = argv[1];
    port = argv[2];

    // establish a connection
	memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; //allow both IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    int r = getaddrinfo(serverIp, port, &hints, &result);

    if (r != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(r));
        exit(EXIT_FAILURE);
    }
    // getaddrinfo() returns a list of address structures.
    // try each address until successfully connect. if socket or
    // connect fails, close the socket and try the next address.

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        clientSd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (clientSd == -1) {
            continue;
        }
        // socket created
        if (connect(clientSd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;// connected
        }
        close(clientSd);
    }

    if (rp == NULL) {
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(result);

    // allocate space
    char databuf[10] = {0};
    std::cout << databuf[0] << std::endl;
    unsigned int posXY[2];
    // receive the board when connected
    bool done = false;
    std::cout << "connected\n";

    while(!done) {
        std::cout << "entered\n";
        int readBoard = 0;
        while (readBoard < 9) {
            readBoard += read(clientSd, databuf + readBoard, 10);
            if (readBoard > 1 && (databuf[1] == 'W'  || databuf[1] == 'D')) {
                break;
            }
            std::cout << "read in while: " << readBoard << std::endl;
        }
        databuf[readBoard] = '\0';
        std::string line = databuf;
        std::cout << line << std::endl;
        std::cout << "read: " << readBoard << std::endl;
        // print out to the player
        // std::cout << databuf << std::endl;
        // std::cout << databuf[0] << std::endl;
        if (databuf[1] == 'W'  || databuf[1] == 'D') {
            if (databuf[1] == 'W'){
                std::cout << "Player 2 Won\n";
            }
            else {
                std::cout << "Drew\n";
            }
            std::string ans;
            std::cout << "Play again?\t(Y)es\t(N)o\n";
            std::cin >> ans;
            int ansNum;
            while(true) {
                if (ans != "Y" && ans != "N") {
                    std::cout << "invalid option\n";
                }
            }
            if (ans == "Y") {
                posXY[0] = 'Y';
            }
            else {
                posXY[0] = 'N';
            }
            posXY[1] = '\0';
            int written = 0;
            while (written < 2) {
                written += write(clientSd, posXY + written, sizeof(posXY));
                std::cout << "written in while: " << written << std::endl;
            }
            break;
        }
        printBoard(databuf);

        //get position from the players
        unsigned int row, col;
        getPosition(&row, &col);
        if( positionValid(row, col, databuf) ) {
            // send his position
            posXY[0] = row;
            posXY[1] = col;
            std::cout << "row: " << row << std::endl;
            std::cout << "col: " << col << std::endl;
            int written = 0;
            while (written < 2) {
                written += write(clientSd, posXY + written, sizeof(posXY));
                std::cout << "written in while: " << written << std::endl;
            }
            std::cout << "written: " << written << std::endl;
        }
    }

    // close socket
    close(clientSd);
    return 0;
}
