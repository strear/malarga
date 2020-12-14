#include <cmath>

#include "malarga_gui.h"

using namespace Malarga;

const wchar_t* const ArenaView::fruit = L"🌶🍄🍒🍎🍓🍉";
const Color ArenaView::fruit_colors[6] = { 0x0000ff, 0x4f70b1, 0x1515a3,
											 0x1c70f8, 0xffaebe, 0x4cb122 };
const Color ArenaView::bk_colors[5] = { 0xf3e3da, 0xe2f0d9, 0xd6e5fb,
										  0xe0daf0, 0xf3dbe1 };
const Color ArenaView::player_colors[5] = { 0xc47244, 0x70ad47, 0x317ded,
											  0xc35585, 0x8a44c4 };
const Color ArenaView::player_colors_light[5] = {
	0xddae92, 0xb2d69a, 0x7cacf4, 0xda95b3, 0xbb92dd };

ArenaView::ArenaView(ArenaModel& m)
	: model(m),
	wui(680, 480, title),
	doublebuf(0, 0),
	btnpause(MsgViewProvider::Msg("pause")),
	btnend(MsgViewProvider::Msg("end")),
	msgbuf(0, 0),
	msgbufprev(0, 0) {
	wui.put(btnend, 645 - btnend.getWidth(), 32);
	wui.setFrameRate(80);

	std::function<void()> hook(std::bind(&ArenaView::quit, this));
	btnend.setClickHook(&hook);

	hook = std::bind(&ArenaView::togglePause, this);
	btnpause.setClickHook(&hook);

	hook = std::bind(&ArenaView::newFrame, this);
	wui.setLoopHook(&hook);
	wui.react = false;
}

void ArenaView::start(int mode, RecordManager* record) {
	this->reset();

	model.ready(mode, record, this);

	highscore = record->highestScore(model.playerOf(0)->getName(),
		model.isHost() ? mode : 3);

	swprintf(title, 40, MsgViewProvider::Msg("title-gaming"),
		model.playerOf(0)->getName(),
		!model.isHost()
		? MsgViewProvider::Msg("online")
		: mode == 0
		? MsgViewProvider::Msg("starter-level")
		: mode == 1 ? MsgViewProvider::Msg("advanced-level")
		: MsgViewProvider::Msg("premium-level"));

	btnpause.setEnabled(false);

	wui.loop();

	model.end();
}

void ArenaView::togglePause() {
	if (!btnpause.isEnabled()) return;
	model.setPaused(!model.isPaused());
	pauseToggled = true;
}

void ArenaView::lighten(int x, int y) {
	lightens[x][y] = true;
	lightentick = clock();
}

namespace {
#define val(n, x) ((unsigned char*)(&x))[n]
	void _HsvToRgbFast(Color& original) {
		unsigned char
			& h = val(0, original),
			& s = val(1, original),
			& v = val(2, original);

		if (s == 0) {
			h = v; s = v;
			return;
		}

		unsigned char region, remainder, p, q, t;

		region = h / 43;
		remainder = (h - (region * 43)) * 6;

		p = (v * (255 - s)) >> 8;
		q = (v * (255 - ((s * remainder) >> 8))) >> 8;
		t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

		switch (region) {
		case 0:
			h = v; s = t; v = p;
			break;
		case 1:
			h = q; s = v; v = p;
			break;
		case 2:
			h = p; s = v; v = t;
			break;
		case 3:
			h = p; s = q;
			break;
		case 4:
			h = t; s = p;
			break;
		default:
			h = v; s = p; v = q;
			break;
		}
	}

