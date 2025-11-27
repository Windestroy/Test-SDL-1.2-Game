#include <iostream>
#include <string>
#include <cmath> 
#include <cstdlib> 
#include <ctime>   
#include <SDL/SDL.h>

// --- Configuration Constants ---
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_BPP = 32;

// Player and Target Dimensions
const int PLAYER_WIDTH = 64;  
const int PLAYER_HEIGHT = 64;
const int TARGET_WIDTH = 100;
const int TARGET_HEIGHT = 100;
const int PLAYER_VELOCITY = 4; // Speed of the player (horizontal)
const int PLAYER_START_X = 50; // Initial safe position
const int PLAYER_START_Y = 150; // Initial safe position

// Beachball Physics Constants
const int BALL_WIDTH = 24;    
const int BALL_HEIGHT = 25;   
const double BOUNCE_FACTOR = 0.8; 

// Platformer Configuration
// Platform dimensions updated to 406x317
const int PLATFORM_WIDTH = 406; 
const int PLATFORM_HEIGHT = 317; 
const int PLATFORM_X = (SCREEN_WIDTH - PLATFORM_WIDTH) / 2; // Centered
const int PLATFORM_Y = SCREEN_HEIGHT - PLATFORM_HEIGHT - 50; // Positioned above the floor

// Player Physics for Platformer Mode
const double JUMP_VELOCITY = -12.0; 
const double PLATFORM_GRAVITY = 0.8; 
const double FREE_ROAM_GRAVITY = 0.5; 

// Button Configuration
const int BUTTON_MARGIN = 10;

// Gravity Toggle Button Dimensions (96x53 px)
const int TOGGLE_BUTTON_WIDTH = 96;
const int TOGGLE_BUTTON_HEIGHT = 53;
const int TOGGLE_BUTTON_X = SCREEN_WIDTH - TOGGLE_BUTTON_WIDTH - BUTTON_MARGIN; // Toggle Button X
const int TOGGLE_BUTTON_Y = BUTTON_MARGIN;                                     // Toggle Button Y

// Retry Button Dimensions (87x45 px)
const int RETRY_BUTTON_WIDTH = 87;
const int RETRY_BUTTON_HEIGHT = 45;
// Retry Position (placed 1 margin to the left of the Gravity Toggle button)
const int RETRY_BUTTON_X = TOGGLE_BUTTON_X - RETRY_BUTTON_WIDTH - BUTTON_MARGIN; 
const int RETRY_BUTTON_Y = TOGGLE_BUTTON_Y; // Keep vertical alignment with margin

// --- Global Variables ---
SDL_Surface* gScreen = NULL;
SDL_Surface* gTextSurface = NULL; 
SDL_Surface* gSignSurface = NULL; 
SDL_Surface* gCursorSurface = NULL; 
SDL_Surface* gCursorClickSurface = NULL; 
SDL_Surface* gPlayerRightSurface = NULL; 
SDL_Surface* gPlayerLeftSurface = NULL;  
SDL_Surface* gBallSurface = NULL;        
SDL_Surface* gTargetSurface = NULL; // Surface for the target image

// Surfaces for Platformer mode
SDL_Surface* gPlatformSurface = NULL;
SDL_Surface* gPlatformLoseSurface = NULL;
SDL_Surface* gButtonOnSurface = NULL;
SDL_Surface* gButtonOffSurface = NULL;
SDL_Surface* gButtonRetrySurface = NULL; 

// Player Direction Enum
enum {
    PLAYER_FACING_RIGHT,
    PLAYER_FACING_LEFT
};

// Game State
int gScore = 0; 

// Follower (Cursor Image) variables
int gFollowerX = SCREEN_WIDTH / 2;
int gFollowerY = SCREEN_HEIGHT / 2;
bool gIsMouseDown = false; 

// Player variables
int gPlayerX = PLAYER_START_X;
int gPlayerY = PLAYER_START_Y;
bool gTargetColliding = false; 
int gPlayerDirection = PLAYER_FACING_RIGHT; 

// Target (Image/Box) variables
int gTargetX = SCREEN_WIDTH - 150;
int gTargetY = SCREEN_HEIGHT - 150;

