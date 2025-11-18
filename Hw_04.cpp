#include <windows.h>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iostream>
#include <random>
#include <stack>
#include <vector>

#include "glut.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

// Maze dimensions
const int MAZE_WIDTH = 10;
const int MAZE_HEIGHT = 10;

// Cell structure
struct Cell {
    bool visited = false;
    bool walls[4] = {true, true, true, true};  // North, East, South, West
};

// Player position and orientation
float playerX = 1.5f;
float playerY = 0.5f;
float playerZ = 1.5f;
float playerAngle = 0.0f;

// View mode
enum ViewMode { FIRST_PERSON, BIRD_EYE };
ViewMode currentView = FIRST_PERSON;

// Maze data
Cell maze[MAZE_WIDTH][MAZE_HEIGHT];

// Add maze destination
int destX = MAZE_WIDTH - 2;
int destZ = MAZE_HEIGHT - 2;
bool reachedDestination = false;

// Add a variable to track if the player has reached the destination
bool gameWon = false;

void initMaze();
void generateMaze();
void ensurePathToDestination();
void idle();
void init();
void display();
void drawMaze();
void drawCell(int x, int z);
void drawPlayer();
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);
void mouseFunc(int button, int state, int x, int y);
void drawMiniMap();
void drawBirdEyeView();
void drawSuccessScreen();

// Direction vectors (North, East, South, West)
const int dx[4] = {0, 1, 0, -1};
const int dz[4] = {-1, 0, 1, 0};

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("3D Maze");

    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouseFunc);

    init();
    initMaze();
    generateMaze();

    glutMainLoop();
    return 0;
}

