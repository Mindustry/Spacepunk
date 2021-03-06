// Player.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Player.hpp"
#include "Random.hpp"
#include "World.hpp"
#include "Client.hpp"
#include "Input.hpp"
#include "Console.hpp"
#include "Mixer.hpp"
#include "Model.hpp"
#include "BBox.hpp"
#include "Camera.hpp"
#include "Frame.hpp"
#include "Renderer.hpp"

const char* Player::defaultName = "Player";
const float Player::standFeetHeight = 48.f;
const Vector Player::standOrigin( 0.f, 0.f, -80.f );
const Vector Player::standScale( 24.f, 24.f, 32.f );
const float Player::crouchFeetHeight = 16.f;
const Vector Player::crouchOrigin( 0.f, 0.f, -40.f );
const Vector Player::crouchScale( 24.f, 24.f, 24.f );

static Cvar cvar_mouseSpeed("player.mousespeed", "adjusts mouse sensitivity", "0.1");
static Cvar cvar_mouselook("player.mouselook", "assigns mouse control to the given player", "0");
static Cvar cvar_gravity("player.gravity", "gravity that players are subjected to", "9");
static Cvar cvar_speed("player.speed", "player movement speed", "28");
static Cvar cvar_crouchSpeed("player.crouchspeed", "movement speed modifier while crouching", ".25");
static Cvar cvar_airControl("player.aircontrol", "movement speed modifier while in the air", ".02");
static Cvar cvar_jumpPower("player.jumppower", "player jump strength", "4.0");
static Cvar cvar_canCrouch("player.cancrouch", "whether player can crouch at all or not", "1");

Player::Player() {
	Random& rand = mainEngine->getRandom();

	float hairR = rand.getFloat();
	float hairG = rand.getFloat();
	float hairB = rand.getFloat();

	float suitR = rand.getFloat();
	float suitG = rand.getFloat();
	float suitB = rand.getFloat();

	// setup a random color

	colors.headRChannel = { .7f, .5f, .2f, 1.f };
	colors.headGChannel = { hairR, hairG, hairB, 1.f };
	colors.headBChannel = { suitR, suitG, suitB, 1.f };

	colors.torsoRChannel = { suitR, suitG, suitB, 1.f };
	colors.torsoGChannel = { .5f, .5f, .5f, 1.f };
	colors.torsoBChannel = { .2f, .2f, .2f, 1.f };

	colors.armsRChannel = { suitR, suitG, suitB, 1.f };
	colors.armsGChannel = { .5f, .5f, .5f, 1.f };
	colors.armsBChannel = { .2f, .2f, .2f, 1.f };

	colors.feetRChannel = { suitR, suitG, suitB, 1.f };
	colors.feetGChannel = { .5f, .5f, .5f, 1.f };
	colors.feetBChannel = { .2f, .2f, .2f, 1.f };
}

Player::Player(const char* _name) : Player() {
	name = _name;
}

Player::Player(const char* _name, colors_t _colors) {
	name = _name;
	colors = _colors;
}

Player::~Player() {

}

void Player::setEntity(Entity* _entity) {
	entity = _entity;
	if( entity ) {
		models = entity->findComponentByName<Component>("models");
		head = entity->findComponentByName<Model>("Head");
		torso = entity->findComponentByName<Model>("Torso");
		arms = entity->findComponentByName<Model>("Arms");
		feet = entity->findComponentByName<Model>("Feet");
		bbox = entity->findComponentByName<BBox>("physics");
		camera = entity->findComponentByName<Camera>("Camera");
		rTool = entity->findComponentByName<Model>("RightTool");
		lTool = entity->findComponentByName<Model>("LeftTool");
		lamp = entity->findComponentByName<Light>("Lamp");
		if( !models || !bbox || !camera ) {
			mainEngine->fmsg(Engine::MSG_WARN,"failed to setup player for third party client: missing bodypart");
		}
	}
}

