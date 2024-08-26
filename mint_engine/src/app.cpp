#if defined(_WIN32)
#include "app.h"
#include "input.h"
#include <Windows.h>
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_opengl3.h"
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "opengl32.lib")


#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001

static int _app_exit = 1;
static bool _app_is_active = false;
static HWND _hwnd = NULL;
static HDC _hdc = {0};
static HGLRC _hglrc = {0};
static vec2 _app_window_size;
static void(*_app_resize_cb)(int width, int height) = 0;

static struct ctx_app {
	struct {
		int x;
		int y;
		int w;
		int h;
		bool fullscreen;
	} window;
} ctx = {};

// imgui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if(ImGui_ImplWin32_WndProcHandler(hwnd, Msg, wParam, lParam))
		return true;

	// skip mouse event
	//if(ImGui::GetCurrentContext() != nullptr){
	//	auto& io = ImGui::GetIO();
	//	if(io.WantCaptureMouse || io.WantCaptureKeyboard) return true;
	//}

	switch (Msg)
	{
	case WM_CREATE:
	{
		_hdc = GetDC(hwnd);
		PIXELFORMATDESCRIPTOR pfd = {0};
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 24;
		pfd.cStencilBits = 8;
		pfd.iLayerType = PFD_MAIN_PLANE;

		int pixelformat = ChoosePixelFormat(_hdc, &pfd);
		if(pixelformat == 0)
			return 0;
		if(!SetPixelFormat(_hdc, pixelformat, &pfd))
			return 0;

		_hglrc = wglCreateContext(_hdc);
		if(!_hglrc)
			return 0;

		wglMakeCurrent(_hdc, _hglrc);

		HGLRC (WINAPI *wglCreateContextAttribsARB)(HDC, HGLRC, const int*) = (HGLRC(WINAPI*)(HDC, HGLRC, const int*))wglGetProcAddress("wglCreateContextAttribsARB");
		int gl_attribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 6,
			WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0,
		};

		HGLRC gl_ctx = wglCreateContextAttribsARB(_hdc, 0, gl_attribs);
		if(gl_ctx)
		{
			BOOL b = wglMakeCurrent(_hdc, gl_ctx);
			wglDeleteContext(_hglrc);
			_hglrc = gl_ctx;
		}

		SendMessage(hwnd, WM_PAINT, 0, 0);
	}
	return 0;

	case WM_ACTIVATE:
		if(wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE)
		{
			_app_is_active = true;
		} else if(wParam == WA_INACTIVE) {
			_app_is_active = false;
		}
		break;

	// Keyboard
	case WM_KEYDOWN:
		input_set_key_state((int)wParam, 1);
		return 0;

	case WM_KEYUP:
		input_set_key_state((int)wParam, 0);
		return 0;

	case WM_SYSKEYDOWN:
		if(wParam == VK_RETURN && (HIWORD(lParam) & KF_ALTDOWN)) {
			app_fullscreen(!app_is_fullscreen());
			return 0;
		}
		else if(wParam == VK_MENU) {
			input_set_key_state(KEY_ALT, 1);
			return 0;
		}
		break;

	case WM_SYSKEYUP:
		if(wParam == VK_MENU) {
			input_set_key_state(KEY_ALT, 0);
			return 0;
		}
		break;

	case WM_SYSCOMMAND:
		// disable window beep sound
		if(wParam == SC_KEYMENU) {
			return 0;
		}
		break;

		// MOUSE
	case  WM_LBUTTONDOWN:
		input_set_key_state(KEY_MOUSE0, 1);
		return 0;

	case  WM_RBUTTONDOWN:
		input_set_key_state(KEY_MOUSE1, 1);
		return 0;

	case  WM_MBUTTONDOWN:
		input_set_key_state(KEY_MOUSE2, 1);
		return 0;

	case WM_XBUTTONDOWN:
		switch(GET_KEYSTATE_WPARAM(wParam)){
		case MK_XBUTTON1:
			input_set_key_state(KEY_MOUSE3, 1);
			return TRUE;
		case MK_XBUTTON2:
			input_set_key_state(KEY_MOUSE4, 1);
			return TRUE;
		}
		break;

	case  WM_LBUTTONUP:
		input_set_key_state(KEY_MOUSE0, 0);
		return 0;

	case  WM_RBUTTONUP:
		input_set_key_state(KEY_MOUSE1, 0);
		return 0;

	case  WM_MBUTTONUP:
		input_set_key_state(KEY_MOUSE2, 0);
		return 0;

	case WM_XBUTTONUP:
		switch(GET_KEYSTATE_WPARAM(wParam)){
		case MK_XBUTTON1:
			input_set_key_state(KEY_MOUSE3, 0);
			return TRUE;
		case MK_XBUTTON2:
			input_set_key_state(KEY_MOUSE4, 0);
			return TRUE;
		}
		break;

	case WM_MOUSEWHEEL:
		input_set_mouse_wheel((int)GET_WHEEL_DELTA_WPARAM(wParam));
		return 0;


	case WM_SIZE:
	{
		int w = LOWORD(lParam); int h = HIWORD(lParam);
		if(_app_resize_cb){_app_resize_cb(w, h);}
		_app_window_size = vec2((float)w, (float)h);
		return 0;
	}
	case WM_CLOSE:
		// uninit imgui
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		DestroyWindow(hwnd);
		_app_exit = 1;
		return 0;
	case WM_DESTROY:
		//wglMakeCurrent(0, 0);
		//wglDeleteContext(_hglrc);
		ReleaseDC(hwnd, _hdc);
		PostQuitMessage(0);
		return 0;
	case WM_QUIT:
		return 0;
	}
	return DefWindowProc(hwnd, Msg, wParam, lParam);
}

