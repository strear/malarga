#ifndef RNVNTN_H
#define RNVNTN_H

#include <atomic>
#include <functional>
#include <list>
#include <map>
#include <mutex>

#include "rnimpl.h"
#include "rncrtdef.h"

namespace Rnvntn {
	using namespace RnImplement;

	class RnWindow;

	class RnWidget
	{
	private:
		int width = 0, height = 0;
		bool enabled = true;
		RnWidget(RnWidget&);

	protected:
		RnWindow* parent = nullptr;
		RnWidget() {}
		~RnWidget() {}

	public:
		static void InitGraphics();

		struct Signpost {
			RnWidget* ref;
			const short direction;
		};

		const Signpost left = { this, 0 }, right = { this, 1 },
			up = { this, 2 }, down = { this, 3 };

		bool compositeBk = false;
		Color bk = 0xf8f5f5;

		bool isEnabled();
		virtual void setEnabled(bool);
		virtual int getWidth();
		virtual int getHeight();
		virtual void setWidth(int);
		virtual void setHeight(int);
		virtual void setSize(int width, int height);

		bool windowVisible();
		RnWindow* getParent();
		virtual void updateParent(RnWindow* newparent);

		virtual void update(bool active = false, bool pressed = false) {}
		virtual bool interactable() { return false; }
		virtual bool needFocus() { return interactable(); }
		virtual void clicked() {}
		virtual void mousehit(MouseEvent&) {}
		virtual void kbhit(KeyEvent&) {}
	};

	class RnWindow :
		public RnWidget
	{
	protected:
		static RnWindow* VisibleOne;

		std::recursive_mutex widgetMutex;

		std::list<RnWidget*> widgetOrder;
		std::map<RnWidget*, Point> widgetPos;
		RnWidget* activeOne = nullptr, * pressedOne = nullptr;

		unsigned int framerate = 50;
		float scale = 1;
		std::function<void()>* loophook = nullptr;
		RnWindow* prevVisibleOne = nullptr;
		std::list<RnWidget*> animators;
		clock_t tic = 0;

		void checkfocus(RnWidget* prevActiveOne, RnWidget* prevPressedOne);
		void findpos(
			int& x, int& y, RnWidget& w,
			RnWidget::Signpost& place, int margin, int shift);

	public:
		static RnWindow* GetVisibleOne();

		std::atomic<bool> threadSafe;

		const wchar_t* title = nullptr;
		RnWidget* kbdListener = nullptr;
		bool navigate = true, react = true;

		RnWindow(int width, int height, const wchar_t* title = nullptr);
		~RnWindow();

		void setLoopHook(std::function<void()>*);
		void loop();
		void close();
		void hide();
		void resume();

		float getScale();
		int getFrameRate();
		void setFrameRate(unsigned int);

		clock_t tick();
		void animateBegin(RnWidget&);
		void animateEnd(RnWidget&);

		RnWindow* getParent() { return nullptr; }
		void updateParent(RnWindow*) {}
		bool windowVisible();

		bool contain(RnWidget&);
		void put(RnWidget&, RnWidget::Signpost place = { nullptr, 3 },
			int margin = 15, int shift = INT_MIN);
		void put(RnWidget&, int x, int y);
		bool fit(RnWidget&, RnWidget::Signpost place,
			int margin = 15, int shift = INT_MIN);
		void drawPart(RnWidget&);
		int getX(RnWidget&);
		int getY(RnWidget&);
		void clarify(RnWidget&);
		void freshPaint(int x, int y, int w, int h,
			std::list<RnWidget*>* ignores = nullptr);
		void remove(RnWidget&);

		std::recursive_mutex& getMutex();
		std::list<RnWidget*>::const_iterator widgetsBegin();
		std::list<RnWidget*>::const_iterator widgetsEnd();
		std::list<RnWidget*>::const_iterator animatorsBegin();
		std::list<RnWidget*>::const_iterator animatorsEnd();
		bool isActive(RnWidget*);
		bool isPressed(RnWidget*);

		void update(bool active, bool pressed);
		void mousehit(MouseEvent& m);
		void kbhit(KeyEvent& k);
	};

