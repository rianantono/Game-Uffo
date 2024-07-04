#include <sl.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <fstream>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float GRAVITY = 800.0f;
const float JUMP_VELOCITY = 400.0f;
const float PIPE_GAP = 200.0f;
const float ANIMATION_INTERVAL = 0.1f;

float pipeSpeed = 200.0f;
int backgroundTextureIDDay;
int backgroundTextureIDNight;
int currentBackgroundTextureID;

struct Uffo {
    float x, y;
    float width, height;
    float velocityY;
    bool isAlive;
    float jumpCooldownTimer;
    float jumpSoundCooldown;
    int textureIDs[3];
    int currentTextureIndex;
    float animationTimer;

    Uffo(float posX, float posY, float w, float h, int texID1, int texID2, int texID3)
        : x(posX), y(posY), width(w), height(h), velocityY(0), isAlive(true), jumpCooldownTimer(0.0f),
        currentTextureIndex(0), animationTimer(0.0f), jumpSoundCooldown(0.0f) {
        textureIDs[0] = texID1;
        textureIDs[1] = texID2;
        textureIDs[2] = texID3;
    }

    void update(float dt) {
        if (isAlive) {
            velocityY += GRAVITY * dt;
            y -= velocityY * dt;

            if (jumpCooldownTimer > 0.0f) {
                jumpCooldownTimer -= dt;
            }

            if (jumpSoundCooldown > 0.0f) {
                jumpSoundCooldown -= dt;
            }

            animationTimer += dt;
            if (animationTimer >= ANIMATION_INTERVAL) {
                animationTimer = 0.0f;
                currentTextureIndex = (currentTextureIndex + 1) % 3;
            }
        }
    }

    void flap() {
        if (isAlive && jumpCooldownTimer <= 0.0f) {
            velocityY = -JUMP_VELOCITY;
            jumpCooldownTimer = 0.5f;
        }
    }

    void draw() {
        slSprite(textureIDs[currentTextureIndex], x, y, width, height);
    }

    void die() {
        isAlive = false;
    }
};

struct Pipe {
    float x, yTop;
    float width, height;
    bool scored;
    int textureIDTop;
    int textureIDBottom;

    Pipe(float posX, float posY, int texIDTop, int texIDBottom)
        : x(posX), yTop(posY), width(80.0f), height(WINDOW_HEIGHT), scored(false), textureIDTop(texIDTop), textureIDBottom(texIDBottom) {}

    void update(float dt) {
        x -= pipeSpeed * dt;
    }

    void draw() const {
        slSprite(textureIDTop, x, yTop + height / 2, width, height); // Top pipe
        slSprite(textureIDBottom, x, yTop - PIPE_GAP - height / 2, width, height); // Bottom pipe
    }

    bool isOffscreen() const {
        return x + width < 0;
    }

    bool isScored() const {
        return scored;
    }

    void setScored(bool value) {
        scored = value;
    }

};

std::vector<Pipe> pipes;
int uffoTextureID1, uffoTextureID2, uffoTextureID3;
int pipeTextureIDTop;
int pipeTextureIDBottom;
int backgroundTextureID;
int logoTextureID;
int startTextureID;
int gameOverTextureID;
Uffo uffo(WINDOW_WIDTH / 4, WINDOW_HEIGHT / 2, 50.0f, 30.0f, uffoTextureID1, uffoTextureID2, uffoTextureID3);
float pipeSpawnTimer = 0.0f;
float pipeSpawnInterval = 2.0f;
bool gameOver = false;
int score = 0;
bool readyToStart = false;
int highScore = 0;

// Sound IDs
int menuBGMID;
int inGameBGMID;
int jumpSoundID;
int scoreSoundID;
int gameOverSoundID;

void loadHighScore() {
    std::ifstream file("highscore.txt");
    if (file.is_open()) {
        file >> highScore;
        file.close();
    }
}

void saveHighScore() {
    std::ofstream file("highscore.txt");
    if (file.is_open()) {
        file << highScore;
        file.close();
    }
}

void resetGame() {
    if (score > highScore) {
        highScore = score;
        saveHighScore();
    }
    pipes.clear();
    uffo = Uffo(WINDOW_WIDTH / 4, WINDOW_HEIGHT / 2, 50.0f, 30.0f, uffoTextureID1, uffoTextureID2, uffoTextureID3);
    pipeSpawnTimer = 0.0f;
    score = 0;
    gameOver = false;
    readyToStart = false;
    pipeSpeed = 200.0f;
    currentBackgroundTextureID = backgroundTextureIDDay;
}

bool checkCollision(const Uffo& uffo, const Pipe& pipe) {
    float uffoLeft = uffo.x - uffo.width / 2;
    float uffoRight = uffo.x + uffo.width / 2;
    float uffoTop = uffo.y + uffo.height / 2;
    float uffoBottom = uffo.y - uffo.height / 2;

    float pipeLeft = pipe.x - pipe.width / 2;
    float pipeRight = pipe.x + pipe.width / 2;
    float gapTop = pipe.yTop;
    float gapBottom = pipe.yTop - PIPE_GAP;

    bool horizontalCollision = uffoRight > pipeLeft && uffoLeft < pipeRight;
    bool verticalCollisionTop = uffoTop > gapTop;
    bool verticalCollisionBottom = uffoBottom < gapBottom;

    // Check if the uffo is not in the safe gap area
    bool notInGapArea = uffoTop < gapTop&& uffoBottom > gapBottom;

    return horizontalCollision && (verticalCollisionTop || verticalCollisionBottom) && !notInGapArea;
}

