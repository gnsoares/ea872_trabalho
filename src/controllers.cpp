#include <cstdlib>
#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>
#include <vector>

#include "constants.hpp"
#include "controllers.hpp"
#include "json.hpp"
#include "models.hpp"
#include "utils.hpp"
#include "views.hpp"

using namespace Controllers;
using nlohmann::json;


/* -=-=-=-=-=-=-=-=-=-=-=-=-  MAP  IMPLEMENTATION  -=-=-=-=-=-=-=-=-=-=-=-=- */
Map::Map(Models::Room &room, Views::Map &mapView)
    : room(room), mapView(mapView) {
    this->mapView.initializeRoom(this->room);
    this->mapView.render(this->room);
}

void Map::changeRooms(Models::Room &room) {
    mapView.destroyTextures(this->room);
    mapView.initializeRoom(room);
    mapView.render(room);
    this->room = room;
}

void Map::damageMetroid(Models::Metroid &metroid) {

    // set cooldown to avoid repeated damage
    damageCooldown = 5;

    // decrease hp
    metroid.hp -= SamusConstants::shotDamage;

    // negative hp: set as minimum
    if (metroid.hp < 0) metroid.hp = 0;
    
    // debug damage
    std::cout << "Metroid damage " << metroid.hp << "\n";
}

void Map::update(std::vector<Models::Shot> shots, Models::Samus &samus, bool canChangeRooms) {
    int prevX, prevY;

    // update metroids
    for (int i = 0; i < room.metroids.size(); i++) {
        prevX = room.metroids[i].rect.x; prevY = room.metroids[i].rect.y;

        // S2 = S1 + V*t + a*t^2/2
        room.metroids[i].rect.x += room.metroids[i].vx + room.metroids[i].ax / 2;
        room.metroids[i].rect.y += room.metroids[i].vy + room.metroids[i].ay / 2;

        // a = -k*x/m
        room.metroids[i].ax = - MetroidConstants::elasticConstantX * (room.metroids[i].rect.x - room.metroids[i].xi) / MetroidConstants::mass;
        room.metroids[i].ay = - MetroidConstants::elasticConstantY * (room.metroids[i].rect.y - room.metroids[i].yi) / MetroidConstants::mass;

        // V2 = V1 + a*t + random factor
        room.metroids[i].vx += room.metroids[i].ax + (rand() % 15 - 7);
        room.metroids[i].vy += room.metroids[i].ay + (rand() % 15 - 7);

        // check collision with blocks and doors
        processMetroidCollisionWithWall(room.metroids[i], room.blocks, prevX, prevY);
        processMetroidCollisionWithWall(room.metroids[i], room.doors, prevX, prevY);

        // check collision with shots
        if (damageCooldown == 0) {
            for (int j = 0; j < shots.size(); j++) {

                // metroid collided with shot: take damage
                if (checkCollision(shots[j].rect, room.metroids[i].rect)) {
                    damageMetroid(room.metroids[i]);

                    // metroid died: remove it
                    if (room.metroids[i].hp == 0)
                        room.metroids.erase(room.metroids.begin() + i);

                    // no need to check other shots
                    break;
                }
            }
        }
    }

    // decrease cooldown
    if (damageCooldown > 0) damageCooldown--;

    // update doors
    for (int i = 0; i < room.doors.size(); i++) {

        // door is open and collided with a Samus that can change rooms: change
        if (canChangeRooms &&
            room.doors[i].isOpen &&
            checkCollision(samus.rect, room.doors[i].rect)) {
            std::string oldRoom = room.name;
            Models::Room newRoom = loadRoom(room.doors[i].leadsTo);

            // go to new room
            changeRooms(newRoom);

            // update Samus' position in new room
            for (int j = 0; j < room.doors.size(); j++) {
                if (room.doors[j].leadsTo == oldRoom) {
                    samus.rect.x = room.doors[j].rect.x + room.doors[j].rect.w / 2;
                    samus.rect.y = room.doors[j].rect.y + room.doors[j].rect.h - samus.rect.h - 10;
                    break;
                }
            }
            // no need to check other doors
            break;
        }

        // door is closed: check collision with shots
        if (!room.doors[i].isOpen) {
            for (int j = 0; j < shots.size(); j++) {

                // collided with shot: open door
                if (checkCollision(shots[j].rect, room.doors[i].rect)) {
                    room.doors[i].isOpen = true;

                    // no need to check other shots
                    break;
                }
            }
        }
    }

    // render map
    mapView.render(room);
}
/* -=-=-=-=-=-=-=-=-=-=-=- END OF MAP IMPLEMENTATION -=-=-=-=-=-=-=-=-=-=-=- */


/* -=-=-=-=-=-=-=-=-=-=-=-=- SAMUS  IMPLEMENTATION -=-=-=-=-=-=-=-=-=-=-=-=- */
Samus::Samus(Models::Samus &samus, Views::Samus &samusView)
    : samus(samus), samusView(samusView) {
    this->samusView.loadTexture(this->samus);
    this->samusView.render(this->samus);
}