void init() {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glClearDepth(1.0f);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Set up light parameters
    GLfloat lightPosition[] = {5.0f, 10.0f, 5.0f, 1.0f};
    GLfloat lightAmbient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat lightDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat lightSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLfloat aspect = (GLfloat)w / (GLfloat)h;
    gluPerspective(60.0, aspect, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
}

void idle() {
    glutPostRedisplay();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Disable lighting completely to avoid issues
    glDisable(GL_LIGHTING);

    // Check if player has reached the destination
    if (!gameWon && floor(playerX) == destX && floor(playerZ) == destZ) {
        gameWon = true;
    }

    // If game is won, display success message
    if (gameWon) {
        drawSuccessScreen();
    } else if (currentView == FIRST_PERSON) {
        // First-person view
        float lookX = playerX + cos(playerAngle);
        float lookZ = playerZ + sin(playerAngle);
        gluLookAt(playerX, playerY, playerZ, lookX, playerY, lookZ, 0.0, 1.0, 0.0);

        // Draw the 3D maze in first-person view
        drawMaze();
        drawPlayer();
    } else {
        // Save the current matrices
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // Draw a 2D representation of the maze
        drawBirdEyeView();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }

    glutSwapBuffers();
}

void initMaze() {
    for (int x = 0; x < MAZE_WIDTH; x++) {
        for (int z = 0; z < MAZE_HEIGHT; z++) {
            maze[x][z].visited = false;

            // Set all walls initially
            for (int i = 0; i < 4; i++) {
                maze[x][z].walls[i] = true;
            }

            // But ensure outer walls are never broken
            if (x == 0) maze[x][z].walls[3] = true;                // Left edge
            if (x == MAZE_WIDTH - 1) maze[x][z].walls[1] = true;   // Right edge
            if (z == 0) maze[x][z].walls[0] = true;                // Top edge
            if (z == MAZE_HEIGHT - 1) maze[x][z].walls[2] = true;  // Bottom edge
        }
    }
}

void generateMaze() {
    random_device rd;
    mt19937 gen(rd());

    // Reset destination reached flag
    reachedDestination = false;

    // Set maze destination closer to the far corner but not at the perimeter
    destX = MAZE_WIDTH - 2;
    destZ = MAZE_HEIGHT - 2;

    stack<pair<int, int>> cellStack;
    int startX = 1;
    int startZ = 1;

    // Initialize all cells as unvisited with all walls intact
    for (int x = 0; x < MAZE_WIDTH; x++) {
        for (int z = 0; z < MAZE_HEIGHT; z++) {
            maze[x][z].visited = false;

            // Set all walls initially
            for (int i = 0; i < 4; i++) {
                maze[x][z].walls[i] = true;
            }

            // But ensure outer walls are never broken
            if (x == 0) maze[x][z].walls[3] = true;                // Left edge
            if (x == MAZE_WIDTH - 1) maze[x][z].walls[1] = true;   // Right edge
            if (z == 0) maze[x][z].walls[0] = true;                // Top edge
            if (z == MAZE_HEIGHT - 1) maze[x][z].walls[2] = true;  // Bottom edge
        }
    }


    maze[startX][startZ].visited = true;
    cellStack.push(make_pair(startX, startZ));

    while (!cellStack.empty()) {
        pair<int, int> current = cellStack.top();
        int x = current.first;
        int z = current.second;

        // Find unvisited neighbors
        vector<int> neighbors;
        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i];
            int nz = z + dz[i];

            // Make sure we don't go outside the maze boundaries
            if (nx >= 0 && nx < MAZE_WIDTH && nz >= 0 && nz < MAZE_HEIGHT && !maze[nx][nz].visited) {
                neighbors.push_back(i);
            }
        }

        if (!neighbors.empty()) {
            // Randomly choose a neighbor
            shuffle(neighbors.begin(), neighbors.end(), gen);

            if (rand() % 5 == 0) {  // 20% chance to bias toward destination
                // Find any neighbor that gets us closer to destination
                for (size_t i = 0; i < neighbors.size(); i++) {
                    int dir = neighbors[i];
                    int nx = x + dx[dir];
                    int nz = z + dz[dir];

                    // Calculate Manhattan distance to destination
                    int currDist = abs(x - destX) + abs(z - destZ);
                    int newDist = abs(nx - destX) + abs(nz - destZ);

                    // If this neighbor gets us closer to destination, prioritize it
                    if (newDist < currDist) {
                        swap(neighbors[0], neighbors[i]);  // Move this direction to front
                        break;
                    }
                }
            }

            int nextDir = neighbors[0];

            int nx = x + dx[nextDir];
            int nz = z + dz[nextDir];

            // Don't allow breaking outer walls
            bool canBreakWall = true;
            if ((x == 0 && nextDir == 3) || (x == MAZE_WIDTH - 1 && nextDir == 1) || (z == 0 && nextDir == 0) ||
                (z == MAZE_HEIGHT - 1 && nextDir == 2)) {
                canBreakWall = false;
            }

            if (canBreakWall) {
                // Remove the wall between current cell and chosen neighbor
                maze[x][z].walls[nextDir] = false;              // Remove current cell's wall
                maze[nx][nz].walls[(nextDir + 2) % 4] = false;  // Remove neighbor's opposite wall
            }

            // Mark the neighbor as visited and push it onto the stack
            maze[nx][nz].visited = true;
            cellStack.push(make_pair(nx, nz));
        } else {
            // Backtrack if no unvisited neighbors
            cellStack.pop();
        }
    }

    ensurePathToDestination();

    // Set player starting position
    playerX = 1.5f;
    playerY = 0.5f;
    playerZ = 1.5f;
    playerAngle = 0.0f;
}

