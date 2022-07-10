#include "mbed.h"
#include "stm32f413h_discovery_ts.h"
#include "stm32f413h_discovery_lcd.h"
#include "vector"
#define PI 4 * atan(1)

using namespace std;

TS_StateTypeDef TS_State = { 0 };

enum State {
    FREE, BLACK, WHITE
};

enum Screen {
    HOME, SPLAYER, MPLAYER, PAUSE, ENDOFGAME
};

Screen current(HOME), last(HOME);
State player = WHITE;

int pressedColumn(int x1, int y1) {
    if (y1 >= 43 && y1 <= 212) {
        for (int i = 15; i <= 225; i += 15) 
            if (x1 >= i-2 && x1 <= i+2) return i/15;
    }
    return -1;
}

int pressedRow(int x1, int y1) {
    if (y1 >= 43 && y1 <= 212) {
        for (int i = 45; i <= 210; i += 15)
            if (y1 >= i-2 && y1 <= i+2) return i/15 - 2;
    }
    return -1;
}

void createHomeView() {
    // Boja pozadine
    BSP_LCD_SetTextColor(LCD_COLOR_BROWN);
    BSP_LCD_FillRect(0,0,BSP_LCD_GetXSize(),BSP_LCD_GetYSize());
    
    // Okvir za naslov igre
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DrawEllipse(BSP_LCD_GetXSize()/2, BSP_LCD_GetYSize()/8 + 5, 40, 20);
    
    // Naslov igre
    BSP_LCD_SetBackColor(LCD_COLOR_BROWN);
    BSP_LCD_SetFont(&Font12);
    BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()/2-21, BSP_LCD_GetYSize()/8, (uint8_t *)"GOMOKU", LEFT_MODE);
    
    // Polje za odabir SinglePlayer igre
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DrawRect(BSP_LCD_GetXSize()/5 + 27, BSP_LCD_GetYSize()/3, 90, 25);
    BSP_LCD_SetBackColor(LCD_COLOR_BROWN);
    BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()/5 + 44, BSP_LCD_GetYSize()/3 + 7, (uint8_t *)"1 PLAYER", LEFT_MODE);
       
    // Polje za odabir MultiPlayer igre
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DrawRect(BSP_LCD_GetXSize()/5 + 27, BSP_LCD_GetYSize()/3 + 60, 90, 25);
    BSP_LCD_SetBackColor(LCD_COLOR_BROWN);
    BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()/5 + 44, BSP_LCD_GetYSize()/3 + 67, (uint8_t *)"2 PLAYERS", LEFT_MODE);
        
    // Ispis teksta koji je fakultet
    BSP_LCD_SetFont(&Font8);
    BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()/8 + 10, BSP_LCD_GetYSize() - 40, (uint8_t *)"Faculty of Electrical Engineering", LEFT_MODE);
    //BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()/8 + 45, BSP_LCD_GetYSize() - 20, (uint8_t *)"By: MC and BB", LEFT_MODE);
}

void pauseView() {
    // Boja pozadine
    BSP_LCD_SetTextColor(LCD_COLOR_BROWN);
    BSP_LCD_FillRect(0, 0, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
    
    // "Gomoku paused" na vrhu
    BSP_LCD_SetBackColor(LCD_COLOR_BROWN);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()/2 - 72, 30, (uint8_t *)"Gomoku paused", LEFT_MODE);
        
    // Crtanje okvira za dugmad
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DrawRect(65, 80, 115, 30);
    BSP_LCD_DrawRect(65, 130, 115, 30);
    BSP_LCD_DrawRect(65, 180, 115, 30);
    
    // Upisivanje teksta u polja 
    BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()/2 - 40 , 87, (uint8_t *)"Continue", LEFT_MODE);
    BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()/2 - 35 , 137, (uint8_t *)"Restart", LEFT_MODE);
    BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()/2 - 20 , 187, (uint8_t *)"Home", LEFT_MODE);
}

class Field {
    int X, Y;
    State fieldState;
public:
    Field(int x, int y, State state) {
        X = x;
        Y = y;
        fieldState = state;
    }
    State getState() { return fieldState; }
    void changeState(State state) { fieldState = state; }
    int getX() { return X; }
    int getY() { return Y; }
};