void Samus::damage() {
    
    // set cooldown to avoid repeated damage
    damageCooldown = 5;

    // decrease hp
    samus.hp -= MetroidConstants::damage;

    // negative hp: set as minimum
    if (samus.hp < 0) samus.hp = 0;

    // debug damage
    std::cout << "Samus damage " << samus.hp << "\n";
}

void Samus::jump() {

    // Samus already jumping: do nothing
    if (samus.isJumping)
        return;

    // update samus vertical velocity and state
    samus.vy = SamusConstants::jumpVy;

    // set Samus' state
    samus.isJumping = true;
    samus.isFalling = false;
}

void Samus::lookUp() {

    // Samus is morphed: unmorph
    if (samus.state == SamusConstants::morphedState) {
        samus.state = SamusConstants::idleState;
        return;
    }

    // update samus sight
    samus.ySight = -1;
    samus.xSight = 0;
}

void Samus::morph() {

    // Samus is in the air: look down
    if (samus.isJumping || samus.isFalling) {
        samus.xSight = 0;
        samus.ySight = 1;
        return;
    }

    // Samus in the ground: morph
    samus.xSight = 0;
    samus.ySight = 0;

    // TODO: check if samus has morphing ball and is not morphed
}

void Samus::moveLeft() {

    // look left
    samus.xSight = -1;
    samus.ySight = 0;

    // walk left
    samus.rect.x -= SamusConstants::horizontalStep;

    // out of window: stay at the border
    if (samus.rect.x < 0)
        samus.rect.x = 0;
}

void Samus::moveRight() {

    // look right
    samus.xSight = 1;
    samus.ySight = 0;

    // walk right
    samus.rect.x += SamusConstants::horizontalStep;

    // out of window: stay at the border
    if (samus.rect.x + samus.rect.w > Screen::width)
        samus.rect.x = Screen::width - samus.rect.w;
}

void Samus::update(std::vector<Models::Block> blocks, std::vector<Models::Door> doors, std::vector<Models::Metroid> metroids) {

    // process user command
    std::string command = samusView.processCommand();
    int prevX = samus.rect.x, prevY = samus.rect.y;

    // execute user command
    if (command == Commands::jump) jump();
    if (command == Commands::lookUp) lookUp();
    if (command == Commands::morph) morph();
    if (command == Commands::moveLeft) moveLeft();
    if (command == Commands::moveRight) moveRight();

    // set new vertical position
    samus.rect.y += samus.vy + SamusConstants::gravity / 2;

    // set new vertical velocity
    samus.vy += SamusConstants::gravity;

    // not negative velocity: set Samus' state
    if (samus.vy >= 0) samus.isFalling = true;

    // check collision with blocks and doors
    processSamusCollisionWithWall(samus, blocks, prevX, prevY);
    processSamusCollisionWithWall(samus, doors, prevX, prevY);

    // out of window: stay at the border
    if (samus.rect.y < 0)
        samus.rect.y = 0;
    else if (samus.rect.y + samus.rect.h > Screen::height)
        samus.rect.y = Screen::height - samus.rect.h;


    // check collision with metroids
    if (damageCooldown == 0) {
        for (int i = 0; i < metroids.size(); i++) {

            // Samus collided with a Metroid: take damage
            if (checkCollision(metroids[i].rect, samus.rect)) {
                damage();

                // no need to check more than one metroid
                break;
            }
        }
    }

    // decrease damage cooldown
    if (damageCooldown > 0) damageCooldown--;

    // render Samus
    samusView.render(samus);
}
/* -=-=-=-=-=-=-=-=-=-=-  END OF SAMUS IMPLEMENTATION  -=-=-=-=-=-=-=-=-=-=- */


/* -=-=-=-=-=-=-=-=-=-=-=-=- SHOTS  IMPLEMENTATION -=-=-=-=-=-=-=-=-=-=-=-=- */
Shots::Shots(std::vector<Models::Shot> &shots, Views::Shots &shotsView)
    : shots(shots), shotsView(shotsView) {}

void Shots::createShot(int x, int y, int vx, int vy) {

    // shot cooldown is not zero: block shot creation
    if (shotCooldown != 0) return;

    // create shot, load its texture and add to vector
    Models::Shot shot(x, y, vx, vy);
    shotsView.loadTexture(shot);
    shots.push_back(shot);

    // reset shot cooldown to avoid shot flooding
    shotCooldown = 10;
}

