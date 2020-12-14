#include "rnimpl.h"

#include <cmath>
#include <cstdio>
#include <ctime>
#include <easyx.h>

namespace EasyX {
	extern HWND g_hConWnd;
}

namespace RnImplement {

	const wchar_t* RnBackendInfo() {
		static wchar_t version[50] = L"EasyX ";
		if (version[10] == '\0') {
			wcscat_s(version, GetEasyXVer());
		}

		return version;
	}

	Font::Font() : impl(new LOGFONT), face(((LOGFONT*)impl)->lfFaceName) {
		((LOGFONT*)impl)->lfQuality = PROOF_QUALITY;
	}

	Font::Font(const Font& f)
		: impl(new LOGFONT(*((LOGFONT*)f.impl))),
		face(((LOGFONT*)impl)->lfFaceName) {}

	Font& Font::operator=(const Font& f) {
		if (&f == this) return *this;

		delete ((LOGFONT*)impl);
		this->impl = new LOGFONT(*((LOGFONT*)f.impl));
		this->face = ((LOGFONT*)impl)->lfFaceName;
		return *this;
	}

	Font::~Font() { delete ((LOGFONT*)impl); }

	int Font::width() {
		return ((LOGFONT*)impl)->lfWidth;
	}

	int Font::height() {
		return ((LOGFONT*)impl)->lfHeight;
	}

	void Font::setWidth(int w) {
		((LOGFONT*)impl)->lfWidth = w;
	}

	void Font::setHeight(int h) {
		((LOGFONT*)impl)->lfHeight = h;
	}

	FontWeight Font::weight() {
		return (FontWeight)((LOGFONT*)impl)->lfWeight;
	}

	void Font::setWeight(FontWeight w) {
		((LOGFONT*)impl)->lfWeight = (LONG)w;
	}

	FontStyle Font::getStyle() {
		return FontStyle(
			(((LOGFONT*)impl)->lfItalic ? (code)FontStyle::italic : 0) |
			(((LOGFONT*)impl)->lfUnderline ? (code)FontStyle::underline : 0) |
			(((LOGFONT*)impl)->lfStrikeOut ? (code)FontStyle::strikeOut : 0));
	}

	void Font::setStyle(FontStyle s) {
		((LOGFONT*)impl)->lfItalic = bool((code)s & (code)FontStyle::italic);
		((LOGFONT*)impl)->lfUnderline = bool((code)s & (code)FontStyle::underline);
		((LOGFONT*)impl)->lfStrikeOut = bool((code)s & (code)FontStyle::strikeOut);
	}

	void ReadFont(Font& f) {
		gettextstyle((LOGFONT*)f.impl);
		((LOGFONT*)f.impl)->lfQuality = PROOF_QUALITY;
	}

	void SetFont(Font& f) {
		settextstyle((LOGFONT*)f.impl);
	}

	Image::Image(int w, int h) : impl(new IMAGE(w, h)) {}
	Image::Image(const Image& img) : impl(new IMAGE(*(IMAGE*)(img.impl))) {}
	Image::~Image() { delete ((IMAGE*)impl); }

	Image& Image::operator=(const Image& i) {
		IMAGE* prevWorkingImage = ::GetWorkingImage();

		float scalex, scaley;
		::SetWorkingImage((IMAGE*)i.impl);
		::getaspectratio(&scalex, &scaley);

		::SetWorkingImage((IMAGE*)impl);
		::setaspectratio(scalex, scaley);

		::SetWorkingImage(prevWorkingImage);

		*((IMAGE*)impl) = *((IMAGE*)i.impl);

		return *this;
	}

	void Image::resize(int w, int h) {
		((IMAGE*)impl)->Resize(w, h);
	}

	int Image::width() {
		return ((IMAGE*)impl)->getwidth();
	}

	int Image::height() {
		return ((IMAGE*)impl)->getheight();
	}