	void _RgbToHsvFast(Color& original) {
		unsigned char
			& r = val(0, original),
			& g = val(1, original),
			& b = val(2, original);

		unsigned char rgbMin, v;
		rgbMin = r < g ? (r < b ? r : b) : (g < b ? g : b);
		v = r > g ? (r > b ? r : b) : (g > b ? g : b);

		if (v == 0) return;
		if (v == rgbMin) {
			r = 0; g = 0;
			return;
		}

		unsigned char s = 255 * long(v - rgbMin) / v;
		if (s == 0) {
			r = 0; b = v;
			return;
		}

		if (v == r)
			r = 0 + 43 * (g - b) / (v - rgbMin);
		else if (v == g)
			r = 85 + 43 * (b - r) / (v - rgbMin);
		else
			r = 171 + 43 * (r - g) / (v - rgbMin);

		g = s; b = v;
	}
}

void ArenaView::_lightenbk(Color c, float amount) {
	_RgbToHsvFast(c);
	val(2, c) -= (unsigned char)(255 * std::min(1.0f, std::max(.0f, amount)));
	_HsvToRgbFast(c);
	SetFg(c);
}
#undef val

void ArenaView::paintbg() {
	if (clock() - lightentick > CLOCKS_PER_SEC) {
		memset(lightens, false, sizeof(lightens));
	}

	Clear(5, 59, 680, 467);

	SetLineColor(0xffffff);
	SetLineStyle(3);
	Rectangle(33, 88, 648, 439);

	float amount =
		((float)CLOCKS_PER_SEC + lightentick - clock()) / CLOCKS_PER_SEC / 3;
	int xi, yi;

	for (int j = 0; j < 9; j++) {
		for (int i = 0; i < 16; i++) {
			xi = 38 + 38 * i;
			yi = 94 + 38 * j;

			if (j % 2) {
				if (lightens[i][j])
					_lightenbk(GetBk(), amount);
				else
					continue;
			} else {
				if (lightens[i][j])
					_lightenbk(bk_colors[j / 2 % 5], amount);
				else
					SetFg(bk_colors[j / 2 % 5]);
			}

			FillRect(xi, yi, xi + 34, yi + 34);
		}
	}
}

