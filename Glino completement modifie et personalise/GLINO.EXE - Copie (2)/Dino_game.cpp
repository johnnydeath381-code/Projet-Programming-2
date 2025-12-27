// ===============================
// INCLUDES
// ===============================
#include "raylib.h"
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cstring>  // Ajouté pour strcpy

// ===============================
// CONSTANTS
// ===============================
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const Vector2 GRAVITY = {0, 980};

// ===============================
// GLOBAL CONFIG
// ===============================
struct GameConfig {
    float musicVolume = 0.5f;
    float sfxVolume = 0.7f;
    float gameSpeed = 1.0f;
    bool fullscreen = false;
    bool vsync = true;
    int difficulty = 1; // 0: Easy, 1: Normal, 2: Hard
};

// ===============================
// ENUMS
// ===============================
enum class GameState {
    INTRO,
    MENU,
    GAME,
    OPTIONS,
    CREDITS,
    HIGHSCORES,
    HELP,
    GAME_OVER,
    EXIT
};

enum ZoroState {
    IDLE,
    WALK,
    JUMP,
    FALL,
    ATTACK1,
    ATTACK2,
    ATTACK3,
    ONI_GIRI,
    SANZEN_SEKAI,
    TATSUMAKI,
    HIRYU_KAEN,
    DEATH,
    VICTORY,
    HIT,
    BLOCK
};

enum AizenState {
    AIZEN_IDLE,
    AIZEN_WALK,
    AIZEN_GUARD,
    AIZEN_HADO,
    AIZEN_SPIRIT_SLASH,
    AIZEN_SUIGETSU_KYOKA,
    AIZEN_ATTACK,
    AIZEN_HIT,
    AIZEN_DEATH
};

// ===============================
// ANIMATION STRUCT
// ===============================
struct Animation {
    Texture2D texture{};
    Rectangle frame{};
    Vector2 position{};
    
    int frames = 1;
    int current = 0;
    float fps = 8.0f;
    float timer = 0.0f;
    bool loop = true;
    bool finished = false;
    int frameWidth = 0;
    int frameHeight = 0;

    void Init(Texture2D tex, int totalWidth, int frameHeight_, int frameCount, float fps_, bool loop_ = true) {
        texture = tex;
        frames = frameCount;
        fps = fps_;
        loop = loop_;
        frameWidth = totalWidth / frameCount;
        frameHeight = frameHeight_;
        frame = {0, 0, (float)frameWidth, (float)frameHeight_};
        finished = false;
    }

    void Update(float dt) {
        if (frames <= 1) return;
        
        timer += dt;
        if (timer >= 1.0f / fps) {
            timer = 0.0f;
            current++;
            
            if (current >= frames) {
                if (loop) current = 0;
                else {
                    current = frames - 1;
                    finished = true;
                }
            } else {
                finished = false;
            }
            
            frame.x = current * frame.width;
        }
    }

    void Draw(bool flip = false, Color tint = WHITE) {
        Rectangle source = frame;
        Rectangle dest = {position.x, position.y, frame.width, frame.height};
        
        if (flip) {
            source.width = -source.width;
            dest.x += frame.width;
        }
        
        DrawTextureRec(texture, source, {dest.x, dest.y}, tint);
    }

    void Reset() {
        current = 0;
        timer = 0.0f;
        finished = false;
        frame.x = 0;
    }

    bool Finished() const { return finished; }
    
    void Unload() {
        if (texture.id > 0) UnloadTexture(texture);
    }
};

// ===============================
// SCORE SYSTEM
// ===============================
struct HighScoreEntry {
    char name[20];
    int score;
    int combo;
    float time;
    
    bool operator<(const HighScoreEntry& other) const {
        return score > other.score;
    }
};

struct ScoreManager {
    std::vector<HighScoreEntry> scores;
    const int maxScores = 10;
    
    void LoadScores() {
        scores.clear();
        std::ifstream file("scores.txt");
        if (!file) return;
        
        HighScoreEntry entry;
        while (file >> entry.name >> entry.score >> entry.combo >> entry.time) {
            scores.push_back(entry);
        }
        file.close();
        SortScores();
    }
    
    void SaveScores() {
        std::ofstream file("scores.txt");
        if (!file) return;
        
        SortScores();
        int count = 0;
        for (const auto& entry : scores) {
            if (count++ >= maxScores) break;
            file << entry.name << " " << entry.score << " " 
                 << entry.combo << " " << entry.time << "\n";
        }
        file.close();
    }
    
    void AddScore(const char* name, int score, int combo, float time) {
        HighScoreEntry entry;
        strcpy(entry.name, name);
        entry.score = score;
        entry.combo = combo;
        entry.time = time;
        scores.push_back(entry);
        SortScores();
        SaveScores();
    }
    
    void SortScores() {
        std::sort(scores.begin(), scores.end());
    }
    
    bool IsHighScore(int score) const {
        if (scores.size() < maxScores) return true;
        return score > scores.back().score;
    }
};

// ===============================
// PARTICLE SYSTEM
// ===============================
struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float size;
    float life;
    float maxLife;
    float rotation;
    float rotationSpeed;
};

struct ParticleSystem {
    std::vector<Particle> particles;
    
    void CreateExplosion(Vector2 pos, int count, Color color, float minSpeed = 100, float maxSpeed = 400) {
        for (int i = 0; i < count; i++) {
            Particle p;
            p.position = pos;
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float speed = (float)GetRandomValue((int)minSpeed, (int)maxSpeed);
            p.velocity = {cosf(angle) * speed, sinf(angle) * speed};
            p.color = color;
            p.size = (float)GetRandomValue(3, 10);
            p.life = p.maxLife = (float)GetRandomValue(5, 15) / 10.0f;
            p.rotation = (float)GetRandomValue(0, 360);
            p.rotationSpeed = (float)GetRandomValue(-500, 500);
            particles.push_back(p);
        }
    }
    
    void CreateSlashTrail(Vector2 start, Vector2 end, Color color) {
        int count = 30;
        float width = 5.0f;
        
        for (int i = 0; i < count; i++) {
            float t = (float)i / (float)count;
            Vector2 pos = {
                start.x + (end.x - start.x) * t + (float)GetRandomValue(-10, 10),
                start.y + (end.y - start.y) * t + (float)GetRandomValue(-10, 10)
            };
            
            Particle p;
            p.position = pos;
            Vector2 dir = {end.x - start.x, end.y - start.y};
            float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
            if (len > 0) {
                dir.x /= len;
                dir.y /= len;
            }
            p.velocity = {dir.x * (float)GetRandomValue(50, 150), dir.y * (float)GetRandomValue(50, 150)};
            p.color = color;
            p.size = width * (1.0f - t) + (float)GetRandomValue(1, 3);
            p.life = p.maxLife = 0.3f + (float)GetRandomValue(0, 5) / 10.0f;
            p.rotation = atan2f(dir.y, dir.x) * RAD2DEG;
            p.rotationSpeed = (float)GetRandomValue(-100, 100);
            particles.push_back(p);
        }
    }
    
    void CreateSparks(Vector2 pos, int count, Color color) {
        for (int i = 0; i < count; i++) {
            Particle p;
            p.position = pos;
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float speed = (float)GetRandomValue(200, 600);
            p.velocity = {cosf(angle) * speed, sinf(angle) * speed};
            p.color = color;
            p.size = (float)GetRandomValue(1, 4);
            p.life = p.maxLife = (float)GetRandomValue(3, 8) / 10.0f;
            p.rotation = 0;
            p.rotationSpeed = (float)GetRandomValue(-1000, 1000);
            particles.push_back(p);
        }
    }
    