void ensurePathToDestination() {
    // Reset visited flags
    for (int x = 0; x < MAZE_WIDTH; x++) {
        for (int z = 0; z < MAZE_HEIGHT; z++) {
            maze[x][z].visited = false;
        }
    }

    // DFS to check if there's a path
    stack<pair<int, int>> pathStack;
    pathStack.push(make_pair(1, 1));  // Start position
    maze[1][1].visited = true;

    while (!pathStack.empty()) {
        pair<int, int> current = pathStack.top();
        pathStack.pop();

        int x = current.first;
        int z = current.second;

        if (x == destX && z == destZ) {
            return;  // Path exists, no need to modify the maze
        }

        for (int dir = 0; dir < 4; dir++) {
            // If there's no wall in this direction
            if (!maze[x][z].walls[dir]) {
                int nx = x + dx[dir];
                int nz = z + dz[dir];

                // If this neighbor hasn't been visited
                if (!maze[nx][nz].visited) {
                    maze[nx][nz].visited = true;
                    pathStack.push(make_pair(nx, nz));
                }
            }
        }
    }

    int x = destX;
    int z = destZ;

    while (x > 1 || z > 1) {
        if (x > 1 && z > 1) {
            if (rand() % 2 == 0) {
                maze[x][z].walls[3] = false;     
                maze[x - 1][z].walls[1] = false;  
                x--;
            } else {
                maze[x][z].walls[0] = false;     
                maze[x][z - 1].walls[2] = false;  
                z--;
            }
        } else if (x > 1) {
            maze[x][z].walls[3] = false;      
            maze[x - 1][z].walls[1] = false;  
            x--;
        } else if (z > 1) {
            maze[x][z].walls[0] = false;     
            maze[x][z - 1].walls[2] = false;  
            z--;
        }
    }
}

void drawMaze() {
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    for (int x = 0; x < MAZE_WIDTH; x++) {
        for (int z = 0; z < MAZE_HEIGHT; z++) {
            drawCell(x, z);
        }
    }

    // Draw floor
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(MAZE_WIDTH, 0.0f, 0.0f);
    glVertex3f(MAZE_WIDTH, 0.0f, MAZE_HEIGHT);
    glVertex3f(0.0f, 0.0f, MAZE_HEIGHT);
    glEnd();

    // Draw ceiling
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, MAZE_HEIGHT);
    glVertex3f(MAZE_WIDTH, 1.0f, MAZE_HEIGHT);
    glVertex3f(MAZE_WIDTH, 1.0f, 0.0f);
    glEnd();

    // Draw destination marker
    glPushMatrix();
    glTranslatef(destX + 0.5f, 0.5f, destZ + 0.5f);
    glColor3f(0.0f, 1.0f, 0.0f);  // Green destination
    glutSolidSphere(0.3f, 16, 16);
    glPopMatrix();

    glDisable(GL_CULL_FACE);
}

