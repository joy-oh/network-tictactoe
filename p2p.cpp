/**
 * -------------------------------------- p2p.cpp --------------------------------------------
 * Joy hyunjung Oh, CSS432, final project
 * Created: 11/29/2024
 * Last edited: 12/06/2024
 * -------------------------------------------------------------------------------------------
 * Summary: p2p tic tac toe game
 *          listen to broadcasting message, choose either create game, join a game, or
 *          refresh the list.
 *          start a game once one person created a game and another person join the game
 * -------------------------------------------------------------------------------------------`
 */
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>// read, write, close
#include <strings.h> // bzero
#include <netinet/tcp.h>  // SO_REUSEADDR
#include <string>
#include <netdb.h>
#include <sys/types.h>// socket, bind
#include <sys/socket.h>// socket, bind, listen, inet_ntoa
#include <arpa/inet.h>// inet_ntoa
#include <netinet/in.h> // htonl, htons, inet_ntoa
#include <stdexcept>
#include <cassert>
#include <cstring>
#include <set>
#include <vector>
#include <thread>
#include <fcntl.h>
#include <atomic>
#include <chrono>
#include <cctype>
#include "Board.cpp"

#define BROADCAST_PORT 8800
#define BROADCAST_MESSAGE "P2P_DISCOVERY"
#define BUFFER_SIZE 1024
#define TCP_PORT 9999
std::atomic<bool> running(false);
enum Choice {create, join, refresh};

/**
 * ------------------------------- makeUpper ------------------------------------
 *  make string upper case
 */
void makeUpper(std::string& s) {
    for (std::string::size_type i = 0; i < s.length(); i++) {
        s[i] = toupper(s[i]);
    }
}

/**
 * --------------------------------- printBoard ----------------------------------
 * pring board on the screen
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
/**
 * --------------------------------- positionValid -----------------------------
 * position validation
 */
bool positionValid(unsigned int row, unsigned int col, char* board) {
    int pos = row * 3 + col;
    return (board[pos] != 'X' && board[pos] != 'O');
}

/**
 * -------------------------- setUpAddr --------------------------------------
 * sockaddr_in set up
 */
void setUpAddr(struct sockaddr_in* addr, int port) {
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(port);
}

/**
 * -------------------------------- createGameList --------------------------------------
 * get broadcast addresses and create a list to choose from
 */
void createAGameList(std::vector<std::string>& games, int sock, sockaddr_in& addr) {
    char buffer[2048];
    socklen_t addrLen = sizeof(addr);
    int i = 0;
    int gameC = 0;
    std::set<std::string> tmpGames;
    while (i < 5) {
        while (int received = recvfrom(sock, buffer, BUFFER_SIZE, MSG_DONTWAIT,
                                         (struct sockaddr *)&addr, &addrLen) > 0) {
            if (gameC < 10){
                std::string game = inet_ntoa(addr.sin_addr);
                tmpGames.insert(game);
            }
            else {
                break;
            }
        }
        // wait for receiving data
        sleep(1);
        i++;
    }
    for (auto iter = tmpGames.begin(); iter != tmpGames.end(); iter++) {
        games.push_back(*iter);
    }
}
/**
 * ------------------------------ printGameList -----------------------------------------
 * print game list for the user to choose
 */
void printGameList(std::vector<std::string>& games) {
    std::cout << "Available Games\n";
    for (int i = 0; i < games.size(); ++i) {
        std::cout << i + 1 << ". " << games.at(i) << '\n';
    }
    std::cout << '\n';
}

/**
 * ---------------------------------- displayChoice -----------------------------------------
 * main menu for a user
 */