    void CreateSmoke(Vector2 pos, int count, Color color) {
        for (int i = 0; i < count; i++) {
            Particle p;
            p.position = pos;
            p.velocity = {(float)GetRandomValue(-50, 50), (float)GetRandomValue(-100, -50)};
            p.color = color;
            p.size = (float)GetRandomValue(10, 25);
            p.life = p.maxLife = (float)GetRandomValue(15, 30) / 10.0f;
            p.rotation = 0;
            p.rotationSpeed = (float)GetRandomValue(-50, 50);
            particles.push_back(p);
        }
    }
    
    void Update(float dt) {
        for (auto& p : particles) {
            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.velocity.y += GRAVITY.y * dt * 0.1f;
            p.rotation += p.rotationSpeed * dt;
            p.life -= dt;
        }
        
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const Particle& p) { return p.life <= 0; }),
            particles.end()
        );
    }
    
    void Draw() {
        for (auto& p : particles) {
            float alpha = p.life / p.maxLife * 255;
            Color color = {p.color.r, p.color.g, p.color.b, (unsigned char)alpha};
            
            DrawRectanglePro(
                {p.position.x, p.position.y, p.size, p.size * 0.3f},
                {p.size/2, p.size * 0.15f},
                p.rotation,
                color
            );
        }
    }
};

// ===============================
// HEALTH BAR
// ===============================
struct HealthBar {
    int maxHp = 100;
    int hp = 100;
    Vector2 pos{20, 20};
    float width = 300;
    float height = 25;
    char name[50] = "";
    
    void Draw() {
        // Background
        DrawRectangle(pos.x - 2, pos.y - 2, width + 4, height + 4, {30, 30, 30, 200});
        
        // Health bar gradient
        float healthPercent = (float)hp / (float)maxHp;
        Color startColor = RED;
        Color endColor = GREEN;
        Color healthColor = {
            (unsigned char)(startColor.r + (endColor.r - startColor.r) * healthPercent),
            (unsigned char)(startColor.g + (endColor.g - startColor.g) * healthPercent),
            (unsigned char)(startColor.b + (endColor.b - startColor.b) * healthPercent),
            255
        };
        
        DrawRectangle(pos.x, pos.y, width * healthPercent, height, healthColor);
        
        // Gloss effect
        DrawRectangle(pos.x, pos.y, width * healthPercent, height/3, {255, 255, 255, 50});
        
        // Border
        DrawRectangleLines(pos.x - 2, pos.y - 2, width + 4, height + 4, WHITE);
        
        // Name and health text
        DrawText(name, pos.x, pos.y - 25, 20, WHITE);
        DrawText(TextFormat("%d/%d", hp, maxHp), pos.x + width - 60, pos.y + 5, 18, WHITE);
    }
};

// ===============================
// AIZEN AI SYSTEM
// ===============================
struct AizenAI {
    float timer = 0.0f;
    float actionTimer = 0.0f;
    float moveTimer = 0.0f;
    Vector2 targetPosition{0, 0};
    bool moving = false;
    
    void Update(float dt, Vector2& position, Vector2& velocity, const Vector2& playerPos, AizenState& state, 
                float& attackCooldown, int difficulty) {
        timer += dt;
        actionTimer += dt;
        moveTimer += dt;
        
        // Difficulty modifiers
        float baseActionDelay = 2.0f - difficulty * 0.5f;
        float moveChance = 0.3f + difficulty * 0.2f;
        float attackChance = 0.4f + difficulty * 0.3f;
        
        // Random movement
        if (!moving && GetRandomValue(0, 100) < (int)(moveChance * 100 * dt) && moveTimer > 1.0f) {
            moving = true;
            targetPosition = {
                (float)GetRandomValue(200, SCREEN_WIDTH - 200),
                430.0f
            };
            moveTimer = 0.0f;
        }
        
        if (moving) {
            Vector2 direction = {targetPosition.x - position.x, 0};
            float distance = sqrtf(direction.x * direction.x + direction.y * direction.y);
            
            if (distance > 10) {
                float len = sqrtf(direction.x * direction.x + direction.y * direction.y);
                if (len > 0) {
                    direction.x /= len;
                    direction.y /= len;
                }
                velocity.x = direction.x * 150.0f;
            } else {
                moving = false;
                velocity.x = 0;
            }
        }
        
        // Random attacks
        if (actionTimer > baseActionDelay && GetRandomValue(0, 100) < (int)(attackChance * 100)) {
            int attackType = GetRandomValue(0, 100);
            if (attackCooldown <= 0) {
                if (attackType < 40) {
                    state = AIZEN_HADO;
                } else if (attackType < 70) {
                    state = AIZEN_SPIRIT_SLASH;
                } else {
                    state = AIZEN_SUIGETSU_KYOKA;
                }
                actionTimer = 0.0f;
            }
        }
    }
};

// ===============================
// SPECIAL EFFECTS
// ===============================
struct HadoEffect {
    Animation anim;
    bool active = false;
    float speed = 400.0f;
    float damage = 30;
    
    void Init() {
        Texture2D tex = LoadTexture("textures/aizen/Hado effect.png");
        anim.Init(tex, 662, 164, 10, 12.0f, false);
    }
    
    void Cast(Vector2 pos, bool facingRight) {
        active = true;
        anim.Reset();
        anim.position = {pos.x - 100, pos.y - 50};
    }
    
    void Update(float dt) {
        if (!active) return;
        anim.Update(dt);
        if (anim.Finished()) active = false;
    }
    
    void Draw(bool facingRight) {
        if (active) anim.Draw(!facingRight, {255, 100, 100, 200});
    }
    
    void Unload() { anim.Unload(); }
};

struct SpiritSlashEffect {
    Animation anim;
    bool active = false;
    float cooldown = 10.0f;
    float currentCooldown = 0.0f;
    float damage = 100;
    
    void Init() {
        Texture2D tex = LoadTexture("textures/aizen/Spirit slash effect.png");
        anim.Init(tex, 268, 102, 3, 8.0f, false);
    }
    
    bool CanCast() { return currentCooldown <= 0; }
    
    void Cast(Vector2 pos) {
        if (!CanCast()) return;
        active = true;
        currentCooldown = cooldown;
        anim.Reset();
        anim.position = {pos.x - 100, pos.y - 50};
    }
    
    void Update(float dt) {
        currentCooldown -= dt;
        if (currentCooldown < 0) currentCooldown = 0;
        if (!active) return;
        anim.Update(dt);
        if (anim.Finished()) active = false;
    }
    
    void Draw() {
        if (active) anim.Draw(false, {200, 150, 255, 220});
    }
    
    void Unload() { anim.Unload(); }
};
//Jai pas pu mettre les effets de cette partie javais terminer le code avant les textures
struct SuigetsuKyokaEffect {
    Animation anim;
    bool active = false;
    float duration = 3.0f;
    float timer = 0.0f;
    float damage = 50;
    
    void Init() {
        Texture2D tex = LoadTexture("textures/aizen/Suigetsu kyoka.png");
        anim.Init(tex, 1058, 110, 12, 15.0f, true);
    }
    
    void Cast(Vector2 pos) {
        active = true;
        timer = duration;
        anim.Reset();
        anim.position = {pos.x - 200, pos.y - 100};
    }
    
    void Update(float dt) {
        if (!active) return;
        timer -= dt;
        anim.Update(dt);
        if (timer <= 0) active = false;
    }
    
    void Draw() {
        if (active) {
            Color tint = {255, 255, 255, (unsigned char)(150 + 105 * sinf(GetTime() * 5))};
            anim.Draw(false, tint);
        }
    }
    
    void Unload() { anim.Unload(); }
};

