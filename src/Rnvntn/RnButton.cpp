#include "rnvntn.h"

using namespace Rnvntn;

RnButton::RnButton(const wchar_t* label, bool icon) { setLabel(label, icon); }

RnButton::~RnButton() {
	if (clickhook != nullptr) delete clickhook;
}

const wchar_t* RnButton::getLabel() { return this->label; }

void RnButton::setLabel(const wchar_t* label) {
	if (windowVisible()) parent->clarify(*this);

	this->label = label;
	if (label == nullptr) return;

	SetFont(this->font);
	this->setWidth(TextWidth(label) + 18);
	this->setHeight(TextHeight(label) + 10);
}

void RnButton::setLabel(const wchar_t* label, bool icon) {
	this->fg = icon ? 0xc47244 : 0x0;
	this->setLabel(label);
}

void RnButton::setClickHook(std::function<void()>* function) {
	if (clickhook != nullptr) delete clickhook;
	if (function == nullptr)
		clickhook = nullptr;
	else
		clickhook = new std::function<void()>(*function);
}

void RnButton::clicked() {
	if (clickhook != nullptr && *clickhook != nullptr) (*clickhook)();
}

bool RnButton::interactable() { return isEnabled(); }

void RnButton::update(bool active, bool pressed) {
	Color fill = pressed ? 0xaba9a9 : 0xe8e8e8;
	Color border = active && isEnabled() ? 0xaba9a9 : fill;

	int x = parent->getX(*this), y = parent->getY(*this);

	SetFg(fill);
	FillRect(x, y, x + getWidth(), y + getHeight());

	SetLineStyle(2);
	SetLineColor(border);
	Rectangle(x, y, x + getWidth(), y + getHeight());

	if (label == nullptr) return;

	SetTextColor(isEnabled() ? fg : 0xa5a5a5);
	SetFont(this->font);
	PutText(x + 9, y + 5, label);
}

void RnButton::CloseWindow() { RnWindow::GetVisibleOne()->close(); }