// Beachball Physics Variables
double gBallX = 300.0;
double gBallY = 50.0;
double gBallVelX = 3.0;
double gBallVelY = 0.0; 

// Platformer Mode & Interaction States
bool gGravityOn = false;        
bool gPlatformLoss = false;     
double gPlayerVelY = 0.0;       
bool gIsOnGround = false;       
bool gBallGrabbed = false;      

// --- Function Declarations ---
bool init();
bool load_media();
void handle_events(bool& running);
bool check_collision(const SDL_Rect& A, const SDL_Rect& B);
void move_target_randomly(); 
void update_ball_physics();
void update_state();
void render_scene();
void clean_up();

/**
 * @brief Initializes the SDL video subsystem, creates the window.
 */
bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    gScreen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_SWSURFACE);
    
    if (gScreen == NULL) {
        std::cerr << "Failed to set video mode! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set the new window title
    SDL_WM_SetCaption("SDL Test Project", NULL);
    SDL_ShowCursor(SDL_DISABLE); 

    return true;
}

/**
 * @brief Cleans up and shuts down SDL.
 */
void clean_up() {
    if (gTextSurface != NULL) SDL_FreeSurface(gTextSurface);
    if (gSignSurface != NULL) SDL_FreeSurface(gSignSurface); 
    if (gCursorSurface != NULL) SDL_FreeSurface(gCursorSurface);
    if (gCursorClickSurface != NULL) SDL_FreeSurface(gCursorClickSurface); 
    if (gPlayerRightSurface != NULL) SDL_FreeSurface(gPlayerRightSurface); 
    if (gPlayerLeftSurface != NULL) SDL_FreeSurface(gPlayerLeftSurface);   
    if (gBallSurface != NULL) SDL_FreeSurface(gBallSurface);     
    if (gTargetSurface != NULL) SDL_FreeSurface(gTargetSurface); 
    if (gPlatformSurface != NULL) SDL_FreeSurface(gPlatformSurface);
    if (gPlatformLoseSurface != NULL) SDL_FreeSurface(gPlatformLoseSurface);
    if (gButtonOnSurface != NULL) SDL_FreeSurface(gButtonOnSurface);
    if (gButtonOffSurface != NULL) SDL_FreeSurface(gButtonOffSurface);
    if (gButtonRetrySurface != NULL) SDL_FreeSurface(gButtonRetrySurface); 

    SDL_Quit();
    std::cout << "Cleanup complete." << std::endl;
}

/**
 * @brief Loads all necessary media: images.
 */