// ===============================
// ZORO CHARACTER
// ===============================
struct Zoro {
    Animation idle;
    Animation walk;
    Animation jump;
    Animation fall;
    Animation attack1;
    Animation attack2;
    Animation attack3;
    Animation oniGiri;
    Animation sanzenSekai;
    Animation tatsumaki;
    Animation hiryuKaen;
    Animation death;
    Animation victory;
    Animation hit;
    Animation block;
    
    ZoroState state = IDLE;
    bool facingRight = true;
    Vector2 position{320, 430};
    Vector2 velocity{0, 0};
    bool isGrounded = false;
    float jumpForce = -450.0f;
    float moveSpeed = 280.0f;
    
    int damage = 20;
    int comboCount = 0;
    float comboTimer = 0.0f;
    float comboWindow = 0.5f;
    float attackCooldown = 0.0f;
    float blockCooldown = 0.0f;
    bool isBlocking = false;
    
    HealthBar healthBar;
    ParticleSystem particles;
    float hitStopTime = 0.0f;
    float shakeTime = 0.0f;
    int score = 0;
    int maxCombo = 0;
    
    void Init() {
        // Load textures with correct dimensions
        idle.Init(LoadTexture("textures/zoro_assets/walk.png"), 1150, 157, 8, 12.0f);
        walk.Init(LoadTexture("textures/zoro_assets/walk.png"), 1150, 157, 8, 12.0f);
        jump.Init(LoadTexture("textures/zoro_assets/Jump.png"), 1190, 300, 7, 10.0f, false);
        fall.Init(LoadTexture("textures/zoro_assets/Fall.png"), 2168, 155, 12, 12.0f, false);
        attack1.Init(LoadTexture("textures/zoro_assets/Attack1.png"), 1122, 170, 6, 15.0f, false);
        attack2.Init(LoadTexture("textures/zoro_assets/Attack2.png"), 937, 179, 5, 15.0f, false);
        attack3.Init(LoadTexture("textures/zoro_assets/Attack3.png"), 1274, 155, 7, 15.0f, false);
        oniGiri.Init(LoadTexture("textures/zoro_assets/Oni Giri.png"), 1280, 244, 6, 12.0f, false);
        sanzenSekai.Init(LoadTexture("textures/zoro_assets/Sanzen sekai.png"), 1280, 160, 8, 12.0f, false);
        tatsumaki.Init(LoadTexture("textures/zoro_assets/Tatsumaki.png"), 1970, 210, 8, 12.0f, false);
        hiryuKaen.Init(LoadTexture("textures/zoro_assets/Hiryu Kaen.png"), 1797, 256, 10, 12.0f, false);
        death.Init(LoadTexture("textures/zoro_assets/Death.png"), 1274, 155, 7, 8.0f, false);
        victory.Init(LoadTexture("textures/zoro_assets/Victory.png"), 1000, 170, 5, 6.0f, false);
        hit.Init(LoadTexture("textures/zoro_assets/Hit.png"), 260, 105, 3, 10.0f, false);
        block.Init(LoadTexture("textures/zoro_assets/IDLE.png"), 894, 213, 1, 1.0f);
        
        strcpy(healthBar.name, "RORONOA ZORO");
        healthBar.maxHp = healthBar.hp = 1000;
        healthBar.pos = {20, 20};
    }
    
    void Update(float dt) {
        if (hitStopTime > 0) {
            hitStopTime -= dt;
            return;
        }
        
        comboTimer -= dt;
        attackCooldown -= dt;
        blockCooldown -= dt;
        
        if (state == HIT) {
            if (hit.Finished()) state = IDLE;
        } else if (state == DEATH) {
            // Quand le joueur meurt il reste au sol et ne bouge pas

        } else {
            HandleInput(dt);
        }
        
        // Physics
        if (state != HIT && state != DEATH) {
            position.x += velocity.x * dt;
            position.y += velocity.y * dt;
            
            if (!isGrounded) {
                velocity.y += GRAVITY.y * dt;
            }
            
            if (position.y > 430) {
                position.y = 430;
                velocity.y = 0;
                isGrounded = true;
                if (state == JUMP || state == FALL) state = IDLE;
            }
        }
        
        UpdateAnimationPositions();
        UpdateCurrentAnimation(dt);
        particles.Update(dt);
        
        if (shakeTime > 0) shakeTime -= dt;
    }
    
    void HandleInput(float dt) {
        velocity.x = 0;
        isBlocking = false;
        
        // Mouvement
        if (IsKeyDown(KEY_RIGHT)) {
            velocity.x = moveSpeed;
            facingRight = true;
            if (isGrounded && !IsAttacking()) state = WALK;
        } else if (IsKeyDown(KEY_LEFT)) {
            velocity.x = -moveSpeed;
            facingRight = false;
            if (isGrounded && !IsAttacking()) state = WALK;
        } else if (isGrounded && state == WALK) {
            state = IDLE;
        }
        
        //  sauter
        if (IsKeyPressed(KEY_SPACE) && isGrounded) {
            velocity.y = jumpForce;
            isGrounded = false;
            state = JUMP;
            jump.Reset();
            particles.CreateSparks({position.x, position.y + 80}, 10, GREEN);
        }



        if (IsKeyDown(KEY_A) && blockCooldown <= 0) {
            isBlocking = true;
            state = BLOCK;
            velocity.x *= 0.5f;
        }
        
        // Les Attaques
        if (attackCooldown <= 0 && !isBlocking) {
            if (IsKeyPressed(KEY_J)) {
                StartAttack(ATTACK1);
                attackCooldown = 0.2f;
            } else if (IsKeyPressed(KEY_K) && isGrounded) {
                StartAttack(ONI_GIRI);
                attackCooldown = 1.0f;
            } else if (IsKeyPressed(KEY_L) && isGrounded) {
                StartAttack(TATSUMAKI);
                attackCooldown = 2.0f;
            } else if (IsKeyDown(KEY_U) && IsKeyDown(KEY_I) && isGrounded) {
                StartAttack(HIRYU_KAEN);
                attackCooldown = 5.0f;
            } else if (IsKeyPressed(KEY_O)) {
                StartAttack(SANZEN_SEKAI);
                attackCooldown = 3.0f;
            }
        }
    }
    
    void StartAttack(ZoroState attackType) {
        state = attackType;
        comboTimer = comboWindow;
        comboCount++;
        if (comboCount > maxCombo) maxCombo = comboCount;
        
        switch (attackType) {
            case ATTACK1: attack1.Reset(); break;
            case ATTACK2: attack2.Reset(); break;
            case ATTACK3: attack3.Reset(); break;
            case ONI_GIRI: oniGiri.Reset(); break;
            case TATSUMAKI: tatsumaki.Reset(); break;
            case HIRYU_KAEN: hiryuKaen.Reset(); break;
            case SANZEN_SEKAI: sanzenSekai.Reset(); break;
            default: break;
        }
        
        Vector2 slashPos = {position.x + (facingRight ? 100 : -100), position.y + 40};
        particles.CreateSlashTrail(slashPos, 
            {slashPos.x + (facingRight ? 200 : -200), slashPos.y + 50}, GREEN);
        particles.CreateSparks(slashPos, 15, {100, 255, 100, 255});
    }
    
    void UpdateAnimationPositions() {
        idle.position = position;
        walk.position = position;
        jump.position = {position.x, position.y - 100};
        fall.position = position;
        attack1.position = position;
        attack2.position = position;
        attack3.position = position;
        oniGiri.position = {position.x, position.y - 50};
        sanzenSekai.position = {position.x, position.y - 30};
        tatsumaki.position = position;
        hiryuKaen.position = {position.x, position.y - 80};
        death.position = position;
        victory.position = position;
        hit.position = position;
        block.position = position;
    }
    