void ArenaView::paintSnakes(int offset) {
	SetLineStyle(8);

	Font font;
	ReadFont(font);
	wcscpy_s(font.face, 30, RnTextWidget::default_fontface);
	font.setHeight(16);
	font.setWeight(FontWeight::demiBold);
	SetFont(font);

	for (int n = 0; n < model.playerCount(); n++) {
		if (model.playerOf(n)->isDead()) continue;

		std::vector<position>& points = model.positionList(n);
		if (points.size() <= 1) continue;

		int toward = model.playerOf(n)->toward();
		position head = model.playerOf(n)->next();

		position* Linepos = new position[points.size() + 1];
		int i = 0;

		if (i + 1 < (int)points.size()) {
			*(Linepos + i) = { int(57 + 38 * points[i].x +
								  38 * (points[(size_t)i + 1].x - points[i].x) *
									  offset / CLOCKS_PER_SEC),
							  int(113 + 38 * points[i].y +
								  38 * (points[(size_t)i + 1].y - points[i].y) *
									  offset / CLOCKS_PER_SEC) };
			i++;
		}

		while (i < (int)points.size()) {
			*(Linepos + i) = { 57 + 38 * points[i].x, 113 + 38 * points[i].y };
			i++;
		}

		*(Linepos + i) = {
			int(57 + 38 * points[(size_t)i - 1].x +
				38 * (head.x - points[(size_t)i - 1].x) * offset / CLOCKS_PER_SEC),
			int(113 + 38 * points[(size_t)i - 1].y +
				38 * (head.y - points[(size_t)i - 1].y) * offset / CLOCKS_PER_SEC) };

		int endx = (Linepos + i)->x, endy = (Linepos + i)->y;
		int eyex, eyey;
		double rotate;

		if (toward == 0) {
			rotate = -1.57;
			(Linepos + i)->y += 16;
			if ((Linepos + i - 1)->y <= (Linepos + i)->y)
				(Linepos + i - 1)->y = (Linepos + i)->y;
			endy += 15;
			eyex = endx - 4;
			eyey = endy + 5;
		} else if (toward == 1) {
			rotate = 1.57;
			(Linepos + i)->y -= 16;
			if ((Linepos + i - 1)->y >= (Linepos + i)->y)
				(Linepos + i - 1)->y = (Linepos + i)->y;
			endy -= 15;
			eyex = endx + 4;
			eyey = endy - 5;
		} else if (toward == 2) {
			rotate = 0;
			(Linepos + i)->x += 16;
			if ((Linepos + i - 1)->x <= (Linepos + i)->x)
				(Linepos + i - 1)->x = (Linepos + i)->x;
			endx += 15;
			eyex = endx + 4;
			eyey = endy - 5;
		} else {
			rotate = 3.14;
			(Linepos + i)->x -= 16;
			if ((Linepos + i - 1)->x >= (Linepos + i)->x)
				(Linepos + i - 1)->x = (Linepos + i)->x;
			endx -= 15;
			eyex = endx - 4;
			eyey = endy - 5;
		}

		Color c = player_colors[n % 5];

		Point* curve = new Point[(size_t)i * 3 - 2]{};
		for (size_t j = 0; j < (size_t)i; j++) {
			(curve + 3 * j)->x = ((Linepos + j)->x + (Linepos + j + 1)->x) / 2;
			(curve + 3 * j)->y = ((Linepos + j)->y + (Linepos + j + 1)->y) / 2;
		}
		for (int n = 1; n <= 2; n++) {
			for (size_t j = 0; j < (size_t)i - 1; j++) {
				memcpy((curve + 3 * j + n), (Linepos + j + 1), sizeof(Point));
			}
		}

		SetLineColor(player_colors_light[n % 5]);
		Line(57 + 38 * points[0].x, 113 + 38 * points[0].y, Linepos->x,
			Linepos->y);

		SetLineColor(c);
		Line(Linepos->x, Linepos->y, (Linepos->x + (Linepos + 1)->x) / 2,
			(Linepos->y + (Linepos + 1)->y) / 2);

		PolyBezier(curve, i * 3 - 2);

		delete[] curve;

		SetFg(c);
		FillPie(endx - 10, endy - 10, endx + 10, endy + 10, rotate - 2.7,
			rotate + 2.7);
		SetFg(0xffffff);
		FillCircle(eyex, eyey, 2);

		wchar_t ch[4];
		swprintf(ch, 3, L"%d", i);
		SetTextColor(c);
		PutText(endx + 10, endy - 18, ch);

		delete[] Linepos;
	}
}

void ArenaView::paintFruits() {
	Font font;
	ReadFont(font);
	wcscpy_s(font.face, 30, L"Segoe UI Emoji");
	font.setHeight(28);
	font.setWeight(FontWeight::ultraBold);
	SetFont(font);

	SetLineColor(0xa5a5a5);
	SetLineStyle(8);

	wchar_t emojiext[3]{ 55356, 0, 0 };

	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 9; j++) {
			if (model.at(i, j) == MapObject::air) continue;

			if (model.at(i, j) == MapObject::body) {
				if (j % 2) {
					_lightenbk(GetBk(), 0.06f);
				} else {
					_lightenbk(bk_colors[j / 2 % 5], 0.06f);
				}

				int xi = 38 + 38 * i;
				int yi = 94 + 38 * j;
				FillRect(xi, yi, xi + 34, yi + 34);
			} else if (model.at(i, j) == MapObject::deadSeg) {
				float angle = (float)fmod(
					120 * model.getGametick() + 0.119f * i + 1.10f * j, 6.28f);
				int xi = 54 + 38 * i + (i + j) % 6;
				int yi = 109 + 38 * j + (i + j) % 8;
				int xd = (int)(12 * cos(angle));
				int yd = (int)(12 * sin(angle));

				Line(xi + xd, yi + yd, xi - xd, yi - yd);
			} else {
				SetTextColor(fruit_colors[(int)model.at(i, j) - (int)MapObject::chili]);

				emojiext[1] = fruit[((int)model.at(i, j) - (int)MapObject::chili) * 2 + 1];
				PutText(41 + 38 * i, 97 + 38 * j, emojiext);
			}
		}
	}
}

