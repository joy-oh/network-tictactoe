#include <utility>
#include <stack>
#include <cassert>
/**
 * ------------------------------------ class Board ----------------------------------------------
 * board for tictactoe
 * board put X or O at a position for each player
 */
class Board{
    char board[10];
    int players[2];
    public:
        Board(int player1, int player2){
            for (int i = 0; i < 9; i++) {
                board[i] = ' ';
            }
            board[9] = '\0';
            players[0] = player1;
            players[1] = player2;
        }
        char* getBoard() {
            return board;
        }

        int getPlayer(int player) {
            if (player == players[0]){
                return players[0];
            }
            else {
                assert(player == players[1]);
                return players[1];
            }
        }

        enum Status {win, lose, draw, notdone};
        Status put(int x, int y, int player){
            int pos = x * 3 + y;
            if (player == players[0]) {
                board[pos] = 'X';
            }
            else {
                board[pos] = 'O';
            }
            return checkStat(player);
        }

        Status checkStat(int player) {
            char cur;
            char con;
            bool available = false;
            if (player == players[0]) {
                cur = 'X';
                con = 'O';
            }
            else {
                cur = 'O';
                con = 'X';
            }
            // check win and available spot left
            for (int i = 0; i < 9; i++ ) {
                if (board[i] == cur) {
                    int r = i / 3;
                    int c = i % 3;
                    // top-left position: 3 ways to win
                    if (r == 0 && c == 0) {
                        // check the next 2 cols
                        if (board[1] == cur && board[2] == cur) {
                            return win;
                        }
                        // check 2 botton rows
                        else if (board[3] == cur && board[6] == cur) {
                            return win;
                        }
                        // check diagonal
                        else if (board[4] == cur && board[8] == cur) {
                            return win;
                        }
                    }
                    // top-middle position: one way to win
                    if ((r == 0 && c == 1) && (board[4] == cur) && (board[7] == cur)) {
                        return win;
                    }
                    // top-right position: 2 ways to win
                    if (r == 0 && c == 2) {
                        // check 2 bottom rows
                        if (board[5] == cur && board[8] == cur) {
                            return win;
                        }
                        // check diagonal
                        else if (board[4] == cur && board[6] == cur) {
                            return win;
                        }
                    }
                    // middle-left
                    if ((r == 1 && c == 0) && (board[4] == cur && board[5] == cur)) {
                        return win;
                    }
                    // bottom-left
                    if ((r == 2 && c == 0) && (board[7] == cur && board[8] == cur)) {
                        return win;
                    }
                }
                else if (board[i] != con) {
                    available = true;
                }
            }
            if (!available) {
                return draw;
            }
            else {
                return notdone;
            }
        }
};