Choice displayChoice() {
    std::string ans;
    while(true) {
        // give options to choose from
        std::cout << "Choose your option.\n1.(C)reate\n2.(J)oin\n3.(R)efresh\n\n";
        std::cin >> ans;
        makeUpper(ans);
        if (ans != "C" && ans != "J" && ans != "R") {
            std::cout << "Not a valid option\n";
            continue;
        }
        else {
            if (ans == "C") {
                return create;
            }
            else if (ans == "J") {
                return join;
            }
            else {
                assert(ans == "R");
                return refresh;
            }
        }
    }
}
/**
 * -------------------------- sendPosition ----------------------------------
 * send position chosen by a player
 */
void sendPosition(int clientSd, unsigned int* posXY) {
    int written = 0;
    while (written < 2) {
        written += write(clientSd, posXY + written, sizeof(posXY));
    }
}

/**
 * --------------------------- playAgain --------------------------------------
 * get answer for play again
 */
std::string playAgain(unsigned int* posXY) {
    while (true) {
        std::cout << "Play again?\t(Y)es\t(N)o\n";
        std::string ans;
        std::cin >> ans;
        makeUpper(ans);
        if (ans != "Y" && ans != "N") {
            std::cout << "invalid option\n";
            continue;
        }
        if (ans == "Y") {
            posXY[0] = 'Y';
        }
        else {
            posXY[0] = 'N';
        }
        posXY[1] = '\0';
        return ans;
    }
}
/**
 * ----------------------------------- getPosition -------------------------------------------
 * get a position from a player
 */
void getPosition(unsigned int& row, unsigned int& column) {
    std::string r, c;
    for(;;){
        std::cout << "Choose your spot between 0-2 for row and column."
                    << "\nEnter in the order of row column\n";
        std::cin >> r >> c;
        try{
            row = stoi(r);
            column = stoi(c);
        }
        catch(std::invalid_argument const &arg) {
            std::cout << "invalid argument: " << arg.what() << '\n';
            continue;
        }
        if (row > 2 || column > 2) {
            std::cout << "invalid choice\n";
            continue;
        }
        else {
            break;
        }
    }
}

/**
 * ----------------------------- sendBoard -----------------------------------
 * send board to the other player
 */
void sendBoard(int player, char* board) {
    int written = 0;
    while (written < 10) {
        written += write (player, board + written, sizeof(char[10]) );
    }
}

/**
 * --------------------------------- recvPosition ----------------------------------------
 * receive chose position from the other player
 */
void recvPosition(int player, unsigned int* posXY){
    int datarecv = 0;
    while (datarecv < 2) {
        datarecv += recv(player, posXY + datarecv, sizeof(posXY), 0);
    }
}

/**
 * --------------------------------- sendMessage ----------------------------------------
 * print the winning or draw stat to the current player and
 * send the status message to the other player along with the final board
 */
void sendMessage(int player, int playerSd, char* board, Board::Status stat) {
    int written = 0;
    // save values to send the final board
    char board0 = board[0];
    char board1 = board[1];
    if (stat == Board::Status::win) {
        // this prints only for player 1
        printf("player %d WON!\n", player);
        if (player == 1) {
            board[0] = '1';
        }
        else {
            assert(player == 2);
            board[0] = '2';
        }
        board[1] = 'W';
        // send the message to player 2
        sendBoard(playerSd, board);
    }
    else if(stat == Board::Status::draw) {
        std::cout << "DRAW!\n";
        board[0] = '3';
        board[1] = 'D';
        // send the message to player 2
        sendBoard(playerSd, board);

    }
    // put orginal content back to send the final board
    board[0] = board0;
    board[1] = board1;
    sendBoard(playerSd, board);
}

/**
 * ---------------------------------- createGame ----------------------------------------
 * game creation
 */
Board createGame(int player1, int player2) {
    Board b = Board(player1, player2);
    // send the board
    return b;
}
/**
 * ---------------------------------- checkMSG ------------------------------------------
 * check message received from the other user about playing again
 * and print out to the current player
 *
 */
