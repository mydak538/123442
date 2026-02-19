/* 
   Linux GTA-like Game
   Compile: gcc -o gta_linux main.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -lXrandr -lXi -lXcursor
*/

#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

// Settings
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define MAX_VEHICLES 25
#define MAX_BUILDINGS 150
#define MAX_PEDS 30
#define MAX_PROJECTILES 200

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

// Structures
typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 size;
    float rotation;
    float pitch;
    float speed;
    float health;
    bool inVehicle;
    int wantedLevel;
    int money;
    Camera3D camera;
    Model model;
    bool isJumping;
    float jumpForce;
    Vector2 lastMousePosition;
} Player;

typedef struct {
    Model model;
    Vector3 position;
    Vector3 velocity;
    Vector3 size;
    float rotation;
    float speed;
    Color color;
    bool occupied;
    float wheelRotation;
    float health;
    bool engineOn;
    float hornTimer;
} Vehicle;

typedef struct {
    Vector3 position;
    Vector3 size;
    Color color;
    Model model;
    bool hasCollision;
    int type; // 0-road, 1-house, 2-skyscraper, 3-shop, 4-sidewalk
} Building;

typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 size;
    Color color;
    float rotation;
    float speed;
    bool inVehicle;
    int type; // 0-civilian, 1-police, 2-gangster
    float stateTime;
    int state; // 0-idle, 1-walking, 2-running
} Pedestrian;

typedef struct {
    Vector3 position;
    Vector3 velocity;
    Color color;
    float size;
    float lifetime;
    bool active;
} Projectile;

typedef struct {
    Vector3 position;
    float radius;
    float time;
    bool active;
} Explosion;

// Global variables
static Player player;
static Vehicle vehicles[MAX_VEHICLES];
static Building buildings[MAX_BUILDINGS];
static Pedestrian peds[MAX_PEDS];
static Projectile projectiles[MAX_PROJECTILES];
static Explosion explosions[10];
static Model policeCarModel, sedanModel, truckModel, suvModel, sportCarModel;
static Model buildingModels[4];
static Texture2D roadTexture, grassTexture, buildingTexture, sidewalkTexture;
static RenderTexture2D screenTarget;
static Shader crtShader;
static int score = 0;
static float gameTime = 0.0f;
static bool gamePaused = false;
static int currentWeapon = 0; // 0-fists, 1-pistol, 2-machinegun
static Texture2D crosshair;
static bool firstPerson = true;
static bool fullscreen = true;
static bool showDebug = false;

// Function prototypes
void InitGame(void);
void UpdateGame(void);
void DrawGame(void);
void DrawHUD(void);
void SpawnVehicle(Vector3 position, int type);
void SpawnBuilding(Vector3 position, Vector3 size, int type);
void SpawnPedestrian(Vector3 position);
void UpdatePlayerMovement(void);
void UpdateVehicle(int index);
void UpdatePedestrian(int index);
void ShootProjectile(Vector3 position, Vector3 direction);
void UpdateProjectiles(void);
void CreateExplosion(Vector3 position);
void UpdateExplosions(void);
bool CheckCollision(Vector3 pos1, Vector3 size1, Vector3 pos2, Vector3 size2);
void DrawMinimap(void);
void DrawWantedStars(void);
void DrawCrosshair(void);
void UpdateVehicleAI(int index);
void HandleProjectileCollisions(int index);
void CreateRoadSystem(void);
void CreateBuildingComplex(Vector3 center, int count);
void DrawVehicleWithDetails(Vehicle *vehicle);
void CreateTextures(void);

int main(void) {
    // Initialize window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "GTA Linux Clone");
    
    if (fullscreen) {
        ToggleFullscreen();
    }
    
    InitAudioDevice();
    SetTargetFPS(60);
    SetExitKey(KEY_NULL); // Disable ESC to exit
    
    // Initialize game
    InitGame();
    
    // Hide cursor and center mouse
    if (firstPerson) {
        DisableCursor();
        player.lastMousePosition = GetMousePosition();
    }
    
    // Main game loop
    while (!WindowShouldClose()) {
        // Toggle fullscreen with F11
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }
        
        // Toggle debug with F3
        if (IsKeyPressed(KEY_F3)) {
            showDebug = !showDebug;
        }
        
        // Toggle first/third person with V
        if (IsKeyPressed(KEY_V)) {
            firstPerson = !firstPerson;
            if (firstPerson) {
                DisableCursor();
            } else {
                EnableCursor();
            }
        }
        
        // Pause game with P
        if (IsKeyPressed(KEY_P)) gamePaused = !gamePaused;
        if (gamePaused) {
            BeginDrawing();
            ClearBackground(DARKBLUE);
            DrawText("GAME PAUSED", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 50, 60, YELLOW);
            DrawText("Press P to continue", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 + 50, 30, WHITE);
            DrawText("V - Toggle camera | F11 - Fullscreen | F3 - Debug", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 + 100, 20, LIGHTGRAY);
            EndDrawing();
            continue;
        }
        
        UpdateGame();
        
        // Start drawing
        BeginDrawing();
            ClearBackground(SKYBLUE);
            
            // 3D rendering
            BeginMode3D(player.camera);
                DrawGame();
            EndMode3D();
            
            // Interface
            DrawHUD();
            DrawMinimap();
            DrawWantedStars();
            DrawCrosshair();
            
            // Debug info
            if (showDebug) {
                DrawFPS(10, SCREEN_HEIGHT - 30);
                DrawText(TextFormat("Money: $%d", player.money), SCREEN_WIDTH - 200, 30, 20, GREEN);
                DrawText(TextFormat("Health: %.0f", player.health), SCREEN_WIDTH - 200, 60, 20, RED);
                DrawText(TextFormat("Score: %d", score), SCREEN_WIDTH - 200, 90, 20, YELLOW);
                DrawText(TextFormat("FPS: %d", GetFPS()), SCREEN_WIDTH - 200, 120, 20, LIGHTGRAY);
                DrawText(TextFormat("Pos: %.1f, %.1f, %.1f", player.position.x, player.position.y, player.position.z), 
                         SCREEN_WIDTH - 400, SCREEN_HEIGHT - 60, 20, WHITE);
                DrawText(TextFormat("Vehicles: %d/%d", 
                         MAX_VEHICLES - 10, MAX_VEHICLES), SCREEN_WIDTH - 400, SCREEN_HEIGHT - 30, 20, WHITE);
            }
            
            // Camera mode indicator
            DrawText(firstPerson ? "First Person [V]" : "Third Person [V]", SCREEN_WIDTH - 200, 150, 20, WHITE);
            
        EndDrawing();
    }
    
    // Cleanup
    UnloadModel(player.model);
    UnloadModel(policeCarModel);
    UnloadModel(sedanModel);
    UnloadModel(truckModel);
    UnloadModel(suvModel);
    UnloadModel(sportCarModel);
    for (int i = 0; i < 4; i++) UnloadModel(buildingModels[i]);
    UnloadTexture(roadTexture);
    UnloadTexture(grassTexture);
    UnloadTexture(sidewalkTexture);
    UnloadTexture(crosshair);
    UnloadShader(crtShader);
    UnloadRenderTexture(screenTarget);
    
    CloseAudioDevice();
    CloseWindow();
    
    return 0;
}

