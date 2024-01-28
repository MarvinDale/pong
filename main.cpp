#include <windows.h>
#include <winuser.h>
#include <d2d1.h>
#include <dwrite.h>
#include <stdint.h>
#include <iostream>
#include <string>

ID2D1Factory          *pFactory          = nullptr;
ID2D1HwndRenderTarget *pRenderTarget     = nullptr;
ID2D1SolidColorBrush  *pBrush            = nullptr;
IDWriteFactory        *pDWriteFactory    = nullptr;
IDWriteTextFormat     *pScoreTextFormat  = nullptr;
IDWriteTextFormat     *pPausedTextFormat = nullptr;

bool gameIsPaused   = false;
bool gameHasStarted = false;
bool gameIsOver     = false;
bool playerWon      = false;

int const WINDOW_WIDTH  = 1920;
int const WINDOW_HEIGHT = 1080;
int const WINDOW_CENTER_Y = WINDOW_HEIGHT / 2;
int const WINDOW_CENTER_X = WINDOW_WIDTH / 2;
int const TARGET_SCORE = 5;

// have the menu just above the screen centre
D2D1_RECT_F menuRect = D2D1::RectF(
    WINDOW_CENTER_X - 200,
    WINDOW_CENTER_Y - 75,
    WINDOW_CENTER_X + 200,
    WINDOW_CENTER_Y + 50
); 

struct Vector2d {
    float x;
    float y;

    static Vector2d up()   { return Vector2d(0, -1); }
    static Vector2d down() { return Vector2d(0,  1); }
    static Vector2d zero() { return Vector2d(0,  0); }

    Vector2d() : x(0), y(0) {}

    Vector2d(float x, float y) {
        this->x = x;
        this->y = y;
    }

    Vector2d operator+(float value) {
        Vector2d result;
        result.x = x + value;
        result.y = y + value;

        return result;
    }

    Vector2d operator+=(Vector2d value) {
        x += value.x;
        y += value.y;

        return *this;
    }

    Vector2d operator*(float value) {
        Vector2d result;
        result.x = x * value;
        result.y = y * value;

        return result;
    }
};

class Paddle {
public:
    Vector2d upperLeft;
    Vector2d lowerRight;
    Vector2d direction;
    Vector2d initialPosition;
    float speed = 0.75;
    float length;

    D2D1_RECT_F getRect() {
        return D2D1::RectF(upperLeft.x, upperLeft.y, lowerRight.x, lowerRight.y);
    };
    
    Paddle() {};
    Paddle(float upperLeftX, float upperLeftY, float lowerRightX, float lowerRightY) {
        this->direction    = Vector2d::zero();
        this->upperLeft.x  = upperLeftX;
        this->upperLeft.y  = upperLeftY;
        this->lowerRight.x = lowerRightX;
        this->lowerRight.y = lowerRightY;
        length = lowerRightY - upperLeftY;
        initialPosition = Vector2d(upperLeftY, lowerRightY);
    };

    float getTop()    { return upperLeft.y; }
    float getBottom() { return lowerRight.y;}
    Vector2d getVelocity() { return direction * speed; }
    void resetPosition() {
        upperLeft.y  = initialPosition.x;
        lowerRight.y = initialPosition.y;
    }
};

class Ball {
public:
    Vector2d  upperLeft;
    Vector2d  lowerRight;
    float     speed = 1.25;
    const int HALF_WIDTH = 12;
    Vector2d  direction;
    bool      hasScored = false;

    Ball() : direction(-1, 0.4) {
        upperLeft.x  = WINDOW_CENTER_X - HALF_WIDTH;
        upperLeft.y  = WINDOW_CENTER_Y - HALF_WIDTH;
        lowerRight.x = WINDOW_CENTER_X + HALF_WIDTH;
        lowerRight.y = WINDOW_CENTER_Y + HALF_WIDTH;
    }

    D2D1_RECT_F getRect() {
        return D2D1::RectF(upperLeft.x, upperLeft.y, lowerRight.x, lowerRight.y);
    };

    Vector2d getVelocity() { return direction * speed; }