void app_create(int width, int height, const char *title)
{
	HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
	WNDCLASSEXW wnd = { 0 };
	wnd.cbSize = sizeof(wnd);
	wnd.style = CS_HREDRAW | CS_VREDRAW;
	wnd.lpfnWndProc = wnd_proc;
	wnd.hInstance = hInstance;
	wnd.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
	wnd.lpszClassName = L"retron";
	wnd.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	RegisterClassExW(&wnd);

	// calc window size with client size
	RECT rc = {0, 0, width, height};
	DWORD window_style = WS_OVERLAPPEDWINDOW|WS_VISIBLE;
	AdjustWindowRect(&rc, window_style, FALSE);

	const char *window_name = title != NULL ? title : "";
	_hwnd = CreateWindowExA(WS_EX_ACCEPTFILES, "retron", window_name, window_style, CW_USEDEFAULT, CW_USEDEFAULT, rc.right-rc.left, rc.bottom-rc.top, NULL, NULL, hInstance, 0);
	_app_exit = 0;
	_app_window_size = vec2((float)width, (float)height);

	input_init();


	// init imgui
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
	ImGui_ImplWin32_InitForOpenGL(_hwnd);
	ImGui_ImplOpenGL3_Init();
}

bool app_process()
{
	input_update();
	time_update();

	// imgui 
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Message
	MSG msg;
	msg.message = WM_NULL;
	while(0 != PeekMessageW(&msg, NULL, 0U, 0U, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	// sleep
	timeBeginPeriod(1);
	Sleep(1);
	timeEndPeriod(1);

	return !_app_exit;
}

void app_swap_buffer()
{
	// rendering imgui
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// swap buffer
	wglMakeCurrent(_hdc, _hglrc);
	SwapBuffers(_hdc);
}

void app_swap_interval(int interval)
{
	BOOL (WINAPI *wglSwapIntervalEXT)(int) = NULL;
	wglMakeCurrent(_hdc, _hglrc);
	wglSwapIntervalEXT = (BOOL (WINAPI*)(int))wglGetProcAddress("wglSwapIntervalEXT");
	if(wglSwapIntervalEXT)
	{
		wglSwapIntervalEXT(interval);
	}
}

void app_quit()
{
	PostMessage(_hwnd, WM_CLOSE, 0, 0 );
}

bool app_is_active()
{
	return _app_is_active;
}

void app_fullscreen(bool fullscreen)
{
	if(fullscreen)
	{
		RECT rw;
		GetWindowRect(_hwnd, &rw);
		ctx.window.x = rw.left;
		ctx.window.y = rw.top;
		ctx.window.w = rw.right - rw.left;
		ctx.window.h = rw.bottom - rw.top;
		SetWindowLong(_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(_hwnd, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED);
	}
	else
	{
		SetWindowLong(_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW	| WS_VISIBLE);
		SetWindowPos(_hwnd, HWND_TOPMOST, ctx.window.x, ctx.window.y, ctx.window.w, ctx.window.h, SWP_FRAMECHANGED);
	}
	ctx.window.fullscreen = fullscreen;
}

bool app_is_fullscreen()
{
	return ctx.window.fullscreen;
}

void app_set_resize_cb(void(*resize_cb)(int width, int height))
{
	_app_resize_cb = resize_cb;
}


void app_set_size()
{
	//_app_window_size = 
}

vec2 app_get_size(){return _app_window_size;}

void* app_get_context(){return (void*)&_hwnd;}







// Time
//static LARGE_INTEGER qpf;
//static LARGE_INTEGER qpc;
static double _start_time = 0;
static double _cur_time = 0;
static double _last_time = 0;
static double _delta_time = 0;

static float _fps = 0.0f;
static float _fps_samples[60];
static int _fps_count = 0;

void time_update()
{
	LARGE_INTEGER qpf;
	LARGE_INTEGER qpc;
	QueryPerformanceFrequency(&qpf);
	QueryPerformanceCounter(&qpc);
	if(_start_time == 0.0)
	{
		// init time
		_start_time = (double)qpc.QuadPart/(double)qpf.QuadPart;
		_cur_time = _start_time;
	}

	_last_time = _cur_time;
	_cur_time = (double)qpc.QuadPart/(double)qpf.QuadPart;
	_delta_time = (_cur_time - _last_time) * 1.0;// scaled
												 //_delta_time = fminf((float)_delta_time, 0.0333f);	// cap delta time

												 // FPS
	_fps_samples[_fps_count++] = _delta_time > 0.0f ? 1.0f/(float)_delta_time : 0.0f;
	if(_fps_count >= 60)
	{
		_fps = 0.0f;
		for(int i=0; i<60; i++)
		{
			_fps += _fps_samples[i];
		}
		_fps = _fps / 60.0f;
		_fps_count = 0;
	}
}

float time_now(){return (float)(_cur_time - _start_time);}

float time_last(){return (float)(_last_time - _start_time);}

float time_dt(){return (float)_delta_time;}

int app_get_fps(){return (int)_fps;}
#endif // _WIN32