// Create textures
void CreateTextures(void) {
    // Grass texture (solid green)
    Image grassImg = GenImageColor(256, 256, DARKGREEN);
    // Add some variation
    for (int y = 0; y < 256; y += 8) {
        for (int x = 0; x < 256; x += 8) {
            Color col = ColorAlpha(GREEN, 0.2f);
            ImageDrawRectangle(&grassImg, x, y, 8, 8, col);
        }
    }
    grassTexture = LoadTextureFromImage(grassImg);
    UnloadImage(grassImg);
    
    // Road texture with markings
    Image roadImg = GenImageColor(256, 256, DARKGRAY);
    // Road markings
    for (int y = 0; y < 256; y += 32) {
        for (int x = 0; x < 256; x += 32) {
            // Center line
            ImageDrawRectangle(&roadImg, 120, y, 16, 16, YELLOW);
            // Side lines
            ImageDrawRectangle(&roadImg, 30, y, 8, 16, WHITE);
            ImageDrawRectangle(&roadImg, 218, y, 8, 16, WHITE);
        }
    }
    roadTexture = LoadTextureFromImage(roadImg);
    UnloadImage(roadImg);
    
    // Sidewalk texture
    Image sidewalkImg = GenImageColor(256, 256, (Color){180, 180, 180, 255});
    for (int y = 0; y < 256; y += 16) {
        for (int x = 0; x < 256; x += 16) {
            if ((x/16 + y/16) % 2 == 0) {
                ImageDrawRectangle(&sidewalkImg, x, y, 16, 16, (Color){200, 200, 200, 255});
            }
        }
    }
    sidewalkTexture = LoadTextureFromImage(sidewalkImg);
    UnloadImage(sidewalkImg);
}

// Initialize game
void InitGame(void) {
    srand(time(NULL));
    
    // Initialize player
    player.position = (Vector3){ 0.0f, 1.0f, 0.0f };
    player.velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
    player.size = (Vector3){ 0.5f, 1.8f, 0.5f };
    player.rotation = 0.0f;
    player.pitch = 0.0f;
    player.speed = 5.0f;
    player.health = 100.0f;
    player.inVehicle = false;
    player.wantedLevel = 0;
    player.money = 1000;
    player.isJumping = false;
    player.jumpForce = 0.0f;
    player.lastMousePosition = (Vector2){0};
    
    // Player camera
    player.camera.position = player.position;
    player.camera.target = (Vector3){ 0.0f, 1.6f, 0.0f };
    player.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    player.camera.fovy = 90.0f;
    player.camera.projection = CAMERA_PERSPECTIVE;
    
    // Create player model (cube instead of loading model)
    player.model = LoadModelFromMesh(GenMeshCube(0.5f, 1.8f, 0.5f));
    player.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = BLUE;
    
    // Create vehicle models with better meshes
    policeCarModel = LoadModelFromMesh(GenMeshCube(2.0f, 1.2f, 4.5f));
    policeCarModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = DARKBLUE;
    
    sedanModel = LoadModelFromMesh(GenMeshCube(1.8f, 1.2f, 4.0f));
    sedanModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = RED;
    
    truckModel = LoadModelFromMesh(GenMeshCube(2.8f, 2.2f, 6.0f));
    truckModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = DARKGREEN;
    
    suvModel = LoadModelFromMesh(GenMeshCube(2.3f, 1.8f, 5.0f));
    suvModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = GRAY;
    
    sportCarModel = LoadModelFromMesh(GenMeshCube(1.6f, 0.9f, 3.8f));
    sportCarModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = YELLOW;
    
    // Create building models
    // House
    buildingModels[0] = LoadModelFromMesh(GenMeshCube(8.0f, 6.0f, 8.0f));
    buildingModels[0].materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){180, 160, 140, 255};
    
    // Skyscraper
    buildingModels[1] = LoadModelFromMesh(GenMeshCube(10.0f, 30.0f, 10.0f));
    buildingModels[1].materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){150, 150, 170, 255};
    
    // Shop
    buildingModels[2] = LoadModelFromMesh(GenMeshCube(12.0f, 4.0f, 12.0f));
    buildingModels[2].materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){200, 100, 100, 255};
    
    // Office building
    buildingModels[3] = LoadModelFromMesh(GenMeshCube(15.0f, 20.0f, 15.0f));
    buildingModels[3].materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){120, 140, 160, 255};
    
    // Create textures
    CreateTextures();
    
    // Crosshair
    Image crosshairImg = GenImageColor(32, 32, BLANK);
    ImageDrawCircle(&crosshairImg, 16, 16, 12, RED);
    ImageDrawCircleLines(&crosshairImg, 16, 16, 12, WHITE);
    ImageDrawLine(&crosshairImg, 16, 0, 16, 10, WHITE);
    ImageDrawLine(&crosshairImg, 16, 22, 16, 32, WHITE);
    ImageDrawLine(&crosshairImg, 0, 16, 10, 16, WHITE);
    ImageDrawLine(&crosshairImg, 22, 16, 32, 16, WHITE);
    crosshair = LoadTextureFromImage(crosshairImg);
    UnloadImage(crosshairImg);
    
    // Initialize vehicles
    for (int i = 0; i < MAX_VEHICLES; i++) {
        vehicles[i].model.meshCount = 0;
        vehicles[i].occupied = false;
        vehicles[i].health = 0;
    }
    
    // Initialize buildings
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        buildings[i].hasCollision = false;
    }
    
    // Initialize pedestrians
    for (int i = 0; i < MAX_PEDS; i++) {
        peds[i].type = -1;
    }
    
    // Initialize projectiles
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        projectiles[i].active = false;
    }
    
    // Initialize explosions
    for (int i = 0; i < 10; i++) {
        explosions[i].active = false;
    }
    
    // Create city
    CreateRoadSystem();
    
    // Create building complexes
    CreateBuildingComplex((Vector3){-40, 0, -40}, 8);
    CreateBuildingComplex((Vector3){40, 0, -40}, 6);
    CreateBuildingComplex((Vector3){-40, 0, 40}, 6);
    CreateBuildingComplex((Vector3){40, 0, 40}, 10);
    
    // Spawn vehicles - more civilian cars, fewer police
    int vehicleCount = 0;
    while (vehicleCount < MAX_VEHICLES - 5) { // Leave room for police
        int x = (rand() % 140) - 70;
        int z = (rand() % 140) - 70;
        
        // Don't spawn on buildings
        bool canSpawn = true;
        for (int i = 0; i < MAX_BUILDINGS; i++) {
            if (buildings[i].hasCollision && buildings[i].type > 0) {
                if (Vector3Distance((Vector3){ (float)x, 0, (float)z }, buildings[i].position) < 10.0f) {
                    canSpawn = false;
                    break;
                }
            }
        }
        
        if (canSpawn) {
            // 80% civilian cars, 20% others
            int type;
            if (rand() % 100 < 80) {
                type = 1 + rand() % 3; // Sedan, truck, SUV
            } else {
                type = rand() % 5; // All types including police and sport
            }
            
            SpawnVehicle((Vector3){ (float)x, 0.5f, (float)z }, type);
            vehicleCount++;
        }
    }
    
    // Spawn pedestrians
    for (int i = 0; i < MAX_PEDS/2; i++) {
        int x = (rand() % 100) - 50;
        int z = (rand() % 100) - 50;
        SpawnPedestrian((Vector3){ (float)x, 1.0f, (float)z });
    }
    
    // Shader (post-processing)
    crtShader = LoadShader(0, "resources/crt.fs");
    screenTarget = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
}