bool checkMSG(int clientSd, unsigned int* posXY) {
    if (posXY[0] == 'Y' && posXY[1] == '\0') {
        //new game
        std::cout << "Player 2 wants a new game" << std::endl;
        std::string ans = playAgain(posXY);
        sendPosition(clientSd, posXY);
        if (ans == "Y"){
            return true;
        }
        else {

            return false;
        }
    }
    else {
        assert(posXY[0] == 'N' && posXY[1] == '\0');
        std::cout << "Player 2 doesn't want a new game" << std::endl;
        return false;
    }
}
/**
 * ---------------------------------- playGameHost ----------------------------------------
 * game host function. Keep the board and update it
 * @param player1 game host
 * @param player2 game client
 * @return true
 */
bool playGameHost(Board b, int player1, int player2) {
    unsigned int posXY[2];
    bool done = false;
    Board::Status stat;
    bool positionVal = false;

    while (!done){
        // send the current board to the player2
        sendBoard(player2, b.getBoard());
        recvPosition(player2, posXY);
        // put position and check winning status
        stat = b.put(posXY[0], posXY[1], player2);
        // place the position from player2
        std::cout << "========================\n" <<
                      "Current Board Received\n" <<
                     "========================\n";
        printBoard(b.getBoard());
        // status for win or draw for player2
        if (stat == Board::Status::win || stat == Board::Status::draw) {

            // send the messages for winning stat and sent the final board
            sendMessage(2, player2, b.getBoard(), stat);
            // get answer for playing again.
            recvPosition(player2, posXY);
            // check message
            return checkMSG(player2, posXY);
        }

        // player2 hasn't won, player1 choose a spot
        unsigned int row, col;
        positionVal = false;
        while (!positionVal) {
            getPosition(row, col);
            positionVal = positionValid (row, col, b.getBoard());
            if (!positionVal) {
                std::cout << "try different spot\n" <<std::endl;
            }
        }

        stat = b.put(row, col, player1);
        std::cout << "=======================\n" <<
                      "Current Board Sending\n" <<
                     "=======================\n";
        printBoard(b.getBoard());
        if (stat == Board::Status::win || stat == Board::Status::draw) {
            // send the messages to player 2
            sendMessage(1, player2, b.getBoard(), stat);
            // play again or not
            recvPosition(player2, posXY);
            // contains message
            return checkMSG(player2, posXY);
        }
    }
    return false;
}
/**
 * ------------------------- convertToInt ------------------------------
 * convert string to integer
 */
int convertToInt(std::string str) {
    int res;
    while(true) {
        try {
            return res = stoi(str);
        }
        catch (std::invalid_argument &arg) {
            std::cout << "Invalid argument: " << arg.what();
        }
    }
}

/**
 * --------------------------------- joinGame ---------------------------------
 * join game chosen from the game list
 */
bool joinGame (std::string& ip, sockaddr_in &senderAddr, int &connectSock) {
    // connect to the choice
    //memset(&senderAddr, 0, sizeof(senderAddr));
    socklen_t senderAddrLen = sizeof(senderAddr);
    connectSock = socket(AF_INET, SOCK_STREAM, 0);
    if (connectSock < 0) {
        perror("Socket creation failed");
        return false;
    }
    senderAddr.sin_port = htons(TCP_PORT); // Replace with the port the sender is listening on
    if (inet_pton(AF_INET, ip.c_str(), &senderAddr.sin_addr) <= 0) {
        perror("Invalid IP address");
        return false;
    }
    if (connect(connectSock, (struct sockaddr*)&senderAddr, sizeof(senderAddr)) < 0) {
        perror("Connection to sender failed");
        close(connectSock);
        return false;
    }
    std::cout << "connected to port 9999\n";
    return true;
}

/**
 * ------------------------------ getBoard -----------------------------------
 * receive the current board from the other player
 */
void recvBoard(int clientSd, char* databuf) {
    int readBoard = 0;
    while (readBoard < 9) {
        readBoard += read(clientSd, databuf + readBoard, 10);
        // game concluded
        if (readBoard > 1 && (databuf[1] == 'W'  || databuf[1] == 'D')) {
            break;
        }
    }
    databuf[readBoard] = '\0';
}