void ArenaView::putmsg(const wchar_t* msg) {
	std::array<wchar_t, 42> text;

	wcsncpy_s(text.data(), 42, msg, 41);

	size_t msglen = wcslen(text.data());
	text.data()[std::min((size_t)41, msglen)] = ' ';
	text.data()[std::min((size_t)41, msglen + 1)] =
		MsgViewProvider::Msg("message")[0];
	text.data()[std::min((size_t)41, msglen + 2)] = '\0';

	msgpend.emplace(text);
}

void ArenaView::_paintmsgimg(Image* img, const wchar_t* msg) {
	Font font;
	ReadFont(font);
	font.setHeight(21);
	font.setWeight(FontWeight::semiBold);

	SetTarget(img);
	SetGraphicsScale(wui.getScale());
	SetFont(font);

	int width = TextWidth(msg) + 16;
	img->resize((int)(wui.getScale() * width), (int)(wui.getScale() * 28));

	SetBk(0x317ded);
	Clear(0, 0, width, 30);

	SetLineColor(0xffffff);
	SetLineStyle(3);
	Rectangle(1, 1, width - 1, 27);

	SetTextColor(0xffffff);
	PutText(8, 3, msg);

	SetTarget(&doublebuf);
}

void ArenaView::_putshadow(int x, int y, int w, int h) {
	static const char blurpx[16][17] = {
		"                ",   "        !!!!!!!!",  "      !!\"\"\"#####",
		"    !!\"\"#$$%%%&&", "   !!\"#$%&&'(())", "   !\"#$%'()*+,,,",
		"  !\"#$%')+,./001",  "  !\"$%'),.023456", "  !#%'),/2468:;;",
		"  !#%(+.258:<>@@",   "  \"$&),048;>@BDE", "  \"$'*-16:>ACFGH",
		"  \"%'+.38<@CFIJK",  "  \"%(,/49>BEHKMN", "  \"%(,05:?CGJMOP",
		"  \"%(,05;@DHKNPQ" };

	static const std::function<Color(Color, short)> darken =
		[](Color c, short d) -> Color {
		return (((c >> 16) - d) << 16) |
			(((c >> 8 & 0xff) - d) << 8) | ((c & 0xff) - d);
	};

	int x1, y1;

	for (int i = 0; i < w + 12; i++) {
		for (int j = 0; j < h + 12; j++) {
			if (i >= 6 && j >= 3 && i < w + 6 && j < h + 3) continue;

			x1 = x + i - 6, y1 = y + j - 3;

			SetFg(darken(GetPixel(x1, y1),
				blurpx[std::min(15, std::min(i, w + 12 - i))]
				[std::min(15, std::min(j, h + 12 - j))] - ' '));
			FillRect(x1, y1, x1, y1);
		}
	}
}