// Create road system with sidewalks
void CreateRoadSystem(void) {
    // Main roads with sidewalks
    for (int x = -60; x <= 60; x += 40) {
        // Road
        SpawnBuilding((Vector3){ (float)x, -0.4f, 0.0f }, 
                     (Vector3){ 12.0f, 0.2f, 140.0f }, 0);
        
        // Sidewalks
        SpawnBuilding((Vector3){ (float)x - 8.0f, -0.3f, 0.0f }, 
                     (Vector3){ 4.0f, 0.1f, 140.0f }, 4);
        SpawnBuilding((Vector3){ (float)x + 8.0f, -0.3f, 0.0f }, 
                     (Vector3){ 4.0f, 0.1f, 140.0f }, 4);
    }
    
    for (int z = -60; z <= 60; z += 40) {
        // Road
        SpawnBuilding((Vector3){ 0.0f, -0.4f, (float)z }, 
                     (Vector3){ 140.0f, 0.2f, 12.0f }, 0);
        
        // Sidewalks
        SpawnBuilding((Vector3){ 0.0f, -0.3f, (float)z - 8.0f }, 
                     (Vector3){ 140.0f, 0.1f, 4.0f }, 4);
        SpawnBuilding((Vector3){ 0.0f, -0.3f, (float)z + 8.0f }, 
                     (Vector3){ 140.0f, 0.1f, 4.0f }, 4);
    }
    
    // Smaller connecting roads
    for (int x = -50; x <= 50; x += 20) {
        for (int z = -50; z <= 50; z += 20) {
            if (abs(x) < 40 && abs(z) < 40) {
                // Small road
                SpawnBuilding((Vector3){ (float)x, -0.45f, (float)z }, 
                             (Vector3){ 6.0f, 0.15f, 6.0f }, 0);
            }
        }
    }
}

// Create building complex
void CreateBuildingComplex(Vector3 center, int count) {
    for (int i = 0; i < count; i++) {
        float x = center.x + (rand() % 40) - 20;
        float z = center.z + (rand() % 40) - 20;
        int type = 1 + rand() % 3;
        float height = 0.0f;
        
        switch(type) {
            case 1: height = 6.0f + (rand() % 10); break; // House
            case 2: height = 15.0f + (rand() % 15); break; // Skyscraper
            case 3: height = 4.0f + (rand() % 6); break; // Shop
        }
        
        Vector3 size = (Vector3){ 
            6.0f + (rand() % 6), 
            height, 
            6.0f + (rand() % 6) 
        };
        
        SpawnBuilding((Vector3){ x, height/2.0f, z }, size, type);
    }
}

// Spawn pedestrian
void SpawnPedestrian(Vector3 position) {
    for (int i = 0; i < MAX_PEDS; i++) {
        if (peds[i].type < 0) {
            peds[i].position = position;
            peds[i].velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
            peds[i].size = (Vector3){ 0.4f, 1.7f, 0.4f };
            peds[i].rotation = (rand() % 360) * DEG2RAD;
            peds[i].speed = 1.0f + (rand() % 3);
            peds[i].inVehicle = false;
            peds[i].type = rand() % 100 < 90 ? 0 : (rand() % 2 + 1); // 90% civilians
            peds[i].stateTime = 0.0f;
            peds[i].state = rand() % 3;
            
            switch(peds[i].type) {
                case 0: // Civilian
                    peds[i].color = (Color){ 
                        200 + rand() % 55, 
                        200 + rand() % 55, 
                        200 + rand() % 55, 
                        255 
                    };
                    break;
                case 1: // Police
                    peds[i].color = DARKBLUE;
                    break;
                case 2: // Gangster
                    peds[i].color = RED;
                    break;
            }
            break;
        }
    }
}

// Update game
void UpdateGame(void) {
    gameTime += GetFrameTime();
    
    // Player movement with mouse look
    UpdatePlayerMovement();
    
    // Update vehicles
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (vehicles[i].model.meshCount > 0) {
            UpdateVehicle(i);
        }
    }
    
    // Update pedestrians
    for (int i = 0; i < MAX_PEDS; i++) {
        if (peds[i].type >= 0) {
            UpdatePedestrian(i);
        }
    }
    
    // Update projectiles
    UpdateProjectiles();
    
    // Update explosions
    UpdateExplosions();
    
    // Spawn police at high wanted level (less frequent)
    if (player.wantedLevel > 0 && rand() % 200 == 0) {
        Vector3 spawnPos = Vector3Add(player.position, (Vector3){ 
            (rand() % 80) - 40, 
            0.5f, 
            (rand() % 80) - 40 
        });
        SpawnVehicle(spawnPos, 0); // Police car
    }
    
    // Spawn random pedestrians
    if (rand() % 300 == 0) {
        Vector3 spawnPos = Vector3Add(player.position, (Vector3){ 
            (rand() % 120) - 60, 
            1.0f, 
            (rand() % 120) - 60 
        });
        SpawnPedestrian(spawnPos);
    }
    
    // Spawn civilian cars occasionally
    if (rand() % 400 == 0) {
        Vector3 spawnPos = Vector3Add(player.position, (Vector3){ 
            (rand() % 150) - 75, 
            0.5f, 
            (rand() % 150) - 75 
        });
        
        // Only spawn civilian cars (1-3: sedan, truck, SUV)
        int type = 1 + rand() % 3;
        SpawnVehicle(spawnPos, type);
    }
}

