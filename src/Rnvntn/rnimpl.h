#ifndef IMPL_H
#define IMPL_H

#include <cstdint>
#include <cstddef>

namespace RnImplement {
	const wchar_t* RnBackendInfo();

	using code = unsigned long;

	using Color = unsigned long;

	struct Point {
		long x, y;
	};

	enum class KeyCode : code {
		mouseLeft = 0x01, mouseRight = 0x02,
		cancel = 0x03, mouseMiddle = 0x04,
		x1 = 0x05, x2 = 0x06,
		back = 0x08, tab = 0x09,
		clear = 0x0c, enter = 0x0d,
		shift = 0x10, ctrl = 0x11,
		alt = 0x12, pause = 0x13, caps = 0x14,
		esc = 0x1b, space = 0x20,
		pageUp = 0x21, pageDown = 0x22, end = 0x23, home = 0x24,
		left = 0x25, up = 0x26, right = 0x27, down = 0x28,
		prtSc = 0x2c, insert = 0x2d, del = 0x2e,
		leftSuper = 0x5b, rightSuper = 0x5c, menu = 0x5d,
		numpad0 = 0x60,
		numpad1 = 0x61, numpad2 = 0x62, numpad3 = 0x63,
		numpad4 = 0x64, numpad5 = 0x65, numpad6 = 0x66,
		numpad7 = 0x67, numpad8 = 0x68, numpad9 = 0x69,
		f1 = 0x70, f2 = 0x71, f3 = 0x72, f4 = 0x73,
		f5 = 0x74, f6 = 0x75, f7 = 0x76, f8 = 0x77,
		f9 = 0x78, f10 = 0x79, f11 = 0x80, f12 = 0x81,
		numLock = 0x90, scroll = 0x91
	};

	enum class KeyScancode : code {
		rightAlt = 0x01,
		leftAlt = 0x02,
		rightCtrl = 0x04,
		leftCtrl = 0x08,
		shift = 0x10,
		numLock = 0x20,
		scrollLock = 0x40,
		capsLock = 0x80
	};

	struct KeyEvent {
		bool pressed;
		KeyScancode controlKeys;
		KeyCode code;
		wchar_t ch;
	};

	enum class MouseEventType {
		move, wheel,
		leftDown, leftUp,
		middleDown, middleUp,
		rightDown, rightUp
	};

	struct MouseEvent {
		MouseEventType type;
		int x, y;
		KeyScancode controlKeys;
		int wheelDistance;
		int order = 0, total = 1;
	};

	class Font;
	void ReadFont(Font&);
	void SetFont(Font&);

	enum class FontWeight : code {
		undefined = 0,
		thin = 100,
		extraLight = 200, ultraLight = 200,
		light = 300,
		normal = 400, regular = 400,
		medium = 500,
		semiBold = 600, demiBold = 600,
		bold = 700,
		extraBold = 800, ultraBold = 800,
		heaby = 900, black = 900
	};

	enum class FontStyle : code {
		italic = 0x01, underline = 0x02, strikeOut = 0x04
	};

	class Font {
		void* impl;

	public:
		wchar_t* face;
		Font();
		Font(const Font&);
		~Font();

		Font& operator=(const Font&);

		int width(), height();
		void setWidth(int), setHeight(int);
		FontWeight weight();
		void setWeight(FontWeight);
		FontStyle getStyle();
		void setStyle(FontStyle);

		friend void ReadFont(Font&);
		friend void SetFont(Font&);
	};

	class Image;
	void SetTarget(Image* = nullptr);
	void PutImage(int dstX, int dstY, Image& src);
	void PutImage(int dstX, int dstY, Image& src,
		int cropWidth, int cropHeight, int srcX, int srcY);
	void LoadImg(Image&, const wchar_t* name, const wchar_t* group = nullptr);
	void LoadIco(Image&, const wchar_t* name, bool fromOS = false);
	uint32_t* GetPointer(Image* = nullptr);

	class Image {
		void* impl;

	public:
		Image(int w = 0, int h = 0);
		Image(const Image&);
		~Image();
		void resize(int w, int h);
		int width();
		int height();

		Image& operator=(const Image&);

		friend void SetTarget(Image*);
		friend void PutImage(int, int, Image&);
		friend void PutImage(int, int, Image&, int, int, int, int);
		friend void LoadImg(Image&, const wchar_t*, const wchar_t*);
		friend void LoadIco(Image&, const wchar_t*, bool);
		friend uint32_t* GetPointer(Image*);
	};

	enum class Cursor {
		arrow, ibeam, backgroundWait, wait, cross, hand, help, forbidden,
		arrowCross, arrowNESW, arrowNS, arrowNWSE, arrowWE, arrowUp
	};

	void InitGraphics();
	void OpenGraphics(int w, int h);
	void ResizeGraphics(int w, int h);
	void SetGraphicsTitle(const wchar_t*);
	void SetGraphicsScale(float);
	void CloseGraphics();

	int GraphicsWidth();
	int GraphicsHeight();
	float GraphicsScale();
	int GraphicsFramerate();

	void Reset();
	void SetCursor(Cursor, bool flush = false);

	void EnterBuffer();
	void FlushBuffer();
	void ExitBuffer();

	bool PopMouseEvent(MouseEvent&);
	bool PopKeyEvent(KeyEvent&);

	void Sleep(long);

	void Clear();
	void Clear(int left, int top, int right, int bottom);

	void SetBk(Color);
	Color GetBk();
	void SetFg(Color);
	Color GetFg();
	void SetLineColor(Color);
	Color GetLineColor();
	void SetTextColor(Color);
	Color GetTextColor();

	enum class LineStyle {
		solid, dash, dot, dashdot, dashdotdot
	};

	void SetLineStyle(int thickness, LineStyle = LineStyle::solid);

	void Rectangle(int left, int top, int right, int bottom);
	void FillRect(int left, int top, int right, int bottom);

	void Line(int x1, int y1, int x2, int y2);
	void PolyBezier(const Point* points, int count);

	void FillPie(int left, int top, int right, int bottom,
		double stangle, double endangle);
	void FillCircle(int x, int y, int radius);
	void Circle(int x, int y, int radius);

	void PutText(int x, int y, wchar_t);
	void PutText(int x, int y, const wchar_t*);
	int TextWidth(const wchar_t*);
	int TextWidth(wchar_t);
	int TextHeight(const wchar_t*);
	int TextHeight(wchar_t);

	void SetClipboardText(const wchar_t*, size_t len);
	void ReadClipboardText(wchar_t* buffer, size_t capacity);

	Color GetPixel(int x, int y);
}

#endif // IMPL_H