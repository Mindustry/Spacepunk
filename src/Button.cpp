// Button.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "Frame.hpp"
#include "Button.hpp"
#include "Image.hpp"
#include "Text.hpp"

Button::Button() {
	size.x = 0; size.w = 32;
	size.y = 0; size.h = 32;
	color = glm::vec4(.5f,.5f,.5f,1.f);
	textColor = glm::vec4(1.f);
}

Button::Button(Frame& _parent) : Button() {
	parent = &_parent;
	_parent.getButtons().addNodeLast(this);
}

Button::~Button() {
	if (callback) {
		delete callback;
		callback = nullptr;
	}
}

void Button::setIcon(const char* _icon) {
	icon = _icon;
	iconImg = mainEngine->getImageResource().dataForString(icon.get());
}

void Button::draw(Renderer& renderer, Rect<int> _size, Rect<int> _actualSize) {
	if( disabled )
		return;

	_size.x += max(0,size.x-_actualSize.x);
	_size.y += max(0,size.y-_actualSize.y);
	_size.w = min( size.w, _size.w-size.x+_actualSize.x ) + min(0,size.x-_actualSize.x);
	_size.h = min( size.h, _size.h-size.y+_actualSize.y ) + min(0,size.y-_actualSize.y);
	if( _size.w <= 0 || _size.h <= 0 )
		return;

	glm::vec4 _color = highlighted ? color*1.5f : color;
	if( pressed ) {
		renderer.drawLowFrame(_size,border,_color);
	} else {
		renderer.drawHighFrame(_size,border,_color);
	}

	if( !text.empty() && style!=STYLE_CHECKBOX ) {
		Text* _text = mainEngine->getTextResource().dataForString(text.get());
		if( _text ) {
			Rect<int> pos;
			int textX = _size.w/2-_text->getWidth()/2;
			int textY = _size.h/2-_text->getHeight()/2;
			pos.x = _size.x+textX; pos.w = min((int)_text->getWidth(),_size.w);
			pos.y = _size.y+textY; pos.h = min((int)_text->getHeight(),_size.h);
			if( pos.w <= 0 || pos.h <= 0 ) {
				return;
			}
			_text->drawColor(Rect<int>(), pos, textColor);
		}
	} else if( iconImg ) {
		// we check a second time, just incase the cache was dumped and the original pointer invalidated.
		iconImg = mainEngine->getImageResource().dataForString(icon.get());
		if( iconImg ) {
			if( style!=STYLE_CHECKBOX || pressed==true ) {
				Rect<int> pos;
				pos.x = _size.x+border; pos.w = _size.w-border*2;
				pos.y = _size.y+border; pos.h = _size.h-border*2;
				if( pos.w <= 0 || pos.h <= 0 ) {
					return;
				}
				iconImg->draw(nullptr,pos);
			}
		}
	}
}

Button::result_t Button::process(Rect<int> _size, Rect<int> _actualSize, const bool usable) {
	result_t result;
	if( style!=STYLE_NORMAL ) {
		result.tooltip = nullptr;
		result.highlightTime = SDL_GetTicks();
		result.highlighted = false;
		result.pressed = pressed;
		result.clicked = false;
	} else {
		result.tooltip = nullptr;
		result.highlightTime = SDL_GetTicks();
		result.highlighted = false;
		result.pressed = false;
		result.clicked = false;
	}
	if( disabled ) {
		highlightTime = result.highlightTime;
		highlighted = false;
		if( style==STYLE_NORMAL ) {
			pressed = false;
		}
		return result;
	}
	if( !usable ) {
		highlightTime = result.highlightTime;
		highlighted = false;
		if( style==STYLE_NORMAL ) {
			pressed = false;
		}
		return result;
	}

	_size.x += max(0,size.x-_actualSize.x);
	_size.y += max(0,size.y-_actualSize.y);
	_size.w = min( size.w, _size.w-size.x+_actualSize.x ) + min(0,size.x-_actualSize.x);
	_size.h = min( size.h, _size.h-size.y+_actualSize.y ) + min(0,size.y-_actualSize.y);
	if( _size.w <= 0 || _size.h <= 0 ) {
		highlightTime = result.highlightTime;
		return result;
	}

	Sint32 mousex = mainEngine->getMouseX();
	Sint32 mousey = mainEngine->getMouseY();
	Sint32 omousex = mainEngine->getOldMouseX();
	Sint32 omousey = mainEngine->getOldMouseY();
	
	if( _size.containsPoint(omousex,omousey) ) {
		result.highlighted = highlighted = true;
		result.highlightTime = highlightTime;
		result.tooltip = tooltip.get();
	} else {
		result.highlighted = highlighted = false;
		result.highlightTime = highlightTime = SDL_GetTicks();
		result.tooltip = nullptr;
	}

	result.clicked = false;
	if( highlighted ) {
		if( mainEngine->getMouseStatus(SDL_BUTTON_LEFT) ) {
			if( _size.containsPoint(mousex,mousey) ) {
				result.pressed = pressed = (reallyPressed==false);
			} else {
				pressed = reallyPressed;
			}
		} else {
			if( pressed!=reallyPressed ) {
				result.clicked = true;
				if( style!=STYLE_NORMAL ) {
					reallyPressed = (reallyPressed==false);
				}
			}
			pressed = reallyPressed;
		}
	} else {
		pressed = reallyPressed;
	}

	return result;
}