// Draw game
void DrawGame(void) {
    // Ground with grass texture
    DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 240.0f, 240.0f }, GREEN);
    
    // Buildings
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (buildings[i].hasCollision) {
            if (buildings[i].type == 0) { // Road
                DrawCube(buildings[i].position,
                              buildings[i].size.x, buildings[i].size.y, buildings[i].size.z, WHITE);
                
                // Road markings
                if (buildings[i].size.x > 50.0f || buildings[i].size.z > 50.0f) {
                    // Center line
                    DrawCube((Vector3){buildings[i].position.x, buildings[i].position.y + 0.03f, buildings[i].position.z},
                            buildings[i].size.x > 50.0f ? 0.5f : buildings[i].size.x,
                            0.02f,
                            buildings[i].size.z > 50.0f ? 0.5f : buildings[i].size.z,
                            YELLOW);
                }
            } else if (buildings[i].type == 4) { // Sidewalk
                DrawCube(buildings[i].position, 
                              buildings[i].size.x, buildings[i].size.y, buildings[i].size.z, WHITE);
            } else { // Building
                DrawModel(buildingModels[buildings[i].type - 1], buildings[i].position, 1.0f, WHITE);
                
                // Windows
                if (buildings[i].type > 1) { // Not houses
                    for (float y = 2.0f; y < buildings[i].size.y; y += 4.0f) {
                        for (float x = -2.0f; x <= 2.0f; x += 4.0f) {
                            for (float z = -2.0f; z <= 2.0f; z += 4.0f) {
                                if (rand() % 3 == 0) { // Random windows
                                    DrawCube((Vector3){ 
                                        buildings[i].position.x + x, 
                                        buildings[i].position.y - buildings[i].size.y/2 + y, 
                                        buildings[i].position.z + z 
                                    }, 1.5f, 2.0f, 0.1f, YELLOW);
                                }
                            }
                        }
                    }
                } else { // House windows
                    DrawCube((Vector3){ 
                        buildings[i].position.x + 3.5f, 
                        buildings[i].position.y - 1.0f, 
                        buildings[i].position.z 
                    }, 0.1f, 2.0f, 1.5f, YELLOW);
                    DrawCube((Vector3){ 
                        buildings[i].position.x - 3.5f, 
                        buildings[i].position.y - 1.0f, 
                        buildings[i].position.z 
                    }, 0.1f, 2.0f, 1.5f, YELLOW);
                }
                
                // Roof
                DrawCube((Vector3){ 
                    buildings[i].position.x, 
                    buildings[i].position.y + buildings[i].size.y/2, 
                    buildings[i].position.z 
                }, buildings[i].size.x + 0.5f, 0.5f, buildings[i].size.z + 0.5f, DARKGRAY);
            }
        }
    }
    
    // Vehicles with details
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (vehicles[i].model.meshCount > 0 && vehicles[i].health > 0) {
            DrawVehicleWithDetails(&vehicles[i]);
        }
    }
    
    // Pedestrians
    for (int i = 0; i < MAX_PEDS; i++) {
        if (peds[i].type >= 0) {
            // Body
            DrawCube(peds[i].position, peds[i].size.x, 
                    peds[i].size.y, peds[i].size.z, peds[i].color);
            
            // Head
            DrawSphere((Vector3){ 
                peds[i].position.x, 
                peds[i].position.y + peds[i].size.y/2 + 0.2f, 
                peds[i].position.z 
            }, 0.2f, (Color){255, 204, 153, 255}); // Skin color
            
            // Arms
            float armAngle = sin(gameTime * 5.0f + i) * 0.3f;
            DrawCube((Vector3){ 
                peds[i].position.x + 0.3f, 
                peds[i].position.y, 
                peds[i].position.z 
            }, 0.15f, 0.8f, 0.15f, peds[i].color);
            DrawCube((Vector3){ 
                peds[i].position.x - 0.3f, 
                peds[i].position.y, 
                peds[i].position.z 
            }, 0.15f, 0.8f, 0.15f, peds[i].color);
            
            // Legs with walking animation
            float legAngle = sin(gameTime * 5.0f + i) * 0.2f;
            DrawCube((Vector3){ 
                peds[i].position.x + 0.15f, 
                peds[i].position.y - peds[i].size.y/2 + 0.4f + legAngle, 
                peds[i].position.z 
            }, 0.15f, 0.8f, 0.15f, DARKBLUE);
            DrawCube((Vector3){ 
                peds[i].position.x - 0.15f, 
                peds[i].position.y - peds[i].size.y/2 + 0.4f - legAngle, 
                peds[i].position.z 
            }, 0.15f, 0.8f, 0.15f, DARKBLUE);
        }
    }
    
    // Player in third person
    if (!player.inVehicle && !firstPerson) {
        // Body
        DrawModel(player.model, player.position, 1.0f, BLUE);
        
        // Head
        DrawSphere((Vector3){ 
            player.position.x, 
            player.position.y + player.size.y/2 + 0.2f, 
            player.position.z 
        }, 0.2f, (Color){255, 204, 153, 255});
        
        // Weapon in hands
        if (currentWeapon > 0) {
            Vector3 weaponPos = player.position;
            weaponPos.x += sin(player.rotation) * 0.5f;
            weaponPos.z += cos(player.rotation) * 0.5f;
            weaponPos.y += 1.0f;
            
            DrawCube(weaponPos, 0.1f, 0.05f, 0.4f, DARKGRAY);
        }
    }
    
    // Projectiles (small, fast bullets)
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
            // Very small, fast bullets - no gravity, straight line
            DrawSphere(projectiles[i].position, projectiles[i].size, projectiles[i].color);
        }
    }
    
    // Explosions
    for (int i = 0; i < 10; i++) {
        if (explosions[i].active) {
            float alpha = 1.0f - (explosions[i].time / 1.0f);
            DrawSphere(explosions[i].position, explosions[i].radius, 
                      ColorAlpha(ORANGE, alpha * 0.7f));
            DrawSphere(explosions[i].position, explosions[i].radius * 0.7f, 
                      ColorAlpha(RED, alpha * 0.5f));
            DrawSphere(explosions[i].position, explosions[i].radius * 0.4f, 
                      ColorAlpha(YELLOW, alpha * 0.3f));
        }
    }
    
    // Sky
    DrawCube((Vector3){ 0.0f, 50.0f, 0.0f }, 300.0f, 1.0f, 300.0f, SKYBLUE);
    
    // Sun
    DrawSphere((Vector3){ 100.0f, 80.0f, -100.0f }, 15.0f, YELLOW);
    
    // Clouds
    for (int i = 0; i < 8; i++) {
        float x = sin(gameTime * 0.1f + i) * 80.0f;
        float z = cos(gameTime * 0.1f + i) * 80.0f;
        DrawSphere((Vector3){ x, 35.0f + sin(gameTime + i) * 3.0f, z }, 4.0f + sin(gameTime + i), WHITE);
    }
}

// Draw vehicle with details
void DrawVehicleWithDetails(Vehicle *vehicle) {
    Vector3 rotAxis = { 0.0f, 1.0f, 0.0f };
    
    // Main body
    DrawModelEx(vehicle->model, vehicle->position, 
               rotAxis, vehicle->rotation * RAD2DEG, 
               (Vector3){ 1.0f, 1.0f, 1.0f }, vehicle->color);
    
    // Headlights
    Vector3 frontPos = vehicle->position;
    frontPos.z += cos(vehicle->rotation) * 2.8f;
    frontPos.x += sin(vehicle->rotation) * 2.8f;
    
    DrawCube((Vector3){frontPos.x - 0.4f, frontPos.y + 0.2f, frontPos.z}, 
            0.3f, 0.2f, 0.1f, YELLOW);
    DrawCube((Vector3){frontPos.x + 0.4f, frontPos.y + 0.2f, frontPos.z}, 
            0.3f, 0.2f, 0.1f, YELLOW);
    
    // Taillights
    Vector3 rearPos = vehicle->position;
    rearPos.z -= cos(vehicle->rotation) * 2.8f;
    rearPos.x -= sin(vehicle->rotation) * 2.8f;
    
    DrawCube((Vector3){rearPos.x - 0.4f, rearPos.y + 0.2f, rearPos.z}, 
            0.3f, 0.2f, 0.1f, RED);
    DrawCube((Vector3){rearPos.x + 0.4f, rearPos.y + 0.2f, rearPos.z}, 
            0.3f, 0.2f, 0.1f, RED);
    
    // Wheels with rotation
    vehicle->wheelRotation += vehicle->speed * GetFrameTime() * 10.0f;
    
    for (int w = 0; w < 4; w++) {
        float xOffset = (w % 2 == 0) ? 0.9f : -0.9f;
        float zOffset = (w < 2) ? 1.8f : -1.8f;
        
        Vector3 wheelPos = vehicle->position;
        wheelPos.x += xOffset;
        wheelPos.z += cos(vehicle->rotation) * zOffset;
        wheelPos.x += sin(vehicle->rotation) * zOffset;
        
        DrawCube(wheelPos, 0.6f, 0.4f, 0.4f, BLACK);
        
        // Wheel rim
        DrawCube((Vector3){wheelPos.x, wheelPos.y, wheelPos.z}, 
                0.3f, 0.42f, 0.42f, GRAY);
    }
    
    // Police lights (only for police cars)
    if (vehicle->color.r == DARKBLUE.r && 
        vehicle->color.g == DARKBLUE.g && 
        vehicle->color.b == DARKBLUE.b) {
        
        float flash = sin(gameTime * 10.0f) > 0 ? 1.0f : 0.0f;
        
        // Red light
        DrawCube((Vector3){vehicle->position.x, vehicle->position.y + 1.2f, vehicle->position.z}, 
                0.8f, 0.3f, 0.3f, ColorAlpha(RED, flash));
        
        // Blue light
        flash = sin(gameTime * 10.0f + 3.14f) > 0 ? 1.0f : 0.0f;
        DrawCube((Vector3){vehicle->position.x, vehicle->position.y + 1.5f, vehicle->position.z}, 
                0.8f, 0.3f, 0.3f, ColorAlpha(BLUE, flash));
    }
}