void updateGame(float dt) {
    if (!gameOver && uffo.isAlive && readyToStart) {
        uffo.update(dt);

        pipeSpawnTimer += dt;
        if (pipeSpawnTimer >= pipeSpawnInterval) {
            pipeSpawnTimer = 0.0f;
            pipes.emplace_back(WINDOW_WIDTH, std::rand() % (WINDOW_HEIGHT - 400) + 200, pipeTextureIDTop, pipeTextureIDBottom);
        }

        for (auto& pipe : pipes) {
            pipe.update(dt);

            if (checkCollision(uffo, pipe)) {
                uffo.die();
                gameOver = true;
                slSoundPlay(gameOverSoundID);
                break;
            }

            if (!pipe.isScored() && pipe.x + pipe.width < uffo.x) {
                pipe.setScored(true);
                score++;
                slSoundPlay(scoreSoundID);
                if (score % 15 == 0) {
                    pipeSpeed += 50.0f;
                    currentBackgroundTextureID = (currentBackgroundTextureID == backgroundTextureIDDay) ? backgroundTextureIDNight : backgroundTextureIDDay;
                }
            }
        }

        pipes.erase(std::remove_if(pipes.begin(), pipes.end(),
            [](const Pipe& pipe) { return pipe.isOffscreen(); }),
            pipes.end());
    }

    if (gameOver && slGetKey(' ')) {
        resetGame();
    }
}

void drawGame() {
    slSprite(currentBackgroundTextureID, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, WINDOW_WIDTH, WINDOW_HEIGHT);

    if (!readyToStart) {
        slSprite(logoTextureID, WINDOW_WIDTH / 2, WINDOW_HEIGHT - 100, 384, 88);
        slSprite(startTextureID, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 - 20, 160, 66);
        //slSoundLoop(menuBGMID);
    }
    else {
        uffo.draw();

        for (const auto& pipe : pipes) {
            pipe.draw();
        }

        slSetForeColor(1.0f, 1.0f, 1.0f, 1.0f);
        slSetFontSize(30);
        slText(50, WINDOW_HEIGHT - 50, ("Score: " + std::to_string(score)).c_str());

        if (gameOver) {
            slSetFontSize(50);
            slText(WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 + 50, "Game Over!");
            slSetFontSize(30);
            slText(WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2, "Press SPACE to Restart");
            slText(WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 50, ("Score: " + std::to_string(score)).c_str());
            slText(WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 100, ("High Score: " + std::to_string(highScore)).c_str());
        }
        else {
            //slSoundLoop(inGameBGMID);
        }
    }
}

int main() {
    slWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Flappy Bird", false);

    slLoadFont("D:/Projrct Game/3/uffo/res/fonts/04B_19__.ttf");
    slSetFont(slLoadFont("D:/Projrct Game/3/uffo/res/fonts/04B_19__.ttf"), 24);

    uffoTextureID1 = slLoadTexture("D:/Projrct Game/3/uffo/res/textures/bird/uffo1.png");
    uffoTextureID2 = slLoadTexture("D:/Projrct Game/3/uffo/res/textures/bird/uffo2.png");
    uffoTextureID3 = slLoadTexture("D:/Projrct Game/3/uffo/res/textures/bird/uffo3.png");
    pipeTextureIDTop = slLoadTexture("D:/Projrct Game/3/uffo/res/textures/pipe2.png");
    pipeTextureIDBottom = slLoadTexture("D:/Projrct Game/3/uffo/res/textures/pipe2.png");
    backgroundTextureIDDay = slLoadTexture("D:/Projrct Game/3/uffo/res/textures/background/bg.png");
    backgroundTextureIDNight = slLoadTexture("D:/Projrct Game/3/uffo/res/textures/background/bg1.png");
    currentBackgroundTextureID = backgroundTextureIDDay;
    logoTextureID = slLoadTexture("D:/Projrct Game/3/uffo/res/textures/logo.png");
    startTextureID = slLoadTexture("D:/Projrct Game/3/uffo/res/textures/start.png");
    gameOverTextureID = slLoadTexture("D:/Projrct Game/3/uffo/res/textures/gameover.png");

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    //menuBGMID = slLoadWAV("D:/Semester 6/Proyek Game/UFO/UFO/res/sounds/bgmmenu.wav");
    inGameBGMID = slLoadWAV("D:/Projrct Game/3/uffo/res/sounds/bgm_ingame.wav");
    jumpSoundID = slLoadWAV("D:/Projrct Game/3/uffo/res/sounds/sfx_jump1.wav");
    scoreSoundID = slLoadWAV("D:/Projrct Game/3/uffo/res/sounds/sfx_point.wav");
    gameOverSoundID = slLoadWAV("D:/Projrct Game/3/uffo/res/sounds/sfx_die.wav");

    loadHighScore();
    slSoundLoop(inGameBGMID);

    while (!slShouldClose()) {
        float dt = slGetDeltaTime();

        if (!readyToStart && slGetKey(' ')) {
            readyToStart = true;
        }

        if (readyToStart && !gameOver) {
            if (slGetKey(' ') && uffo.jumpSoundCooldown <= 0.0f) {
                uffo.flap();
                slSoundPlay(jumpSoundID);
                uffo.jumpSoundCooldown = 0.5f; // cooldown waktu 0.5 detik
            }
        }

        updateGame(dt);
        drawGame();

        slRender();
    }

    slClose();
    return 0;
}
