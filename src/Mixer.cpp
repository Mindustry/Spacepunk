// Mixer.cpp

#define GLM_FORCE_RADIANS
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "Camera.hpp"
#include "Mixer.hpp"
#include "Tile.hpp"

Mixer::Mixer() {
}

Mixer::~Mixer() {
	alDeleteFilters(1, &filter_lowpass);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);
}

void Mixer::init() {
	mainEngine->fmsg(Engine::MSG_INFO,"initializing OpenAL context...");
	if( (device=alcOpenDevice(NULL))==NULL ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to open audio device: %d",alGetError());
		return;
	}
	if( alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT")==AL_TRUE ) {
		listDevices();
	}
	context = alcCreateContext(device, NULL);
	if( !alcMakeContextCurrent(context) ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to set current audio context");
		return;
	}

	// create low pass filter
	alGetError();
	alGenFilters(1, &filter_lowpass);
	if( alIsFilter(filter_lowpass) && alGetError() == AL_NO_ERROR ) {
		alFilteri(filter_lowpass, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
		if (alGetError() != AL_NO_ERROR) {
			mainEngine->fmsg(Engine::MSG_ERROR,"failed to setup lowpass filter");
		} else {
			alFilterf(filter_lowpass, AL_LOWPASS_GAIN, 0.25f);
			alFilterf(filter_lowpass, AL_LOWPASS_GAINHF, 0.25f);
		}
	} else {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to create lowpass filter");
	}

	initialized = true;
}

void Mixer::listDevices() {
	const ALCchar* device = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
	const ALCchar* next = device + 1;
	size_t len = 0;

	mainEngine->fmsg(Engine::MSG_INFO,"audio devices found:");
	mainEngine->fmsg(Engine::MSG_INFO,"----------------");
	while( device && *device != '\0' && next && *next != '\0' ) {
		mainEngine->fmsg(Engine::MSG_INFO, device);
		len = strlen(device);
		device += (len + 1);
		next += (len + 2 );
	}
	mainEngine->fmsg(Engine::MSG_INFO,"----------------");
	const ALCchar* defaultDevice = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	mainEngine->fmsg(Engine::MSG_INFO,"selected audio device: %s",defaultDevice);
}

void Mixer::setListener(Camera* camera) {
	listener = camera;
	if (!camera) {
		return;
	}

	float f = 2.f / Tile::size;

	// find orientation
	Angle ang = camera->getGlobalAng();
	Vector forward = ang.toVector();
	ang.pitch -= PI/2;
	Vector up = ang.toVector();
	ALfloat orientation[6] = { -forward.x, -forward.z, -forward.y, up.x, up.z, up.y };

	// set listener
	const Vector& pos = camera->getGlobalPos();
	const Vector& vel = camera->getEntity()->getVel();
	alListener3f(AL_POSITION, pos.x*f, -pos.z*f, pos.y*f);
	alListener3f(AL_VELOCITY, vel.x*f, -vel.z*f, vel.y*f);
	alListenerfv(AL_ORIENTATION, orientation);
}

int Mixer::playSound(const char* name, const bool loop) {
	Sound* sound = mainEngine->getSoundResource().dataForString(StringBuf<64>("sounds/%s", 1, name).get());
	if( sound ) {
		return sound->play(loop);
	}
	return -1;
}