void drawCell(int x, int z) {
    float wallHeight = 1.0f;

    if (currentView == BIRD_EYE) {
        if (x == floor(playerX) && z == floor(playerZ)) {
            glColor3f(0.7f, 0.7f, 1.0f); 
        } else if (x == destX && z == destZ) {
            // Destination cell
            glColor3f(0.7f, 1.0f, 0.7f);  
        } else {
            // Regular cell
            glColor3f(0.7f, 0.7f, 0.7f);  
        }

        glBegin(GL_QUADS);
        glVertex3f(x, 0.01f, z);  
        glVertex3f(x + 1.0f, 0.01f, z);
        glVertex3f(x + 1.0f, 0.01f, z + 1.0f);
        glVertex3f(x, 0.01f, z + 1.0f);
        glEnd();
    }

    if (currentView == BIRD_EYE) {
        glColor3f(0.0f, 0.0f, 0.8f); 
    } else {
        glColor3f(0.0f, 0.7f, 1.0f);  
    }

    if (maze[x][z].walls[0]) {
        glBegin(GL_QUADS);
        glVertex3f(x, 0.0f, z);
        glVertex3f(x + 1.0f, 0.0f, z);
        glVertex3f(x + 1.0f, wallHeight, z);
        glVertex3f(x, wallHeight, z);
        glEnd();

        if (currentView == BIRD_EYE) {
            glColor3f(0.0f, 0.0f, 0.0f);  
            glLineWidth(2.0f);
            glBegin(GL_LINES);
            glVertex3f(x, wallHeight, z);
            glVertex3f(x + 1.0f, wallHeight, z);
            glEnd();
            glLineWidth(1.0f);
        }
    }

    if (maze[x][z].walls[1]) {
        if (currentView == BIRD_EYE) {
            glColor3f(0.0f, 0.0f, 0.8f);
        } else {
            glColor3f(0.0f, 0.7f, 1.0f);
        }

        glBegin(GL_QUADS);
        glVertex3f(x + 1.0f, 0.0f, z);
        glVertex3f(x + 1.0f, 0.0f, z + 1.0f);
        glVertex3f(x + 1.0f, wallHeight, z + 1.0f);
        glVertex3f(x + 1.0f, wallHeight, z);
        glEnd();

        if (currentView == BIRD_EYE) {
            glColor3f(0.0f, 0.0f, 0.0f);
            glLineWidth(2.0f);
            glBegin(GL_LINES);
            glVertex3f(x + 1.0f, wallHeight, z);
            glVertex3f(x + 1.0f, wallHeight, z + 1.0f);
            glEnd();
            glLineWidth(1.0f);
        }
    }

    if (maze[x][z].walls[2]) {
        if (currentView == BIRD_EYE) {
            glColor3f(0.0f, 0.0f, 0.8f);
        } else {
            glColor3f(0.0f, 0.7f, 1.0f);
        }

        glBegin(GL_QUADS);
        glVertex3f(x, 0.0f, z + 1.0f);
        glVertex3f(x, wallHeight, z + 1.0f);
        glVertex3f(x + 1.0f, wallHeight, z + 1.0f);
        glVertex3f(x + 1.0f, 0.0f, z + 1.0f);
        glEnd();

        if (currentView == BIRD_EYE) {
            glColor3f(0.0f, 0.0f, 0.0f);
            glLineWidth(2.0f);
            glBegin(GL_LINES);
            glVertex3f(x, wallHeight, z + 1.0f);
            glVertex3f(x + 1.0f, wallHeight, z + 1.0f);
            glEnd();
            glLineWidth(1.0f);
        }
    }

    if (maze[x][z].walls[3]) {
        if (currentView == BIRD_EYE) {
            glColor3f(0.0f, 0.0f, 0.8f);
        } else {
            glColor3f(0.0f, 0.7f, 1.0f);
        }

        glBegin(GL_QUADS);
        glVertex3f(x, 0.0f, z);
        glVertex3f(x, wallHeight, z);
        glVertex3f(x, wallHeight, z + 1.0f);
        glVertex3f(x, 0.0f, z + 1.0f);
        glEnd();

        if (currentView == BIRD_EYE) {
            glColor3f(0.0f, 0.0f, 0.0f);
            glLineWidth(2.0f);
            glBegin(GL_LINES);
            glVertex3f(x, wallHeight, z);
            glVertex3f(x, wallHeight, z + 1.0f);
            glEnd();
            glLineWidth(1.0f);
        }
    }
}

void drawPlayer() {
    if (currentView == BIRD_EYE) {
        glPushMatrix();
        glTranslatef(playerX, 0.5f, playerZ);

        glColor3f(1.0f, 0.0f, 0.0f);  

        glutSolidSphere(0.4f, 16, 16);

        glColor3f(1.0f, 1.0f, 0.0f); 
        glLineWidth(3.0f);
        glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(cos(playerAngle) * 0.8f, 0.0f, sin(playerAngle) * 0.8f);
        glEnd();

        glBegin(GL_TRIANGLES);
        float tipX = cos(playerAngle) * 0.8f;
        float tipZ = sin(playerAngle) * 0.8f;
        float arrowSize = 0.2f;
        float angle1 = playerAngle + 2.5f;
        float angle2 = playerAngle - 2.5f;

        glVertex3f(tipX, 0.0f, tipZ);
        glVertex3f(tipX - cos(angle1) * arrowSize, 0.0f, tipZ - sin(angle1) * arrowSize);
        glVertex3f(tipX - cos(angle2) * arrowSize, 0.0f, tipZ - sin(angle2) * arrowSize);
        glEnd();

        glLineWidth(1.0f);
        glPopMatrix();
    }
}

