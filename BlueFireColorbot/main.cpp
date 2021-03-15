//MADE BY BLUEFIRE1337
//THIS PROJECT IS UNDER GNU General Public License v3.0
//SO NO SELLING THIS AS YOUR OWN
//TO MAKE YOUR OWN LOOK AT https://www.unknowncheats.me/forum/valorant/444573-perfect-valorant-colorbot.html

#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>
#include "resource.h"
#include <WinUser.h>
#include "imgui.h"
#include "imgui_impl_dx9.h"

#include <windows.h>
#include <iostream>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <stdlib.h>
#include <fstream>
#include <chrono> // for high_resolution_clock
#include <list>
#include <wrl/client.h>
#include <thread>

#include "interception.h"
#include <string>

#pragma comment(lib,"d3d11.lib")
using namespace std;

#define PROCESS_NAME L"VALORANT  " 
#define NAMEOF(name) #name

using Microsoft::WRL::ComPtr;

ComPtr<ID3D11Device> lDevice;
ComPtr<ID3D11DeviceContext> lImmediateContext;
D3D11_TEXTURE2D_DESC desc;

D3D_DRIVER_TYPE gDriverTypes[] = {
	D3D_DRIVER_TYPE_HARDWARE
};
UINT gNumDriverTypes = ARRAYSIZE(gDriverTypes);

// Feature levels supported
D3D_FEATURE_LEVEL gFeatureLevels[] = {
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_1
};
UINT gNumFeatureLevels = ARRAYSIZE(gFeatureLevels);


#define M_PI 3.14159265358979323846  
#define RADTODEG 180 / M_PI
#define DEGTORAD M_PI / 180 

struct Vector2 {
	int x;
	int y;
	Vector2(int X, int Y) {
		x = X;
		y = Y;
	}
	float Len() {
		return sqrt(pow(x, 2) + pow(y, 2));
	}
	Vector2 operator+(const Vector2& a) const
	{
		return Vector2(a.x + x, a.y + y);
	}
};

bool flickAim = false;
int flickAimTime = 20;
float speed = 0.2;
int maxX = 600;
int maxY = 300;
int full360 = 0;//21428;
int holdKeyIndex = 0;
int holdKey = VK_MENU;
bool isHold = false;
bool invertHold = false;
int offset[2] = {
	0,5
};

bool isZommed = false;
const int hold_arry_size = 15;
static const char* holdKeys[hold_arry_size]{
   "Left mouse button",
   "Right mouse button",
   "Middle mouse button",
   "TAB key",
   "SHIFT key",
   "CTRL key",
   "ALT key",
   "DEL key",
   "INS key",
   "Numeric keypad 0 key",
   "NUM LOCK key",
   "Left SHIFT key",
   "Right SHIFT key",
   "Left CONTROL key",
   "Right CONTROL key",
};

static const int holdKeysCodes[hold_arry_size]{
   VK_LBUTTON,
   VK_RBUTTON,
   VK_MBUTTON,
   VK_TAB,
   VK_SHIFT,
   VK_CONTROL,
   VK_MENU,
   VK_DELETE,
   VK_INSERT,
   VK_NUMPAD0,
   VK_NUMLOCK,
   VK_LSHIFT,
   VK_RSHIFT,
   VK_LCONTROL,
   VK_RCONTROL,
};

// Data
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

uint32_t width;
uint32_t height;
ComPtr<IDXGISurface1> gdiSurface;
ComPtr<ID3D11Texture2D> texture;
HWND game_window;


InterceptionContext context;
InterceptionDevice device;
InterceptionStroke stroke;

void NormalMouse() {
	while(interception_receive(context, device = interception_wait(context), &stroke, 1) > 0) {
		if(interception_is_mouse(device))
		{
			InterceptionMouseStroke& mstroke = *(InterceptionMouseStroke*)&stroke;
			interception_send(context, device, &stroke, 1);
		}
	}
}