class Board {
    vector<vector<Field> > fields;
public:
    Board(){}
    Board (vector<vector<Field> > f) { fields = f; }
    void makeBoard() {
        player = WHITE;
        vector<vector<Field> > temporary;
        for (int i = 45; i <= 210; i += 15) {
            vector<Field> temp;
            for (int j = 15; j <= 225; j += 15) {
                Field tempField(j,i,FREE);
                temp.push_back(tempField);
            }
            temporary.push_back(temp);
        }
        fields = temporary;
    }
    bool checkAvailability(int x, int y) {
        return true ? fields.at(x-1).at(y-1).getState() == FREE : false;
    }
    bool endOfGame() {
        // Provjera za 5 u redu
        for (int i = 0; i < fields.size(); i++) {
            int counter(0);
            for (int j = 0; j < fields.at(i).size(); j++) {
                if (fields.at(i).at(j).getState() != player) counter = 0;
                else {
                    counter++;
                    if (counter == 5) return true;
                }
            }
        }
        
        // Provjera za 5 u koloni
        for (int i = 0; i < fields.at(0).size(); i++) {
            int counter(0);
            for (int j = 0; j < fields.size(); j++) {
                if (fields.at(j).at(i).getState() != player) counter = 0;
                else {
                    counter++;
                    if (counter == 5) return true;
                }
            }
        }
        
        // Provjera za dijagonale
        for (int i = 0; i < fields.size(); i++) {
            for (int j = 0; j < fields.at(i).size(); j++) {
                if (fields.at(i).at(j).getState() != player) continue;
                int counter(1), i1(i), j1(j);
                while (i1++ != 11 && j1++ != 14) {
                    if (i1 == 12 || j1 == 15 || fields.at(i1).at(j1).getState() != player) {
                        counter = 1;
                        break;
                    }
                    else {
                        counter++;
                        if (counter == 5) return true;
                    }
                }
                
                counter = 1, i1 = i, j1 = j;
                while (i1++ != 11 && j1-- != 0) {
                    if (i1 == 12 || j1 == 0 || fields.at(i1).at(j1).getState() != player) {
                        counter = 1;
                        break;
                    }
                    else {
                        counter++;
                        if (counter == 5) return true;
                    }
                }
                
                counter = 1, i1 = i, j1 = j;
                while (i1-- != 0 && j1++ != 14) {
                    if (i1 == 0 || j1 == 15 || fields.at(i1).at(j1).getState() != player) {
                        counter = 1;
                        break;
                    }
                    else {
                        counter++;
                        if (counter == 5) return true;
                    }
                }
                
                counter = 1, i1 = i, j1 = j;
                while (i1-- != 0 && j1-- != 0) {
                    if (i1 == 0 || j1 == 0 || fields.at(i1).at(j1).getState() != player) {
                        counter = 1;
                        break;
                    }
                    else {
                        counter++;
                        if (counter == 5) return true;
                    }
                }
            }
            
        }
        
        return false;
    }
    bool draw() {
        for (int i = 0; i < fields.size(); i++)
            for (int j = 0; j < fields.at(i).size(); j++)
                if (fields.at(i).at(j).getState() == FREE) return false;
        return true;                
    }
    void drawView() {
        // Pozadina
        BSP_LCD_SetTextColor(LCD_COLOR_BROWN);
        BSP_LCD_FillRect(0,0,240,240);
        
        // Tekst "DRAW" na sredini
        BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
        BSP_LCD_SetBackColor(LCD_COLOR_BROWN);
        BSP_LCD_SetFont(&Font24);
        BSP_LCD_DisplayStringAt(88,100,(uint8_t *)"DRAW",LEFT_MODE);
        
        // Dugme Home
        BSP_LCD_SetFont(&Font16);
        BSP_LCD_DrawRect(65, 180, 115, 30);
        BSP_LCD_DisplayStringAt(100 , 187, (uint8_t *)"Home", LEFT_MODE);
    }
    void winnerView (int p) {
        // Pozadina
        BSP_LCD_SetTextColor(LCD_COLOR_BROWN);
        BSP_LCD_FillRect(0,0,240,240);
        
        // Pisanje ko je pobijedio
        BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
        BSP_LCD_SetBackColor(LCD_COLOR_BROWN);
        BSP_LCD_SetFont(&Font16);
        BSP_LCD_DisplayStringAt(70,50,(uint8_t*)"Winner is:",LEFT_MODE);
        
        // Pisanje po igračima
        if (p == 1) {
            BSP_LCD_SetFont(&Font24);
            BSP_LCD_DisplayStringAt(57,100,(uint8_t*)"PLAYER 1",LEFT_MODE);
        }
        else {
            BSP_LCD_SetFont(&Font24);
            BSP_LCD_DisplayStringAt(57,100,(uint8_t*)"PLAYER 2",LEFT_MODE);
        }
        
        // Dugme Home
        BSP_LCD_SetFont(&Font16);
        BSP_LCD_DrawRect(65, 180, 115, 30);
        BSP_LCD_DisplayStringAt(BSP_LCD_GetXSize()/2 - 20 , 187, (uint8_t *)"Home", LEFT_MODE);
    } 
    void update() {
        for (int i = 0; i < fields.size(); i++) {
            for (int j = 0; j < fields.at(i).size(); j++) {
                if (fields.at(i).at(j).getState() == WHITE) {
                    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
                    BSP_LCD_FillCircle(fields.at(i).at(j).getX(), fields.at(i).at(j).getY(), 5);
                }
                else if (fields.at(i).at(j).getState() == BLACK) {
                    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
                    BSP_LCD_FillCircle(fields.at(i).at(j).getX(), fields.at(i).at(j).getY(), 5);
                }
            }
        }
    }
    void insert (int x, int y) {
        fields.at(x).at(y).changeState(player);
        if (endOfGame()) {
            current = ENDOFGAME;
            if (player == WHITE) winnerView(1);
            else winnerView(2);
        }
        
        if (draw()) {
            current = ENDOFGAME;
            drawView();
        }
        
        if (player == WHITE) player = BLACK;
        else player = WHITE;
    }
    void bot() {
        for (;;) {
            int row = rand()%12 + 1;
            int column = rand()%15 + 1;
            if (checkAvailability(row,column)) {
                insert(row-1,column-1);
                break;
            }
        }
    }
};