void ArenaView::paint() {
	wchar_t ch[21]{};

	Font font;
	ReadFont(font);
	font.setHeight(18);
	font.setWeight(FontWeight::normal);

	if (!inited) {
		if (model.isHost() && !model.isSyncing()) {
			btnpause.fg = 0x577f33;
			btnpause.setLabel(MsgViewProvider::Msg("pause"));
			wui.put(btnpause, btnend.left, -btnpause.getWidth() - 10);
		} else {
			wui.remove(btnpause);
		}

		doublebuf.resize(GraphicsWidth(), GraphicsHeight());

		float scale = GraphicsScale();
		Color bg = GetBk();

		SetTarget(&doublebuf);
		SetGraphicsScale(scale);
		SetBk(bg);
		Clear();

		newscore = true;
		scorepos.clear();
		scoretick.clear();
		scoretick.push_back(0);
		for (int i = 0; i < model.playerCount(); i++) {
			scoretick.push_back(0);
		}

		int i = 0, xi = 35, wl;
		const wchar_t* cptr = MsgViewProvider::Msg("highscore");
		SetFg(player_colors[i]);
		SetFont(font);

		while (i <= model.playerCount()) {
			FillPie(xi, 35, xi + 20, 55, 1.57, 6.28);
			scorepos.push_back(xi);

			wl = TextWidth(cptr);
			if (wl > 42) {
				wcsncpy(ch, cptr, 3);
				wcsncpy_s(ch + 3, 9, MsgViewProvider::Msg("ellipsis-dots"), 4);
				cptr = ch;
			}

			SetTextColor(0x0);
			PutText(xi + 27, 36, cptr);

			xi += 42 + TextWidth(cptr);
			i++;
			if (i > model.playerCount()) break;

			cptr = model.playerOf((size_t)i - 1)->getName();
			SetFg(player_colors[(i - 1) % 5]);
		}

		SetTextColor(0x577f33);
		SetLineColor(0x577f33);
		SetLineStyle(3);
		PutText(
			615 - btnend.getWidth() -
			(model.isHost() && !model.isSyncing() ? btnpause.getWidth() + 10
				: 0),
			32, MsgViewProvider::Msg("stopwatch"));

		paintbg();
		paintSnakes(0);

		inited = true;
	}

	if (pauseToggled) {
		btnpause.setLabel(
			MsgViewProvider::Msg(model.isPaused() ? "continue" : "pause"));

		wui.drawPart(btnpause);
		pauseToggled = false;
	}

	SetTarget(&doublebuf);

	if (!model.isPaused() && (!model.isStarted() || model.isPlaying())) {
		ch[0] = '\0';
		if (!model.isStarted()) {
			swprintf(ch, 15, L"%d >>",
				3 - (clock() - model.getGametick()) / CLOCKS_PER_SEC);
		} else {
			int sec = (clock() - model.getGametick()) / CLOCKS_PER_SEC;
			swprintf(ch, 15, L"%d:%02d", sec / 60, sec % 60);
		}

		int x0 =
			620 - btnend.getWidth() -
			(model.isHost() && !model.isSyncing() ? btnpause.getWidth() + 10 : 0);
		int w = TextWidth(ch) + 8;

		Clear(x0 - 50, 32, x0, 60);

		SetFg(0x577f33);
		FillRect(x0 - w, 32, x0, 50);

		SetTextColor(0xffffff);
		SetFont(font);
		PutText(x0 - w + 4, 32, ch);
	}

	if (model.isPlaying() || !(msgpend.empty() && msgtick == 0)) {
		paintbg();
		paintFruits();
		paintSnakes(model.isStarted() && model.isPlaying()
			? std::min(2 * (clock() - model.getFrametick()), CLOCKS_PER_SEC)
			: 0);
	}

	if (newscore) {
		font.setHeight(16);
		SetFont(font);

		for (int i = 0; i <= model.playerCount(); i++) {
			swprintf(ch, 4, L"%d",
				i == 0 ? highscore : model.scoreOf((size_t)i - 1));
			SetTextColor(i == 0 ? player_colors[0]
				: model.playerOf((size_t)i - 1)->isDead()
				? 0xa5a5a5
				: player_colors[(i - 1) % 5]);

			Clear(scorepos[i] + 20, 18, scorepos[i] + 20 + TextWidth(ch),
				32);
			PutText(scorepos[i] + 20, 18, ch);
		}

		newscore = false;
	}

	for (int i = 0; i < (int)scoretick.size(); i++) {
		if (scoretick[i] < clock() && scoretick[i] + CLOCKS_PER_SEC / 2 > clock()) {
			Clear(scorepos[i], 35, scorepos[i] + 20, 55);
			SetFg(player_colors[i == 0 ? 0 : (i - 1) % 5]);

			double angle = (double)clock() - scoretick[i];
			angle *= 10 * (CLOCKS_PER_SEC / 2 - angle);
			angle /= (double)CLOCKS_PER_SEC * CLOCKS_PER_SEC;

			FillPie(scorepos[i], 35, scorepos[i] + 20, 55, 1.57 - angle,
				6.28 + angle);
		} else if (scoretick[i] != 0) {
			Clear(scorepos[i], 35, scorepos[i] + 20, 55);
			SetFg(player_colors[i == 0 ? 0 : (i - 1) % 5]);
			FillPie(scorepos[i], 35, scorepos[i] + 20, 55, 1.57, 6.28);
			scoretick[i] = 0;
		}
	}

	if (!(msgpend.empty() && msgtick == 0)) {
		if (msgtick == 0 ||
			(clock() - msgtick >= 3 * CLOCKS_PER_SEC / 2 && !msgpend.empty())) {
			if (msgtick != 0) msgbufprev = msgbuf;

			_paintmsgimg(&msgbuf, msgpend.front().data());
			msgpend.pop();

			msgtick = clock();
		}

		if (clock() - msgtick < CLOCKS_PER_SEC / 2) {

			int offset = clock() - msgtick;
			offset *= (int)((CLOCKS_PER_SEC - offset) * 116);
			offset /= CLOCKS_PER_SEC * CLOCKS_PER_SEC;

			if (msgbufprev.width() != 1) {
				int w = (int)(msgbufprev.width() / wui.getScale());
				PutImage(340 - w / 2, 249, msgbufprev, w, 28 - offset, 0, offset);
				_putshadow(340 - w / 2, 249, w, 28 - offset);
			}

			int w = (int)(msgbuf.width() / wui.getScale());
			PutImage(340 - w / 2, 277 - offset, msgbuf, w, offset, 0, 0);
			_putshadow(340 - w / 2, 277 - offset, w, offset);

		} else if (clock() - msgtick < 3 * CLOCKS_PER_SEC / 2) {

			int w = (int)(msgbuf.width() / wui.getScale());
			PutImage(340 - w / 2, 249, msgbuf, w, 28, 0, 0);
			_putshadow(340 - w / 2, 249, w, 28);

		} else if (clock() - msgtick < 2 * CLOCKS_PER_SEC) {

			int offset = clock() - msgtick - 3 * CLOCKS_PER_SEC / 2;
			offset *= (int)(offset * 116);
			offset /= CLOCKS_PER_SEC * CLOCKS_PER_SEC;

			int w = (int)(msgbuf.width() / wui.getScale());
			PutImage(340 - w / 2, 249, msgbuf, w, 28 - offset, 0, offset);
			_putshadow(340 - w / 2, 249, w, 28 - offset);

		} else {
			msgbufprev.resize(1, 1);
			msgtick = 0;
		}
	}

	SetTarget();
	EnterBuffer();
	PutImage(0, 0, doublebuf);
	wui.drawPart(btnpause);
	wui.drawPart(btnend);
	ExitBuffer();
}