    void UpdateCurrentAnimation(float dt) {
        switch (state) {
            case IDLE: idle.Update(dt); break;
            case WALK: walk.Update(dt); break;
            case JUMP: jump.Update(dt); break;
            case FALL: fall.Update(dt); break;
            case ATTACK1: 
                attack1.Update(dt);
                if (attack1.Finished()) state = IDLE;
                break;
            case ATTACK2: 
                attack2.Update(dt);
                if (attack2.Finished()) state = IDLE;
                break;
            case ATTACK3: 
                attack3.Update(dt);
                if (attack3.Finished()) state = IDLE;
                break;
            case ONI_GIRI: 
                oniGiri.Update(dt);
                if (oniGiri.Finished()) state = IDLE;
                break;
            case TATSUMAKI: 
                tatsumaki.Update(dt);
                if (tatsumaki.Finished()) state = IDLE;
                break;
            case HIRYU_KAEN: 
                hiryuKaen.Update(dt);
                if (hiryuKaen.Finished()) state = IDLE;
                break;
            case SANZEN_SEKAI: 
                sanzenSekai.Update(dt);
                if (sanzenSekai.Finished()) state = IDLE;
                break;
            case DEATH: death.Update(dt); break;
            case VICTORY: victory.Update(dt); break;
            case HIT: hit.Update(dt); break;
            case BLOCK: break;
        }
    }
    
    void TakeDamage(int amount) {
        if (isBlocking) {
            amount /= 4;
            particles.CreateSparks({position.x + (facingRight ? 50 : -50), position.y + 50}, 
                                  5, {255, 255, 100, 255});
        }
        
        healthBar.hp -= amount;
        comboCount = 0;
        
        if (healthBar.hp <= 0) {
            healthBar.hp = 0;
            state = DEATH;
            death.Reset();
            particles.CreateExplosion({position.x, position.y + 50}, 30, {200, 50, 50, 255});
        } else {
            state = HIT;
            hit.Reset();
            hitStopTime = 0.1f;
            shakeTime = 0.2f;
            particles.CreateExplosion({position.x, position.y + 50}, 20, RED);
        }
    }
    
    void AddScore(int points) {
        score += points * (comboCount + 1);
    }
    
    void Draw() {
        Color tint = isBlocking ? SKYBLUE : WHITE;
        
        switch (state) {
            case IDLE: idle.Draw(!facingRight, tint); break;
            case WALK: walk.Draw(!facingRight, tint); break;
            case JUMP: jump.Draw(!facingRight, tint); break;
            case FALL: fall.Draw(!facingRight, tint); break;
            case ATTACK1: attack1.Draw(!facingRight, tint); break;
            case ATTACK2: attack2.Draw(!facingRight, tint); break;
            case ATTACK3: attack3.Draw(!facingRight, tint); break;
            case ONI_GIRI: oniGiri.Draw(!facingRight, tint); break;
            case TATSUMAKI: tatsumaki.Draw(!facingRight, tint); break;
            case HIRYU_KAEN: hiryuKaen.Draw(!facingRight, tint); break;
            case SANZEN_SEKAI: sanzenSekai.Draw(!facingRight, tint); break;
            case DEATH: death.Draw(!facingRight, tint); break;
            case VICTORY: victory.Draw(!facingRight, tint); break;
            case HIT: hit.Draw(!facingRight, tint); break;
            case BLOCK: idle.Draw(!facingRight, {100, 150, 255, 200}); break;
        }
        
        particles.Draw();
        healthBar.Draw();
    }
    
    Rectangle GetAttackBounds() const {
        Rectangle bounds = {0, 0, 0, 0};
        if (!IsAttacking()) return bounds;
        
        float width = 100;
        float height = 100;
        
        switch (state) {
            case ATTACK1:
            case ATTACK2:
            case ATTACK3:
                bounds = {position.x + (facingRight ? 50 : -150), position.y, 100, 100};
                break;
            case ONI_GIRI:
                bounds = {position.x + (facingRight ? 50 : -200), position.y - 50, 150, 150};
                break;
            case SANZEN_SEKAI:
                bounds = {position.x + (facingRight ? 0 : -300), position.y - 30, 300, 130};
                break;
            case TATSUMAKI:
                bounds = {position.x + (facingRight ? 0 : -300), position.y, 300, 150};
                break;
            case HIRYU_KAEN:
                bounds = {position.x + (facingRight ? 0 : -400), position.y - 100, 400, 256};
                break;
            default: break;
        }
        return bounds;
    }
    
    bool IsAttacking() const {
        return state == ATTACK1 || state == ATTACK2 || state == ATTACK3 ||
               state == ONI_GIRI || state == TATSUMAKI || state == HIRYU_KAEN ||
               state == SANZEN_SEKAI;
    }
    
    bool IsVulnerable() const {
        return state != HIT && state != DEATH;
    }
    
    void Unload() {
        idle.Unload();
        walk.Unload();
        jump.Unload();
        fall.Unload();
        attack1.Unload();
        attack2.Unload();
        attack3.Unload();
        oniGiri.Unload();
        sanzenSekai.Unload();
        tatsumaki.Unload();
        hiryuKaen.Unload();
        death.Unload();
        victory.Unload();
        hit.Unload();
        block.Unload();
    }
};

// ===============================
// AIZEN CHARACTER
// ===============================
struct Aizen {
    Animation idle;
    Animation walk;
    Animation guard;
    Animation attack;
    Animation hit;
    Animation death;
    
    AizenState state = AIZEN_IDLE;
    Vector2 position{900, 430};
    Vector2 velocity{0, 0};
    bool facingRight = false;
    bool isGuarding = false;
    
    HealthBar healthBar;
    HadoEffect hado;
    SpiritSlashEffect spiritSlash;
    SuigetsuKyokaEffect suigetsuKyoka;
    float attackTimer = 0.0f;
    float attackInterval = 3.0f;
    AizenAI ai;
    ParticleSystem particles;
    int difficulty = 1;
    
    void Init(int diff) {
        difficulty = diff;
        
        idle.Init(LoadTexture("textures/aizen/Idle.png"), 264, 104, 4, 4.0f);
        walk.Init(LoadTexture("textures/aizen/Suigetsu kyoka.png"), 1058, 110, 12, 4.0f);
        guard.Init(LoadTexture("textures/aizen/Guard.png"), 70, 105, 1, 1.0f);
        attack.Init(LoadTexture("textures/aizen/Hado.png"), 482, 104, 5, 10.0f, false);
        hit.Init(LoadTexture("textures/zoro_assets/Hit.png"), 260, 105, 3, 10.0f, false);
        death.Init(LoadTexture("textures/aizen/Guard.png"), 75, 105, 1, 1.0f);
        
        strcpy(healthBar.name, "SOSUKE AIZEN");
        healthBar.maxHp = healthBar.hp = 800 + diff * 200;
        healthBar.pos = {SCREEN_WIDTH - 320, 20};
        
        hado.Init();
        spiritSlash.Init();
        suigetsuKyoka.Init();
    }
    