void keyboard(unsigned char key, int x, int y) {
    float moveSpeed = 0.1f;
    float newX, newZ;

    // Handle game won state
    if (gameWon) {
        switch (key) {
            case 'r':
            case 'R':
                // Reset the game
                gameWon = false;
                initMaze();
                generateMaze();
                break;

            case 'q':
            case 'Q':
            case 27:  // ESC key
                exit(0);
                break;
        }
        return;  
    }

    // Normal game controls
    switch (key) {
        case 'q':
        case 'Q':
        case 27:  
            exit(0);
            break;

        case 'w':
        case 'W':
            // Move forward
            newX = playerX + cos(playerAngle) * moveSpeed;
            newZ = playerZ + sin(playerAngle) * moveSpeed;
            if (newX > 0 && newX < MAZE_WIDTH && newZ > 0 && newZ < MAZE_HEIGHT) {
                int cellX = floor(playerX);
                int cellZ = floor(playerZ);
                int newCellX = floor(newX);
                int newCellZ = floor(newZ);

                if (newCellX != cellX) {
                    int wallDir = (newCellX > cellX) ? 1 : 3;
                    if (maze[cellX][cellZ].walls[wallDir]) {
                        newX = playerX;
                    }
                }

                if (newCellZ != cellZ) {
                    int wallDir = (newCellZ > cellZ) ? 2 : 0;
                    if (maze[cellX][cellZ].walls[wallDir]) {
                        newZ = playerZ;
                    }
                }

                playerX = newX;
                playerZ = newZ;
            }
            break;

        case 's':
        case 'S':
            // Move backward
            newX = playerX - cos(playerAngle) * moveSpeed;
            newZ = playerZ - sin(playerAngle) * moveSpeed;
            // Check for collision
            if (newX > 0 && newX < MAZE_WIDTH && newZ > 0 && newZ < MAZE_HEIGHT) {
                int cellX = floor(playerX);
                int cellZ = floor(playerZ);
                int newCellX = floor(newX);
                int newCellZ = floor(newZ);

                if (newCellX != cellX) {
                    int wallDir = (newCellX > cellX) ? 1 : 3;
                    if (maze[cellX][cellZ].walls[wallDir]) {
                        newX = playerX;
                    }
                }

                if (newCellZ != cellZ) {
                    int wallDir = (newCellZ > cellZ) ? 2 : 0;
                    if (maze[cellX][cellZ].walls[wallDir]) {
                        newZ = playerZ;
                    }
                }

                playerX = newX;
                playerZ = newZ;
            }
            break;

        case 'a':
        case 'A':
            // Strafe left
            newX = playerX + cos(playerAngle - M_PI / 2) * moveSpeed;
            newZ = playerZ + sin(playerAngle - M_PI / 2) * moveSpeed;
            if (newX > 0 && newX < MAZE_WIDTH && newZ > 0 && newZ < MAZE_HEIGHT) {
                int cellX = floor(playerX);
                int cellZ = floor(playerZ);
                int newCellX = floor(newX);
                int newCellZ = floor(newZ);

                if (newCellX != cellX) {
                    int wallDir = (newCellX > cellX) ? 1 : 3;
                    if (maze[cellX][cellZ].walls[wallDir]) {
                        newX = playerX;
                    }
                }

                if (newCellZ != cellZ) {
                    int wallDir = (newCellZ > cellZ) ? 2 : 0;
                    if (maze[cellX][cellZ].walls[wallDir]) {
                        newZ = playerZ;
                    }
                }

                playerX = newX;
                playerZ = newZ;
            }
            break;

        case 'd':
        case 'D':
            // Strafe right
            newX = playerX + cos(playerAngle + M_PI / 2) * moveSpeed;
            newZ = playerZ + sin(playerAngle + M_PI / 2) * moveSpeed;
            if (newX > 0 && newX < MAZE_WIDTH && newZ > 0 && newZ < MAZE_HEIGHT) {
                int cellX = floor(playerX);
                int cellZ = floor(playerZ);
                int newCellX = floor(newX);
                int newCellZ = floor(newZ);

                if (newCellX != cellX) {
                    int wallDir = (newCellX > cellX) ? 1 : 3;
                    if (maze[cellX][cellZ].walls[wallDir]) {
                        newX = playerX;
                    }
                }

                if (newCellZ != cellZ) {
                    int wallDir = (newCellZ > cellZ) ? 2 : 0;
                    if (maze[cellX][cellZ].walls[wallDir]) {
                        newZ = playerZ;
                    }
                }

                playerX = newX;
                playerZ = newZ;
            }
            break;

        case 'f':
        case 'F':
            // First-person view
            currentView = FIRST_PERSON;
            break;

        case 'b':
        case 'B':
            // Bird's eye view
            currentView = BIRD_EYE;
            break;

        case 'r':
        case 'R':
            // Reset/regenerate maze
            gameWon = false;
            initMaze();
            generateMaze();
            break;
    }

    glutPostRedisplay();
}