// Interface
void DrawHUD(void) {
    // Health bar
    DrawRectangle(20, 20, 250, 35, Fade(DARKGRAY, 0.8f));
    DrawRectangle(20, 20, (int)(player.health * 2.5), 35, RED);
    DrawRectangleLines(20, 20, 250, 35, WHITE);
    DrawText("HEALTH", 25, 25, 22, WHITE);
    
    // Weapon
    const char* weapons[] = { "FISTS", "PISTOL", "MACHINEGUN" };
    DrawRectangle(20, 70, 250, 45, Fade(DARKGRAY, 0.8f));
    DrawText(TextFormat("WEAPON: %s", weapons[currentWeapon]), 25, 75, 22, YELLOW);
    DrawText("R - Change weapon", 25, 100, 16, LIGHTGRAY);
    
    // Money and score
    DrawRectangle(SCREEN_WIDTH - 270, 20, 250, 120, Fade(DARKGRAY, 0.8f));
    DrawText(TextFormat("MONEY: $%d", player.money), SCREEN_WIDTH - 260, 30, 22, GREEN);
    DrawText(TextFormat("SCORE: %d", score), SCREEN_WIDTH - 260, 60, 22, YELLOW);
    DrawText(TextFormat("TIME: %.1f", gameTime), SCREEN_WIDTH - 260, 90, 22, WHITE);
    
    // Controls
    DrawRectangle(20, SCREEN_HEIGHT - 200, 350, 180, Fade(DARKBLUE, 0.8f));
    DrawText("CONTROLS:", 25, SCREEN_HEIGHT - 190, 22, WHITE);
    DrawText("WASD - Movement", 25, SCREEN_HEIGHT - 160, 18, LIGHTGRAY);
    DrawText("MOUSE - Look around", 25, SCREEN_HEIGHT - 135, 18, LIGHTGRAY);
    DrawText("SPACE - Jump/Brake", 25, SCREEN_HEIGHT - 110, 18, LIGHTGRAY);
    DrawText("F - Enter/Exit vehicle", 25, SCREEN_HEIGHT - 85, 18, LIGHTGRAY);
    DrawText("E - Horn", 25, SCREEN_HEIGHT - 60, 18, LIGHTGRAY);
    DrawText("LMB - Shoot/Punch", 25, SCREEN_HEIGHT - 35, 18, LIGHTGRAY);
    DrawText("P - Pause | V - Camera | F11 - Fullscreen", 
             SCREEN_WIDTH - 400, SCREEN_HEIGHT - 30, 18, LIGHTGRAY);
}

// Minimap
void DrawMinimap(void) {
    int mapSize = 220;
    int mapX = SCREEN_WIDTH - mapSize - 20;
    int mapY = 150;
    
    // Minimap background
    DrawRectangle(mapX, mapY, mapSize, mapSize, Fade(BLACK, 0.7f));
    DrawRectangleLines(mapX, mapY, mapSize, mapSize, WHITE);
    
    // Scale for minimap
    float scale = 4.0f;
    
    // Player on minimap
    int playerMapX = mapX + mapSize/2 + (int)(player.position.x / scale);
    int playerMapY = mapY + mapSize/2 + (int)(player.position.z / scale);
    DrawCircle(playerMapX, playerMapY, 6.0f, BLUE);
    
    // Player direction
    float dirX = sin(player.rotation) * 12.0f;
    float dirZ = cos(player.rotation) * 12.0f;
    DrawLine(playerMapX, playerMapY, 
             playerMapX + (int)dirX, 
             playerMapY + (int)dirZ, GREEN);
    
    // Vehicles on minimap
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (vehicles[i].model.meshCount > 0 && vehicles[i].health > 0) {
            int carX = mapX + mapSize/2 + (int)(vehicles[i].position.x / scale);
            int carY = mapY + mapSize/2 + (int)(vehicles[i].position.z / scale);
            
            Color carColor = vehicles[i].color;
            if (vehicles[i].occupied) carColor = RED;
            if (vehicles[i].color.r == DARKBLUE.r && 
                vehicles[i].color.g == DARKBLUE.g && 
                vehicles[i].color.b == DARKBLUE.b) {
                carColor = BLUE; // Police
            }
            DrawCircle(carX, carY, 4.0f, carColor);
        }
    }
    
    // Buildings on minimap
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (buildings[i].hasCollision && buildings[i].type > 0 && buildings[i].type != 4) {
            int buildX = mapX + mapSize/2 + (int)(buildings[i].position.x / scale);
            int buildY = mapY + mapSize/2 + (int)(buildings[i].position.z / scale);
            int size = max(2, (int)(buildings[i].size.x / scale / 2));
            DrawRectangle(buildX - size/2, buildY - size/2, size, size, GRAY);
        }
    }
    
    // Road network on minimap
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (buildings[i].hasCollision && buildings[i].type == 0) {
            int roadX = mapX + mapSize/2 + (int)(buildings[i].position.x / scale);
            int roadY = mapY + mapSize/2 + (int)(buildings[i].position.z / scale);
            int width = max(1, (int)(buildings[i].size.x / scale));
            int height = max(1, (int)(buildings[i].size.z / scale));
            DrawRectangle(roadX - width/2, roadY - height/2, width, height, DARKGRAY);
        }
    }
    
    DrawText("MINIMAP", mapX + mapSize/2 - 40, mapY - 25, 22, WHITE);
}

// Wanted stars
void DrawWantedStars(void) {
    if (player.wantedLevel > 0) {
        int starSize = 35;
        int startX = SCREEN_WIDTH/2 - (player.wantedLevel * starSize)/2;
        int yPos = 30;
        
        // Background
        DrawRectangle(startX - 15, yPos - 15, 
                     player.wantedLevel * starSize + 30, starSize + 30, 
                     Fade(BLACK, 0.7f));
        
        for (int i = 0; i < player.wantedLevel; i++) {
            DrawPoly((Vector2){ startX + i * starSize + starSize/2, yPos + starSize/2 }, 
                    5, starSize/2, 0, YELLOW);
            DrawPolyLines((Vector2){ startX + i * starSize + starSize/2, yPos + starSize/2 }, 
                        5, starSize/2, 0, BLACK);
        }
        
        DrawText("WANTED!", SCREEN_WIDTH/2 - 80, yPos + starSize + 15, 32, RED);
        
        // Police chase warning
        if (player.wantedLevel >= 2) {
            DrawText("POLICE IN PURSUIT!", SCREEN_WIDTH/2 - 150, yPos + starSize + 60, 28, ORANGE);
        }
    }
}

// Crosshair
void DrawCrosshair(void) {
    if (currentWeapon > 0) {
        DrawTexture(crosshair, SCREEN_WIDTH/2 - 16, SCREEN_HEIGHT/2 - 16, WHITE);
    } else {
        // Simple circle for fists
        DrawCircleLines(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 25, WHITE);
    }
}