void InitMoveMouse() {
	cout << "Loading Interception..." << endl;

	context = interception_create_context();
	interception_set_filter(context, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_MOVE);
	device = interception_wait(context);

	while(interception_receive(context, device = interception_wait(context), &stroke, 1) > 0) {
		if(interception_is_mouse(device))
		{
			InterceptionMouseStroke& mstroke = *(InterceptionMouseStroke*)&stroke;
			interception_send(context, device, &stroke, 1);
			break;
		}
	}
	cout << "Interception Loaded" << endl;
	thread normal(NormalMouse);
	normal.detach();
}

void MoveMouse(int dx, int dy) {
	InterceptionMouseStroke& mstroke = *(InterceptionMouseStroke*)&stroke;
	mstroke.flags = 0;
	mstroke.information = 0;
	mstroke.x = dx + offset[0];
	mstroke.y = dy + offset[1];
	interception_send(context, device, &stroke, 1);
}

typedef void(*ColorSortingMethod)(char*, int, int);
ColorSortingMethod currentSortingMethod;

void SetIsZoomed() { // CALL THIS EVERY FRAME
	isZommed = GetAsyncKeyState(VK_RBUTTON);
}

int Full360() {
	return isZommed ? full360 : (full360 * 8 / 10);
}

int GetCoordsX(int delta, int total) {

	double lookAt = delta * 2.0 / total;
	double degrees = atan(lookAt * tan((isZommed ? 41.5 : 52.0) * DEGTORAD)) * RADTODEG;
	return (Full360() * degrees) / 360;
}

int GetCoordsY(int delta, int total) {

	double lookAt = delta * 2.0 / total;
	double degrees = atan(lookAt * tan((isZommed ? 26.5 : 36) * DEGTORAD)) * RADTODEG;
	return (Full360() * degrees) / 360;
}

void MoveMouseFromScreenPosition(Vector2 front, int height, int width) {
	SetIsZoomed();
	int moveX = GetCoordsX(front.x, width);
	int moveY = GetCoordsY(front.y, height);
	if(flickAim) {
		MoveMouse(moveX, moveY);
		Sleep(flickAimTime);
	}
	else {
		MoveMouse(moveX * speed, moveY * speed);
	}
}

bool IsPurpleColor(unsigned short red, unsigned short green, unsigned short blue) {
	// updated PURPLE FROM https://www.unknowncheats.me/forum/valorant/437368-updated-colors-pixel-bot-act-4-a.html
	if(green >= 170) {
		return false;
	}

	if(green >= 120) {
		return abs(red - blue) <= 8 &&
			red - green >= 50 &&
			blue - green >= 50 &&
			red >= 105 &&
			blue >= 105;
	}

	return abs(red - blue) <= 13 &&
		red - green >= 60 &&
		blue - green >= 60 &&
		red >= 110 &&
		blue >= 100;
	
	//return red > 240 && green > 90 && green < 190 && blue > 240; // OLD COLOR FUNCTION
}

void FirstColorSorting(char* data, int height, int width) {
	int hWidth = width / 2;
	int hHeight = height / 2;
	for(int y = hHeight - maxY; y < hHeight + maxY; y++) {
		for(int x = hWidth - maxX; x < hWidth + maxX; x++) {
			int base = (x + y * desc.Width) * 4;
			unsigned short red = data[base + 2] & 255;
			unsigned short green = data[base + 1] & 255;
			unsigned short blue = data[base] & 255;
			if(IsPurpleColor(red, green, blue)) {
				MoveMouseFromScreenPosition(Vector2(x - hWidth, y - hHeight), height, width);
				return;
			}
		}
	}
}

