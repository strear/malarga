#include <typeinfo>

#include "rnvntn.h"

using namespace Rnvntn;

bool RnWidget::isEnabled() { return this->enabled; }

void RnWidget::setEnabled(bool enabled) {
	this->enabled = enabled;
	if (parent != nullptr) parent->drawPart(*this);
}

int RnWidget::getWidth() { return this->width; }

int RnWidget::getHeight() { return this->height; }

void RnWidget::setWidth(int width) {
	if (parent != nullptr) parent->clarify(*this);
	this->width = width;
	if (parent != nullptr) parent->drawPart(*this);
}

void RnWidget::setHeight(int height) {
	if (parent != nullptr) parent->clarify(*this);
	this->height = height;
	if (parent != nullptr) parent->drawPart(*this);
}

void RnWidget::setSize(int width, int height) {
	if (parent != nullptr) parent->clarify(*this);
	this->width = width;
	this->height = height;
	if (parent != nullptr) parent->drawPart(*this);
}

void RnWidget::updateParent(RnWindow* newparent) {
	if (newparent == parent) return;
	if (newparent != nullptr && !newparent->contain(*this)) return;
	if (parent != nullptr) {
		parent->remove(*this);
	}
	parent = newparent;
}

RnWindow* RnWidget::getParent() { return parent; }

bool RnWidget::windowVisible() {
	return parent != nullptr && parent == RnWindow::GetVisibleOne();
}

const wchar_t* const RnTextWidget::default_fontface = L"Microsoft Yahei UI";

RnTextWidget::RnTextWidget() {
	ReadFont(font);
	wcscpy_s(font.face, 30, default_fontface);
	font.setHeight(18);
}

void RnTextWidget::setFont(Font& font) { this->font = font; }