    void Update(float dt, const Zoro& zoro) {
        hado.Update(dt);
        spiritSlash.Update(dt);
        suigetsuKyoka.Update(dt);
        particles.Update(dt);
        
        if (state == AIZEN_HIT) {
            if (hit.Finished()) state = AIZEN_IDLE;
        } else if (state == AIZEN_DEATH) {
            // Stay in death
        } else {
            ai.Update(dt, position, velocity, zoro.position, state, attackTimer, difficulty);
            
            // Update state based on AI
            if (isGuarding) {
                state = AIZEN_GUARD;
            } else if (state == AIZEN_HADO || state == AIZEN_SPIRIT_SLASH || 
                       state == AIZEN_SUIGETSU_KYOKA || state == AIZEN_ATTACK) {
                // Handle attack animations
                if (state == AIZEN_HADO && !hado.active) state = AIZEN_IDLE;
                if (state == AIZEN_SPIRIT_SLASH && !spiritSlash.active) state = AIZEN_IDLE;
                if (state == AIZEN_SUIGETSU_KYOKA && !suigetsuKyoka.active) state = AIZEN_IDLE;
                if (state == AIZEN_ATTACK && attack.Finished()) state = AIZEN_IDLE;
            } else {
                if (velocity.x != 0) state = AIZEN_WALK;
                else state = AIZEN_IDLE;
            }
            
            // Update facing
            if (velocity.x > 0) facingRight = true;
            else if (velocity.x < 0) facingRight = false;
            
            // Apply movement
            position.x += velocity.x * dt;
            if (position.x < 200) position.x = 200;
            if (position.x > SCREEN_WIDTH - 200) position.x = SCREEN_WIDTH - 200;
            
            // Execute attacks
            if (state == AIZEN_HADO) {
                if (!hado.active) {
                    hado.Cast(position, facingRight);
                    particles.CreateExplosion({position.x - 100, position.y + 50}, 
                                              10, {255, 100, 100, 255});
                }
            } else if (state == AIZEN_SPIRIT_SLASH) {
                if (!spiritSlash.active && spiritSlash.CanCast()) {
                    spiritSlash.Cast(position);
                    particles.CreateExplosion({position.x - 100, position.y}, 
                                              20, {200, 150, 255, 255});
                }
            } else if (state == AIZEN_SUIGETSU_KYOKA) {
                if (!suigetsuKyoka.active) {
                    suigetsuKyoka.Cast(position);
                    particles.CreateSmoke({position.x, position.y + 50}, 10, {200, 200, 255, 150});
                }
            } else if (state == AIZEN_ATTACK) {
                attack.Update(dt);
            }
        }
        
        // Update animations
        idle.position = position;
        walk.position = position;
        guard.position = position;
        attack.position = position;
        hit.position = position;
        death.position = position;
    }
    
    void TakeDamage(int amount) {
        if (state == AIZEN_GUARD) {
            amount /= 2;
            particles.CreateSparks({position.x, position.y + 50}, 5, {255, 255, 100, 255});
        }
        
        healthBar.hp -= amount;
        
        if (healthBar.hp <= 0) {
            healthBar.hp = 0;
            state = AIZEN_DEATH;
            particles.CreateExplosion({position.x, position.y + 50}, 30, {150, 50, 200, 255});
        } else if (state != AIZEN_HIT) {
            state = AIZEN_HIT;
            hit.Reset();
            particles.CreateExplosion({position.x, position.y + 50}, 15, PURPLE);
        }
    }
    
    void Draw() {
        hado.Draw(facingRight);
        spiritSlash.Draw();
        suigetsuKyoka.Draw();
        
        Color tint = WHITE;
        if (state == AIZEN_GUARD) tint = SKYBLUE;
        
        switch (state) {
            case AIZEN_IDLE: idle.Draw(facingRight, tint); break;
            case AIZEN_WALK: walk.Draw(facingRight, tint); break;
            case AIZEN_GUARD: guard.Draw(facingRight, tint); break;
            case AIZEN_ATTACK: attack.Draw(facingRight, tint); break;
            case AIZEN_HIT: hit.Draw(facingRight, tint); break;
            case AIZEN_DEATH: death.Draw(facingRight, {150, 50, 200, 200}); break;
            default: idle.Draw(facingRight, tint); break;
        }
        
        particles.Draw();
        healthBar.Draw();
    }
    
    Rectangle GetBounds() const {
        return {position.x - 37, position.y, 75, 105};
    }
    
    Rectangle GetHadoBounds() const {
        if (!hado.active) return {0, 0, 0, 0};
        return {hado.anim.position.x, hado.anim.position.y, 120, 120};
    }
    
    Rectangle GetSpiritSlashBounds() const {
        if (!spiritSlash.active) return {0, 0, 0, 0};
        return {spiritSlash.anim.position.x, spiritSlash.anim.position.y, 268/3, 102};
    }
    
    Rectangle GetSuigetsuBounds() const {
        if (!suigetsuKyoka.active) return {0, 0, 0, 0};
        return {suigetsuKyoka.anim.position.x, suigetsuKyoka.anim.position.y, 1058/12, 110};
    }
    
    bool IsVulnerable() const {
        return state != AIZEN_HIT && state != AIZEN_DEATH;
    }
    
    void Unload() {
        idle.Unload();
        walk.Unload();
        guard.Unload();
        attack.Unload();
        hit.Unload();
        death.Unload();
        hado.Unload();
        spiritSlash.Unload();
        suigetsuKyoka.Unload();
    }
};

// ===============================
// GAME SCENE
// ===============================
struct GameScene {
    Texture2D bg{};
    Texture2D ground{};
    Zoro zoro;
    Aizen aizen;
    bool initialized = false;
    float gameTime = 0.0f;
    float hitStop = 0.0f;
    ScoreManager scoreManager;
    char playerName[20] = "PLAYER";
    bool nameInput = false;
    
    void Init(int difficulty) {
        bg = LoadTexture("textures/background.png");
        ground = LoadTexture("textures/ground.png");
        zoro.Init();
        aizen.Init(difficulty);
        scoreManager.LoadScores();
        strcpy(playerName, "PLAYER");
        initialized = true;
    }
    
    void Update(float dt) {
        if (!initialized || nameInput) return;
        
        if (hitStop > 0) {
            hitStop -= dt;
            return;
        }
        
        gameTime += dt;
        zoro.Update(dt);
        aizen.Update(dt, zoro);
        
        CheckCollisions();
        CheckWinCondition();
        
        // Name input (if high score)
        if (IsKeyPressed(KEY_ENTER)) {
            if (zoro.healthBar.hp <= 0 || aizen.healthBar.hp <= 0) {
                if (scoreManager.IsHighScore(zoro.score)) {
                    nameInput = true;
                }
            }
        }
    }
    
    void CheckCollisions() {
        // Zoro attacks Aizen
        if (zoro.IsAttacking() && aizen.IsVulnerable()) {
            Rectangle zoroAttack = zoro.GetAttackBounds();
            Rectangle aizenBounds = aizen.GetBounds();
            
            if (CheckCollisionRecs(zoroAttack, aizenBounds)) {
                int damage = zoro.damage;
                if (zoro.state == ONI_GIRI) damage *= 2;
                if (zoro.state == TATSUMAKI) damage *= 3;
                if (zoro.state == HIRYU_KAEN) damage *= 5;
                if (zoro.state == SANZEN_SEKAI) damage *= 4;
                
                aizen.TakeDamage(damage);
                zoro.AddScore(damage * 10);
                hitStop = 0.05f;
            }
        }
        
        // Aizen attacks Zoro
        if (aizen.IsVulnerable()) {
            Rectangle hadoBounds = aizen.GetHadoBounds();
            Rectangle spiritBounds = aizen.GetSpiritSlashBounds();
            Rectangle suigetsuBounds = aizen.GetSuigetsuBounds();
            Rectangle zoroBounds = {zoro.position.x - 50, zoro.position.y, 100, 160};
            
            if (CheckCollisionRecs(hadoBounds, zoroBounds)) {
                zoro.TakeDamage(aizen.hado.damage);
            }
            if (CheckCollisionRecs(spiritBounds, zoroBounds)) {
                zoro.TakeDamage(aizen.spiritSlash.damage);
            }
            if (CheckCollisionRecs(suigetsuBounds, zoroBounds)) {
                zoro.TakeDamage(aizen.suigetsuKyoka.damage);
            }
        }
    }
    