	bool PopMouseEvent(MouseEvent& event) {
		if (::MouseHit()) {
			MOUSEMSG m = ::GetMouseMsg();

			switch (m.uMsg) {
			case WM_MOUSEWHEEL:
				event.type = MouseEventType::wheel;
				break;
			case WM_LBUTTONDOWN:
				event.type = MouseEventType::leftDown;
				break;
			case WM_LBUTTONUP:
				event.type = MouseEventType::leftUp;
				break;
			case WM_MBUTTONDOWN:
				event.type = MouseEventType::middleDown;
				break;
			case WM_MBUTTONUP:
				event.type = MouseEventType::middleUp;
				break;
			case WM_RBUTTONDOWN:
				event.type = MouseEventType::rightDown;
				break;
			case WM_RBUTTONUP:
				event.type = MouseEventType::rightUp;
				break;
			default:
				event.type = MouseEventType::move;
				break;
			}

			float scalex = 1, scaley = 1;
			getaspectratio(&scalex, &scaley);
			event.x = (int)(m.x / scalex);
			event.y = (int)(m.y / scaley);

			event.wheelDistance = m.wheel;

			event.controlKeys = KeyScancode(
				(m.mkCtrl ? (code)KeyScancode::leftCtrl : 0) |
				(m.mkShift ? (code)KeyScancode::shift : 0));

			return true;
		}

		return false;
	}

	bool PopKeyEvent(KeyEvent& event) {
		INPUT_RECORD inRec = {};
		DWORD res;
		HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);

		if (WaitForSingleObject(hInput, 0) == WAIT_OBJECT_0) {
			ReadConsoleInput(hInput, &inRec, 1, &res);
			if (inRec.EventType == KEY_EVENT) {
				event.pressed = inRec.Event.KeyEvent.bKeyDown;
				event.controlKeys = (KeyScancode)
					inRec.Event.KeyEvent.dwControlKeyState;
				event.code = (KeyCode)
					inRec.Event.KeyEvent.wVirtualKeyCode;
				event.ch = inRec.Event.KeyEvent.uChar.UnicodeChar;

				return true;
			}
		}
		return false;
	}

	void InitGraphics() {
		FreeConsole();

		RECT r;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);

		AllocConsole();
		HWND console = GetConsoleWindow();
		SetWindowLong(console, GWL_EXSTYLE, WS_EX_LAYERED);
		SetLayeredWindowAttributes(console, 0, 0, LWA_ALPHA);
		SetWindowPos(console, HWND_BOTTOM,
			r.right, r.bottom, 0, 0, SWP_HIDEWINDOW);

		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
			ENABLE_EXTENDED_FLAGS);
		SetConsoleCtrlHandler(nullptr, true);

		HWND graph = initgraph(0, 0);
		ShowWindow(graph, SW_HIDE);

#if (WINVER >= _WIN32_WINNT_WIN10)
		float scale = GetDpiForWindow(graph) / 96.0f;
		setaspectratio(scale, scale);