bool load_media() {
    bool success = true;

    // --- Transparency Key Correction ---
    // Using the specific Blue color provided: R=0, G=162, B=232
    const Uint8 BLUE_R = 0;
    const Uint8 BLUE_G = 162;
    const Uint8 BLUE_B = 232;
    Uint32 transparency_key = SDL_MapRGB(gScreen->format, BLUE_R, BLUE_G, BLUE_B); 
    
    // --- Helper function for loading BMPs ---
    auto load_and_optimize = [](const char* filename, SDL_Surface*& surface, Uint32 colorKey) -> bool {
        surface = SDL_LoadBMP(filename);
        if (surface == NULL) {
            std::cerr << "ERROR: Failed to load " << filename << "! SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }
        SDL_Surface* optimized = SDL_DisplayFormat(surface);
        SDL_FreeSurface(surface);
        surface = optimized;
        
        // Apply the specified color key for transparency
        if (colorKey != 0) {
            SDL_SetColorKey(surface, SDL_SRCCOLORKEY, colorKey);
        }
        return true;
    };
    
    // Load all assets using the specific blue transparency key
    success &= load_and_optimize("text.bmp", gTextSurface, transparency_key);
    success &= load_and_optimize("sign.bmp", gSignSurface, transparency_key);
    success &= load_and_optimize("cursor.bmp", gCursorSurface, transparency_key); 
    success &= load_and_optimize("cursor_click.bmp", gCursorClickSurface, transparency_key); 
    success &= load_and_optimize("beachball.bmp", gBallSurface, transparency_key); 
    success &= load_and_optimize("target.bmp", gTargetSurface, transparency_key);
    success &= load_and_optimize("player_right.bmp", gPlayerRightSurface, transparency_key); 
    success &= load_and_optimize("player_left.bmp", gPlayerLeftSurface, transparency_key);
    success &= load_and_optimize("platform.bmp", gPlatformSurface, transparency_key);
    success &= load_and_optimize("platformlose.bmp", gPlatformLoseSurface, transparency_key);
    success &= load_and_optimize("but_grav_on.bmp", gButtonOnSurface, transparency_key);
    success &= load_and_optimize("but_grav_off.bmp", gButtonOffSurface, transparency_key);
    success &= load_and_optimize("but_grav_retry.bmp", gButtonRetrySurface, transparency_key); 

    if (!success) {
        std::cerr << "FATAL: One or more required images failed to load." << std::endl;
    }
    
    return success; 
}

/**
 * @brief Handles user input and system events.
 */
void handle_events(bool& running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            running = false;
        }
        
        // --- Mouse Button Tracking ---
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) { 
                gIsMouseDown = true;
                
                // 1. Check for Gravity Toggle Button Click (Only available if NOT in loss state)
                SDL_Rect toggleRect = {TOGGLE_BUTTON_X, TOGGLE_BUTTON_Y, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT};
                if (!gPlatformLoss && 
                    event.button.x >= toggleRect.x && event.button.x < toggleRect.x + toggleRect.w &&
                    event.button.y >= toggleRect.y && event.button.y < toggleRect.y + toggleRect.h) 
                {
                    gGravityOn = !gGravityOn; // Toggle gravity mode
                    
                    // Reset platformer state when changing mode
                    gPlatformLoss = false;
                    gPlayerVelY = 0.0;
                    gIsOnGround = false;
                    gPlayerX = PLAYER_START_X; // Reset player to safe start point
                    gPlayerY = PLAYER_START_Y;
                    
                    // Reset ball physics if switching off gravity and ball isn't grabbed
                    if (!gGravityOn && !gBallGrabbed) {
                        gBallVelY = 0.0;
                    }
                }

                // 2. Check for Retry Button Click (Only available if IN loss state)
                if (gPlatformLoss) {
                    SDL_Rect retryRect = {RETRY_BUTTON_X, RETRY_BUTTON_Y, RETRY_BUTTON_WIDTH, RETRY_BUTTON_HEIGHT};
                    
                    if (event.button.x >= retryRect.x && event.button.x < retryRect.x + retryRect.w &&
                        event.button.y >= retryRect.y && event.button.y < retryRect.y + retryRect.h) 
                    {
                        // Reset the loss state and player position
                        gPlatformLoss = false;
                        gPlayerX = PLATFORM_X + (PLATFORM_WIDTH / 2) - (PLAYER_WIDTH / 2); // Start near the platform center
                        gPlayerY = PLATFORM_Y - PLAYER_HEIGHT - 10; // Start slightly above the platform
                        gPlayerVelY = 0.0;
                        gIsOnGround = false;
                    }
                }
                
                // 3. Check for Beachball Grab
                // Only allow grabbing if we are not in the loss state
                if (!gPlatformLoss) {
                    SDL_Rect ballBox = {(Sint16)gBallX, (Sint16)gBallY, (Uint16)BALL_WIDTH, (Uint16)BALL_HEIGHT};
                    SDL_Rect clickArea = {(Sint16)event.button.x, (Sint16)event.button.y, 1, 1}; 
                    
                    if (check_collision(ballBox, clickArea)) {
                        gBallGrabbed = true;
                        gBallVelX = 0.0; // Stop ball physics when grabbed
                        gBallVelY = 0.0;
                    }
                }
            }
        } else if (event.type == SDL_MOUSEBUTTONUP) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                gIsMouseDown = false;
                gBallGrabbed = false; // Release the ball
            }
        }

        // --- Follower (Mouse) Position Logic ---
        if (event.type == SDL_MOUSEMOTION) {
            int mouseX = event.motion.x;
            int mouseY = event.motion.y;
            
            if (gCursorSurface != NULL) {
                // gFollowerX/Y is calculated to be the top-left corner needed to center the cursor image
                int cursorWidth = gCursorSurface->w;
                int cursorHeight = gCursorSurface->h;
                
                gFollowerX = mouseX - (cursorWidth / 2);
                gFollowerY = mouseY - (cursorHeight / 2);
            }
        }
    }
}

