#ifndef SCREEN_X11_H
#define SCREEN_X11_H

#include "common.h"
#include "log.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef HAVE_XFT
#include <X11/Xft/Xft.h>
#endif

#ifdef HAVE_X11_XRANDR
#include <X11/extensions/Xrandr.h>
#endif

#include <array>
#include <cassert>

enum color_index_t
{
	BLACK = 0,
	WHITE = 1,
	GRAY = 2,
	DIMGRAY = 3,
	RED = 4,
	BLUE,
	COLOR_INDEX_END
};

// X11 color names. See XParseColor()
std::array<const char*, COLOR_INDEX_END> color_name_list{"BLACK", "WHITE", "GRAY", "DIMGRAY", "RED", "BLUE"};

struct rect_t
{
	xy_t ul;
	xy_t lr;

	coordinate_t width(){ return lr.x - ul.x; }
	coordinate_t height(){ return lr.y - ul.y; }
	xy_t center(){ return xy_t{(lr.x + ul.x) / 2, (lr.y + ul.y) / 2}; }
};

struct color_t
{
};

constexpr int invalid_screen_num = -1;

struct screen_x11_t
{
	screen_x11_t(int screen_num = invalid_screen_num)
    : is_valid(false)
    , display_(nullptr)
    , screen_num_(screen_num)
    , win_()
    , gc_()
    , width_(-1)
    , height_(-1)
    , pixel_()
    , font_info_(nullptr)
#ifdef HAVE_XFT
	, font_(nullptr)
	, xftdraw_(nullptr)
	, xrcolor_()
	, xftcolor_()
#endif  // HAVE_XFT
	{
		display_ = XOpenDisplay(NULL);
		if (display_ == nullptr)
			ERR("failed: XOpenDisplay(): Unable to connect to X server");
		LOG("screen_num: %d", screen_num_);
		if (screen_num_ == invalid_screen_num)
			screen_num_ = DefaultScreen(display_);
		get_display_size();

		XSetWindowAttributes attributes;
		attributes.override_redirect = True;
		attributes.event_mask = ExposureMask | KeyPressMask | ButtonPressMask;

		win_ = XCreateWindow(display_, RootWindow(display_, screen_num_),
					0, 0, width_, height_, 0,
					CopyFromParent, InputOutput, CopyFromParent,
					CWOverrideRedirect | CWEventMask,
					&attributes);
		XMapWindow(display_, win_);
		XGrabKeyboard(display_, win_,
			False, GrabModeAsync, GrabModeAsync, CurrentTime);
		XGrabPointer(display_, win_,
			False, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

		Colormap colormap = DefaultColormap(display_, screen_num_);
		XColor color;
		for (size_t idx = 0; idx < COLOR_INDEX_END; ++idx)
		{
			XParseColor(display_, colormap, color_name_list[idx], &color);
			XAllocColor(display_, colormap, &color);
			pixel_[idx] = color.pixel;
		}
		XSetWindowBackground(display_, win_, pixel_[GRAY]);
		XClearWindow(display_, win_);
		gc_ = XCreateGC(display_, win_, 0, NULL);

		text_init();  // TODO check return
		is_valid = true;
	}

	~screen_x11_t()
	{
		// TODO check if varibles valid
		text_uninit();
		XUngrabPointer(display_, CurrentTime);
		XUngrabKeyboard(display_, CurrentTime);
		XFreeGC(display_, gc_);
		XCloseDisplay(display_);
	}

#ifdef HAVE_XFT
	bool text_init()
	{
		font_ = XftFontOpenName(display_, DefaultScreen(display_), "Arial-16");
		assert(font_);
		xftdraw_ = XftDrawCreate(display_, win_,
			DefaultVisual(display_, DefaultScreen(display_)),
			DefaultColormap(display_, DefaultScreen(display_)));

		xrcolor_.red = 0x0;
		xrcolor_.green = 0x0;
		xrcolor_.blue = 0x0;
		xrcolor_.alpha = 0xffff;
		XftColorAllocValue(display_,
			DefaultVisual(display_, DefaultScreen(display_)),
			DefaultColormap(display_, DefaultScreen(display_)),
			&xrcolor_, &xftcolor_);

		return true;
	}

	void text_uninit()
	{
		XftColorFree(display_,
			DefaultVisual(display_, DefaultScreen(display_)),
			DefaultColormap(display_, DefaultScreen(display_)), &xftcolor_);
		XftDrawDestroy(xftdraw_);
		XftFontClose(display_, font_);
	}

	void text(xy_t xy, const char* text)
	{

		XSetForeground(display_, gc_, pixel_[BLACK]);
		XSetLineAttributes(display_, gc_, 1, LineSolid, CapRound, JoinRound);

		XftDrawStringUtf8(xftdraw_, &xftcolor_, font_,
			xy.x, xy.y, reinterpret_cast<const XftChar8 *>(text), strlen(text));

		XSync(display_, false);
	}


	XGlyphInfo text_extents(const char* text, size_t text_len)
	{
		XGlyphInfo extents;
		XftTextExtentsUtf8(display_, font_,
			reinterpret_cast<const FcChar8*>(text), text_len, &extents);
		return extents;
	}

	int text_width(const char* text, size_t text_len)
	{
		XGlyphInfo extents = text_extents(text, text_len);
		return extents.width;
	}

	int text_width(const char* text)
	{
		return text_width(text, strlen(text));
	}

	int text_height()
	{
		return font_->ascent + font_->descent;
	}

#endif

	void get_display_size()
	{
		width_ = DisplayWidth(display_, screen_num_);
		height_ = DisplayHeight(display_, screen_num_);
	}

	void rect(rect_t rect, color_index_t color_idx)
	{
		XSetForeground(display_, gc_, pixel_[color_idx]);
		XSetLineAttributes(display_, gc_, 1, LineSolid, CapRound, JoinRound);

		XDrawRectangle(display_, win_, gc_,
			rect.ul.x, rect.ul.y,
			rect.lr.x - rect.ul.x, rect.lr.y - rect.ul.y);

		XSync(display_, false);
	}

	void circle(xy_t center, coordinate_t radius, color_index_t color_idx)
	{
		XSetForeground(display_, gc_, pixel_[color_idx]);
		XSetLineAttributes(display_, gc_, 1, LineSolid, CapRound, JoinRound);

		XDrawArc(display_, win_, gc_,
			center.x - (radius / 2), center.y - (radius / 2),
			radius, radius,
			90*64, 360 * 64);

		XSync(display_, false);
	}

	void cross(xy_t center, coordinate_t size, color_index_t color_idx)
	{
		XSetForeground(display_, gc_, pixel_[color_idx]);
		XSetLineAttributes(display_, gc_, 1, LineSolid, CapRound, JoinRound);

		XDrawLine(display_, win_, gc_,
			center.x - (size / 2), center.y,
			center.x + (size / 2), center.y);

		XDrawLine(display_, win_, gc_,
			center.x, center.y - (size / 2),
			center.x, center.y + (size / 2));

		XSync(display_, false);
	}

	bool get_touch_button_event(xy_t& xy, unsigned int& keycode)
	{
		xy.x = -1;
		xy.y = -1;
		keycode = 0;

		XEvent event;
		if (!XCheckWindowEvent(display_, win_, -1, &event))
			return false;

		// ButtonPress - mouse button or touch
		if (event.type != KeyPress && event.type != ButtonPress)
			return false;
		
		if (event.type == KeyPress)
		{
			LOG("KeyPress: keycode: %x", event.xkey.keycode);
			keycode = event.xkey.keycode;
			return false;
		}

		xy.x = event.xbutton.x;
		xy.y = event.xbutton.y;

		return true;
	}

	bool is_valid;
    Display* display_;
    int screen_num_;
    Window win_;
    GC gc_;
    int width_;
    int height_;
    unsigned long pixel_[COLOR_INDEX_END];
    XFontStruct* font_info_;

#ifdef HAVE_XFT
	XftFont* font_;
	XftDraw* xftdraw_;
	XRenderColor xrcolor_;
	XftColor xftcolor_;
#endif  // HAVE_XFT

};
#endif  // SCREEN_X11_H