// Player movement with mouse look
void UpdatePlayerMovement(void) {
    // Mouse look (first person)
    if (firstPerson && !player.inVehicle) {
        Vector2 mouseDelta = Vector2Subtract(GetMousePosition(), player.lastMousePosition);
        
        // Sensitivity
        float sensitivity = 0.002f;
        
        // Update rotation and pitch
        player.rotation -= mouseDelta.x * sensitivity;
        player.pitch -= mouseDelta.y * sensitivity;
        
        // Clamp pitch
        if (player.pitch > 1.5f) player.pitch = 1.5f;
        if (player.pitch < -1.5f) player.pitch = -1.5f;
        
        // Reset mouse position to center
        SetMousePosition(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
        player.lastMousePosition = GetMousePosition();
        
        // Update camera for first person
        Vector3 cameraOffset = {
            sin(player.rotation) * cos(player.pitch),
            sin(player.pitch),
            cos(player.rotation) * cos(player.pitch)
        };
        
        player.camera.position = player.position;
        player.camera.position.y += 1.6f; // Eye height
        player.camera.target = Vector3Add(player.camera.position, Vector3Scale(cameraOffset, 10.0f));
    }
    
    // Keyboard input
    Vector3 moveDir = { 0.0f, 0.0f, 0.0f };
    
    if (IsKeyDown(KEY_W)) moveDir.z = 1.0f;
    if (IsKeyDown(KEY_S)) moveDir.z = -1.0f;
    if (IsKeyDown(KEY_A)) moveDir.x = -1.0f;
    if (IsKeyDown(KEY_D)) moveDir.x = 1.0f;
    
    // Jump
    if (IsKeyPressed(KEY_SPACE) && !player.isJumping) {
        player.isJumping = true;
        player.jumpForce = 8.0f;
    }
    
    // Gravity
    if (player.isJumping) {
        player.position.y += player.jumpForce * GetFrameTime();
        player.jumpForce -= 20.0f * GetFrameTime();
        
        if (player.position.y <= 1.0f) {
            player.position.y = 1.0f;
            player.isJumping = false;
            player.jumpForce = 0.0f;
        }
    }
    
    // Movement on ground
    if (!player.inVehicle) {
        Vector3 velocity = Vector3Scale(moveDir, player.speed * GetFrameTime());
        
        // Rotate movement vector based on camera direction
        Vector3 rotatedVelocity = {
            velocity.x * cos(player.rotation) - velocity.z * sin(player.rotation),
            velocity.y,
            velocity.x * sin(player.rotation) + velocity.z * cos(player.rotation)
        };
        
        // Check collisions with buildings
        Vector3 newPos = Vector3Add(player.position, rotatedVelocity);
        bool collision = false;
        
        for (int i = 0; i < MAX_BUILDINGS; i++) {
            if (buildings[i].hasCollision && 
                CheckCollision(newPos, player.size, 
                             buildings[i].position, buildings[i].size)) {
                collision = true;
                break;
            }
        }
        
        if (!collision) {
            player.position = newPos;
        }
        
        // Walk on sidewalks - less wanted level increase
        bool onSidewalk = false;
        for (int i = 0; i < MAX_BUILDINGS; i++) {
            if (buildings[i].hasCollision && buildings[i].type == 4) {
                if (CheckCollision(player.position, (Vector3){0.5f, 0.1f, 0.5f}, 
                                 buildings[i].position, buildings[i].size)) {
                    onSidewalk = true;
                    break;
                }
            }
        }
        
        // Less wanted level for sidewalk walking
        if (onSidewalk) {
            player.wantedLevel = max(player.wantedLevel - 1, 0);
        }
    }
    
    // Enter/exit vehicle
    if (IsKeyPressed(KEY_F)) {
        if (!player.inVehicle) {
            // Find nearest vehicle
            float minDist = 9999.0f;
            int closestCar = -1;
            
            for (int i = 0; i < MAX_VEHICLES; i++) {
                if (vehicles[i].model.meshCount > 0 && vehicles[i].health > 0 && !vehicles[i].occupied) {
                    float dist = Vector3Distance(player.position, vehicles[i].position);
                    if (dist < 3.0f && dist < minDist) {
                        minDist = dist;
                        closestCar = i;
                    }
                }
            }
            
            if (closestCar >= 0) {
                player.inVehicle = true;
                vehicles[closestCar].occupied = true;
                player.position = vehicles[closestCar].position;
                
                // Switch to third person in vehicle
                firstPerson = false;
                EnableCursor();
            }
        } else {
            // Exit vehicle
            for (int i = 0; i < MAX_VEHICLES; i++) {
                if (vehicles[i].occupied) {
                    player.inVehicle = false;
                    vehicles[i].occupied = false;
                    
                    // Exit to the side
                    Vector3 exitOffset = {
                        sin(vehicles[i].rotation + PI/2) * 2.5f,
                        1.0f,
                        cos(vehicles[i].rotation + PI/2) * 2.5f
                    };
                    
                    player.position = Vector3Add(vehicles[i].position, exitOffset);
                    break;
                }
            }
        }
    }
    
    // Vehicle control
    if (player.inVehicle) {
        for (int i = 0; i < MAX_VEHICLES; i++) {
            if (vehicles[i].occupied) {
                Vehicle *car = &vehicles[i];
                
                // Movement
                float move = 0.0f;
                float steer = 0.0f;
                
                if (IsKeyDown(KEY_W)) move = 1.0f;
                if (IsKeyDown(KEY_S)) move = -0.6f;
                if (IsKeyDown(KEY_A)) steer = 1.0f;
                if (IsKeyDown(KEY_D)) steer = -1.0f;
                
                // Steering
                car->rotation += steer * 1.8f * GetFrameTime() * (car->speed/25.0f + 0.5f);
                
                // Acceleration/braking
                if (move > 0.0f) {
                    car->speed = min(car->speed + 25.0f * GetFrameTime(), 35.0f);
                } else if (move < 0.0f) {
                    car->speed = max(car->speed - 20.0f * GetFrameTime(), -12.0f);
                } else {
                    // Natural braking
                    car->speed *= 0.96f;
                    if (fabs(car->speed) < 0.5f) car->speed = 0.0f;
                }
                
                // Brake with space
                if (IsKeyDown(KEY_SPACE)) {
                    car->speed *= 0.9f;
                }
                
                // Calculate movement
                Vector3 forward = (Vector3){ 
                    sin(car->rotation), 
                    0.0f, 
                    cos(car->rotation) 
                };
                
                Vector3 newPos = Vector3Add(car->position, 
                                          Vector3Scale(forward, car->speed * GetFrameTime()));
                
                // Check collisions
                bool collision = false;
                for (int j = 0; j < MAX_BUILDINGS; j++) {
                    if (buildings[j].hasCollision && buildings[j].type > 0 && buildings[j].type != 4) {
                        Vector3 carSize = (Vector3){ 2.2f, 1.2f, 4.5f };
                        if (CheckCollision(newPos, carSize, 
                                         buildings[j].position, buildings[j].size)) {
                            collision = true;
                            car->speed *= -0.4f;
                            car->health -= 8.0f;
                            
                            if (car->health <= 0.0f) {
                                CreateExplosion(car->position);
                                car->model.meshCount = 0;
                                player.inVehicle = false;
                                player.wantedLevel = min(player.wantedLevel + 2, 5);
                                score += 150;
                            }
                            break;
                        }
                    }
                }
                
                if (!collision) {
                    car->position = newPos;
                    player.position = car->position;
                }
                
                // Update camera for vehicle
                if (!firstPerson) {
                    float cameraDistance = 12.0f;
                    float cameraHeight = 5.0f;
                    
                    Vector3 cameraOffset = {
                        sin(car->rotation) * -cameraDistance,
                        cameraHeight,
                        cos(car->rotation) * -cameraDistance
                    };
                    
                    player.camera.position = Vector3Add(car->position, cameraOffset);
                    player.camera.target = Vector3Add(car->position, 
                                                    Vector3Scale(forward, 8.0f));
                    player.camera.target.y = 1.5f;
                } else {
                    // First person in vehicle
                    player.camera.position = Vector3Add(car->position, (Vector3){0, 2.5f, 0});
                    player.camera.target = Vector3Add(car->position, 
                                                    Vector3Scale(forward, 10.0f));
                    player.camera.target.y = 2.5f;
                }
                
                // Horn
                if (IsKeyPressed(KEY_E)) {
                    car->hornTimer = 1.0f;
                    player.wantedLevel = min(player.wantedLevel + 1, 5);
                }
                
                break;
            }
        }
    }
    
    // Shooting - fast bullets with no gravity
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && currentWeapon > 0) {
        Vector3 shootDir;
        
        if (firstPerson) {
            // Shoot in look direction
            shootDir = Vector3Subtract(player.camera.target, player.camera.position);
            shootDir = Vector3Normalize(shootDir);
        } else {
            // Shoot forward
            shootDir = (Vector3){ 
                sin(player.rotation), 
                0.0f, 
                cos(player.rotation) 
            };
        }
        
        Vector3 shootPos = Vector3Add(player.position, (Vector3){ 0.0f, 1.5f, 0.0f });
        if (player.inVehicle) {
            shootPos = Vector3Add(player.position, (Vector3){ 0.0f, 2.5f, 0.0f });
        }
        
        ShootProjectile(shootPos, shootDir);
        player.wantedLevel = min(player.wantedLevel + 1, 5);
        score -= 15;
    }
    
    // Punch
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && currentWeapon == 0) {
        Vector3 punchDir = (Vector3){ 
            sin(player.rotation), 
            0.0f, 
            cos(player.rotation) 
        };
        Vector3 punchPos = Vector3Add(player.position, Vector3Scale(punchDir, 1.2f));
        punchPos.y += 1.0f;
        
        for (int i = 0; i < MAX_PEDS; i++) {
            if (peds[i].type >= 0 && 
                Vector3Distance(punchPos, peds[i].position) < 1.5f) {
                peds[i].type = -1;
                player.wantedLevel = min(player.wantedLevel + 1, 5);
                score -= 30;
                break;
            }
        }
    }
    
    // Change weapon
    if (IsKeyPressed(KEY_R)) {
        currentWeapon = (currentWeapon + 1) % 3;
    }
    
    // Heal
    if (IsKeyPressed(KEY_H) && player.money >= 100) {
        player.health = min(player.health + 50, 100);
        player.money -= 100;
    }
    
    // Bribe police
    if (IsKeyPressed(KEY_B) && player.money >= 500) {
        player.wantedLevel = 0;
        player.money -= 500;
    }
}