    void resetPosition() {
        upperLeft.x  = WINDOW_CENTER_X - HALF_WIDTH;
        upperLeft.y  = WINDOW_CENTER_Y - HALF_WIDTH;
        lowerRight.x = WINDOW_CENTER_X + HALF_WIDTH;
        lowerRight.y = WINDOW_CENTER_Y + HALF_WIDTH;
    };

    float getTop()    { return upperLeft.y;  }
    float getBottom() { return lowerRight.y; }
};

struct Score {
    D2D_RECT_F playerRect =
        D2D1::RectF(WINDOW_CENTER_X - 300, 100, WINDOW_CENTER_X - 150, 250);

    D2D_RECT_F npcRect =
        D2D1::RectF(WINDOW_CENTER_X + 150, 100, WINDOW_CENTER_X + 300, 250);

    int player = 0;
    int npc    = 0;
};

Score score;

Paddle paddle(200, WINDOW_CENTER_Y - 100, 225, WINDOW_CENTER_Y + 100);
Paddle paddleNPC(WINDOW_WIDTH - 225,
                 WINDOW_CENTER_Y - 100,
                 WINDOW_WIDTH - 200,
                 WINDOW_CENTER_Y + 100
);

Ball ball;

const WORD SCAN_CODE_W = 17;
const WORD SCAN_CODE_S = 31;

void restartGame() {
    gameIsPaused = false;
    gameIsOver = false;
    
    score.player = 0;
    score.npc = 0;

    paddle.resetPosition();
    paddleNPC.resetPosition();
}

void onKeyDown(WPARAM wParam) {
    WORD vkCode   = LOWORD(wParam);
    WORD scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);
    
    if (vkCode == VK_ESCAPE && gameHasStarted) { 
        if (gameIsPaused) { gameIsPaused = false; }
        else { gameIsPaused = true; }
    }
    if (vkCode == VK_SPACE) { 
        if (gameIsOver) { restartGame(); } else
        if (!gameHasStarted) { gameHasStarted = true; }
    }
    if (scanCode == SCAN_CODE_W) { paddle.direction = Vector2d::up();   }
    if (scanCode == SCAN_CODE_S) { paddle.direction = Vector2d::down(); }
}

void handleGoals(RECT windowRect) {
    bool ballIsAbovePaddle = ball.getBottom() < paddle.getTop();
    bool ballIsBelowPaddle = ball.getTop() > paddle.getBottom();
    // detect if the ball has passed the paddle
    if (ball.upperLeft.x <= paddle.lowerRight.x &&
        (ballIsAbovePaddle || ballIsBelowPaddle)) {
        // stops ball changing direction before going off screen after passing the paddle
        ball.hasScored = true; 
    }

    bool ballIsAboveNPCPaddle = ball.getBottom() < paddleNPC.getTop();
    bool ballIsBelowNPCPaddle = ball.getTop() > paddleNPC.getBottom();

    if (ball.lowerRight.x >= paddleNPC.upperLeft.x &&
        (ballIsAboveNPCPaddle || ballIsBelowNPCPaddle)) {

        ball.hasScored = true;
    }

    // handle ball going off screen left
    if (ball.lowerRight.x < windowRect.left) {
        score.npc++;
        ball.resetPosition();
        ball.direction.x = -ball.direction.x;
        ball.hasScored = false;
        if (score.npc >= TARGET_SCORE) {
            gameIsOver = true;
            playerWon = false;
        }
    }

    // handle ball going off screen right
    if (ball.upperLeft.x > windowRect.right) {
        score.player++;
        ball.resetPosition();
        ball.direction.x = -ball.direction.x;
        ball.hasScored = false;
        if (score.player >= TARGET_SCORE) {
            gameIsOver = true;
            playerWon = true;
        }
    }
}