#endif
	}

	void OpenGraphics(int w, int h) {
		EasyX::g_hConWnd = 0;
		closegraph();

		srand((unsigned)time(0));
		int n = 5 + rand() % 5;
		RECT r;
		GetWindowRect(GetDesktopWindow(), &r);
		SetWindowPos(GetConsoleWindow(), HWND_BOTTOM,
			(r.right - r.left) / n + r.left,
			(r.bottom - r.top) / n + r.top, 0, 0, SWP_HIDEWINDOW);

		initgraph(w, h);
	}

	void ResizeGraphics(int w, int h) {
		static int stepa[21]{};
		if (stepa[0] == 0) {
			for (int i = 0; i <= 20; i++) {
				stepa[i] = (int)(pow(i - 4.88, 4) / pow(i + 2.33, 2)) + 150;
			}
		}

		int stepx[21], stepy[21];
		int vw = w - getwidth(), vh = h - getheight();
		for (int i = -20; i <= 0; i++) {
			stepx[i + 20] = w + vw * i * i * i / 8000;
			stepy[i + 20] = h + vh * i * i * i / 8000;
		}

		bool underWine = GetProcAddress(
			GetModuleHandle(L"ntdll.dll"), "wine_get_version");
		HWND hwnd = GetHWnd();

		setaspectratio(1, 1);
		if (!underWine) SetWindowLong(hwnd, GWL_EXSTYLE,
			GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

		cleardevice();
		BeginBatchDraw();

		clock_t anitick, framegap;
		for (int i = 0; i < 21; i += framegap) {
			anitick = clock();

			Resize(nullptr, stepx[i], stepy[i]);
			FlushBatchDraw();
			if (!underWine)
				SetLayeredWindowAttributes(hwnd, 0, stepa[i], LWA_ALPHA);

			do {
				Sleep(12 + anitick - clock());
				framegap = (clock() - anitick) / 12;
			} while (framegap <= 0);
		}

		EndBatchDraw();
		if (!underWine) SetWindowLong(hwnd, GWL_EXSTYLE,
			GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
		ShowWindow(hwnd, SW_RESTORE);
		Resize(nullptr, w, h);
	}

	void SetGraphicsTitle(const wchar_t* t) {
		SetWindowText(GetHWnd(), t);
	}

	void CloseGraphics() {
		closegraph();
	}

	int GraphicsWidth() {
		return getwidth();
	}

	int GraphicsHeight() {
		return getheight();
	}

	int GraphicsFramerate() {
		DEVMODE devMode;
		devMode.dmSize = sizeof(devMode);
		if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode)) {
			return devMode.dmDisplayFrequency;
		}
		return -1;
	}

	float GraphicsScale() {
		float scale = 1;
#if (WINVER >= _WIN32_WINNT_WIN10)
		scale = GetDpiForWindow(GetHWnd()) / 96.0f;
		if (scale == 0) scale = 1;
#endif
		return scale;
	}

	void SetGraphicsScale(float f) {
		setaspectratio(f, f);
	}

	void SetTarget(Image* i) {
		SetWorkingImage(i == nullptr ? nullptr : (IMAGE*)i->impl);
	}

	void Reset() {
		graphdefaults();
	}

	void SetBk(Color c) {
		setbkcolor(c);
		setbkmode(TRANSPARENT);
	}

	Color GetBk() {
		return getbkcolor();
	}

	void SetCursor(Cursor c, bool flush) {
		static LPWSTR native[] = {
			IDC_ARROW, IDC_IBEAM, IDC_APPSTARTING, IDC_WAIT,
			IDC_CROSS, IDC_HAND, IDC_HELP, IDC_NO, IDC_SIZEALL,
			IDC_SIZENESW, IDC_SIZENS, IDC_SIZENWSE, IDC_SIZEWE
		};

		SetClassLongPtr(GetHWnd(), -12, (LONG_PTR)LoadCursor(nullptr, native[(int)c]));
		if (flush) {
			POINT cursorPos;
			GetCursorPos(&cursorPos);
			SetCursorPos(cursorPos.x, cursorPos.y);
		}
	}

	void EnterBuffer() {
		BeginBatchDraw();
	}

	void FlushBuffer() {
		FlushBatchDraw();
	}

	void ExitBuffer() {
		EndBatchDraw();
	}

	void Sleep(long l) {
		::Sleep(max(0, l / long(CLOCKS_PER_SEC / 1000.0)));
	}

	void Clear() {
		cleardevice();
	}

	void Clear(int left, int top, int right, int bottom) {
		clearrectangle(left, top, right, bottom);
	}

	void SetFg(Color c) {
		setfillcolor(c);
	}

	Color GetFg() {
		return getfillcolor();
	}

	void SetLineColor(Color c) {
		setlinecolor(c);
	}

	Color GetLineColor() {
		return getlinecolor();
	}

	void SetTextColor(Color c) {
		settextcolor(c);
	}

	Color GetTextColor() {
		return gettextcolor();
	}

	void SetLineStyle(int thickness, LineStyle l) {
		setlinestyle((int)l, thickness);
	}

	void Rectangle(int left, int top, int right, int bottom) {
		::rectangle(left, top, right, bottom);
	}

	void FillRect(int left, int top, int right, int bottom) {
		::solidrectangle(left, top, right, bottom);
	}

	void Line(int x1, int y1, int x2, int y2) {
		::line(x1, y1, x2, y2);
	}

	void PolyBezier(const Point* points, int count) {
		::polybezier((const POINT*)points, count);
	}

	void FillPie(int left, int top, int right, int bottom,
		double stangle, double endangle) {
		::solidpie(left, top, right, bottom, stangle, endangle);
	}

	void FillCircle(int x, int y, int radius) {
		::solidcircle(x, y, radius);
	}

	void Circle(int x, int y, int radius) {
		::circle(x, y, radius);
	}

	void PutText(int x, int y, wchar_t c) {
		::outtextxy(x, y, c);
	}

	void PutText(int x, int y, const wchar_t* str) {
		::outtextxy(x, y, str);
	}

	int TextWidth(const wchar_t* str) {
		return ::textwidth(str);
	}

	int TextWidth(wchar_t c) {
		return ::textwidth(c);
	}

	int TextHeight(const wchar_t* str) {
		return ::textheight(str);
	}

	int TextHeight(wchar_t c) {
		return ::textheight(c);
	}

	Color GetPixel(int x, int y) {
		return ::getpixel(x, y);
	}

	void PutImage(int dstX, int dstY, Image& src) {
		::putimage(dstX, dstX, ((IMAGE*)src.impl));
	}

	void PutImage(int dstX, int dstY, Image& src,
		int cropWidth, int cropHeight, int srcX, int srcY) {
		::putimage(dstX, dstY, cropWidth, cropHeight,
			((IMAGE*)src.impl), srcX, srcY);
	}

	uint32_t* GetPointer(Image* img) {
		return (uint32_t*) ::GetImageBuffer(
			img == nullptr ? nullptr : (IMAGE*)img->impl);
	}

	void LoadImg(Image& img, const wchar_t* name, const wchar_t* group) {
		::loadimage(((IMAGE*)img.impl), name);
	}

	void LoadIco(Image& img, const wchar_t* name, bool fromOS) {
		HICON hIco = (HICON)LoadImage(
			fromOS ? 0 : GetModuleHandle(0), name,
			IMAGE_ICON, 256, 256, LR_DEFAULTCOLOR);

		((IMAGE*)img.impl)->Resize(0, 0);
		((IMAGE*)img.impl)->Resize(256, 256);
		DrawIconEx(GetImageHDC((IMAGE*)img.impl), 0, 0, hIco,
			256, 256, 0, nullptr, DI_NORMAL);

		DestroyIcon(hIco);
	}

	void SetClipboardText(const wchar_t* str, size_t len) {
		if (!OpenClipboard(nullptr)) return;

		HGLOBAL hmem = GlobalAlloc(GHND, (len + 1) * sizeof(wchar_t));
		if (hmem == nullptr) return;
		wchar_t* pmem = (wchar_t*)GlobalLock(hmem);

		if (pmem != nullptr) {
			EmptyClipboard();
			pmem[len] = '\0';
			memcpy(pmem, str, len * sizeof(wchar_t));

			SetClipboardData(CF_UNICODETEXT, hmem);
		}

		CloseClipboard();
		GlobalUnlock(hmem);
		GlobalFree(hmem);
	}

	void ReadClipboardText(wchar_t* buffer, size_t capacity) {
		if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) return;
		if (!OpenClipboard(nullptr)) return;

		HGLOBAL hmem = GetClipboardData(CF_UNICODETEXT);

		if (hmem != NULL) {
			wchar_t* pmem = (wchar_t*)GlobalLock(hmem);
			if (pmem != nullptr) wcscpy_s(buffer, capacity, pmem);
			GlobalUnlock(hmem);
		}

		CloseClipboard();
	}
};