bool Player::spawn(World& _world, const Vector& pos, const Angle& ang) {
	if( entity ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to spawn player: already spawned");
		return false;
	}

	Uint32 uid;
	if( _world.isClientObj() && clientID == invalidID ) {
		// this is for clients, and causes the entity to get a non-canonical uid.
		uid = UINT32_MAX - 1;
	} else {
		// this is for the server, and causes the entity to get a new canonical uid.
		uid = UINT32_MAX;
	}

	// create 
	const Entity::def_t* def = Entity::findDef("Player");
	if( def ) {
		entity = Entity::spawnFromDef(&_world, *def, pos, ang, uid);
	} else {
		entity = nullptr;
	}
	if( !entity ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to spawn player: spawn from def failed");
		return false;
	}
	entity->setShouldSave(false);
	entity->setPlayer(this);

	// get body parts
	models = entity->findComponentByName<Component>("models");
	head = entity->findComponentByName<Model>("Head");
	torso = entity->findComponentByName<Model>("Torso");
	arms = entity->findComponentByName<Model>("Arms");
	feet = entity->findComponentByName<Model>("Feet");
	bbox = entity->findComponentByName<BBox>("physics");
	camera = entity->findComponentByName<Camera>("Camera");
	rTool = entity->findComponentByName<Model>("RightTool");
	lTool = entity->findComponentByName<Model>("LeftTool");
	lamp = entity->findComponentByName<Light>("Lamp");
	if( !models || !bbox || !camera ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to spawn player: missing bodypart");
		entity->remove();
		entity = nullptr;
		return false;
	}

	camera->setLocalAng(lookDir);

	// update colors
	updateColors(colors);

	// setup collision mesh and check that we spawned in a valid position
	entity->update();
	if( bbox->checkCollision() ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to spawn player: no room at spawn location");
		entity->remove();
		return false;
	}

	entity->setFlag(static_cast<int>(Entity::flag_t::FLAG_UPDATE));
	if( _world.isClientObj() ) {
		if( clientID == invalidID ) {
			Rect<Sint32> rect;
			rect.x = 0;
			rect.y = 0;
			rect.w = mainEngine->getXres();
			rect.h = mainEngine->getYres();
			camera->setWin(rect);

			setupGUI();
		} else {
			// we don't own this player and shouldn't see their camera
			Rect<Sint32> rect;
			rect.x = 0;
			rect.y = 0;
			rect.w = 0;
			rect.h = 0;
			camera->setWin(rect);
		}

		mainEngine->fmsg(Engine::MSG_INFO,"Client spawned player (%d) at (%1.f, %.1f, %.1f)", serverID, entity->getPos().x, entity->getPos().y, entity->getPos().z);
	} else {
		mainEngine->fmsg(Engine::MSG_INFO,"Server spawned player (%d) at (%1.f, %.1f, %.1f)", serverID, entity->getPos().x, entity->getPos().y, entity->getPos().z);
	}

	return true;
}

void Player::process() {
	updateGUI();
}

void Player::setupGUI() {
	Client* client = mainEngine->getLocalClient(); assert(client);
	Frame* gui = client->getGUI(); assert(gui);
	Renderer* renderer = client->getRenderer(); assert(renderer);

	// create reticle
	Image* reticle = mainEngine->getImageResource().dataForString("images/gui/reticle_1.png");
	int w = reticle->getWidth();
	int h = reticle->getHeight();
	int x = camera->getWin().x + camera->getWin().w / 2 - w / 2;
	int y = camera->getWin().y + camera->getWin().h / 2 - h / 2;
	char name[9] = "reticle"; name[7] = '0' + localID; name[8] = '\0';
	gui->addImage(Rect<Sint32>(x, y, w, h), glm::vec4(1.f), reticle, name);
}

void Player::updateGUI() {
	if (!entity) {
		return;
	}
	Client* client = mainEngine->getLocalClient(); assert(client);
	Frame* gui = client->getGUI(); assert(gui);

	// update reticle position (in-case it moves)
	Rect<Sint32> pos;
	char name[9] = "reticle"; name[7] = '0' + localID; name[8] = '\0';
	auto reticle = gui->findImage(name); assert(reticle);
	pos.w = reticle->image->getWidth();
	pos.h = reticle->image->getHeight();
	pos.x = camera->getWin().x + camera->getWin().w / 2 - pos.w / 2;
	pos.y = camera->getWin().y + camera->getWin().h / 2 - pos.h / 2;
	reticle->pos = pos;
}