void handleCollisions(RECT windowRect) {
    // detect collision with player paddle
    if (ball.upperLeft.x  <= paddle.lowerRight.x &&
        ball.getBottom() >= paddle.getTop()      &&
        ball.getTop()  <= paddle.getBottom()     &&
        !ball.hasScored) {

        ball.direction.x = -ball.direction.x;
        if (paddle.direction.y != 0) {
            ball.direction.y = (paddle.direction.y * ball.direction.x) * 0.4;
        }
    }

    // detect collision with npc paddle
    if (ball.lowerRight.x >= paddleNPC.upperLeft.x &&
        ball.getBottom() >= paddleNPC.getTop()     &&
        ball.getTop()  <= paddleNPC.getBottom()    &&
        !ball.hasScored) {

        ball.direction.x = -ball.direction.x;
        if (paddle.direction.y != 0) {
            ball.direction.y = (paddle.direction.y * ball.direction.x) * 0.4;
        }
    }

    // bounce the ball off the top and bottom of the screen
    if (ball.lowerRight.y > windowRect.bottom || ball.upperLeft.y < windowRect.top) {
        ball.direction.y = -ball.direction.y;
    }
}

void updatePaddlePositions(float deltaTime) {
    // update paddle position
    paddle.upperLeft     += paddle.getVelocity() * deltaTime;
    paddle.lowerRight    += paddle.getVelocity() * deltaTime;

    paddleNPC.upperLeft  += Vector2d(0, ball.getVelocity().y) * deltaTime;
    paddleNPC.lowerRight += Vector2d(0, ball.getVelocity().y) * deltaTime;
    
    // if a paddle goes off the top of the screen reset it's position to be at the edge of the screen
    if (paddle.upperLeft.y < 0) {
        paddle.upperLeft.y  = 0;
        paddle.lowerRight.y = paddle.length;
    } 

    if (paddleNPC.upperLeft.y < 0) {
        paddleNPC.upperLeft.y  = 0;
        paddleNPC.lowerRight.y = paddleNPC.length;
    } 

    // if a paddle goes off the bottom of the screen reset it's position to be at the edge of the screen
    if (paddle.lowerRight.y > WINDOW_HEIGHT) {
        paddle.lowerRight.y  = WINDOW_HEIGHT;
        paddle.upperLeft.y   = WINDOW_HEIGHT - paddle.length;
    } 

    if (paddleNPC.lowerRight.y > WINDOW_HEIGHT) {
        paddleNPC.lowerRight.y  = WINDOW_HEIGHT;
        paddleNPC.upperLeft.y   = WINDOW_HEIGHT - paddleNPC.length;
    } 
}

void updateBallPosition(float deltaTime) {
    ball.upperLeft  += ball.getVelocity() * deltaTime;
    ball.lowerRight += ball.getVelocity() * deltaTime;
}

void update(float deltaTime, RECT windowRect) {

    if (!gameHasStarted) { return; }
    if (gameIsPaused)    { return; }
    if (gameIsOver)      { return; }

    handleGoals(windowRect);
    handleCollisions(windowRect);
    
    updatePaddlePositions(deltaTime);
    updateBallPosition(deltaTime);
}

void render(HWND hwnd) {
    HRESULT hr = S_OK;
    RECT rc;
    GetClientRect(hwnd, &rc); // retrieves the coordinates of a windows client area
    
    // get device resources
    if (!pRenderTarget) {
        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, size),
            &pRenderTarget);

        if (SUCCEEDED(hr)) {
            const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 1.0f);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
        }
    }

    if (SUCCEEDED(hr)) {
        PAINTSTRUCT paintStruct;
        BeginPaint(hwnd, &paintStruct);

        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::DarkCyan));
        pRenderTarget->FillRectangle(paddle.getRect(),    pBrush);
        pRenderTarget->FillRectangle(paddleNPC.getRect(), pBrush);
        pRenderTarget->FillRectangle(ball.getRect(),      pBrush);

        std::wstring playerScoreText = std::to_wstring(score.player);
        std::wstring npcScoreText    = std::to_wstring(score.npc);

        if (gameIsOver) {
            std::wstring menuText;
            if (playerWon) {
                menuText = L"Game Over. You Won!\nPress Space To Play Again";
            } else {
                menuText = L"Game Over. You Lost!\nPress Space To Play Again";
            }
            pRenderTarget->DrawText(menuText.c_str(),
                                    menuText.length(),
                                    pPausedTextFormat,
                                    menuRect,
                                    pBrush
            );
        } else {
            if (gameIsPaused) {
                std::wstring menuText = L"Paused";
                pRenderTarget->DrawText(menuText.c_str(),
                                        menuText.length(),
                                        pPausedTextFormat,
                                        menuRect,
                                        pBrush
                );
            }
            if (!gameHasStarted) {
                std::wstring menuText = L"Press Space To Start";
                pRenderTarget->DrawText(menuText.c_str(),
                                        menuText.length(),
                                        pPausedTextFormat,
                                        menuRect,
                                        pBrush
                );
            }
        }
        pRenderTarget->DrawText(playerScoreText.c_str(),
                                playerScoreText.length(), 
                                pScoreTextFormat,
                                score.playerRect,
                                pBrush
        );
        pRenderTarget->DrawText(npcScoreText.c_str(),
                                npcScoreText.length(),
                                pScoreTextFormat,
                                score.npcRect,
                                pBrush
        );

        hr = pRenderTarget->EndDraw();

        if (FAILED(hr) || hr == (long) D2DERR_RECREATE_TARGET) {
            // DiscardGraphicsResources
            pRenderTarget->Release();
            pBrush->Release();
            pFactory->Release();
            pRenderTarget = nullptr;
            pBrush        = nullptr;
            pFactory      = nullptr;
        }
        EndPaint(hwnd, &paintStruct);
    }
}

