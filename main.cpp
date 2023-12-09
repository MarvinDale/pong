#include <windows.h>
#include <winuser.h>
#include <d2d1.h>
#include <iostream>

ID2D1Factory          *pFactory      = nullptr;
ID2D1HwndRenderTarget *pRenderTarget = nullptr;
ID2D1SolidColorBrush  *pBrush        = nullptr;

struct Vector2d {
    float x = 0;
    float y = 0;
};

class Paddel {
public:
    float upperLeftX  = 50;
    float upperLeftY  = 250;
    float lowerRightX = 75;
    float lowerRightY = 450;
    float speed       = 1;
    Vector2d velocity;

    D2D1_RECT_F getRect() {
        return D2D1::RectF(upperLeftX, upperLeftY, lowerRightX, lowerRightY);
    };
    
    Paddel() {};
    Paddel(float upperLeftX, float upperLeftY, float lowerRightX, float lowerRightY) {
        this->upperLeftX  = upperLeftX;
        this->upperLeftY  = upperLeftY;
        this->lowerRightX = lowerRightX;
        this->lowerRightY = lowerRightY;
    };
};

class Ball {
public:
    float upperLeftX  = 700;
    float upperLeftY  = 250;
    float lowerRightX = 725;
    float lowerRightY = 275;
    float speed       = 0.5;
    Vector2d direction;

    D2D1_RECT_F getRect() {
        return D2D1::RectF(upperLeftX, upperLeftY, lowerRightX, lowerRightY);
    };

    float getVelocityX() { return direction.x * speed; }

    Ball() {
        direction.x = -1;
        direction.y = 0;
    }

    void resetPosition() {
        upperLeftX  = 700;
        upperLeftY  = 250;
        lowerRightX = 725;
        lowerRightY = 275;
    };
};

Paddel paddel;
Paddel paddelNPC(1375, 250, 1400, 450);
Ball   ball;

const WORD SCAN_CODE_W = 17;
const WORD SCAN_CODE_S = 31;

void onKeyDown(WPARAM wParam) {
    WORD vkCode   = LOWORD(wParam);
    WORD scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);
    
    if (vkCode == VK_ESCAPE)     { PostQuitMessage(0); }
    if (scanCode == SCAN_CODE_W) { paddel.velocity.y = -paddel.speed; } // TODO: direction shouldn't be controlled by speed
    if (scanCode == SCAN_CODE_S) { paddel.velocity.y =  paddel.speed; } //       speed is the magnatude, velocity is a dirction + magnatude
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
        pRenderTarget->FillRectangle(paddel.getRect(),    pBrush);
        pRenderTarget->FillRectangle(paddelNPC.getRect(), pBrush);
        pRenderTarget->FillRectangle(ball.getRect(),      pBrush);
        hr = pRenderTarget->EndDraw();

        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) {
            //DisgardGraphicsResources
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
        } break;

        case WM_KEYUP:
        {
            WORD vkCode = LOWORD(wParam);
            WORD scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);
            
            if (scanCode == SCAN_CODE_W) { paddel.velocity.y = 0; } 
            if (scanCode == SCAN_CODE_S) { paddel.velocity.y = 0; } 
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
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
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

    std::cout << "top " << windowRect.top << "\nBottom " << windowRect.bottom
              << "\nLeft " << windowRect.left << "\nRight " << windowRect.right;

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

        // detect collision with player paddel
        if (ball.upperLeftX <= paddel.lowerRightX &&
            ball.lowerRightY >= paddel.upperLeftY &&
            ball.upperLeftY <= paddel.lowerRightY) {
            ball.speed = -ball.speed;
        }

        // detect collision with npc paddel
        if (ball.lowerRightX >= paddelNPC.upperLeftX &&
            ball.lowerRightY >= paddelNPC.upperLeftY &&
            ball.upperLeftY <= paddel.lowerRightY) {
            ball.speed = -ball.speed;
        }

        // handle ball going off screen
        if (ball.lowerRightX < windowRect.left || ball.upperLeftX > windowRect.right) { 
            ball.resetPosition();
        }

        paddel.upperLeftY  += paddel.velocity.y * deltaTime;
        paddel.lowerRightY += paddel.velocity.y * deltaTime;

        paddelNPC.upperLeftY  += paddel.velocity.y * deltaTime;
        paddelNPC.lowerRightY += paddel.velocity.y * deltaTime;
        
        ball.upperLeftX  += ball.getVelocityX() * deltaTime;
        ball.lowerRightX += ball.getVelocityX() * deltaTime;

        render(hwnd);
    }
    return 0;
}