void mouseFunc(int button, int state, int x, int y) {
    if (state == GLUT_DOWN && !gameWon) {
        switch (button) {
            case GLUT_LEFT_BUTTON:
                // Turn the user 90 degrees to the left
                playerAngle -= M_PI / 2.0f;
                break;

            case GLUT_RIGHT_BUTTON:
                // Turn the user 90 degrees to the right
                playerAngle += M_PI / 2.0f;
                break;

            case GLUT_MIDDLE_BUTTON:
                // Move the user forward
                float newX = playerX + cos(playerAngle) * 0.5f;
                float newZ = playerZ + sin(playerAngle) * 0.5f;

                // Check for collision
                if (newX > 0 && newX < MAZE_WIDTH && newZ > 0 && newZ < MAZE_HEIGHT) {
                    int cellX = floor(playerX);
                    int cellZ = floor(playerZ);
                    int newCellX = floor(newX);
                    int newCellZ = floor(newZ);

                    // Check if we're crossing a wall
                    bool canMove = true;

                    if (newCellX != cellX) {
                        int wallDir = (newCellX > cellX) ? 1 : 3;
                        if (maze[cellX][cellZ].walls[wallDir]) {
                            canMove = false;
                        }
                    }

                    if (newCellZ != cellZ) {
                        int wallDir = (newCellZ > cellZ) ? 2 : 0;
                        if (maze[cellX][cellZ].walls[wallDir]) {
                            canMove = false;
                        }
                    }

                    if (canMove) {
                        playerX = newX;
                        playerZ = newZ;
                    }
                }
                break;
        }

        glutPostRedisplay();
    }
}