LRESULT CALLBACK 
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
        {
            if (FAILED(D2D1CreateFactory(
                    D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory))) {
                return -1;
            }

            DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                __uuidof(IDWriteFactory),
                                reinterpret_cast<IUnknown**>(&pDWriteFactory)
            );
            pDWriteFactory->CreateTextFormat(
                L"Lucida Console",
                nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                100.0f,
                L"en-US",
                &pScoreTextFormat
            );
            pScoreTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

            pDWriteFactory->CreateTextFormat(
                L"Lucida Console",
                nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                20.0f,
                L"en-US",
                &pPausedTextFormat
            );
            pPausedTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        } break;

        case WM_KEYUP:
        {
            WORD vkCode = LOWORD(wParam);
            WORD scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);
            
            if (scanCode == SCAN_CODE_W) { paddle.direction = Vector2d::zero(); } 
            if (scanCode == SCAN_CODE_S) { paddle.direction = Vector2d::zero(); } 
        } break;

        case WM_KEYDOWN:
        {
            onKeyDown(wParam);
        } break;

        case WM_CLOSE:
        {
            DestroyWindow(hwnd); 
        } break;
        
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        } break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, 
                   HINSTANCE hInstPrev, 
                   PSTR      cmdline, 
                   int       cmdshow) {
    WNDCLASS WindowClass = {};
    WindowClass.lpfnWndProc   = WindowProc;
    WindowClass.hInstance     = hInst;
    WindowClass.lpszClassName = "Window Class"; 

    RegisterClass(&WindowClass);

    HWND hwnd = CreateWindowEx(
        0,
        "Window Class",
        "Pong",
        (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX),
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL,
        NULL,
        hInst,
        NULL
    );

    if (hwnd == NULL) { return 0; }

    ShowWindow(hwnd, cmdshow);

    RECT windowRect;
    GetClientRect(hwnd, &windowRect);

    LARGE_INTEGER ticksPerSecond;
    LARGE_INTEGER previousTickCount;
    LARGE_INTEGER currentTickCount;

    QueryPerformanceFrequency(&ticksPerSecond);
    QueryPerformanceCounter(&previousTickCount);

    bool running = true; 
    MSG msg = { };
    uint64_t elapsedTicks;
    uint64_t elapsedTimeInMicroseconds;
    float deltaTime;

    while(running) {
        while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            if(msg.message == WM_QUIT) {
                running = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // get delta time
        QueryPerformanceCounter(&currentTickCount);
        elapsedTicks = currentTickCount.QuadPart - previousTickCount.QuadPart;
        elapsedTimeInMicroseconds = (elapsedTicks * 1000000) / ticksPerSecond.QuadPart;

        // deltaTime in milliseconds
        deltaTime = elapsedTimeInMicroseconds / 1000.0f;
        previousTickCount = currentTickCount;

        // update game state below
        update(deltaTime, windowRect);
        render(hwnd);
    }
    return 0;
}