void Player::updateColors(const colors_t& _colors) {
	colors = _colors;

	Mesh::shadervars_t shaderVars;

	// load head colors
	if (head) {
		shaderVars = head->getShaderVars();
		shaderVars.customColorR = colors.headRChannel;
		shaderVars.customColorG = colors.headGChannel;
		shaderVars.customColorB = colors.headBChannel;
		shaderVars.customColorA = colors.headGChannel;
		head->setShaderVars(shaderVars);
	}

	// load torso colors
	if (torso) {
		shaderVars = torso->getShaderVars();
		shaderVars.customColorR = colors.torsoRChannel;
		shaderVars.customColorG = colors.torsoGChannel;
		shaderVars.customColorB = colors.torsoBChannel;
		shaderVars.customColorA = colors.headGChannel;
		torso->setShaderVars(shaderVars);
	}

	// load arm colors
	if (arms) {
		shaderVars = arms->getShaderVars();
		shaderVars.customColorR = colors.armsRChannel;
		shaderVars.customColorG = colors.armsGChannel;
		shaderVars.customColorB = colors.armsBChannel;
		shaderVars.customColorA = colors.headGChannel;
		arms->setShaderVars(shaderVars);
	}

	// load feet colors
	if (feet) {
		shaderVars = feet->getShaderVars();
		shaderVars.customColorR = colors.feetRChannel;
		shaderVars.customColorG = colors.feetGChannel;
		shaderVars.customColorB = colors.feetBChannel;
		shaderVars.customColorA = colors.headGChannel;
		feet->setShaderVars(shaderVars);
	}
}

void Player::putInCrouch(bool crouch) {
	crouching = crouch;
	const Vector& scale = crouching ? crouchScale : standScale;
	const Vector& origin = crouching ? crouchOrigin : standOrigin;
	if( bbox ) {
		bbox->setLocalScale(scale);
		bbox->setLocalPos(origin);
	}
	if( entity ) {
		entity->update();
	}
}

Cvar cvar_maxHTurn("player.turn.horizontal", "maximum turn range for player, horizontal", "0.0");
Cvar cvar_maxVTurn("player.turn.vertical", "maximum turn range for player, vertical", "90.0");

