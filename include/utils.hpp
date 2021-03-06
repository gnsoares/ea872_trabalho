#ifndef METROID_UTILS_H
#define METROID_UTILS_H

#include <map>
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include "models.hpp"


/*! \brief Collision checker between two rectangles
 *
 * \param rect1 First rectangle
 * \param rect2 Second rectangle
 * \return Whether or not the rectangles are colliding
 */
bool checkCollision(SDL_Rect rect1, SDL_Rect rect2);

/*! \brief Collision checker between an object and a wall
 *
 * \param object object rectangle
 * \param wall wall rectangle
 * \param prevX object's previous x position
 * \param prevY object's previous y position
 * \return Which of the foor walls the object collided with
 */
std::string collideWithWall(SDL_Rect object, SDL_Rect wall, int prevX, int prevY);

/*! \brief cheks Metroid collision with walls
 *
 * \param metroid Metroid object
 * \param block vector of blocks
 * \param prevX object's previous x position
 * \param prevY object's previous y position
 * \return nothing
 */
void processMetroidCollisionWithWall(Models::Metroid &metroid, std::vector<Models::Block> blocks, int prevX, int prevY);

/*! \brief cheks Metroid collision with doors
 *
 * \param metroid Metroid object
 * \param block vector of doors
 * \param prevX object's previous x position
 * \param prevY object's previous y position
 * \return nothing
 */
void processMetroidCollisionWithWall(Models::Metroid &metroid, std::vector<Models::Door> doors, int prevX, int prevY);

/*! \brief cheks Samus collision with wall
 *
 * \param metroid Metroid object
 * \param block vector of blocks
 * \param prevX object's previous x position
 * \param prevY object's previous y position
 * \return nothing
 */
void processSamusCollisionWithWall(Models::Samus &samus, std::vector<Models::Block> blocks, int prevX, int prevY);

/*! \brief cheks Samus collision with doors
 *
 * \param metroid Metroid object
 * \param block vector of blocks
 * \param prevX object's previous x position
 * \param prevY object's previous y position
 * \return nothing
 */
void processSamusCollisionWithWall(Models::Samus &samus, std::vector<Models::Door> doors, int prevX, int prevY);

/*! \brief Room loader
 *
 * \param name room file name
 * \return room object
 */
Models::Room loadRoom(std::string name);

/*! \brief Creates window
 *
 * Creates windows and, if window creation fails,
 * prints error on the screen
 * 
 * \return window pointer
 */
SDL_Window *loadWindow();

/*! \brief Creates renderer
 *
 * Creates renderer and, if renderer creation fails,
 * prints error on the screen
 * 
 * \param window pointer to window
 * \return renderer pointer
 */
SDL_Renderer *loadRenderer(SDL_Window *window);


/*! \brief Desallocates renderer and window
 * 
 * \param window pointer to window
 * \param renderer pointer to renderer
 * \return renderer
 */
void unloadSDL(SDL_Window *window, SDL_Renderer *renderer);

/*! \brief Certifies that there are no random characteres with the json
 * 
 * Gets only the json part of a string (from a { to another })
 * 
 * \param jsonString string we want to verify
 * \return string
 */
std::string validateJsonString(std::string jsonString);
#endif