// Update vehicle
void UpdateVehicle(int index) {
    Vehicle *car = &vehicles[index];
    
    if (car->hornTimer > 0) {
        car->hornTimer -= GetFrameTime();
    }
    
    // AI for unoccupied vehicles
    if (!car->occupied) {
        UpdateVehicleAI(index);
    }
    
    // Auto-repair
    if (car->health < 100.0f && car->health > 0.0f) {
        car->health += 0.1f * GetFrameTime();
    }
    
    // Smoke from damaged vehicles
    if (car->health < 50.0f && car->health > 0.0f && rand() % 10 == 0) {
        Vector3 smokePos = car->position;
        smokePos.y += 1.0f;
        
        // Create smoke particle
        for (int i = 0; i < MAX_PROJECTILES; i++) {
            if (!projectiles[i].active) {
                projectiles[i].position = smokePos;
                projectiles[i].velocity = (Vector3){ 
                    (rand() % 100 - 50) / 50.0f,
                    2.0f + (rand() % 100) / 50.0f,
                    (rand() % 100 - 50) / 50.0f
                };
                projectiles[i].color = GRAY;
                projectiles[i].size = 0.3f + (rand() % 100) / 100.0f;
                projectiles[i].lifetime = 1.0f;
                projectiles[i].active = true;
                break;
            }
        }
    }
}

// AI for vehicles
void UpdateVehicleAI(int index) {
    Vehicle *car = &vehicles[index];
    
    // Police chase player (only if police car)
    if (player.wantedLevel > 0 && 
        car->color.r == DARKBLUE.r && 
        car->color.g == DARKBLUE.g && 
        car->color.b == DARKBLUE.b) {
        
        Vector3 toPlayer = Vector3Subtract(player.position, car->position);
        float distance = Vector3Length(toPlayer);
        
        if (distance < 50.0f) {
            // Chase
            toPlayer = Vector3Normalize(toPlayer);
            float targetRotation = atan2f(toPlayer.x, toPlayer.z);
            
            // Smooth turn to target
            float angleDiff = targetRotation - car->rotation;
            while (angleDiff > PI) angleDiff -= 2*PI;
            while (angleDiff < -PI) angleDiff += 2*PI;
            
            car->rotation += angleDiff * 0.05f;
            car->speed = min(car->speed + 10.0f * GetFrameTime(), 25.0f);
            
            // Shoot at player (less frequent)
            if (distance < 20.0f && rand() % 200 == 0) {
                Vector3 shootDir = Vector3Normalize(toPlayer);
                ShootProjectile(Vector3Add(car->position, (Vector3){0, 2.0f, 0}), shootDir);
            }
        } else {
            // Random movement
            car->speed *= 0.98f;
            if (car->speed < 5.0f) car->speed = 5.0f;
            
            if (rand() % 100 == 0) {
                car->rotation += (rand() % 100 - 50) / 50.0f;
            }
        }
    } else {
        // Normal civilian vehicle movement
        car->speed = 5.0f + sin(gameTime * 0.5f + index) * 2.0f;
        
        // Follow roads - try to stay on road texture
        bool onRoad = false;
        for (int i = 0; i < MAX_BUILDINGS; i++) {
            if (buildings[i].hasCollision && buildings[i].type == 0) {
                Vector3 carSize = (Vector3){ 2.0f, 0.1f, 4.0f };
                if (CheckCollision(car->position, carSize, 
                                 buildings[i].position, buildings[i].size)) {
                    onRoad = true;
                    break;
                }
            }
        }
        
        if (!onRoad && rand() % 100 == 0) {
            // Try to find road
            car->rotation += (rand() % 100 - 50) / 20.0f;
        } else if (rand() % 200 == 0) {
            // Minor course adjustments
            car->rotation += (rand() % 100 - 50) / 40.0f;
        }
    }
    
    // Move forward
    Vector3 forward = (Vector3){ 
        sin(car->rotation), 
        0.0f, 
        cos(car->rotation) 
    };
    
    Vector3 newPos = Vector3Add(car->position, 
                              Vector3Scale(forward, car->speed * GetFrameTime()));
    
    // Check building collisions (avoid buildings)
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (buildings[i].hasCollision && buildings[i].type > 0 && buildings[i].type != 4) {
            Vector3 carSize = (Vector3){ 2.2f, 1.2f, 4.5f };
            if (CheckCollision(newPos, carSize, 
                             buildings[i].position, buildings[i].size)) {
                // Turn around on collision
                car->rotation += PI * 0.8f;
                car->speed *= 0.5f;
                return;
            }
        }
    }
    
    car->position = newPos;
    
    // Stay within map bounds
    if (fabs(car->position.x) > 90 || fabs(car->position.z) > 90) {
        car->rotation += PI;
        car->speed = 10.0f;
    }
}

// Update pedestrian
void UpdatePedestrian(int index) {
    Pedestrian *ped = &peds[index];
    
    ped->stateTime += GetFrameTime();
    
    // Change state
    if (ped->stateTime > 3.0f) {
        ped->state = rand() % 3;
        ped->stateTime = 0.0f;
        
        if (ped->state == 0) {
            ped->speed = 0.0f;
        } else if (ped->state == 1) {
            ped->speed = 1.0f + (rand() % 100) / 100.0f;
        } else {
            ped->speed = 2.0f + (rand() % 100) / 50.0f;
        }
        
        ped->rotation = (rand() % 360) * DEG2RAD;
    }
    
    // Police chase player
    if (ped->type == 1 && player.wantedLevel > 0) {
        Vector3 toPlayer = Vector3Subtract(player.position, ped->position);
        float distance = Vector3Length(toPlayer);
        
        if (distance < 20.0f) {
            toPlayer = Vector3Normalize(toPlayer);
            ped->rotation = atan2f(toPlayer.x, toPlayer.z);
            ped->speed = 3.0f;
            ped->state = 2;
        }
    }
    
    // Movement
    if (ped->speed > 0.0f) {
        Vector3 forward = (Vector3){ 
            sin(ped->rotation), 
            0.0f, 
            cos(ped->rotation) 
        };
        
        Vector3 newPos = Vector3Add(ped->position, 
                                  Vector3Scale(forward, ped->speed * GetFrameTime()));
        
        // Check building collisions
        bool collision = false;
        for (int i = 0; i < MAX_BUILDINGS; i++) {
            if (buildings[i].hasCollision && buildings[i].type > 0 && buildings[i].type != 4) {
                if (CheckCollision(newPos, ped->size, 
                                 buildings[i].position, buildings[i].size)) {
                    collision = true;
                    ped->rotation += PI * 0.5f;
                    break;
                }
            }
        }
        
        if (!collision) {
            ped->position = newPos;
        }
    }
    
    // Walking animation
    ped->position.y = 1.0f + sin(gameTime * 10.0f + index) * 0.1f;
}