void pointingToPlayer(int player) {
    // Okviri koji pokazuju koji igrač je na redu
    if (player == 1) BSP_LCD_DrawRect(3,218,52,14);
    else if (player == 2) BSP_LCD_DrawRect(183,218,52,14);
}

void singlePlayerView() {
    // Glavna pozadina
    BSP_LCD_SetTextColor(LCD_COLOR_BROWN);
    BSP_LCD_FillRect(0,0,240,240);
    
    // Horizontalne rešetke za polja
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    for (int i = 45; i <= 210; i += 15) BSP_LCD_DrawLine(0,i,240,i);
    
    // Vertikalne rešetke
    for (int i = 15; i <= 225; i += 15) BSP_LCD_DrawLine(i,45,i,210);
    
    // Natpis iznad rešetki
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    BSP_LCD_SetBackColor(LCD_COLOR_BROWN);
    BSP_LCD_DisplayStringAt(20,15, (uint8_t *)"CLICK ON FIELD", LEFT_MODE);
    
    // Player1 tekst
    BSP_LCD_SetFont(&Font12);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_BROWN);
    BSP_LCD_DisplayStringAt(5,220,(uint8_t *)"Player1", LEFT_MODE);
    
    // BOT tekst
    BSP_LCD_SetFont(&Font12);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_BROWN);
    BSP_LCD_DisplayStringAt(200,220,(uint8_t *)"BOT", LEFT_MODE);
    
    // Dugme za pauzu
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    BSP_LCD_FillRect(215, 15, 5, 10);
    BSP_LCD_FillRect(222, 15, 5, 10);
    
    if (last == ENDOFGAME) player = WHITE;
    if (player == WHITE) pointingToPlayer(1);
    else pointingToPlayer(2);
}

