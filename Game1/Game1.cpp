#include <SFML/Graphics.hpp>
#include <time.h>

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
using namespace sf;
STARTUPINFO sti = { 0 };
SECURITY_ATTRIBUTES sats = { 0 };
PROCESS_INFORMATION pi = { 0 };
HANDLE pipin_w, pipin_r, pipout_w, pipout_r;
BYTE buffer[2048];
DWORD writ, excode, read, available;
void ConnectToEngine(char* path)
{
    pipin_w = pipin_r = pipout_w = pipout_r = NULL;
    sats.nLength = sizeof(sats);
    sats.bInheritHandle = TRUE;
    sats.lpSecurityDescriptor = NULL;

    CreatePipe(&pipout_r, &pipout_w, &sats, 0);
    CreatePipe(&pipin_r, &pipin_w, &sats, 0);

    sti.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    sti.wShowWindow = SW_HIDE;
    sti.hStdInput = pipin_r;
    sti.hStdOutput = pipout_w;
    sti.hStdError = pipout_w;

    CreateProcess(NULL, (LPWSTR)path, NULL, NULL, TRUE, 0, NULL, NULL, &sti, &pi);
}


std::string getNextMove(std::string position)
{
    std::string str;
    position = "position startpos moves " + position + "\ngo\n";

    WriteFile(pipin_w, position.c_str(), position.length(), &writ, NULL);
    Sleep(500);

    PeekNamedPipe(pipout_r, buffer, sizeof(buffer), &read, &available, NULL);
    do
    {
        ZeroMemory(buffer, sizeof(buffer));
        if (!ReadFile(pipout_r, buffer, sizeof(buffer), &read, NULL) || !read) break;
        buffer[read] = 0;
        str += (char*)buffer;
    } while (read >= sizeof(buffer));

    int n = str.find("bestmove");
    if (n != -1) return str.substr(n + 9, 4);

    return "error";
}


void CloseConnection()
{
    WriteFile(pipin_w, "quit\n", 5, &writ, NULL);
    if (pipin_w != NULL) CloseHandle(pipin_w);
    if (pipin_r != NULL) CloseHandle(pipin_r);
    if (pipout_w != NULL) CloseHandle(pipout_w);
    if (pipout_r != NULL) CloseHandle(pipout_r);
    if (pi.hProcess != NULL) CloseHandle(pi.hProcess);
    if (pi.hThread != NULL) CloseHandle(pi.hThread);
}
int size = 56;
Vector2f offset(28, 28);

Sprite f[32]; //figures
std::string position = "";

int board[8][8] =
{ -1,-2,-3,-4,-5,-3,-2,-1,
 -6,-6,-6,-6,-6,-6,-6,-6,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  6, 6, 6, 6, 6, 6, 6, 6,
  1, 2, 3, 4, 5, 3, 2, 1 };

std::string toChessNote(Vector2f p)
{
    std::string s = "";
    s += char(p.x / size + 97);
    s += char(7 - p.y / size + 49);
    return s;
}

Vector2f toCoord(char a, char b)
{
    int x = int(a) - 97;
    int y = 7 - int(b) + 49;
    return Vector2f(x * size, y * size);
}

void move(std::string str)
{
    Vector2f oldPos = toCoord(str[0], str[1]);
    Vector2f newPos = toCoord(str[2], str[3]);

    for (int i = 0; i < 32; i++)
        if (f[i].getPosition() == newPos) f[i].setPosition(-100, -100);

    for (int i = 0; i < 32; i++)
        if (f[i].getPosition() == oldPos) f[i].setPosition(newPos);

    //castling       //if the king didn't move
    if (str == "e1g1") if (position.find("e1") == -1) move("h1f1");
    if (str == "e8g8") if (position.find("e8") == -1) move("h8f8");
    if (str == "e1c1") if (position.find("e1") == -1) move("a1d1");
    if (str == "e8c8") if (position.find("e8") == -1) move("a8d8");
}

void loadPosition()
{
    int k = 0;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
        {
            int n = board[i][j];
            if (!n) continue;
            int x = abs(n) - 1;
            int y = n > 0 ? 1 : 0;
            f[k].setTextureRect(IntRect(size * x, size * y, size, size));
            f[k].setPosition(size * j, size * i);
            k++;
        }

    for (int i = 0; i < position.length(); i += 5)
        move(position.substr(i, 4));
}


int main()
{
    RenderWindow window(VideoMode(504, 504), "The Chess! (press SPACE)");
    char engine[] = "stockfish.exe";
    ConnectToEngine(engine);

    Texture t1, t2;
    t1.loadFromFile("images/figures.png");
    t2.loadFromFile("images/board.png");

    for (int i = 0; i < 32; i++) f[i].setTexture(t1);
    Sprite sBoard(t2);

    loadPosition();

    bool isMove = false;
    float dx = 0, dy = 0;
    Vector2f oldPos, newPos;
    std::string str;
    int n = 0;

    while (window.isOpen())
    {
        Vector2i pos = Mouse::getPosition(window) - Vector2i(offset);

        Event e;
        while (window.pollEvent(e))
        {
            if (e.type == Event::Closed)
                window.close();

            ////move back//////
            if (e.type == Event::KeyPressed)
                if (e.key.code == Keyboard::BackSpace)
                {
                    if (position.length() > 6) position.erase(position.length() - 6, 5); loadPosition();
                }

            /////drag and drop///////
            if (e.type == Event::MouseButtonPressed)
                if (e.key.code == Mouse::Left)
                    for (int i = 0; i < 32; i++)
                        if (f[i].getGlobalBounds().contains(pos.x, pos.y))
                        {
                            isMove = true; n = i;
                            dx = pos.x - f[i].getPosition().x;
                            dy = pos.y - f[i].getPosition().y;
                            oldPos = f[i].getPosition();
                        }

            if (e.type == Event::MouseButtonReleased)
                if (e.key.code == Mouse::Left)
                {
                    isMove = false;
                    Vector2f p = f[n].getPosition() + Vector2f(size / 2, size / 2);
                    newPos = Vector2f(size * int(p.x / size), size * int(p.y / size));
                    str = toChessNote(oldPos) + toChessNote(newPos);
                    move(str);
                    if (oldPos != newPos) position += str + " ";
                    f[n].setPosition(newPos);
                }
        }

        //comp move
        if (Keyboard::isKeyPressed(Keyboard::Space))
        {
            str = getNextMove(position);

            oldPos = toCoord(str[0], str[1]);
            newPos = toCoord(str[2], str[3]);

            for (int i = 0; i < 32; i++) if (f[i].getPosition() == oldPos) n = i;

            /////animation///////
            for (int k = 0; k < 50; k++)
            {
                Vector2f p = newPos - oldPos;
                f[n].move(p.x / 50, p.y / 50);
                window.draw(sBoard);
                for (int i = 0; i < 32; i++) f[i].move(offset);
                for (int i = 0; i < 32; i++) window.draw(f[i]); window.draw(f[n]);
                for (int i = 0; i < 32; i++) f[i].move(-offset);
                window.display();
            }

            move(str);  position += str + " ";
            f[n].setPosition(newPos);
        }

        if (isMove) f[n].setPosition(pos.x - dx, pos.y - dy);

        ////// draw  ///////
        window.clear();
        window.draw(sBoard);
        for (int i = 0; i < 32; i++) f[i].move(offset);
        for (int i = 0; i < 32; i++) window.draw(f[i]); window.draw(f[n]);
        for (int i = 0; i < 32; i++) f[i].move(-offset);
        window.display();
    }

    CloseConnection();

    return 0;
}