void Player::control() {
	Client* client = mainEngine->getLocalClient();
	if( !entity || !client ) {
		return;
	}

	if( client->isConsoleActive() || cvar_mouselook.toInt() < 0 ) {
		mainEngine->setMouseRelative(false);
	} else {
		mainEngine->setMouseRelative(true);
	}

	Angle rot = entity->getRot();
	//Vector vel = entity->getVel();
	Vector vel = originalVel;
	Vector pos = entity->getPos();

	Entity* entityStandingOn = nullptr;

	float totalHeight = standScale.z - standOrigin.z;
	float nearestCeiling = bbox->nearestCeiling();
	float nearestFloor = bbox->nearestFloor(entityStandingOn);
	float distToFloor = bbox->distToFloor(nearestFloor);
	float distToCeiling = max( 0.f, pos.z - nearestCeiling );

	// collect movement inputs
	Input& input = mainEngine->getInput(localID);
	buttonRight = 0.f;
	buttonLeft = 0.f;
	buttonForward = 0.f;
	buttonBackward = 0.f;
	buttonJump = false;
	buttonCrouch = cvar_canCrouch.toInt() ? distToCeiling < totalHeight : false;
	float feetHeight = buttonCrouch ? crouchFeetHeight : standFeetHeight;
	if( !client->isConsoleActive() ) {
		if( !entity->isFalling() ) {
			buttonCrouch |= input.binary(Input::MOVE_DOWN);
		}

		if( input.binary(Input::MOVE_UP) && !jumped ) {
			if( (nearestFloor - nearestCeiling) > totalHeight && !buttonCrouch ) {
				buttonJump = true;
			}
		} else if( !input.binary(Input::MOVE_UP) && jumped ) {
			jumped = false;
		}

		if( !input.binary(Input::LEAN_MODIFIER) ) {
			buttonRight = input.analog(Input::MOVE_RIGHT);
			buttonLeft = input.analog(Input::MOVE_LEFT);
			buttonForward = input.analog(Input::MOVE_FORWARD);
			buttonBackward = input.analog(Input::MOVE_BACKWARD);

			float dir = atan2( buttonForward - buttonBackward, buttonRight - buttonLeft);
			float cosDir = abs(cos(dir));
			float sinDir = abs(sin(dir));

			buttonRight = min(cosDir,buttonRight);
			buttonLeft = min(cosDir,buttonLeft);
			buttonForward = min(sinDir,buttonForward);
			buttonBackward = min(sinDir,buttonBackward);
		}
	}
	if( buttonRight || buttonLeft || buttonForward || buttonBackward ) {
		moving = true;
	} else {
		moving = false;
	}

	// animation
	if( buttonCrouch ) {
		crouching = true;
	} else {
		crouching = false;
	}
	if (cvar_canCrouch.toInt() == 0) {
		crouching = false;
	}

	// time and speed
	float speedFactor = (crouching ? cvar_crouchSpeed.toFloat() : 1.f) * (entity->isFalling() ? cvar_airControl.toFloat() : 1.f) * cvar_speed.toFloat();
	float timeFactor = 1.f / 60.f;

	// set bbox and origin
	const Vector& scale = crouching ? crouchScale : standScale;
	const Vector& origin = crouching ? crouchOrigin : standOrigin;
	bbox->setLocalScale(scale);
	bbox->setLocalPos(origin);

	// set falling state and do attach-to-ground
	if( entity->isFalling() ) {
		vel.z += cvar_gravity.toFloat() * timeFactor;
		if( distToFloor <= feetHeight && vel.z >= 0.f ) {
			entity->setFalling(false);
			distToFloor = feetHeight;
			//pos.z = nearestFloor;
			//vel.z = 0;
			vel.z = (nearestFloor - pos.z) / 10.f;
		}
	} else {
		if( buttonJump && distToFloor <= feetHeight+16 ) {
			jumped = true;
			entity->setFalling(true);
			//pos.z = nearestFloor;
			distToFloor = feetHeight;
			vel.z = -cvar_jumpPower.toFloat();
		} else {
			if( distToFloor > feetHeight+16 ) {
				entity->setFalling(true);
			} else {
				distToFloor = feetHeight;
				//pos.z = nearestFloor;
				//vel.z = 0.f;
				vel.z = (nearestFloor - pos.z) / 10.f;
			}
		}
	}

	// calculate movement vectors
	Angle forwardAng = entity->getAng();
	forwardAng.pitch = 0;
	forwardAng.roll = 0;
	vel += forwardAng.toVector() * buttonForward * speedFactor * timeFactor;
	vel -= forwardAng.toVector() * buttonBackward * speedFactor * timeFactor;
	Angle rightAng = forwardAng;
	rightAng.yaw += PI/2;
	vel += rightAng.toVector() * buttonRight * speedFactor * timeFactor;
	vel -= rightAng.toVector() * buttonLeft * speedFactor * timeFactor;

	// friction
	if( !entity->isFalling() ) {
		vel *= .9;
	}
	rot.yaw *= .5;
	rot.pitch *= .5;
	rot.roll *= .5;

	// looking
	if( !client->isConsoleActive() ) {
		if( mainEngine->isMouseRelative() && cvar_mouselook.toInt() == localID ) {
			float mousex = mainEngine->getMouseMoveX();
			float mousey = mainEngine->getMouseMoveY();
			
			rot.yaw += mousex * timeFactor * cvar_mouseSpeed.toFloat();
			rot.pitch += mousey * timeFactor  * cvar_mouseSpeed.toFloat();
		}
		rot.yaw += (input.analog(Input::LOOK_RIGHT) - input.analog(Input::LOOK_LEFT)) * timeFactor * 2.f;
		rot.pitch += (input.analog(Input::LOOK_DOWN) - input.analog(Input::LOOK_UP)) * timeFactor * 2.f;
	}
	rot.wrapAngles();

	// change look dir
	float hLimit = cvar_maxHTurn.toFloat() * PI / 180.f;
	float vLimit = cvar_maxVTurn.toFloat() * PI / 180.f;
	lookDir.yaw += rot.yaw;
	lookDir.yaw = fmod(lookDir.yaw, PI);
	if (lookDir.yaw > hLimit) {
		float diff = lookDir.yaw - hLimit;
		lookDir.yaw = hLimit;
		rot.yaw = diff;
	} else if (lookDir.yaw < -hLimit) {
		float diff = lookDir.yaw + hLimit;
		lookDir.yaw = -hLimit;
		rot.yaw = diff;
	} else {
		rot.yaw = 0.f;
	}
	lookDir.pitch += rot.pitch;
	lookDir.pitch = fmod(lookDir.pitch, PI);
	if (lookDir.pitch > vLimit) {
		lookDir.pitch = vLimit;
	} else if (lookDir.pitch < -vLimit) {
		lookDir.pitch = -vLimit;
	}

	// don't actually turn the entity vertically
	rot.pitch = 0.f;


	if ( !client->isConsoleActive() ) {
		//Interacting with entities.
		World* world = entity->getWorld();
		if(camera && world)
		{
			if (input.binaryToggle(Input::INTERACT)) {
				if (holdingInteract)
				{
					// 60hz
					interactHoldTime += 1 / 60;
				}
				holdingInteract = true;
				input.consumeBinaryToggle(Input::INTERACT);
				Vector start = camera->getGlobalPos();
				Vector dest = start + camera->getGlobalAng().toVector() * 128;
				World::hit_t hit = entity->lineTrace(start, dest);

				if (hit.hitEntity)
				{
					Entity* hitEntity = nullptr;
					if ((hitEntity = world->uidToEntity(hit.index)) != nullptr)
					{
						previousInteractedEntity = hitEntity;

						if (previousInteractedEntity->isPickupable())
						{
							entity->depositInAvailableSlot(previousInteractedEntity);
						}

						auto hitBBox = static_cast<BBox*>(hit.pointer);
						if (hitBBox)
						{
							if (hitEntity->isFlag(Entity::flag_t::FLAG_INTERACTABLE))
							{
								mainEngine->fmsg(Engine::MSG_DEBUG, "clicked on entity '%s': UID %d", hitEntity->getName().get(), hitEntity->getUID());
								Packet packet;
								packet.write32(hitBBox->getUID());
								packet.write32(hitEntity->getUID());
								packet.write32(client->indexForWorld(world));
								packet.write32(localID);
								packet.write("ESEL");
								client->getNet()->signPacket(packet);
								client->getNet()->sendPacketSafe(0, packet);
							}
						}
					}
				}
			}
			else
			{
				holdingInteract = false;

				interactHoldTime = 0;
			}
			// Toggling inventory
			if (input.binaryToggle(Input::TOGGLE_INVENTORY))
			{
				input.consumeBinaryToggle(Input::TOGGLE_INVENTORY);
				entity->setInventoryVisibility(!inventoryVisible);
				inventoryVisible = !inventoryVisible;
			}
		}
	}

	// update entity vectors
	if (bbox->getMass() == 0.f) {
		entity->setPos(pos);
	}
	Vector standingOnVel;
	originalVel = vel;
	if (entityStandingOn) {
		standingOnVel = entityStandingOn->getVel();
	}
	entity->setVel(vel + standingOnVel);
	entity->setRot(rot);
	entity->update();

	// using hand items (shooting)
	if (lTool && input.binaryToggle(Input::bindingenum_t::HAND_LEFT)) {
		Model::bone_t bone = lTool->findBone("emitter");
		glm::mat4 mat = lTool->getGlobalMat();
		if (bone.valid) {
			 mat *= bone.mat;
		}
		lTool->shootLaser(mat, WideVector(1.f, 0.f, 0.f, 1.f), 8.f, 20.f);
	}
	if (rTool && input.binaryToggle(Input::bindingenum_t::HAND_RIGHT)) {
		Model::bone_t bone = rTool->findBone("emitter");
		glm::mat4 mat = rTool->getGlobalMat();
		if (bone.valid) {
			mat *= bone.mat;
		}
		rTool->shootLaser(mat, WideVector(1.f, 0.f, 0.f, 1.f), 8.f, 20.f);
	}
	input.consumeBinaryToggle(Input::bindingenum_t::HAND_LEFT);
	input.consumeBinaryToggle(Input::bindingenum_t::HAND_RIGHT);

	// lamp
	if (lamp && input.binaryToggle(Input::bindingenum_t::INVENTORY1)) {
		lamp->setIntensity(lamp->getIntensity() == 0.f ? 1.f : 0.f);
	}
	input.consumeBinaryToggle(Input::bindingenum_t::INVENTORY1);
}