void BlueFireColorSorting(char* data, int height, int width) {
	const int maxCount = 5;
	const int forSize = 100;

	list<Vector2> vects;
	int hWidth = width / 2;
	int hHeight = height / 2;

	for(int y = hHeight - maxY; y < hHeight + maxY; y++) {
		for(int x = hWidth - maxX; x < hWidth + maxX; x++) {
			int base = (x + y * desc.Width) * 4;
			unsigned short red = data[base + 2] & 255;
			unsigned short green = data[base + 1] & 255;
			unsigned short blue = data[base] & 255;
			if(IsPurpleColor(red, green, blue)) {
				vects.push_back(Vector2(x - hWidth, y - hHeight));
			}
		}
	}

	if(vects.size() > 0) {
		vects.sort([](const Vector2& lhs, const Vector2& rhs) // SORT BY BIGGEST Y
			{
				return  lhs.y < rhs.y;
			});
		list<Vector2> forbidden;
		for(auto& current : vects) // access by reference to avoid copying
		{
			bool canUpdate = true;
			if(abs(current.x) > maxX || abs(current.y) > maxY) {
				continue;
			}
			for(auto& forb : forbidden) // access by reference to avoid copying
			{
				if((current + forb).Len() < forSize) {
					canUpdate = false;
					break;
				}
				if(abs(current.x + forb.x) < forSize) {
					canUpdate = false;
					break;
				}
			}
			if(canUpdate) {
				forbidden.push_front(current);
				if(forbidden.size() > maxCount) {
					break;
				}
			}
		}
		if(forbidden.size() > 0) {
			forbidden.sort([](const Vector2& lhs, const Vector2& rhs)
				{
					return sqrt(pow(lhs.x, 2) + pow(lhs.y * 10, 2)) < sqrt(pow(rhs.x, 2) + pow(rhs.y * 10, 2));
				});
			Vector2 front = forbidden.front();
			MoveMouseFromScreenPosition(front, height, width);
		}
	}
}


bool InitColor() {
	// ==== FIND WINDOW ==== 
	RECT rect;
	game_window = FindWindowW(NULL, PROCESS_NAME);
	GetClientRect(game_window, &rect);

	// ==== SCALING FACTOR ====
	HDC monitor = GetDC(game_window); // GetDC(NULL);

	int current = GetDeviceCaps(monitor, VERTRES);
	int total = GetDeviceCaps(monitor, DESKTOPVERTRES);

	width = (rect.right - rect.left) * total / current;
	height = (rect.bottom - rect.top) * total / current;

	// ==== CREATE DEVICE ==== 

	HRESULT hr(E_FAIL);
	D3D_FEATURE_LEVEL lFeatureLevel;

	for(UINT DriverTypeIndex = 0; DriverTypeIndex < gNumDriverTypes; ++DriverTypeIndex)
	{
		hr = D3D11CreateDevice(
			nullptr,
			gDriverTypes[DriverTypeIndex],
			nullptr,
			0,
			gFeatureLevels,
			gNumFeatureLevels,
			D3D11_SDK_VERSION,
			&lDevice,
			&lFeatureLevel,
			&lImmediateContext);

		if(SUCCEEDED(hr))
		{
			// Device creation success, no need to loop anymore
			break;
		}

		lDevice.Reset();

		lImmediateContext.Reset();
	}

	// ==== CREATE TEXTURE ====

	desc.Width = width;
	desc.Height = height;
	desc.ArraySize = 1;
	desc.MipLevels = 1;

	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	desc.Usage = D3D11_USAGE_DEFAULT;

	desc.BindFlags = 40;
	desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
	desc.CPUAccessFlags = 0;

	hr = lDevice->CreateTexture2D(&desc, NULL, &texture);

	if(FAILED(hr)) {
		cout << "Failed to create texture" << endl;
		return false;
	}

	hr = texture->QueryInterface(__uuidof(IDXGISurface1), (void**)&gdiSurface);

	if(FAILED(hr)) {
		cout << "Failed to create GDI surface" << endl;
		return false;
	}

	// REUSE desc FOR FRAMECOPY
	desc.BindFlags = 0;
	desc.MiscFlags &= D3D11_RESOURCE_MISC_TEXTURECUBE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;
	InitMoveMouse();
	cout << "Starting at " << width << "x" << height << endl;

	return true;
}
HDC hdc_target;