/**
 * @brief Performs AABB (Axis-Aligned Bounding Box) collision detection.
 */
bool check_collision(const SDL_Rect& A, const SDL_Rect& B) {
    if (A.y + A.h <= B.y) return false; 
    if (A.y >= B.y + B.h) return false; 
    if (A.x + A.w <= B.x) return false; 
    if (A.x >= B.x + B.w) return false; 
    
    return true;
}

/**
 * @brief Moves the target box to a random, safe location on screen.
 */
void move_target_randomly() {
    int maxX = SCREEN_WIDTH - TARGET_WIDTH;
    int maxY = SCREEN_HEIGHT - TARGET_HEIGHT;
    
    gTargetX = rand() % maxX;
    gTargetY = rand() % maxY;
}

/**
 * @brief Updates the position and velocity of the beachball based on physics.
 */
void update_ball_physics() {
    if (gBallGrabbed) {
        // Ball follows the cursor (centered on the cursor image)
        if (gCursorSurface != NULL) {
            gBallX = gFollowerX + (gCursorSurface->w / 2) - (BALL_WIDTH / 2);
            gBallY = gFollowerY + (gCursorSurface->h / 2) - (BALL_HEIGHT / 2);
        }
        
        // Clamp to screen bounds
        if (gBallX < 0) gBallX = 0;
        if (gBallX + BALL_WIDTH > SCREEN_WIDTH) gBallX = SCREEN_WIDTH - BALL_WIDTH;
        if (gBallY < 0) gBallY = 0;
        if (gBallY + BALL_HEIGHT > SCREEN_HEIGHT) gBallY = SCREEN_HEIGHT - BALL_HEIGHT;
        
        return; 
    }

    // 1. Apply Gravity to Y Velocity
    gBallVelY += FREE_ROAM_GRAVITY;

    // 2. Update Position
    gBallX += gBallVelX;
    gBallY += gBallVelY;
    
    // 3. Screen Edge Collision (Walls)
    
    // Horizontal Bounds
    if (gBallX < 0) {
        gBallX = 0;
        gBallVelX *= -BOUNCE_FACTOR;
    } else if (gBallX + BALL_WIDTH > SCREEN_WIDTH) {
        gBallX = SCREEN_WIDTH - BALL_WIDTH;
        gBallVelX *= -BOUNCE_FACTOR;
    }

    // Vertical Bounds
    if (gBallY < 0) { // Top edge
        gBallY = 0;
        gBallVelY *= -BOUNCE_FACTOR;
    } else if (gBallY + BALL_HEIGHT > SCREEN_HEIGHT) { // Bottom edge
        gBallY = SCREEN_HEIGHT - BALL_HEIGHT;
        gBallVelY *= -BOUNCE_FACTOR;
        if (std::abs(gBallVelY) < FREE_ROAM_GRAVITY) {
            gBallVelY = 0; 
        }
    }
    
    // 4. Player Collision (AABB) - Check only if not in Platform Loss mode
    if (!gPlatformLoss) {
        SDL_Rect playerBox = {(Sint16)gPlayerX, (Sint16)gPlayerY, (Uint16)PLAYER_WIDTH, (Uint16)PLAYER_HEIGHT};
        SDL_Rect ballBox = {(Sint16)gBallX, (Sint16)gBallY, (Uint16)BALL_WIDTH, (Uint16)BALL_HEIGHT}; 
        
        if (check_collision(playerBox, ballBox)) {
            // Simple bounce logic (simplified for AABB)
            int playerCenterX = gPlayerX + PLAYER_WIDTH / 2;
            int playerCenterY = gPlayerY + PLAYER_HEIGHT / 2;
            int ballCenterX = (int)gBallX + BALL_WIDTH / 2;
            int ballCenterY = (int)gBallY + BALL_HEIGHT / 2;

            int dx = ballCenterX - playerCenterX;
            int dy = ballCenterY - playerCenterY;
            
            if (std::abs(dx) > std::abs(dy)) {
                gBallVelX = std::copysign(gBallVelX * -BOUNCE_FACTOR, (double)dx);
                if (dx > 0) gBallX = gPlayerX + PLAYER_WIDTH;
                else gBallX = gPlayerX - BALL_WIDTH;
            } else {
                gBallVelY = std::copysign(gBallVelY * -BOUNCE_FACTOR, (double)dy);
                if (dy > 0) gBallY = gPlayerY + PLAYER_HEIGHT;
                else gBallY = gPlayerY - BALL_HEIGHT;
            }
        }
    }
}