void Player::updateCamera() {
	Client* client = mainEngine->getLocalClient();
	if( !entity || !client ) {
		return;
	}

	Input& input = mainEngine->getInput(localID);

	// move camera
	if( camera && head ) {
		head->updateSkin();
		Model::bone_t bone = head->findBone("Bone_Head");
		if( bone.valid ) {
			models->setLocalPos(Vector(-bone.pos.x, 0.f, 0.f));
			models->update();
			camera->setLocalPos(bone.pos + models->getLocalPos());
			camera->update();
		}

		Angle ang = camera->getLocalAng();
		ang += lookDir - oldLookDir;
		oldLookDir = lookDir;
		camera->setLocalAng(ang);

		/*if( !client->isConsoleActive() ) {
			if( input.binary(Input::LEAN_RIGHT) ||
				(input.binary(Input::MOVE_RIGHT) && input.binary(Input::LEAN_MODIFIER)) ) {
				ang.roll -= .01f;
			}
			if( input.binary(Input::LEAN_LEFT) ||
				(input.binary(Input::MOVE_LEFT) && input.binary(Input::LEAN_MODIFIER)) ) {
				ang.roll += .01f;
			}
		}*/

		if( localID == 0 ) {
			client->getMixer()->setListener(camera);
		}

		int localPlayerCount = client->numLocalPlayers();

		Rect<Sint32> rect;
		if( localPlayerCount < 2 ) {
			rect.x = 0;
			rect.w = mainEngine->getXres();
			rect.y = 0;
			rect.h = mainEngine->getYres();
		} else if( localPlayerCount < 3 ) {
			rect.x = 0;
			rect.w = mainEngine->getXres();
			rect.y = (localID % 2) * (mainEngine->getYres() / 2);
			rect.h = mainEngine->getYres() / 2;
		} else {
			rect.x = (localID % 2) * (mainEngine->getXres() / 2);
			rect.w = mainEngine->getXres() / 2;
			rect.y = (localID > 1) * (mainEngine->getYres() / 2);
			rect.h = mainEngine->getYres() / 2;
		}
		camera->setWin(rect);
		if( bone.valid ) {
			camera->translate(Vector(16.f, 4.f, 0.f));
		}
		camera->update();

		/*if( bone.valid ) {
			World* world = entity->getWorld();

			Vector start = camera->getGlobalPos();
			Vector pos = bone.pos;
			pos.x += 36.f;
			camera->setLocalPos(pos);
			camera->update();
			Vector dest = camera->getGlobalPos();

			bool traceEnabled = entity->isFlag(Entity::flag_t::FLAG_ALLOWTRACE);
			entity->resetFlag(static_cast<int>(Entity::flag_t::FLAG_ALLOWTRACE));
			World::hit_t hit = world->lineTrace( start, dest );
			if( traceEnabled ) {
				entity->setFlag(static_cast<int>(Entity::flag_t::FLAG_ALLOWTRACE));
			}

			if( hit.hitTile || hit.hitEntity ) {
				float x = (hit.pos - start).length() - 36.f;
				bone.pos.x += x;
				camera->setLocalPos(bone.pos);
				camera->update();

				models->setLocalPos(Vector(x - 16.f, 0.f, 0.f));
				models->update();
			} else {
				bone.pos.x += 16.f;
				camera->setLocalPos(bone.pos);
				camera->update();
				models->setLocalPos(Vector(0.f));
				models->update();
			}
		}*/
	}
}

bool Player::despawn() {
	if( !entity ) {
		return false;
	}
	entity->remove();
	entity = nullptr;
	return true;
}

void Player::onEntityDeleted(Entity* _entity) {
	if( entity == _entity ) {
		entity = nullptr;
	}
	return;
}