void Shots::update(int x, int y, int vx, int vy) {

    // process user command
    std::string command = shotsView.processCommand();

    // received command to shoot: create shot
    if (command == Commands::shot) createShot(x, y, vx, vy);

    // decrease shot cooldown
    if (shotCooldown > 0) shotCooldown--;

    // update position
    for (int i = 0; i < shots.size(); i++) {
        shots[i].rect.x += shots[i].vx;
        shots[i].rect.y += shots[i].vy;
    }

    // if there is at least one shot and the first exited the window: remove it
    while (shots.size() > 0 && (
           shots[0].rect.x <= 0 ||
           shots[0].rect.y <= 0 ||
           shots[0].rect.x >= Screen::width ||
           shots[0].rect.y >= Screen::height
          ))
        shots.erase(shots.begin());

    // render shots
    shotsView.render(shots);
}
/* -=-=-=-=-=-=-=-=-=-=-  END OF SHOTS IMPLEMENTATION  -=-=-=-=-=-=-=-=-=-=- */


/* -=-=-=-=-=-=-=-=-=-=-=-=-  GAME IMPLEMENTATION  -=-=-=-=-=-=-=-=-=-=-=-=- */
void Game::update(json &state, std::vector<std::string> otherPlayers) {

    // clear scene
    SDL_RenderClear(renderer);

    // is not host: check for changes in room
    if (!isHost && state.contains(otherPlayers[0]) && state[otherPlayers[0]]["room"] != map.room.name && state[otherPlayers[0]]["samus"] != nullptr) {

        // change rooms
        Models::Room newRoom = loadRoom(state[otherPlayers[0]]["room"]);
        map.changeRooms(newRoom);

        // update own samus position to host's
        samus.update(state[otherPlayers[0]]["samus"]);
    }

    // update all members
    samusController.update(map.room.blocks,
                            map.room.doors,
                            map.room.metroids);
    shotsController.update(
        samus.rect.x + samus.rect.w/2,
        samus.rect.y + samus.rect.h/2,
        1.5 * samus.xSight * SamusConstants::horizontalStep,
        1.5 * samus.ySight * SamusConstants::horizontalStep
    );
    map.update(shots, samus, isHost);

    // render other players' stuff
    for (int i = 0; i < otherPlayers.size(); i++) {
        if (!state.contains(otherPlayers[i]) ||
            state[otherPlayers[i]]["samus"] == nullptr ||
            state[otherPlayers[i]]["shots"] == nullptr)
            continue;

        // could not find other player's samus model: create it
        if (otherSamuses.find(otherPlayers[i]) == otherSamuses.end()) {
            otherSamuses[otherPlayers[i]] = Models::Samus(state[otherPlayers[i]]["samus"]);
            samusView.loadTexture(otherSamuses[otherPlayers[i]]);

        // found other player's samus model: update it
        } else {
            otherSamuses[otherPlayers[i]].update(state[otherPlayers[i]]["samus"]);
        }

        // render other player's samus
        samusView.render(otherSamuses[otherPlayers[i]]);

        // render other player's shots
        std::vector<Models::Shot> tmpShots;
        for (int j = 0; j < state[otherPlayers[i]]["shots"].size(); j++) {
            Models::Shot tmpShot(state[otherPlayers[i]]["shots"][j]);
            shotsView.loadTexture(tmpShot);
            tmpShots.push_back(tmpShot);
        }
        shotsView.render(tmpShots);
    }

    // save new state
    state[my_ip_address] = json::object({});

    state[my_ip_address]["samus"] = samus;
    state[my_ip_address]["shots"] = shots;

    if (isHost) {
        state[my_ip_address]["room"] = map.room.name;
        state[my_ip_address]["metroids"] = map.room.metroids;
    }

    // render scene
    SDL_RenderPresent(renderer);

}

Game::~Game() {
    // stop SDL
    unloadSDL(window, renderer);
}

void Game::save() {
    json state;

    state["samus"] = samus;
    state["room"] = room;

    std::ofstream f;
    f.open("../assets/save.json");
    f << state;
    f.close();
}

void Game::load() {
    std::ifstream f1;
    json state;
    f1.open("../assets/save.json");
    f1 >> state;
    f1.close();

    // samus
    samus.hp = state["samus"]["hp"];
    samus.isFalling = state["samus"]["isFalling"];
    samus.isJumping = state["samus"]["isJumping"];
    samus.missileCounter = state["samus"]["missileCounter"];
    samus.rect.x = state["samus"]["rect.x"];
    samus.rect.y = state["samus"]["rect.y"];
    samus.state = state["samus"]["state"];
    samus.vy = state["samus"]["vy"];
    samus.xSight = state["samus"]["xSight"];
    samus.ySight = state["samus"]["ySight"];

    // room
    Models::Room newRoom;
    newRoom.name = state["room"]["name"];
    for (Models::Block block: state["room"]["blocks"]) {
        newRoom.blocks.push_back(block);
    }
    for (Models::Door door: state["room"]["doors"]) {
        newRoom.doors.push_back(door);
    }
    for (Models::Metroid metroid: state["room"]["metroids"]) {
        newRoom.metroids.push_back(metroid);
    }
    map.changeRooms(newRoom);

}
/* -=-=-=-=-=-=-=-=-=-=- END  OF  GAME  IMPLEMENTATION -=-=-=-=-=-=-=-=-=-=- */
