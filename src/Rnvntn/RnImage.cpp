#include "rnvntn.h"

using namespace Rnvntn;

RnImage::RnImage(const wchar_t* path, int width, int height) {
	this->setImage(path, width, height);
}

RnImage::RnImage(Image& img) { this->setImage(img); }

RnImage::~RnImage() {
	if (this->img != nullptr) delete this->img;
}

Image* RnImage::getImage() { return this->img; }

bool RnImage::setImage(Image& img) {
	if (img.width() == 0 && img.height() == 0) {
		return false;
	}

	if (this->img != nullptr) delete this->img;

	this->img = new Image(img);
	if (this->getWidth() == 0 || this->getHeight() == 0) {
		this->setWidth(img.width());
		this->setHeight(img.height());
	}

	return true;
}

bool RnImage::setImage(const wchar_t* path, int width, int height) {
	if (path == nullptr) {
		this->setWidth(width);
		this->setHeight(height);
		return false;
	}

	Image* img = new Image;
	LoadImg(*img, path);

	if (img->width() == 0 && img->height() == 0) {
		delete img;
		return false;
	}

	if (this->img != nullptr) delete this->img;
	this->img = img;

	if (this->getWidth() == 0 || this->getHeight() == 0) {
		if (width == 0) width = img->width();
		if (height == 0) height = img->height();
	}

	if (width != 0 && height != 0) {
		this->setWidth(width);
		this->setHeight(height);
	}

	return true;
}

void RnImage::update(bool active, bool pressed) {
	if (this->img == nullptr || parent == nullptr) return;

	float scale = parent->getScale();

	int xt0 = (int)(parent->getX(*this) * scale);
	int xte = xt0 + (int)(this->getWidth() * scale);
	int ytt = GraphicsWidth();
	int yt = ytt * (int)(parent->getY(*this) * scale);
	int yte = yt + (int)(ytt * this->getHeight() * scale);

	int yst = img->width();
	float ys = 0;
	float xsv = yst / scale / this->getWidth();
	float ysv = img->height() / scale / this->getHeight();

	uint32_t* view = GetPointer(), * pic = GetPointer(img);
	uint32_t pixel;
	float opacity;

	while (yt < yte) {
		float xs = 0;
		int xt = xt0;
		while (xt < xte) {
			pixel = pic[(int)ys * yst + (int)xs];
			opacity = (pixel >> 24) / 255.0f;

			if (opacity == 1) {
				view[yt + xt] = pixel;
			} else if (opacity != 0) {
#define mix(a) ((int(((pixel >> a) & 0xff) + \
                 (1 - opacity) * ((view[yt + xt] >> a) & 0xff))) << a)

				view[yt + xt] = ((0xff - (0xff - (view[yt + xt] >> 24)) *
					(0xff - (pixel >> 24)) / 0xff) << 24) | mix(0) | mix(8) | mix(16);
			}

			xs += xsv, xt++;
		}
		ys += ysv, yt += ytt;
	}
}