bool ScreenGrab() {
	// ==== SCEENGRAB ==== 

	HDC hDC = nullptr;

	gdiSurface->GetDC(true, &hDC);
	hdc_target = GetDC(game_window);

	// === THE COPY TEXTURE ===
	while(!BitBlt(hDC, 0, 0, width, height, hdc_target, 0, 0, SRCCOPY)) {
		cout << "FAILED" << endl;
		Sleep(1000);
	}

	// VERY IMPORTANT TO RELEASE BEFORE COPY
	ReleaseDC(NULL, hdc_target);
	gdiSurface->ReleaseDC(nullptr);

	// === COPY TO CPU ===
	D3D11_MAPPED_SUBRESOURCE tempsubsource;
	ID3D11Texture2D* pFrameCopy = nullptr;
	HRESULT hr = lDevice->CreateTexture2D(&desc, nullptr, &pFrameCopy);
	if(FAILED(hr)) {
		return false;
	}

	lImmediateContext->CopyResource(pFrameCopy, texture.Get());

	hr = lImmediateContext->Map(pFrameCopy, 0, D3D11_MAP_READ, 0, &tempsubsource);
	void* d = tempsubsource.pData;
	char* data = reinterpret_cast<char*>(d);

	if(FAILED(hr)) {
		return false;
	}

	currentSortingMethod(data, desc.Height, desc.Width);

	if(pFrameCopy != nullptr) {
		lImmediateContext->Unmap(pFrameCopy, 0);

		pFrameCopy->Release();
		lImmediateContext->Flush();
	}
	return true;
}

bool isRunning = false;
bool isReallyRunning = false;

void ScreenGrabMain() {
	while(true) {
		bool shouldRun = false;
		if(isRunning) {
			if(isHold) {
				shouldRun = (GetKeyState(holdKey) & 0x8000);
				if(invertHold) shouldRun = !shouldRun;
			}
			else {
				shouldRun = (GetKeyState(holdKey) == 1);
			}

			if(shouldRun) {
				ScreenGrab();
			}
			else {
				Sleep(10);
			}
		}
		else {
			Sleep(100);
		}
		isReallyRunning = shouldRun;
	}
}

int sortingCounter = 0;
const char* currentSortingMethodName;
const char* currentSortingMethodDescript;
void UpdateSortingMethod(int id) {
	switch(id % 2)
	{
	case 0:
		currentSortingMethod = BlueFireColorSorting;
		currentSortingMethodName = "Blue Fire Sorter";
		currentSortingMethodDescript = "A bit slower, but priorities heads";
		break;
	case 1:
		currentSortingMethod = FirstColorSorting;
		currentSortingMethodName = "First Color Sorter";
		currentSortingMethodDescript = "Fast, but no sorting";
		break;
	default:
		break;
	}
}

bool ReadConfig() {
	ifstream cFile("config.txt");
	std::string line;
	int offsetX = offset[0];
	int offsetY = offset[1];
	if(cFile.is_open())
	{
		while(std::getline(cFile, line)) {
			line.erase(std::remove_if(line.begin(), line.end(), isspace),
				line.end());
			if(line[0] == '#' || line.empty()) // COMMETS
				continue;
			auto delimiterPos = line.find("=");
			auto name = line.substr(0, delimiterPos);
			auto value = line.substr(delimiterPos + 1);
#define READ(_name) if(name == NAMEOF(_name)) _name = atof(value.c_str());

			READ(speed);
			READ(maxX);
			READ(maxY);
			READ(offsetX);
			READ(offsetY);
			READ(flickAim);
			READ(flickAimTime);
			READ(full360);
			READ(sortingCounter);
			READ(holdKey);
			READ(isHold);
			READ(invertHold);
		}

		offset[0] = offsetX;
		offset[1] = offsetY;
		return true;
	}
	else {
		return false;
	}
}

void SaveConfig() {
	ofstream cFile("config.txt");
#define WRITE(_name) cFile << NAMEOF(_name) << "=" << _name << "\n";

	int offsetX = offset[0];
	int offsetY = offset[1];

	WRITE(speed);
	WRITE(maxX);
	WRITE(maxY);
	WRITE(offsetX);
	WRITE(offsetY);
	WRITE(flickAim);
	WRITE(flickAimTime);
	WRITE(full360);
	WRITE(sortingCounter);

	cFile << "#All keycodes can be found at https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes\n";
	if(holdKeyIndex > 0) {
		cFile << NAMEOF(holdKey) << "=" << holdKeysCodes[holdKeyIndex] << "\n";
	}
	else {
		WRITE(holdKey);
	}
	WRITE(isHold);
	WRITE(invertHold);
	cout << "Saved config" << endl;
}