/**
 * --------------------------------- playGameClient -----------------------------------
 * play game logic who joined the game
 */
void playGameClient(int& clientSd) {
    std::cout << "entered\n";
    char databuf[10] = {0};
    unsigned int posXY[2];
    bool done = false;
    while(!done) {
        // getBoard from the game host
        recvBoard(clientSd, databuf);
        // game result msg
        if (databuf[1] == 'W' || databuf[1] == 'D') {
            if (databuf[1] == 'W'){
                std::string player{databuf[0]};
                std::cout << "Player "<< player << " Won\n";
            }
            else {
                std::cout << "Draw\n";
            }
            // get the final board
            recvBoard(clientSd, databuf);
            std::cout << "======================\n" <<
                          "Final Board Received\n" <<
                         "======================\n";
            printBoard(databuf);
            std::string ans = playAgain(posXY);
            // send play again message
            sendPosition(clientSd, posXY);
            if (ans == "N") { // game finished
                break;
            }
            else {
                // recv answer from player 1
                recvPosition(clientSd, posXY);
                if (posXY[0] == 'Y') {
                    continue;
                }
                else {
                    std::cout << "Player 1 doesn't want a new game, bye bye..." << std::endl;
                    break;
                }
            }
        }
        std::cout << "========================\n" <<
                      "Current Board Received\n" <<
                     "========================\n";
        printBoard(databuf);
        //get position from the player2
        unsigned int row, col;
        bool valid = false;
        while (!valid) {
            getPosition(row, col);
            valid = positionValid(row, col, databuf);
            if (!valid) {
                std::cout << "try different spot" << std::endl;
            }
        }
        // send client position
        posXY[0] = row;
        posXY[1] = col;
        databuf[row * 3 + col] = 'O';
        // print board after placing a mark
        std::cout << "=======================\n" <<
                      "Current Board Sending\n" <<
                     "=======================\n";
        printBoard(databuf);
        sendPosition(clientSd, posXY);
    }
}

/**
 * --------------------------------- broadcasting -----------------------------------
 * broadcasting messages
 */
void broadcasting() {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        perror("UDP socket creation failed");
        return;
    }

    int broadcastPermission = 1;
    if (setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &broadcastPermission, sizeof(broadcastPermission)) == -1) {
        perror("Setting broadcast permission failed");
        close(udpSocket);
        return;
    }
    sockaddr_in broadcastAddr;
    setUpAddr(&broadcastAddr, BROADCAST_PORT);
    broadcastAddr.sin_addr.s_addr = inet_addr("255.255.255.255");


    broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

    const char* broadcastMessage = "Hello! Connect to port 9999";

    while (!running) {
        if (sendto(udpSocket, broadcastMessage, strlen(broadcastMessage), 0,
                   (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) == -1) {
            perror("Broadcast failed");
        } else {
            std::cout << "Broadcasting message on port " << BROADCAST_PORT << "\n";
        }
        sleep(3);  // Broadcast every 2 seconds
    }
    std::cout << "Broadcasting is stopping...\n";

    close(udpSocket);
}

/**
 * ----------------------------------- makeTCPConnection -------------------------------------
 * making TCP connection for playing game with the connector
 */
bool makeTCPConnection (sockaddr_in& listenAddr, int tcpSocket, int& player2) {
    if (bind(tcpSocket, (struct sockaddr*)&listenAddr, sizeof(listenAddr)) == -1) {
        perror("Bind failed");
        close(tcpSocket);
        return false;
    }

    if (listen(tcpSocket, 5) == -1) {
        perror("Listen failed");
        close(tcpSocket);
        return false;
    }

    std::cout << "Listening for incoming connections on TCP port " << TCP_PORT << "...\n";

    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    // Wait for incoming connections
    player2 = accept(tcpSocket, (struct sockaddr*)&clientAddr, &clientLen);
    return true;
}

