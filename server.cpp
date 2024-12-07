/**
 * ------------------------------------server.cpp------------------------------------
 * Joy hyunjung Oh, CSS432, final project
 * Created: 11/30/2024
 * Last edited: 12/0/2024
 * ----------------------------------------------------------------------------------
 * Summary: Accept connections from clients and create a thread to service the request
 *          and wait for another connection on the main thread.
 *          servicing consists of 1. reading the data sent by the client for repetition
 *                                2. sending the number of reads performed
 * -----------------------------------------------------------------------------------
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
#include <sys/time.h>
#include <stdio.h>
#include "Board.cpp"
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

bool positionValid(unsigned int row, unsigned int col, char* board) {
    int pos = row * 3 + col;
    if (board[pos] == 'X' || board[pos] == 'Y'){
        return false;
    }
    return true;
}
const int BUFFSIZE = 1500;
const int NUM_CON = 5;
struct timeval start, stop;
/**
 * structure for thread data
 */
struct thread_data {
    int repetition;
    int sd;
};

// void* worker(void* arg);

int main (int argc, char* argv[]) {

    int port;
    int num_open_fds, nfds;

    // Argument validation
    if (argc != 2){
        std::cerr << "Usage: " << argv[0] << "missing arguments" << std::endl;
        return -1;
    }

    port = std::stoi(argv[1]);
    // nfds = std::stoi(argv[2]);
    // set up the data structure
    sockaddr_in acceptSockAddr;
    // zero initialize it
    bzero((char*)&acceptSockAddr, sizeof(acceptSockAddr));
    acceptSockAddr.sin_family = AF_INET; //IPv4
    acceptSockAddr.sin_addr.s_addr = htonl(INADDR_ANY); // connection address that is made from
    acceptSockAddr.sin_port = htons(port);

    // Create the socket
    int serverSd = socket(AF_INET, SOCK_STREAM, 0);
    // error handling
    if (serverSd == -1) {
        std::cerr << "socket failed: " << errno << std::endl;
        return -1;
    }

    // Enable socket reuse without waiting for the OS to recycle it
    const int on = 1;
    setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(int));

    // Bind the socket
    int bindStat = bind(serverSd, (sockaddr*)&acceptSockAddr, sizeof(acceptSockAddr));
    // error handling
    if(bindStat != 0){
        std::cerr << "Bind Failed: " << errno << std::endl;
        return -1;
    }

    // Listen on the socket
    int res = listen(serverSd, NUM_CON);
    if (res != 0) {
        std::cerr << "listen failed: " << errno << std::endl;
        return -1;
    }

    // new socket to receive requests
    sockaddr_in newSockAddr;
    socklen_t newSockAddSize = sizeof(newSockAddr);
    int player1 = 0;
    while (1) {
        // Accept the connection as a new socket from the other player
        int player2 = accept(serverSd, (sockaddr*)&newSockAddr, &newSockAddSize);
        if (player2 == -1) {
            std::cout << "new socket requires" << std::endl;
            continue;
        }
        // create a game
        Board b = Board(player1, player2);
        bool done = false;
        // send the board
        unsigned int posXY[2];
        char* board = b.getBoard();
        Board::Status stat;
        while (!done){
            // send the current board to the player2

            int written = 0;

            if (written < 10) {
                written += write (player2, board + written, sizeof(char[10]) );
            }
            // std::cout << "written" << std::endl;
            int datarecv = 0;
            while (datarecv < 2) {
                datarecv += recv(player2, posXY + datarecv, sizeof(posXY), 0);
                // std::cout << "recv in while: " << datarecv << std::endl;
                // std::cout << "posXY[0]: " << posXY[0] << std::endl;
                // std::cout << "posXY[1]: " << posXY[1] << std::endl;
            }
            if( positionValid(posXY[0], posXY[1], board) ) {
                // std::cout << "valid" << std::endl;
                stat = b.put(posXY[0], posXY[1], player2);
                printBoard(board);
                written = 0;
                if (stat == Board::Status::win) {
                    std::cout << "Player2 Won\n";
                    board[0] = '2';
                    board[1] = 'W';
                    while (written < 2) {
                        written += write (player2, board + written, 2);
                    }
                    break;
                }
                else if(stat == Board::Status::draw) {
                    std::cout << "Drew\n";
                    board[0] = '3';
                    board[1] = 'D';
                    while (written < 2) {
                        written += write (player2, board + written, 2);
                    }
                    break;
                }
            }
            else {
                // send the current board to get a position
                continue;
            }
            // find the empty spot and place a 'O'
            for (int i = 0; i < 9; i++) {
                if (board[i] != 'X' && board[i] != 'O') {
                    int r = i / 3;
                    int c = i % 3;
                    //for checking. delete later
                    b.put(r, c, player1);
                    printBoard(board);
                    break;
                }
            }
        }


    }
    return 0;
}