void multiPlayerView() {
    // Glavna pozadina
    BSP_LCD_SetTextColor(LCD_COLOR_BROWN);
    BSP_LCD_FillRect(0,0,240,240);
    
    // Horizontalne rešetke za polja
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    for (int i = 45; i <= 210; i += 15) BSP_LCD_DrawLine(0,i,240,i);
    
    // Vertikalne rešetke
    for (int i = 15; i <= 225; i += 15) BSP_LCD_DrawLine(i,45,i,210);
    
    // Natpis iznad rešetki
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    BSP_LCD_SetBackColor(LCD_COLOR_BROWN);
    BSP_LCD_DisplayStringAt(20,15, (uint8_t *)"CLICK ON FIELD", LEFT_MODE);
    
    // Player1 tekst
    BSP_LCD_SetFont(&Font12);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_BROWN);
    BSP_LCD_DisplayStringAt(5,220,(uint8_t *)"Player1", LEFT_MODE);
    
    // Player2 tekst
    BSP_LCD_SetFont(&Font12);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_BROWN);
    BSP_LCD_DisplayStringAt(185,220,(uint8_t *)"Player2", LEFT_MODE);
    
    // Dugme za pauzu
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    BSP_LCD_FillRect(215, 15, 5, 10);
    BSP_LCD_FillRect(222, 15, 5, 10);
    
    if (last == ENDOFGAME) player = WHITE; // Provjera ukoliko je kraj igre da opet počne bijeli
    if (player == WHITE) pointingToPlayer(1); // Pokazivač na igrača 1 (pravougaonik)
    else pointingToPlayer(2); // Pokazivač na igrača 1 (pravougaonik)
}

bool singlePressed(int x, int y) {
    return x >= 75 && x <= 165 && y >= 80 && y <= 105;
}

bool multiPressed(int x, int y) {
    return x >= 75 && x <= 165 && y >= 140 && y <= 165;
}

bool continuePressed(int x, int y) {
    return x >= 65 && x <= 170 && y >= 80 && y <= 110;
}

bool restartPressed(int x, int y) {
    return x >= 65 && x <= 170 && y >= 130 && y <= 160;
}

bool homePressed(int x, int y) {
    return x >= 65 && x <= 170 && y >= 180 && y <= 210; 
}

bool pausePressed(int x, int y) {
    return x >= 215 && x <= 227 && y >= 15 && y <= 25; 
}

int main() {
    Board board;
    board.makeBoard();
    createHomeView();
    while (1) {
        BSP_TS_GetState(&TS_State);
        if (TS_State.touchDetected) {
            
            // Detektovanje klika mišem
            uint16_t x1 = TS_State.touchX[0];
            uint16_t y1 = TS_State.touchY[0];
            printf("Pressed coordinates are %d %d\n",x1,y1);
            
            // Ovdje smo na početnom zaslonu i biramo single ili multiplayer
            if (current == HOME) {
                if (singlePressed(x1,y1)) {
                    current = SPLAYER;
                    singlePlayerView();
                    last = HOME;
                }
                else if (multiPressed(x1,y1)) {
                    current = MPLAYER;
                    multiPlayerView();
                    last = HOME;
                }
            }
            
            // Ovdje smo ušli u proces igranja
            else if (current == SPLAYER || current == MPLAYER) {
                if (pausePressed(x1,y1)) {
                    last = current;
                    current = PAUSE;
                    pauseView();
                }
                int column = pressedColumn(x1,y1);
                int row = pressedRow(x1,y1);
                printf("Current position is: %d %d\n",row,column);
                if (column >= 1 && column <= 15 && row >= 1 && row <= 12) {
                    if (board.checkAvailability(row,column)) {
                        board.insert(row-1,column-1);
                        if (current == SPLAYER) {
                            singlePlayerView();
                            board.update();
                            wait(0.2);
                            board.bot();
                            singlePlayerView();
                            board.update();
                        }
                        if (current == MPLAYER) {
                            multiPlayerView();
                            board.update();
                        }
                    }
                }
            }
            // Ovdje kontrolišemo šta možemo učiniti ukoliko pritisnemo pauzu
            else if(current == PAUSE){
                if(homePressed(x1, y1)){
                    last = current;
                    current = HOME;
                    board.makeBoard();
                    createHomeView();
                }
                else if(continuePressed(x1, y1)){
                    current = last;
                    if(current == SPLAYER) singlePlayerView();
                    else multiPlayerView();
                    board.update();
                }
                else if(restartPressed(x1, y1)){
                    current = last;
                    board.makeBoard();
                    if(current == SPLAYER) singlePlayerView();
                    else multiPlayerView();
                }
            }
            
            // Ovdje smo na kraju igre i jedino možemo pritisnuti Home
            else if(current == ENDOFGAME){
                if(homePressed(x1,y1)){
                    last = current;
                    current = HOME;
                    board.makeBoard();
                    createHomeView();
                }
            }
            wait_ms(10);
        }
    }
}