    void CheckWinCondition() {
        if (zoro.healthBar.hp <= 0 || aizen.healthBar.hp <= 0) {
            if (scoreManager.IsHighScore(zoro.score) && !nameInput) {
                nameInput = true;
            }
        }
    }
    
    void Draw() {
        if (!initialized) return;
        
        // Background
        DrawTexturePro(bg, {0, 0, (float)bg.width, (float)bg.height},
                      {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT},
                      {0, 0}, 0, WHITE);
        
        // Ground
        DrawTexturePro(ground, {0, 0, (float)ground.width, (float)ground.height},
                      {0, 430, (float)SCREEN_WIDTH, 50},
                      {0, 0}, 0, WHITE);
        
        // Characters
        aizen.Draw();
        zoro.Draw();
        
        // UI
        DrawText(TextFormat("SCORE: %d", zoro.score), SCREEN_WIDTH/2 - 60, 20, 25, YELLOW);
        DrawText(TextFormat("TIME: %.1f", gameTime), SCREEN_WIDTH/2 - 60, 50, 20, WHITE);
        
        // Combo avec couleur qui change
        int greenValue = 255 - zoro.comboCount * 10;
        if (greenValue < 0) greenValue = 0;
        DrawText(TextFormat("COMBO: x%d", zoro.comboCount), SCREEN_WIDTH/2 - 60, 80, 25, 
                Color{255, (unsigned char)greenValue, 0, 255});
        
        // Controls help
        DrawText("CONTROLS: ARROWS/MOVE | SPACE/JUMP | J/ATTACK | K/ONI-GIRI", 20, SCREEN_HEIGHT - 100, 18, GRAY);
        DrawText("L/TATSUMAKI | O/SANZEN-SEKAI | U+I/HIRYU-KAEN | A/BLOCK", 20, SCREEN_HEIGHT - 75, 18, GRAY);
        DrawText("ESC/MENU | ENTER/SUBMIT SCORE", 20, SCREEN_HEIGHT - 50, 18, GRAY);
        
        // Name input dialog
        if (nameInput) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 200});
            DrawRectangle(SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 100, 400, 200, {30, 30, 40, 255});
            DrawRectangleLines(SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 100, 400, 200, GOLD);
            
            DrawText("NEW HIGH SCORE!", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 80, 25, GOLD);
            DrawText(TextFormat("SCORE: %d", zoro.score), SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 50, 22, WHITE);
            DrawText("ENTER YOUR NAME:", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 - 10, 20, GRAY);
            
            // Draw name box
            DrawRectangle(SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 + 20, 300, 40, {50, 50, 60, 255});
            DrawRectangleLines(SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 + 20, 300, 40, WHITE);
            DrawText(playerName, SCREEN_WIDTH/2 - 140, SCREEN_HEIGHT/2 + 30, 25, WHITE);
            
            // Draw cursor
            if (((int)(GetTime() * 2) % 2) == 0) {
                int textWidth = MeasureText(playerName, 25);
                DrawText("_", SCREEN_WIDTH/2 - 140 + textWidth, SCREEN_HEIGHT/2 + 30, 25, WHITE);
            }
            
            DrawText("PRESS ENTER TO SAVE", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 + 80, 18, GREEN);
        }
    }
    
    void Unload() {
        if (bg.id > 0) UnloadTexture(bg);
        if (ground.id > 0) UnloadTexture(ground);
        zoro.Unload();
        aizen.Unload();
        initialized = false;
    }
};

// ===============================
// MENU SYSTEM
// ===============================
struct MenuSystem {
    struct MenuItem {
        const char* text;
        int x, y;
        Color color;
        bool selected;
    };
    
    std::vector<MenuItem> items;
    int selectedIndex = 0;
    float pulse = 0.0f;
    
    void AddItem(const char* text, int x, int y) {
        items.push_back({text, x, y, WHITE, false});
    }
    
    void Update() {
        pulse += GetFrameTime() * 2.0f;
        
        if (IsKeyPressed(KEY_DOWN)) {
            selectedIndex = (selectedIndex + 1) % items.size();
        }
        if (IsKeyPressed(KEY_UP)) {
            selectedIndex = (selectedIndex - 1 + items.size()) % items.size();
        }
        
        for (int i = 0; i < items.size(); i++) {
            items[i].selected = (i == selectedIndex);
            float pulseValue = sinf(pulse) * 0.3f + 0.7f;
            items[i].color = items[i].selected ? 
                Color{255, (unsigned char)(100 * pulseValue), 100, 255} : WHITE;
        }
    }
    
    void Draw() {
        for (const auto& item : items) {
            int textWidth = MeasureText(item.text, 30);
            int drawX = item.x - textWidth/2;
            
            if (item.selected) {
                DrawText(">", drawX - 40, item.y, 30, item.color);
                DrawText("<", item.x + textWidth/2 + 10, item.y, 30, item.color);
                DrawRectangle(item.x - textWidth/2 - 20, item.y - 5, textWidth + 40, 40, {255, 0, 0, 30});
            }
            
            DrawText(item.text, drawX, item.y, 30, item.color);
        }
    }
};

// ===============================
// MAIN GAME CLASS
// ===============================
class Game {
private:
    GameState currentState = GameState::INTRO;
    GameState nextState = GameState::INTRO;
    float transitionAlpha = 0.0f;
    bool transitioning = false;
    
    // Scenes
    GameScene gameScene;
    MenuSystem mainMenu;
    MenuSystem optionsMenu;
    ScoreManager scoreManager;
    GameConfig config;
    
    // Textures
    Texture2D titleTexture;
    Texture2D backgroundTexture;
    
    // Audio
    Music backgroundMusic;
    Sound swordSound;
    Sound hitSound;
    Sound specialSound;
    
    // Variables
    float introTimer = 0.0f;
    int selectedDifficulty = 1;
    
public:
    void Init() {
        // Load textures
        titleTexture = LoadTexture("textures/title.png");
        backgroundTexture = LoadTexture("textures/menu_bg.png");
        
        // Setup main menu
        mainMenu.AddItem("START GAME", SCREEN_WIDTH/2, 250);
        mainMenu.AddItem("HIGH SCORES", SCREEN_WIDTH/2, 310);
        mainMenu.AddItem("OPTIONS", SCREEN_WIDTH/2, 370);
        mainMenu.AddItem("CREDITS", SCREEN_WIDTH/2, 430);
        mainMenu.AddItem("HELP", SCREEN_WIDTH/2, 490);
        mainMenu.AddItem("EXIT", SCREEN_WIDTH/2, 550);
        
        // Setup options
        optionsMenu.AddItem("MUSIC VOLUME", SCREEN_WIDTH/2, 250);
        optionsMenu.AddItem("SFX VOLUME", SCREEN_WIDTH/2, 310);
        optionsMenu.AddItem("DIFFICULTY", SCREEN_WIDTH/2, 370);
        optionsMenu.AddItem("FULLSCREEN", SCREEN_WIDTH/2, 430);
        optionsMenu.AddItem("BACK", SCREEN_WIDTH/2, 490);
        
        // Load scores
        scoreManager.LoadScores();
        
        // Load config
        LoadConfig();
        
        // Initialize audio
        InitAudioDevice();
        backgroundMusic = LoadMusicStream("audio/music.mp3");
        swordSound = LoadSound("audio/sword.wav");
        hitSound = LoadSound("audio/hit.wav");
        specialSound = LoadSound("audio/special.wav");
        
        PlayMusicStream(backgroundMusic);
        SetMusicVolume(backgroundMusic, config.musicVolume);
        SetSoundVolume(swordSound, config.sfxVolume);
        SetSoundVolume(hitSound, config.sfxVolume);
        SetSoundVolume(specialSound, config.sfxVolume);
    }
    
