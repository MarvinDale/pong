#include <windows.h>
#include <winuser.h>
#include <d2d1.h>
#include <dwrite.h>
#include <stdint.h>
#include <iostream>
#include <string>

ID2D1Factory          *pFactory       = nullptr;
ID2D1HwndRenderTarget *pRenderTarget  = nullptr;
ID2D1SolidColorBrush  *pBrush         = nullptr;
ID2D1SolidColorBrush  *pTextBrush     = nullptr;
IDWriteFactory        *pDWriteFactory = nullptr;
IDWriteTextFormat     *pTextFormat    = nullptr;

int const WINDOW_WIDTH = 1920;
int const WINDOW_HIGHT = 1080;
int const WINDOW_CENTER_Y = WINDOW_HIGHT / 2;
int const WINDOW_CENTER_X = WINDOW_WIDTH / 2;

struct Vector2d {
    float x;
    float y;

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
    float speed = 1;
    Vector2d velocity;

    D2D1_RECT_F getRect() {
        return D2D1::RectF(upperLeft.x, upperLeft.y, lowerRight.x, lowerRight.y);
    };
    
    Paddle() {};
    Paddle(float upperLeftX, float upperLeftY, float lowerRightX, float lowerRightY) {
        this->upperLeft.x  = upperLeftX;
        this->upperLeft.y  = upperLeftY;
        this->lowerRight.x = lowerRightX;
        this->lowerRight.y = lowerRightY;
    };
};

class Ball {
public:
    Vector2d  upperLeft;
    Vector2d  lowerRight;
    float     speed = 1;
    const int HALF_WIDTH = 12;
    Vector2d  direction;

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
};

struct Score {
    D2D_RECT_F playerRect = D2D1::RectF(WINDOW_CENTER_X - 300,  100, WINDOW_CENTER_X - 150, 250);
    D2D_RECT_F npcRect    = D2D1::RectF(WINDOW_CENTER_X + 150 , 100, WINDOW_CENTER_X + 300, 250);
    int player = 0;
    int npc    = 0;
};

Score score;

Paddle paddle(100, WINDOW_CENTER_Y - 100, 125, WINDOW_CENTER_Y + 100);
Paddle paddleNPC(WINDOW_WIDTH - 125,
                 WINDOW_CENTER_Y - 100,
                 WINDOW_WIDTH - 100,
                 WINDOW_CENTER_Y + 100
       );

Ball ball;

const WORD SCAN_CODE_W = 17;
const WORD SCAN_CODE_S = 31;

void onKeyDown(WPARAM wParam) {
    WORD vkCode   = LOWORD(wParam);
    WORD scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);
    
    if (vkCode == VK_ESCAPE)     { PostQuitMessage(0); }
    if (scanCode == SCAN_CODE_W) { paddle.velocity.y = -paddle.speed; }
    if (scanCode == SCAN_CODE_S) { paddle.velocity.y =  paddle.speed; }
}

void update(float deltaTime, RECT windowRect) {
    // detect collision with player paddle
    if (ball.upperLeft.x  <= paddle.lowerRight.x &&
        ball.lowerRight.y >= paddle.upperLeft.y  &&
        ball.upperLeft.y  <= paddle.lowerRight.y) {
        ball.direction.x = -ball.direction.x;
    }

    // detect collision with npc paddle
    if (ball.lowerRight.x >= paddleNPC.upperLeft.x &&
        ball.lowerRight.y >= paddleNPC.upperLeft.y &&
        ball.upperLeft.y  <= paddleNPC.lowerRight.y) {
        ball.direction.x = -ball.direction.x;
    }

    // handle ball going off screen left
    if (ball.lowerRight.x < windowRect.left) {
        score.npc++;
        ball.resetPosition();
        ball.direction.x = -ball.direction.x;
    }

    // handle ball going off screen right
    if (ball.upperLeft.x > windowRect.right) {
        score.player++;
        ball.resetPosition();
        ball.direction.x = -ball.direction.x;
    }
    
    // bounce the ball off the top and bottom of the screen
    if (ball.lowerRight.y > windowRect.bottom || ball.upperLeft.y < windowRect.top) {
        ball.direction.y = -ball.direction.y;
    }

    paddle.upperLeft     += paddle.velocity * deltaTime;
    paddle.lowerRight    += paddle.velocity * deltaTime;

    paddleNPC.upperLeft  += paddle.velocity * deltaTime;
    paddleNPC.lowerRight += paddle.velocity * deltaTime;

    ball.upperLeft  += ball.getVelocity() * deltaTime;
    ball.lowerRight += ball.getVelocity() * deltaTime;
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
        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::DarkSeaGreen));
        pRenderTarget->FillRectangle(paddle.getRect(),    pBrush);
        pRenderTarget->FillRectangle(paddleNPC.getRect(), pBrush);
        pRenderTarget->FillRectangle(ball.getRect(),      pBrush);

        std::wstring playerScoreText = std::to_wstring(score.player);
        std::wstring npcScoreText    = std::to_wstring(score.npc);
        pRenderTarget->DrawText(playerScoreText.c_str(),
                                playerScoreText.length(), 
                                pTextFormat,
                                score.playerRect,
                                pBrush
        );
        pRenderTarget->DrawText(npcScoreText.c_str(),
                                npcScoreText.length(),
                                pTextFormat,
                                score.npcRect,
                                pBrush
        );

        hr = pRenderTarget->EndDraw();

        if (FAILED(hr) || hr == (long) D2DERR_RECREATE_TARGET) {
            //DiscardGraphicsResources
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
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            if (FAILED(D2D1CreateFactory(
                    D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory))) {
                return -1;
            }

            DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory));
            pDWriteFactory->CreateTextFormat(
                L"Arial",
                nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                100.0f,
                L"en-US",
                &pTextFormat);
        } break;

        case WM_KEYUP:
        {
            WORD vkCode = LOWORD(wParam);
            WORD scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);
            
            if (scanCode == SCAN_CODE_W) { paddle.velocity.y = 0; } 
            if (scanCode == SCAN_CODE_S) { paddle.velocity.y = 0; } 
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
        WINDOW_WIDTH, WINDOW_HIGHT,
        NULL,
        NULL,
        hInst,
        NULL
    );

    if (hwnd == NULL) { return 0; }

    ShowWindow(hwnd, cmdshow);

    // print window size on creation
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