void ArenaView::newFrame() {
	model.frameLogic();

	if (model.isPlaying() && !this->playing && !this->quitted) {
		btnpause.setEnabled(true);
		btnend.setLabel(MsgViewProvider::Msg("end"));
		this->playing = true;
	}

	if (model.playerOf(0)->isDead() && this->playing) {
		btnend.setLabel(MsgViewProvider::Msg("leave"));
		this->playing = false;
		this->quitted = true;
	}

	if (model.isStarted() && !model.isPlaying() && !this->over) {
		btnpause.setEnabled(false);
		putmsg(MsgViewProvider::Msg("game-end"));
		this->playing = false;
		this->over = true;
	}

	paint();

	if (playing) {
		int toward = -1;

		MouseEvent m;
		while (PopMouseEvent(m)) {
			if (_mousedownx == -1 &&
				m.type == MouseEventType::leftDown) {
				_mousedownx = m.x;
				_mousedowny = m.y;
			} else if (_mousedownx != -1 &&
				m.type != MouseEventType::leftDown) {
				int vx = m.x - _mousedownx;
				int vy = m.y - _mousedowny;

				if (vx != 0 || vy != 0) {
					if (vy < 0 && vx > vy && vx < -vy)
						toward = 0;
					else if (vy > 0 && vx < vy && vx > -vy)
						toward = 1;
					else if (vx < 0)
						toward = 2;
					else
						toward = 3;
				}

				_mousedownx = _mousedowny = -1;
			}

			wui.mousehit(m);
		}

		KeyEvent k;
		while (PopKeyEvent(k)) {
			if (k.pressed) {
				switch (k.code) {
				case KeyCode::up:
					toward = 0;
					break;
				case KeyCode::down:
					toward = 1;
					break;
				case KeyCode::left:
					toward = 2;
					break;
				case KeyCode::right:
					toward = 3;
					break;
				case KeyCode::space:
					togglePause();
					break;
				case KeyCode::esc:
					quit();
					break;

				default:
					switch (towlower(k.ch)) {
					case 'w':
					case 'k':
						toward = 0;
						break;
					case 's':
					case 'j':
						toward = 1;
						break;
					case 'a':
					case 'h':
						toward = 2;
						break;
					case 'd':
					case 'l':
						toward = 3;
						break;
					}
				}
			}
		}

		if (toward != -1) {
			if (model.playerOf(0)->toward() != toward) {
				model.playerOf(0)->operate(toward);
			} else {
				model.accelerate();
			}
		}
	} else {
		MouseEvent m;
		while (PopMouseEvent(m)) wui.mousehit(m);

		KeyEvent k;
		while (PopKeyEvent(k)) {
			if (k.pressed) {
				if (k.code == KeyCode::esc) {
					quit();
				} else if (k.code == KeyCode::space) {
					togglePause();
				} else {
					wui.kbhit(k);
				}
			}
		}
	}
}