    void LoadConfig() {
        std::ifstream file("config.txt");
        if (file) {
            file >> config.musicVolume >> config.sfxVolume 
                 >> config.gameSpeed >> config.fullscreen 
                 >> config.vsync >> config.difficulty;
            file.close();
        }
        
        if (config.fullscreen) {
            ToggleFullscreen();
        }
        SetWindowState(config.vsync ? FLAG_VSYNC_HINT : 0);
    }
    
    void SaveConfig() {
        std::ofstream file("config.txt");
        if (file) {
            file << config.musicVolume << " " << config.sfxVolume << " "
                 << config.gameSpeed << " " << config.fullscreen << " "
                 << config.vsync << " " << config.difficulty;
            file.close();
        }
    }
    
    void Update(float dt) {
        UpdateMusicStream(backgroundMusic);
        
        if (transitioning) {
            transitionAlpha += dt * 2.0f;
            if (transitionAlpha >= 1.0f) {
                transitionAlpha = 0.0f;
                transitioning = false;
                currentState = nextState;
            }
            return;
        }
        
        switch (currentState) {
            case GameState::INTRO:
                introTimer += dt;
                if (introTimer > 3.0f || IsKeyPressed(KEY_ENTER)) {
                    StartTransition(GameState::MENU);
                }
                break;
                
            case GameState::MENU:
                mainMenu.Update();
                if (IsKeyPressed(KEY_ENTER)) {
                    switch (mainMenu.selectedIndex) {
                        case 0: StartTransition(GameState::GAME); break;
                        case 1: currentState = GameState::HIGHSCORES; break;
                        case 2: currentState = GameState::OPTIONS; break;
                        case 3: currentState = GameState::CREDITS; break;
                        case 4: currentState = GameState::HELP; break;
                        case 5: currentState = GameState::EXIT; break;
                    }
                }
                break;
                
            case GameState::GAME:
                gameScene.Update(dt * config.gameSpeed);
                if (IsKeyPressed(KEY_ESCAPE)) {
                    gameScene.Unload();
                    StartTransition(GameState::MENU);
                }
                break;
                
            case GameState::OPTIONS:
                optionsMenu.Update();
                if (IsKeyPressed(KEY_ENTER)) {
                    if (optionsMenu.selectedIndex == 4) {
                        currentState = GameState::MENU;
                    } else if (optionsMenu.selectedIndex == 3) {
                        config.fullscreen = !config.fullscreen;
                        ToggleFullscreen();
                        SaveConfig();
                    }
                }
                // Handle volume adjustment
                if (optionsMenu.selectedIndex == 0) {
                    if (IsKeyPressed(KEY_LEFT)) config.musicVolume = std::max(0.0f, config.musicVolume - 0.1f);
                    if (IsKeyPressed(KEY_RIGHT)) config.musicVolume = std::min(1.0f, config.musicVolume + 0.1f);
                    SetMusicVolume(backgroundMusic, config.musicVolume);
                }
                if (optionsMenu.selectedIndex == 1) {
                    if (IsKeyPressed(KEY_LEFT)) config.sfxVolume = std::max(0.0f, config.sfxVolume - 0.1f);
                    if (IsKeyPressed(KEY_RIGHT)) config.sfxVolume = std::min(1.0f, config.sfxVolume + 0.1f);
                    SetSoundVolume(swordSound, config.sfxVolume);
                    SetSoundVolume(hitSound, config.sfxVolume);
                    SetSoundVolume(specialSound, config.sfxVolume);
                }
                if (optionsMenu.selectedIndex == 2) {
                    if (IsKeyPressed(KEY_LEFT)) selectedDifficulty = std::max(0, selectedDifficulty - 1);
                    if (IsKeyPressed(KEY_RIGHT)) selectedDifficulty = std::min(2, selectedDifficulty + 1);
                    config.difficulty = selectedDifficulty;
                }
                if (IsKeyPressed(KEY_ESCAPE)) {
                    SaveConfig();
                    currentState = GameState::MENU;
                }
                break;
                
            case GameState::HIGHSCORES:
                if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
                    currentState = GameState::MENU;
                }
                break;
                
            case GameState::CREDITS:
                if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
                    currentState = GameState::MENU;
                }
                break;
                