	class RnTextWidget :
		public RnWidget
	{
	protected:
		Font font;

	public:
		static const wchar_t* const default_fontface;

		Color fg = 0x0;

		RnTextWidget();
		void setFont(Font&);
	};

	class RnButton :
		public RnTextWidget
	{
	protected:
		const wchar_t* label;
		std::function<void()>* clickhook = nullptr;

	public:
		RnButton(const wchar_t* label, bool icon = false);
		~RnButton();

		const wchar_t* getLabel();
		void setLabel(const wchar_t*);
		void setLabel(const wchar_t*, bool icon);
		void update(bool active, bool pressed);
		bool interactable();

		void setClickHook(std::function<void()>*);
		void clicked();

		static void CloseWindow();
	};

	class RnLabel :
		public RnTextWidget
	{
	protected:
		const wchar_t* label = nullptr;

	public:
		RnLabel() : RnLabel(nullptr) {}
		RnLabel(const wchar_t* label, bool caption = false, Color color = 0x0);
		virtual ~RnLabel() {}

		const wchar_t* getLabel();
		void setLabel(const wchar_t*);
		void update(bool active, bool pressed);
	};

	class RnCombo :
		public RnTextWidget
	{
	protected:
		const wchar_t* label;
		std::list<RnCombo*>* excludes = nullptr;

	public:
		bool selected;

		RnCombo(const wchar_t* label, std::list<RnCombo*>* exclude = nullptr);

		const wchar_t* getLabel();
		void setLabel(const wchar_t*);
		void update(bool active, bool pressed);
		bool interactable();
		void clicked();

		void setExclude(std::list<RnCombo*>*);
	};

	class RnImage :
		public RnWidget
	{
	protected:
		Image* img = nullptr;

	public:
		RnImage() {}
		RnImage(const wchar_t* path, int width = 0, int height = 0);
		RnImage(Image&);
		~RnImage();

		Image* getImage();
		bool setImage(const wchar_t*, int width = 0, int height = 0);
		bool setImage(Image&);
		void update(bool active, bool pressed);
	};

	class RnHourglass :
		public RnWidget
	{
	public:
		void setWidth(int);
		void setHeight(int);
		int getWidth();
		int getHeight();

		void setEnabled(bool);
		void updateParent(RnWindow* newparent);

		void update(bool, bool);
	};

	class RnTextbox :
		public RnTextWidget
	{
	private:
		wchar_t* text = nullptr;
		float* offsets = nullptr;
		int caret = 0, select = 0, selectLength = 0;
		bool changed = true, selecting = false;
		unsigned int lenlim;
		std::function<void()>* changehook = nullptr, * enterhook = nullptr;
		Image canvas;
		int choffset = 0, pxoffset = 0;

		void refresh();
		void refreshCharBg(int x0, int y0, int w, bool reverse);
		void refreshChar(wchar_t ch, int x0, int y0, bool reverse);
		void paintCaret(int caret,
			int x0, int y0, int w0, int h0, int x, int y, int chw, bool on);
		void refreshCaret(int);
		void putCaret(int);
		void checkOffsets(int end = 0);
		void moveView(int);
		void newSelect(int);
		void checkSelection();
		void delSelection();
		void putChar(wchar_t);

	public:
		static void trim(wchar_t*);

		RnTextbox(const wchar_t* text, const int lenlim = 20);
		RnTextbox() : RnTextbox(nullptr) {}
		~RnTextbox();

		void getText(wchar_t* t);
		void setText(const wchar_t* text);
		void setChangeHook(std::function<void()>*);
		void setEnterHook(std::function<void()>*);
		void update(bool active, bool pressed);

		void updateParent(RnWindow* parent);
		bool interactable();
		void mousehit(MouseEvent& m);
		void kbhit(KeyEvent& k);

		void copy();
		void paste();
	};
}

#endif // RNVNTN_H