// Shoot projectile - fast bullets with no gravity
void ShootProjectile(Vector3 position, Vector3 direction) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) {
            projectiles[i].position = position;
            
            // MUCH faster bullets - 200 units per second, no gravity
            projectiles[i].velocity = Vector3Scale(direction, 200.0f);
            
            // Very small bullets
            projectiles[i].color = (currentWeapon == 1) ? YELLOW : RED;
            projectiles[i].size = (currentWeapon == 1) ? 0.05f : 0.08f; // Much smaller
            
            // Longer range
            projectiles[i].lifetime = 3.0f; // 3 seconds lifetime
            
            projectiles[i].active = true;
            break;
        }
    }
}

// Update projectiles - fast, no gravity
void UpdateProjectiles(void) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
            // Movement - NO GRAVITY, straight line
            projectiles[i].position = Vector3Add(projectiles[i].position, 
                                               Vector3Scale(projectiles[i].velocity, GetFrameTime()));
            
            // Lifetime
            projectiles[i].lifetime -= GetFrameTime();
            if (projectiles[i].lifetime <= 0.0f) {
                projectiles[i].active = false;
                continue;
            }
            
            // Check collisions
            HandleProjectileCollisions(i);
        }
    }
}

// Handle projectile collisions
void HandleProjectileCollisions(int index) {
    Projectile *proj = &projectiles[index];
    
    // Building collisions
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (buildings[i].hasCollision && buildings[i].type > 0 && buildings[i].type != 4) {
            if (CheckCollision(proj->position, (Vector3){proj->size, proj->size, proj->size}, 
                             buildings[i].position, buildings[i].size)) {
                CreateExplosion(proj->position);
                proj->active = false;
                score += 5;
                return;
            }
        }
    }
    
    // Vehicle collisions
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (vehicles[i].model.meshCount > 0 && vehicles[i].health > 0) {
            Vector3 carSize = (Vector3){ 2.2f, 1.2f, 4.5f };
            if (CheckCollision(proj->position, (Vector3){proj->size, proj->size, proj->size}, 
                             vehicles[i].position, carSize)) {
                CreateExplosion(proj->position);
                vehicles[i].health -= 25.0f;
                proj->active = false;
                
                if (vehicles[i].health <= 0.0f) {
                    CreateExplosion(vehicles[i].position);
                    vehicles[i].model.meshCount = 0;
                    score += 100;
                } else {
                    score += 25;
                }
                
                if (vehicles[i].occupied) {
                    player.wantedLevel = min(player.wantedLevel + 2, 5);
                }
                return;
            }
        }
    }
    
    // Pedestrian collisions
    for (int i = 0; i < MAX_PEDS; i++) {
        if (peds[i].type >= 0) {
            if (CheckCollision(proj->position, (Vector3){proj->size, proj->size, proj->size}, 
                             peds[i].position, peds[i].size)) {
                CreateExplosion(proj->position);
                peds[i].type = -1;
                proj->active = false;
                
                if (peds[i].type == 1) { // Police
                    player.wantedLevel = min(player.wantedLevel + 3, 5);
                    score -= 100;
                } else {
                    score -= 50;
                }
                return;
            }
        }
    }
}

// Create explosion
void CreateExplosion(Vector3 position) {
    for (int i = 0; i < 10; i++) {
        if (!explosions[i].active) {
            explosions[i].position = position;
            explosions[i].radius = 2.0f;
            explosions[i].time = 1.0f;
            explosions[i].active = true;
            
            // Create explosion particles
            for (int j = 0; j < 20; j++) {
                for (int k = 0; k < MAX_PROJECTILES; k++) {
                    if (!projectiles[k].active) {
                        projectiles[k].position = position;
                        projectiles[k].velocity = (Vector3){ 
                            (rand() % 100 - 50) / 10.0f,
                            (rand() % 100) / 10.0f,
                            (rand() % 100 - 50) / 10.0f
                        };
                        projectiles[k].color = (Color){ 
                            255, 
                            100 + rand() % 155, 
                            0, 
                            255 
                        };
                        projectiles[k].size = 0.2f + (rand() % 100) / 100.0f;
                        projectiles[k].lifetime = 1.0f + (rand() % 100) / 100.0f;
                        projectiles[k].active = true;
                        break;
                    }
                }
            }
            break;
        }
    }
}

// Update explosions
void UpdateExplosions(void) {
    for (int i = 0; i < 10; i++) {
        if (explosions[i].active) {
            explosions[i].time -= GetFrameTime();
            explosions[i].radius += 10.0f * GetFrameTime();
            
            if (explosions[i].time <= 0.0f) {
                explosions[i].active = false;
            }
        }
    }
}

// Spawn vehicle
void SpawnVehicle(Vector3 position, int type) {
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (vehicles[i].model.meshCount == 0) {
            vehicles[i].position = position;
            vehicles[i].velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
            vehicles[i].rotation = (rand() % 360) * DEG2RAD;
            vehicles[i].speed = 5.0f + (rand() % 15);
            vehicles[i].occupied = false;
            vehicles[i].health = 100.0f;
            vehicles[i].engineOn = true;
            vehicles[i].hornTimer = 0.0f;
            
            switch(type) {
                case 0: // Police (rare)
                    vehicles[i].model = policeCarModel;
                    vehicles[i].color = DARKBLUE;
                    vehicles[i].size = (Vector3){ 2.0f, 1.2f, 4.5f };
                    break;
                case 1: // Sedan (common)
                    vehicles[i].model = sedanModel;
                    vehicles[i].color = (Color){ 
                        rand() % 255, 
                        rand() % 255, 
                        rand() % 255, 
                        255 
                    };
                    vehicles[i].size = (Vector3){ 1.8f, 1.2f, 4.0f };
                    break;
                case 2: // Truck (common)
                    vehicles[i].model = truckModel;
                    vehicles[i].color = DARKGREEN;
                    vehicles[i].size = (Vector3){ 2.8f, 2.2f, 6.0f };
                    break;
                case 3: // SUV (common)
                    vehicles[i].model = suvModel;
                    vehicles[i].color = (Color){ 
                        50 + rand() % 100, 
                        50 + rand() % 100, 
                        50 + rand() % 100, 
                        255 
                    };
                    vehicles[i].size = (Vector3){ 2.3f, 1.8f, 5.0f };
                    break;
                case 4: // Sport car (rare)
                    vehicles[i].model = sportCarModel;
                    vehicles[i].color = YELLOW;
                    vehicles[i].size = (Vector3){ 1.6f, 0.9f, 3.8f };
                    break;
            }
            break;
        }
    }
}

// Spawn building
void SpawnBuilding(Vector3 position, Vector3 size, int type) {
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (!buildings[i].hasCollision) {
            buildings[i].position = position;
            buildings[i].size = size;
            buildings[i].type = type;
            buildings[i].hasCollision = true;
            break;
        }
    }
}

// Check collision
bool CheckCollision(Vector3 pos1, Vector3 size1, Vector3 pos2, Vector3 size2) {
    return (fabs(pos1.x - pos2.x) < (size1.x/2 + size2.x/2)) &&
           (fabs(pos1.y - pos2.y) < (size1.y/2 + size2.y/2)) &&
           (fabs(pos1.z - pos2.z) < (size1.z/2 + size2.z/2));
}