void drawMiniMap() {
    // Save current matrices
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 800, 0, 600, -1, 1);  
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    glColor3f(0.1f, 0.1f, 0.1f);  
    glBegin(GL_QUADS);
    glVertex2f(10, 10);
    glVertex2f(160, 10);
    glVertex2f(160, 160);
    glVertex2f(10, 160);
    glEnd();

    float cellSize = 140.0f / MAZE_WIDTH;

    for (int x = 0; x < MAZE_WIDTH; x++) {
        for (int z = 0; z < MAZE_HEIGHT; z++) {
            float mapX = 10 + x * cellSize;
            float mapZ = 10 + z * cellSize;

            // Draw cell
            if (x == floor(playerX) && z == floor(playerZ)) {
                // Current cell
                glColor3f(0.2f, 0.2f, 0.5f);
            } else if (x == destX && z == destZ) {
                // Destination cell
                glColor3f(0.0f, 0.5f, 0.0f);
            } else {
                // Regular cell
                glColor3f(0.1f, 0.1f, 0.1f);
            }

            glBegin(GL_QUADS);
            glVertex2f(mapX, mapZ);
            glVertex2f(mapX + cellSize, mapZ);
            glVertex2f(mapX + cellSize, mapZ + cellSize);
            glVertex2f(mapX, mapZ + cellSize);
            glEnd();

            // Draw walls
            glColor3f(0.7f, 0.7f, 0.7f);
            glLineWidth(2.0f);

            // North wall
            if (maze[x][z].walls[0]) {
                glBegin(GL_LINES);
                glVertex2f(mapX, mapZ);
                glVertex2f(mapX + cellSize, mapZ);
                glEnd();
            }

            // East wall
            if (maze[x][z].walls[1]) {
                glBegin(GL_LINES);
                glVertex2f(mapX + cellSize, mapZ);
                glVertex2f(mapX + cellSize, mapZ + cellSize);
                glEnd();
            }

            // South wall
            if (maze[x][z].walls[2]) {
                glBegin(GL_LINES);
                glVertex2f(mapX, mapZ + cellSize);
                glVertex2f(mapX + cellSize, mapZ + cellSize);
                glEnd();
            }

            // West wall
            if (maze[x][z].walls[3]) {
                glBegin(GL_LINES);
                glVertex2f(mapX, mapZ);
                glVertex2f(mapX, mapZ + cellSize);
                glEnd();
            }
        }
    }

    // Draw player position on mini-map
    float playerMapX = 10 + playerX * cellSize;
    float playerMapZ = 10 + playerZ * cellSize;

    glColor3f(1.0f, 0.0f, 0.0f);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    glVertex2f(playerMapX, playerMapZ);
    glEnd();

    // Draw player direction
    glColor3f(1.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex2f(playerMapX, playerMapZ);
    glVertex2f(playerMapX + cos(playerAngle) * cellSize * 0.5f, playerMapZ + sin(playerAngle) * cellSize * 0.5f);
    glEnd();

    // Reset settings
    glLineWidth(1.0f);
    glPointSize(1.0f);
    glEnable(GL_DEPTH_TEST);

    // Restore matrices
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawBirdEyeView() {
    int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
    int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

    int minDimension = min(windowWidth, windowHeight) - 40;  
    float cellSize = minDimension / (float)max(MAZE_WIDTH, MAZE_HEIGHT);

    float startX = (windowWidth - MAZE_WIDTH * cellSize) / 2;
    float startY = (windowHeight - MAZE_HEIGHT * cellSize) / 2;

    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(windowWidth, 0);
    glVertex2f(windowWidth, windowHeight);
    glVertex2f(0, windowHeight);
    glEnd();

    for (int x = 0; x < MAZE_WIDTH; x++) {
        for (int z = 0; z < MAZE_HEIGHT; z++) {
            float cellX = startX + x * cellSize;
            float cellY = startY + z * cellSize;

            if (x == floor(playerX) && z == floor(playerZ)) {
                glColor3f(0.0f, 0.0f, 0.8f);
            } else if (x == destX && z == destZ) {
                glColor3f(0.0f, 0.8f, 0.0f);
            } else {
                glColor3f(0.8f, 0.8f, 0.8f);
            }

            glBegin(GL_QUADS);
            glVertex2f(cellX, cellY);
            glVertex2f(cellX + cellSize, cellY);
            glVertex2f(cellX + cellSize, cellY + cellSize);
            glVertex2f(cellX, cellY + cellSize);
            glEnd();

            glColor3f(0.0f, 0.0f, 0.0f);
            glLineWidth(2.0f);

            // North wall
            if (maze[x][z].walls[0]) {
                glBegin(GL_LINES);
                glVertex2f(cellX, cellY);
                glVertex2f(cellX + cellSize, cellY);
                glEnd();
            }

            // East wall
            if (maze[x][z].walls[1]) {
                glBegin(GL_LINES);
                glVertex2f(cellX + cellSize, cellY);
                glVertex2f(cellX + cellSize, cellY + cellSize);
                glEnd();
            }

            // South wall
            if (maze[x][z].walls[2]) {
                glBegin(GL_LINES);
                glVertex2f(cellX, cellY + cellSize);
                glVertex2f(cellX + cellSize, cellY + cellSize);
                glEnd();
            }

            // West wall
            if (maze[x][z].walls[3]) {
                glBegin(GL_LINES);
                glVertex2f(cellX, cellY);
                glVertex2f(cellX, cellY + cellSize);
                glEnd();
            }
        }
    }

    float playerCellX = startX + playerX * cellSize;
    float playerCellY = startY + playerZ * cellSize;

    glColor3f(1.0f, 0.0f, 0.0f);

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(playerCellX, playerCellY);  // center
    float radius = cellSize * 0.3f;
    for (int i = 0; i <= 20; i++) {
        float angle = i * (2.0f * M_PI / 20);
        glVertex2f(playerCellX + radius * cos(angle), playerCellY + radius * sin(angle));
    }
    glEnd();

    glColor3f(1.0f, 1.0f, 0.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex2f(playerCellX, playerCellY);
    glVertex2f(playerCellX + radius * 1.5f * cos(playerAngle), playerCellY + radius * 1.5f * sin(playerAngle));
    glEnd();
    glLineWidth(1.0f);

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(10, 20);
    const char* text = "Press 'f' to return to first-person view";
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
}

void drawSuccessScreen() {
    int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
    int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(0.0f, 0.2f, 0.4f);  // Dark blue background
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(windowWidth, 0);
    glVertex2f(windowWidth, windowHeight);
    glVertex2f(0, windowHeight);
    glEnd();

    glColor3f(1.0f, 1.0f, 0.0f);  // Yellow text
    const char* congratsText = "Congratulations!";
    const char* successText = "You have successfully completed the maze!";
    const char* restartText = "Press 'R' to restart";
    const char* exitText = "Press 'Q' to exit";

    // Calculate text positions
    int textWidth = 0;
    for (const char* c = congratsText; *c != '\0'; c++) {
        textWidth += glutBitmapWidth(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    }
    float congratsX = (windowWidth - textWidth) / 2.0f;
    float congratsY = windowHeight * 0.6f;

    textWidth = 0;
    for (const char* c = successText; *c != '\0'; c++) {
        textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *c);
    }
    float successX = (windowWidth - textWidth) / 2.0f;
    float successY = windowHeight * 0.5f;

    textWidth = 0;
    for (const char* c = restartText; *c != '\0'; c++) {
        textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, *c);
    }
    float restartX = (windowWidth - textWidth) / 2.0f;
    float restartY = windowHeight * 0.3f;

    textWidth = 0;
    for (const char* c = exitText; *c != '\0'; c++) {
        textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, *c);
    }
    float exitX = (windowWidth - textWidth) / 2.0f;
    float exitY = windowHeight * 0.25f;

    // Draw the text
    glRasterPos2f(congratsX, congratsY);
    for (const char* c = congratsText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    }

    glRasterPos2f(successX, successY);
    for (const char* c = successText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    glRasterPos2f(restartX, restartY);
    for (const char* c = restartText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    glRasterPos2f(exitX, exitY);
    for (const char* c = exitText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // Draw a decorative border
    glColor3f(1.0f, 0.8f, 0.0f);  // Gold color
    glLineWidth(3.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(windowWidth * 0.2f, windowHeight * 0.2f);
    glVertex2f(windowWidth * 0.8f, windowHeight * 0.2f);
    glVertex2f(windowWidth * 0.8f, windowHeight * 0.7f);
    glVertex2f(windowWidth * 0.2f, windowHeight * 0.7f);
    glEnd();
    glLineWidth(1.0f);

    // Restore matrices
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