            case GameState::HELP:
                if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
                    currentState = GameState::MENU;
                }
                break;
        }
    }
    
    void Draw() {
        BeginDrawing();
        ClearBackground(BLACK);
        
        switch (currentState) {
            case GameState::INTRO:
                DrawIntro();
                break;
                
            case GameState::MENU:
                DrawMenu();
                break;
                
            case GameState::GAME:
                gameScene.Draw();
                break;
                
            case GameState::OPTIONS:
                DrawOptions();
                break;
                
            case GameState::HIGHSCORES:
                DrawHighScores();
                break;
                
            case GameState::CREDITS:
                DrawCredits();
                break;
                
            case GameState::HELP:
                DrawHelp();
                break;
        }
        
        // Draw transition
        if (transitioning) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 
                         {0, 0, 0, (unsigned char)(transitionAlpha * 255)});
        }
        
        // FPS counter
        DrawFPS(10, 10);
        EndDrawing();
    }
    
    void DrawIntro() {
        float alpha = std::min(1.0f, introTimer);
        DrawTexturePro(backgroundTexture, {0, 0, (float)backgroundTexture.width, (float)backgroundTexture.height},
                      {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT}, {0, 0}, 0, 
                      {255, 255, 255, (unsigned char)(alpha * 255)});
        
        DrawText("HAMDI STUDIOS PRESENTS", SCREEN_WIDTH/2 - 180, SCREEN_HEIGHT/2 - 50, 30,
                {255, 255, 255, (unsigned char)(alpha * 255)});
        DrawText("ZORO: LOSTMULTIVERSE", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2, 40,
                {255, 50, 50, (unsigned char)(alpha * 255)});
        DrawText("Press ENTER to continue...", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT - 100, 20,
                {255, 255, 255, (unsigned char)((sinf(GetTime() * 3) * 0.5f + 0.5f) * alpha * 255)});
    }
    
    void DrawMenu() {
        DrawTexturePro(backgroundTexture, {0, 0, (float)backgroundTexture.width, (float)backgroundTexture.height},
                      {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT}, {0, 0}, 0, WHITE);
        
        // Title
        DrawTexture(titleTexture, SCREEN_WIDTH/2 - titleTexture.width/2, 50, WHITE);
        
        // Menu items
        mainMenu.Draw();
        
        // Version info
        DrawText("v1.0 - © 2025 HAMDI Studios", SCREEN_WIDTH - 250, SCREEN_HEIGHT - 30, 15, GRAY);
    }
    
    void DrawOptions() {
        DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 
                              {20, 20, 40, 255}, {10, 10, 20, 255});
        
        DrawText("OPTIONS", SCREEN_WIDTH/2 - 80, 100, 50, BLUE);
        
        optionsMenu.Draw();
        
        // Draw current values
        for (int i = 0; i < (int)optionsMenu.items.size(); i++) {
            int y = optionsMenu.items[i].y;
            switch (i) {
                case 0:
                    DrawText(TextFormat("%.1f", config.musicVolume), SCREEN_WIDTH/2 + 150, y, 25, WHITE);
                    break;
                case 1:
                    DrawText(TextFormat("%.1f", config.sfxVolume), SCREEN_WIDTH/2 + 150, y, 25, WHITE);
                    break;
                case 2: {
                    const char* diff[] = {"EASY", "NORMAL", "HARD"};
                    DrawText(diff[selectedDifficulty], SCREEN_WIDTH/2 + 150, y, 25, WHITE);
                    break;
                }
                case 3:
                    DrawText(config.fullscreen ? "ON" : "OFF", SCREEN_WIDTH/2 + 150, y, 25, WHITE);
                    break;
            }
        }
        
        DrawText("Use LEFT/RIGHT to adjust values", SCREEN_WIDTH/2 - 180, SCREEN_HEIGHT - 100, 20, GRAY);
        DrawText("Press ESC to save and return", SCREEN_WIDTH/2 - 160, SCREEN_HEIGHT - 70, 20, GRAY);
    }
    
    void DrawHighScores() {
        DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 
                              {30, 20, 40, 255}, {15, 10, 20, 255});
        
        DrawText("HIGH SCORES", SCREEN_WIDTH/2 - 120, 50, 50, GOLD);
        
        // Draw score table
        DrawRectangle(SCREEN_WIDTH/2 - 300, 120, 600, 400, {0, 0, 0, 150});
        DrawRectangleLines(SCREEN_WIDTH/2 - 300, 120, 600, 400, GOLD);
        
        // Headers
        DrawText("RANK", SCREEN_WIDTH/2 - 280, 140, 25, YELLOW);
        DrawText("NAME", SCREEN_WIDTH/2 - 180, 140, 25, YELLOW);
        DrawText("SCORE", SCREEN_WIDTH/2, 140, 25, YELLOW);
        DrawText("COMBO", SCREEN_WIDTH/2 + 150, 140, 25, YELLOW);
        DrawText("TIME", SCREEN_WIDTH/2 + 250, 140, 25, YELLOW);
        
        // Scores
        int y = 180;
        for (int i = 0; i < std::min(10, (int)scoreManager.scores.size()); i++) {
            const auto& entry = scoreManager.scores[i];
            
            // Highlight current player
            Color color = (i % 2 == 0) ? WHITE : LIGHTGRAY;
            
            DrawText(TextFormat("%d.", i + 1), SCREEN_WIDTH/2 - 280, y, 22, color);
            DrawText(entry.name, SCREEN_WIDTH/2 - 180, y, 22, color);
            DrawText(TextFormat("%d", entry.score), SCREEN_WIDTH/2, y, 22, color);
            DrawText(TextFormat("%d", entry.combo), SCREEN_WIDTH/2 + 150, y, 22, color);
            DrawText(TextFormat("%.1fs", entry.time), SCREEN_WIDTH/2 + 250, y, 22, color);
            
            y += 35;
        }
        
        if (scoreManager.scores.empty()) {
            DrawText("NO SCORES YET!", SCREEN_WIDTH/2 - 100, 200, 30, GRAY);
        }
        
        DrawText("Press ESC to return", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT - 50, 22, GRAY);
    }
    
    void DrawCredits() {
        DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 
                              {20, 30, 40, 255}, {10, 15, 20, 255});
        
        DrawText("CREDITS", SCREEN_WIDTH/2 - 80, 50, 50, SKYBLUE);
        
        // Read credits from file
        std::ifstream file("credits.txt");
        if (file) {
            std::string line;
            int y = 120;
            while (std::getline(file, line)) {
                if (line[0] == '#') {
                    // Section header
                    DrawText(line.substr(1).c_str(), SCREEN_WIDTH/2 - 200, y, 30, YELLOW);
                    y += 40;
                } else if (!line.empty()) {
                    DrawText(line.c_str(), SCREEN_WIDTH/2 - 200, y, 22, WHITE);
                    y += 30;
                }
            }
            file.close();
        } else {
            // Default credits
            const char* credits[] = {
                "# DEVELOPMENT TEAM",
                "Project Lead: HAMDI",
                "Programming: HAMDI",
                "Art & Design: HAMDI",
                "Animation: HAMDI",
                "Sound Design: HAMDI",
                "",
                "# SPECIAL THANKS",
                "Beta Testers",
                "Community Support",
                "Open Source Contributors",
                "",
                "# TECHNOLOGIES",
                "Raylib 5.0",
                "C++23",
                "Aseprite"
            };
            
            int y = 120;
            for (int i = 0; i < 17; i++) {
                if (strlen(credits[i]) > 0 && credits[i][0] == '#') {
                    DrawText(credits[i] + 1, SCREEN_WIDTH/2 - 200, y, 30, YELLOW);
                    y += 40;
                } else {
                    DrawText(credits[i], SCREEN_WIDTH/2 - 200, y, 22, WHITE);
                    y += 30;
                }
            }
        }
        
        DrawText("Press ESC to return", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT - 50, 22, GRAY);
    }
    
    void DrawHelp() {
        DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 
                              {40, 30, 20, 255}, {20, 15, 10, 255});
        
        DrawText("HELP & CONTROLS", SCREEN_WIDTH/2 - 150, 50, 50, ORANGE);
        
        // Read help from file
        std::ifstream file("help.txt");
        if (file) {
            std::string line;
            int y = 120;
            while (std::getline(file, line)) {
                DrawText(line.c_str(), 100, y, 22, WHITE);
                y += 30;
            }
            file.close();
        } else {
            // Default help
            const char* help[] = {
                "=== ZORO CONTROLS ===",
                "ARROW KEYS: Move left/right",
                "SPACE: Jump",
                "J: Basic Attack",
                "K: Oni-Giri (Powerful Slash)",
                "L: Tatsumaki (Tornado Attack)",
                "O: Sanzen Sekai (Triple Attack)",
                "U + I: Hiryu Kaen (Dragon Finisher)",
                "A: Block (Reduce Damage)",
                "",
                "=== GAME MECHANICS ===",
                "- Chain attacks for combos",
                "- Block at the right moment",
                "- Special attacks have cooldowns",
                "- Higher combos = More points",
                "",
                "=== AIZEN AI ===",
                "- Uses Hado spells",
                "- Can perform Spirit Slash",
                "- Activates Suigetsu Kyoka",
                "- Difficulty affects aggression"
            };
            
            int y = 120;
            for (int i = 0; i < 22; i++) {
                DrawText(help[i], 100, y, 22, WHITE);
                y += 30;
            }
        }
        
        DrawText("Press ESC to return", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT - 50, 22, GRAY);
    }
    
    void StartTransition(GameState newState) {
        if (newState == GameState::GAME) {
            gameScene.Init(selectedDifficulty);
        }
        transitioning = true;
        nextState = newState;
        transitionAlpha = 0.0f;
    }
    
    void Unload() {
        UnloadTexture(titleTexture);
        UnloadTexture(backgroundTexture);
        gameScene.Unload();
        
        UnloadMusicStream(backgroundMusic);
        UnloadSound(swordSound);
        UnloadSound(hitSound);
        UnloadSound(specialSound);
        
        CloseAudioDevice();
        SaveConfig();
    }
    
    bool ShouldClose() const {
        return currentState == GameState::EXIT;
    }
};

// ===============================
// MAIN FUNCTION
// ===============================
int main() {
    // Initialize window
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "ZORO: MULTIVERSE WARRIOR");
    SetTargetFPS(144);
    
    // Initialize game
    Game game;
    game.Init();
    
    // Main game loop
    while (!WindowShouldClose() && !game.ShouldClose()) {
        float dt = GetFrameTime();
        
        game.Update(dt);
        game.Draw();
    }
    
    // Cleanup
    game.Unload();
    CloseWindow();
    
    return 0;
}