/**
 * @brief Updates the positions of all game objects and checks for collisions.
 */
void update_state() {
    Uint8 *keystates = SDL_GetKeyState(NULL);
    
    if (!gGravityOn) {
        // ------------------------------------------------
        // A. FREE-ROAM MODE (Existing Movement Logic)
        // ------------------------------------------------
        
        bool isHorizontal = keystates[SDLK_LEFT] || keystates[SDLK_RIGHT];
        bool isVertical = keystates[SDLK_UP] || keystates[SDLK_DOWN];
        double speedScale = (isHorizontal && isVertical) ? 0.707 : 1.0; 
        
        int moveX = 0;
        int moveY = 0;

        if (keystates[SDLK_UP]) moveY -= PLAYER_VELOCITY;
        if (keystates[SDLK_DOWN]) moveY += PLAYER_VELOCITY;
        
        if (keystates[SDLK_LEFT]) {
            moveX -= PLAYER_VELOCITY;
            gPlayerDirection = PLAYER_FACING_LEFT;
        }
        if (keystates[SDLK_RIGHT]) {
            moveX += PLAYER_VELOCITY;
            gPlayerDirection = PLAYER_FACING_RIGHT;
        }

        gPlayerX += (int)(moveX * speedScale);
        gPlayerY += (int)(moveY * speedScale);
        
        // Reset platformer variables 
        gIsOnGround = false;
        gPlayerVelY = 0.0;
        gPlatformLoss = false;
        
    } else {
        // ------------------------------------------------
        // B. PLATFORMER MODE (New Logic)
        // ------------------------------------------------
        
        // If loss state is active, player movement is locked
        if (!gPlatformLoss) {
            // 1. Horizontal Movement (Left/Right)
            if (keystates[SDLK_LEFT]) {
                gPlayerX -= PLAYER_VELOCITY;
                gPlayerDirection = PLAYER_FACING_LEFT;
            }
            if (keystates[SDLK_RIGHT]) {
                gPlayerX += PLAYER_VELOCITY;
                gPlayerDirection = PLAYER_FACING_RIGHT;
            }

            // 2. Jumping (only if on ground)
            if (keystates[SDLK_UP] && gIsOnGround) {
                gPlayerVelY = JUMP_VELOCITY; 
                gIsOnGround = false;         
            }
            
            // 3. Apply Player Gravity & Vertical Movement
            gPlayerVelY += PLATFORM_GRAVITY;
            gPlayerY += (int)gPlayerVelY;

            // 4. Platform and Floor Collision
            
            // Rectangles for collision checks
            SDL_Rect playerBox = {(Sint16)gPlayerX, (Sint16)gPlayerY, (Uint16)PLAYER_WIDTH, (Uint16)PLAYER_HEIGHT};
            SDL_Rect platformBox = {PLATFORM_X, PLATFORM_Y, PLATFORM_WIDTH, PLATFORM_HEIGHT};

            // Check 4a: Player vs. Platform
            if (check_collision(playerBox, platformBox) && gPlayerY + PLAYER_HEIGHT < platformBox.y + PLAYER_VELOCITY) { 
                // Collision from above (player is falling slowly or resting)
                if (gPlayerVelY >= 0.0) {
                    gPlayerY = platformBox.y - PLAYER_HEIGHT; // Snap to the top
                    gPlayerVelY = 0.0;                       // Stop falling
                    gIsOnGround = true;
                }
            } else if (gIsOnGround) {
                // Check if player walked off the platform
                gIsOnGround = false;
            }

            // Check 4b: Player vs. Bottom of Screen (Loss Condition)
            if (gPlayerY + PLAYER_HEIGHT >= SCREEN_HEIGHT) {
                gPlayerY = SCREEN_HEIGHT - PLAYER_HEIGHT; // Snap to floor
                gPlayerVelY = 0.0;
                gIsOnGround = true;
                
                // Loss condition: player touches the lowest point (floor)
                gPlatformLoss = true;
            }
        }
    } // END PLATFORMER MODE

    // --- Screen Boundary Check (Player) ---
    if (gPlayerX < 0) gPlayerX = 0;
    else if (gPlayerX + PLAYER_WIDTH > SCREEN_WIDTH) gPlayerX = SCREEN_WIDTH - PLAYER_WIDTH;
    if (!gGravityOn) {
        if (gPlayerY < 0) gPlayerY = 0;
        else if (gPlayerY + PLAYER_HEIGHT > SCREEN_HEIGHT) gPlayerY = SCREEN_HEIGHT - PLAYER_HEIGHT;
    }
    
    // --- Ball Physics (Applies in both modes) ---
    update_ball_physics();
    
    // --- Target Collision & Scoring Check (applies in both modes) ---
    SDL_Rect playerBox = {(Sint16)gPlayerX, (Sint16)gPlayerY, (Uint16)PLAYER_WIDTH, (Uint16)PLAYER_HEIGHT};
    // The target box uses the defined constants for collision area
    SDL_Rect targetBox = {(Sint16)gTargetX, (Sint16)gTargetY, (Uint16)TARGET_WIDTH, (Uint16)TARGET_HEIGHT}; 
    
    bool wasColliding = gTargetColliding; 
    gTargetColliding = check_collision(playerBox, targetBox);
    
    if (gTargetColliding && !wasColliding) { 
        gScore++;
        move_target_randomly(); 
    }
}