// Main code
int main(int, char**)
{
	cout << "Fetching Config..." << endl;
	if(!ReadConfig()) {
		cout << "Failed to read config" << endl;
	}
	else {
		cout << "Loaded Config" << endl;
	}

	if(!InitColor()) {
		cin.get();
		return -1;
	}
	thread t1(ScreenGrabMain);
	t1.detach();

	UpdateSortingMethod(sortingCounter);

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("BLUEFIRE1337's Colorbot   "), WS_OVERLAPPEDWINDOW, 0, 0, 400, 500, NULL, NULL, wc.hInstance, NULL);


	HICON hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(MAINICON));

	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

	// Initialize Direct3D
	if(!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX9_Init(g_pd3dDevice);

	ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.10f, 1.00f);

	holdKeyIndex = -1;
	for(size_t i = 0; i < hold_arry_size; i++)
	{
		if(holdKeysCodes[i] == holdKey) {
			holdKeyIndex = i;
			break;
		}
	}

	// Main loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while(msg.message != WM_QUIT)
	{
		if(::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			continue;
		}

		// Start the Dear ImGui frame
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		auto flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;

		{

			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(ImVec2(400, 500), 0);
			ImGui::Begin("Settings", 0, flags);

			ImGui::Text("Setting");
			ImGui::SliderFloat("Speed", &speed, 0.0f, 1.0f);
			ImGui::SliderInt("FovX", &maxX, 50, width / 2);
			ImGui::SliderInt("FovY", &maxY, 50, height / 2);
			ImGui::InputInt2("Offset XY", offset);
			ImGui::InputInt("Full360", &full360);

			ImGui::Text("Flick Aimbot");
			ImGui::Checkbox("Flick", &flickAim);
			ImGui::InputInt("Flick Update ms", &flickAimTime);

			if(flickAimTime < 0) {
				flickAimTime = 0;
			}

			ImGui::Text("Input Settings");
			ImGui::Checkbox("Hold", &isHold);
			if(isHold) {
				ImGui::Checkbox("Invert Hold", &invertHold);
			}

			if(holdKeyIndex > 0) {
				ImGui::Combo(isHold ? "Hold key" : "Toggle Key", &holdKeyIndex, holdKeys, hold_arry_size);
			}
			else {
				ImGui::TextColored(ImVec4(0.4f, 0, 1, 1), "Custom key used: 0x%llX", holdKey);
			}

			ImGui::Text("Sorting Method");

			if(ImGui::Button(currentSortingMethodName)) {
				sortingCounter++;
				UpdateSortingMethod(sortingCounter);
			}
			ImGui::SameLine();
			ImGui::Text(currentSortingMethodDescript);
			if(ImGui::Button("Save Config")) {
				SaveConfig();
			}
			ImGui::SameLine();
			if(ImGui::Button(isRunning ? "Stop colorbot" : "Start colorbot")) {
				isRunning = !isRunning;
			}
			if(isRunning && isReallyRunning) {
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0.4f, 0, 1, 1), "Running");
			}
			if(full360 <= 0) {
				ImGui::Text("TO USE THIS COLORBOT \nFULL360 MUST BE CONFIGURED CORRECTLY\nTHIS IS DIFFERENT FOR ALL COMPUTERS\n\nTHE OPTIMAL SPEED I FOUND OUT TO BE AROUND 0.2\nSO IMO, ONLY CHANGE FULL360\n(FULL360 SHOULD BE AROUND 5000-25000)");
			}
			//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		// Rendering
		ImGui::EndFrame();
		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
		if(g_pd3dDevice->BeginScene() >= 0)
		{
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			g_pd3dDevice->EndScene();
		}
		HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

		// Handle loss of D3D9 device
		if(result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
			ResetDevice();
	}

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
	if((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return false;

	// Create the D3DDevice
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
	//g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
	if(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
		return false;

	return true;
}

void CleanupDeviceD3D()
{
	if(g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
	if(g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
	if(hr == D3DERR_INVALIDCALL)
		IM_ASSERT(0);
	ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch(msg)
	{
	case WM_SIZE:
		if(g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			g_d3dpp.BackBufferWidth = LOWORD(lParam);
			g_d3dpp.BackBufferHeight = HIWORD(lParam);
			ResetDevice();
		}
		return 0;
	case WM_SYSCOMMAND:
		if((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}