/**
 * ------------------------------- testConnection -----------------------------
 * verifying connection and stop broadcasting if TCP connection is made
 */
bool testConnection(int &player2, std::thread& broadcaster) {
    if (player2 == -1) {
        perror("Accept failed");
        return false;
    }
    else {
        running = true;
        broadcaster.join();
        std::cout << "connected!\n";
        return true;
    }
}
/**
 * -------------------------------- main ------------------------------------
 * main function
 */
int main(int argc, char* argv[]) {

    int sock;
    struct sockaddr_in broadcastAddr, listenAddr, clientAddr, senderAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    char buffer[BUFFER_SIZE];
    // Create socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Enable broadcast option
    int broadcastPermission = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastPermission, sizeof(broadcastPermission)) < 0) {
        perror("Setting broadcast permission failed");
        close(sock);
        return -1;
    }
    int opt = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setting reuse option failed");
        close(sock);
        return -1;
    }
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
    perror("setsockopt failed");
    close(sock);
    return -1;
}

    // Set up listening address
    setUpAddr(&listenAddr, BROADCAST_PORT);

    // Bind the socket to listen for incoming messages
    if (bind(sock, (struct sockaddr *)&listenAddr, sizeof(listenAddr)) < 0) {
        perror("Bind failed in broadcast");
        close(sock);
        return -1;
    }
    while (true) {
        // games to the list
        std::vector<std::string> games;
        createAGameList(games, sock, senderAddr);
        // print list
        if (games.size() > 0){
            printGameList(games);
        }
        else {
            std::cout << "No available game currently...\n";
        }
        // display choices: create, join, refresh
        Choice ans = displayChoice();
        switch (ans) {
            case create:
            {
                int player1 = 1;
                int tcpSocket;
                sockaddr_in newSockAddr;
                socklen_t newSockAddSize = sizeof(newSockAddr);

                int player2;
                bool connected = false;
                while(true) {
                    std::thread broadcaster(broadcasting);

                    // TCP server setup
                    tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
                    if (tcpSocket == -1) {
                        perror("TCP socket creation failed");
                        return -1;
                    }

                    sockaddr_in listenAddr;
                    setUpAddr(&listenAddr, TCP_PORT);

                    makeTCPConnection(listenAddr, tcpSocket, player2);
                    connected = testConnection(player2, broadcaster);
                    if (connected) {
                        break;
                    }
                    else {
                        assert(connected == false);
                        exit(EXIT_FAILURE);
                    }
                }
                // create a game and play
                bool again = true;
                while (again) {
                    Board board = createGame(player1, player2);
                    again = playGameHost(board, player1, player2);
                }
                // game concluded
                close(tcpSocket);
                return 0;
                break;
            }
            case join:
            {
                // no game to choose from
                if (games.size() == 0) {
                    std::cout << "Looks like no game to join\nGoing back to the main menu...\n";
                    break;
                }
                std::string choice;
                int choiceInt;
                while(true) {
                    std::cout << "Choose a game from the list\n";
                    std::cin >> choice;
                    choiceInt = convertToInt(choice);
                    if (choiceInt > games.size()) {
                        std::cout << "invalid choice\n";
                    }
                    else {
                        break;
                    }
                }
                bool connected = false;
                int connectedSock;
                connected = joinGame(games.at(choiceInt - 1), senderAddr, connectedSock);
                if (connected) {
                    std::cout << "joined the game\n";
                    close(sock);
                    playGameClient(connectedSock);
                    //finished game
                    close(connectedSock);
                    return 0;
                }
                std::cout << "Sorry! the connection couldn't be made\nGoing back to the main menu...\n";
                // assert (connected == false);
                break;
            }
            case refresh:
                break;
        }
    }
    //game concluded
    return 0;
}