/**
 * @brief Clears the screen and draws all game elements.
 */
void render_scene() {
    // 1. Clear the screen (Fill with black)
    Uint32 black = SDL_MapRGB(gScreen->format, 0, 0, 0);
    SDL_FillRect(gScreen, NULL, black);

    // 2. Draw the Sign Image (Background element)
    if (gSignSurface != NULL) {
        SDL_Rect destRect = {(Sint16)((SCREEN_WIDTH - gSignSurface->w) / 2), (Sint16)((SCREEN_HEIGHT - gSignSurface->h) / 2), 0, 0}; 
        SDL_BlitSurface(gSignSurface, NULL, gScreen, &destRect);
    }

    // 3. Draw the pre-rendered text image (e.g., "SDL 1998")
    if (gTextSurface != NULL) {
        SDL_Rect destRect = {20, 20, 0, 0}; 
        SDL_BlitSurface(gTextSurface, NULL, gScreen, &destRect);
    }
    
    // 4. Draw Gravity Button and Retry Button

    if (gGravityOn && gPlatformLoss) {
        // Draw the Retry Button only if gravity is on AND we lost
        if (gButtonRetrySurface != NULL) {
            SDL_Rect retryDest = {RETRY_BUTTON_X, RETRY_BUTTON_Y, 0, 0}; // Dimensions handled by blit
            SDL_BlitSurface(gButtonRetrySurface, NULL, gScreen, &retryDest);
        }
    }
    
    // Draw the Toggle Button (but only if NOT in loss state, so player must retry first)
    if (!gPlatformLoss) {
        SDL_Surface* currentButton = gGravityOn ? gButtonOnSurface : gButtonOffSurface;
        if (currentButton != NULL) {
            // Use the new TOGGLE button constants for drawing
            SDL_Rect buttonDest = {TOGGLE_BUTTON_X, TOGGLE_BUTTON_Y, 0, 0}; 
            SDL_BlitSurface(currentButton, NULL, gScreen, &buttonDest);
        }
    }


    // 5. Draw Platform (if Gravity is ON)
    if (gGravityOn) {
        SDL_Surface* currentPlatform = gPlatformSurface;
        if (gPlatformLoss && gPlatformLoseSurface != NULL) {
            currentPlatform = gPlatformLoseSurface; // Switch to loss texture
        }

        // To visualize the new, larger collision area, we draw a filled rect as a placeholder:
        SDL_Rect platformDest = {PLATFORM_X, PLATFORM_Y, PLATFORM_WIDTH, PLATFORM_HEIGHT};
        Uint32 platformColor = SDL_MapRGB(gScreen->format, 100, 100, 100); // Dark gray fill
        SDL_FillRect(gScreen, &platformDest, platformColor);
        
        if (currentPlatform != NULL) {
            // We draw the original platform image over the top-left of the filled rectangle for visual context.
            SDL_Rect imageDest = {PLATFORM_X, PLATFORM_Y, 0, 0};
            SDL_BlitSurface(currentPlatform, NULL, gScreen, &imageDest);
        }
    }

    // 6. Draw Player Image based on direction
    SDL_Surface* currentSurface = NULL;
    if (gPlayerDirection == PLAYER_FACING_RIGHT && gPlayerRightSurface != NULL) {
        currentSurface = gPlayerRightSurface;
    } else if (gPlayerDirection == PLAYER_FACING_LEFT && gPlayerLeftSurface != NULL) {
        currentSurface = gPlayerLeftSurface;
    }
    
    if (currentSurface != NULL) {
        SDL_Rect playerDest = {(Sint16)gPlayerX, (Sint16)gPlayerY, 0, 0};
        SDL_BlitSurface(currentSurface, NULL, gScreen, &playerDest);
    } else {
        SDL_Rect playerBox = {(Sint16)gPlayerX, (Sint16)gPlayerY, (Uint16)PLAYER_WIDTH, (Uint16)PLAYER_HEIGHT};
        Uint32 fallbackRed = SDL_MapRGB(gScreen->format, 255, 0, 0);
        SDL_FillRect(gScreen, &playerBox, fallbackRed);
    }


    // 7. Draw Target Image (Replaces Blue/Yellow Box)
    if (gTargetSurface != NULL) {
        // Draw the target image. The collision area is still based on TARGET_WIDTH/HEIGHT constants.
        SDL_Rect targetDest = {(Sint16)gTargetX, (Sint16)gTargetY, 0, 0};
        SDL_BlitSurface(gTargetSurface, NULL, gScreen, &targetDest);
        
    } else {
        // Fallback (original blue box drawing) if the target image fails to load
        SDL_Rect blueBox = {(Sint16)gTargetX, (Sint16)gTargetY, (Uint16)TARGET_WIDTH, (Uint16)TARGET_HEIGHT};
        Uint32 fallbackBlue = SDL_MapRGB(gScreen->format, 0, 0, 255);
        SDL_FillRect(gScreen, &blueBox, fallbackBlue);
    }
    
    // 8. Draw Beachball
    if (gBallSurface != NULL) {
        SDL_Rect ballDest = {(Sint16)gBallX, (Sint16)gBallY, 0, 0};
        SDL_BlitSurface(gBallSurface, NULL, gScreen, &ballDest);
    }
    

    // 9. Draw the Cursor Follower (Foreground element)
    SDL_Surface* currentCursor = NULL;
    
    // Select the cursor image based on whether the mouse button is down
    if (gIsMouseDown && gCursorClickSurface != NULL) {
        currentCursor = gCursorClickSurface;
    } else if (gCursorSurface != NULL) {
        currentCursor = gCursorSurface;
    }
    
    if (currentCursor != NULL) {
        // gFollowerX/Y are calculated to center the cursor image
        SDL_Rect followerDest = {(Sint16)gFollowerX, (Sint16)gFollowerY, 0, 0}; 
        SDL_BlitSurface(currentCursor, NULL, gScreen, &followerDest);
    }

    // 10. Update the Screen
    if (SDL_Flip(gScreen) == -1) {
        std::cerr << "SDL_Flip failed!" << std::endl;
    }
}

int main(int argc, char* args[]) {
    srand(time(NULL)); 

    if (!init()) {
        return 1;
    }
    
    if (!load_media()) {
        clean_up();
        return 1;
    }

    bool isRunning = true;

    // --- Main Game Loop ---
    while (isRunning) {
        handle_events(isRunning);
        update_state(); 
        SDL_Delay(10); 
        render_scene();
    }

    clean_up();
    return 0;
}