void ArenaView::reset() {
	btnend.setLabel(MsgViewProvider::Msg("leave"));
	title[0] = '\0';

	while (!msgpend.empty()) msgpend.pop();
	msgbufprev.resize(1, 1);
	msgtick = 0;

	playing = false;
	inited = false;
	over = false;
	quitted = false;
	highscore = 0;

	newscore = false;
	_mousedownx = -1, _mousedowny = -1;

	lightentick = 0;
	memset(lightens, false, sizeof(lightens));
}

void ArenaView::newScore(int id) {
	if (id + 1 > (int)scoretick.size()) return;

	this->newscore = true;
	scoretick[(size_t)id + 1] = clock();

	if (id == 0 && model.scoreOf(0) > highscore) {
		highscore = model.scoreOf(0);
		scoretick[0] = clock();
	}
}

void ArenaView::tellReady() {
	inited = false;

	if (!msgpend.empty()) return;

	wchar_t startmsg[30]{};
	swprintf(startmsg, 29, MsgViewProvider::Msg("ready-start"),
		model.playerCount());
	putmsg(startmsg);
}

void ArenaView::tellDeath(const wchar_t* name, int type) {
	wchar_t startmsg[30]{};
	swprintf(
		startmsg, 29,
		type == 8
		? MsgViewProvider::Msg("quitted-player")
		: type == 0
		? MsgViewProvider::Msg("death-wall")
		: type == 1
		? MsgViewProvider::Msg("death-hurt")
		: type == 2
		? MsgViewProvider::Msg("death-noscore")
		: type == 3 ? MsgViewProvider::Msg("death-exhaust")
		: MsgViewProvider::Msg("player-lost"),
		name);
	putmsg(startmsg);
}

void ArenaView::tellConnectionLost() {
	putmsg(MsgViewProvider::Msg("disconnected"));
}

void ArenaView::quit() {
	if (!playing) {
		wui.close();
	} else {
		btnend.setLabel(MsgViewProvider::Msg("leave"));

		if (!model.playerOf(0)->isDead()) {
			model.quitMe();
			this->quitted = true;
		}
	}
}
