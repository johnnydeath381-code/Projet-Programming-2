#include "raylib.h"
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <cstring>
struct AnimData
{
    Rectangle rec;
    Vector2 pos;
    int frame;
    float updateTime;
    float runningTime;
};

struct HighScore
{
    int score;
    char date[20];
};

bool isOnGround(AnimData data, int windowHeight)
{
   return data.pos.y >= (windowHeight - 80) - data.rec.height;
}

AnimData updateAnimData(AnimData data, float dT, int maxFrame)
{
    data.runningTime += dT;
    if (data.runningTime >= data.updateTime)
    {
        data.runningTime = 0.0;
        data.rec.x = data.frame * data.rec.width;
        data.frame++;
        if (data.frame > maxFrame)
        {
            data.frame = 0;
        }
    }
    return data;
}

// Fonction pour sauvegarder les scores
void SaveHighScores(const std::vector<HighScore>& scores)
{
    FILE* file = fopen("highscores.dat", "wb");
    if (file)
    {
        int count = scores.size();
        fwrite(&count, sizeof(int), 1, file);
        for (const auto& score : scores)
        {
            fwrite(&score, sizeof(HighScore), 1, file);
        }
        fclose(file);
    }
}

// Fonction pour charger les scores
std::vector<HighScore> LoadHighScores()
{
    std::vector<HighScore> scores;
    FILE* file = fopen("highscores.dat", "rb");
    if (file)
    {
        int count;
        fread(&count, sizeof(int), 1, file);
        for (int i = 0; i < count; i++)
        {
            HighScore score;
            fread(&score, sizeof(HighScore), 1, file);
            scores.push_back(score);
        }
        fclose(file);
    }
    return scores;
}

// Fonction pour ajouter un nouveau score
void AddHighScore(std::vector<HighScore>& scores, int newScore)
{
    // Obtenir la date actuelle
    time_t now = time(0);
    struct tm* timeinfo = localtime(&now);
    char dateStr[20];
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", timeinfo);
    
    HighScore newEntry;
    newEntry.score = newScore;
    strcpy(newEntry.date, dateStr);
    
    scores.push_back(newEntry);
    
    // Trier par score (décroissant)
    std::sort(scores.begin(), scores.end(), 
              [](const HighScore& a, const HighScore& b) { return a.score > b.score; });
    
    // Garder seulement les 5 meilleurs
    if (scores.size() > 5) scores.resize(5);
    
    SaveHighScores(scores);
}

int main()
{
    // Dimensions de la fenêtre
    const int windowWidth = 1280;
    const int windowHeight = 720;

    InitWindow(windowWidth, windowHeight, "ZORO RUNNER");
       
    // Sons
    InitAudioDevice();
    Sound jumpSound = LoadSound("Sounds/jump.wav");
    Sound collectSound = LoadSound("Sounds/pickupCoin.wav");
    Sound hitSound = LoadSound("Sounds/hitHurt.wav");
    Sound slashSound = LoadSound("Sounds/slash.wav"); // Son du slash
    Music bgMusic = LoadMusicStream("Sounds/Pixel Kings.wav");

    // Background - un seul pour chaque écran
    Texture2D bgIntro = LoadTexture("textures/intro_bg.jpg"); // Background pour l'intro
    Texture2D bgGame = LoadTexture("textures/game_bg.jpg");   // Background pour le jeu
    
    // Zoro textures
    Texture2D zoroWalk = LoadTexture("textures/zoro_assets/Walk.png");
    Texture2D zoroJump = LoadTexture("textures/zoro_assets/Jump.png");
    Texture2D zoroIdle = LoadTexture("textures/zoro_assets/IDLE.png");
    Texture2D zoroVictory = LoadTexture("textures/zoro_assets/Victory.png");
    Texture2D zoroSlash = LoadTexture("textures/zoro_assets/Slash.png"); // Animation de slash
    
    // Obstacles - style japonais
    Texture2D kunai = LoadTexture("textures/kunai.png");        // Projectile bas
    Texture2D shuriken = LoadTexture("textures/shuriken.png");  // Projectile haut
    
    // Items
    Texture2D sake = LoadTexture("textures/sake.png");          // Remplace le melon
    Texture2D coin = LoadTexture("textures/coin.png");          // Pièces bonus
    
    // Données d'animation pour Zoro
    AnimData zoroData;
    zoroData.rec.width = zoroWalk.width / 8.0f;
    zoroData.rec.height = zoroWalk.height;
    zoroData.rec.x = 0;
    zoroData.rec.y = 0;
    zoroData.pos.x = windowWidth / 3 - zoroData.rec.width / 2;
    zoroData.pos.y = (windowHeight - 80) - zoroData.rec.height;
    zoroData.frame = 0;
    zoroData.runningTime = 0;
    zoroData.updateTime = 0.1f;
    
    // Données pour l'attaque slash
    AnimData slashData;
    slashData.rec.width = zoroSlash.width / 6.0f; // 6 frames pour le slash
    slashData.rec.height = zoroSlash.height;
    slashData.rec.x = 0;
    slashData.rec.y = 0;
    slashData.pos.x = zoroData.pos.x + zoroData.rec.width - 50;
    slashData.pos.y = zoroData.pos.y;
    slashData.frame = 0;
    slashData.runningTime = 0;
    slashData.updateTime = 0.05f; // Animation rapide
    bool isAttacking = false;
    float attackCooldown = 0.0f;
    const float attackCooldownTime = 0.3f; // Cooldown de 0.3 secondes
    
    // Obstacles
    const int NumOfKunais = 6;
    const int NumOfShurikens = 4;
    AnimData kunais[NumOfKunais];
    AnimData shurikens[NumOfShurikens];
    
    // Initialisation des obstacles
    int kunaiDist = 100;
    for (int i = 0; i < NumOfKunais; i++)
    {
        kunais[i].rec.width = kunai.width / 1.0f;
        kunais[i].rec.height = kunai.height;
        kunais[i].pos.x = windowWidth + kunaiDist;
        kunais[i].pos.y = (windowHeight - 100) - kunai.height;  // Bas
        kunais[i].frame = 0;
        kunais[i].runningTime = 0;
        kunais[i].updateTime = 0.2f;
        kunaiDist += 10000;
    }
    
    int shurikenDist = 300;
    for (int i = 0; i < NumOfShurikens; i++)
    {
        shurikens[i].rec.width = shuriken.width / 4.0f;
        shurikens[i].rec.height = shuriken.height;
        shurikens[i].pos.x = windowWidth + shurikenDist;
        shurikens[i].pos.y = (windowHeight - 300) - shuriken.height;  // Haut
        shurikens[i].frame = 0;
        shurikens[i].runningTime = 0;
        shurikens[i].updateTime = 0.15f;
        shurikenDist += 15000;
    }
    
    // Items
    const int NumOfItems = 2;
    AnimData items[NumOfItems];
    int itemDist = 1000;
    for (int i = 0; i < NumOfItems; i++)
    {
        items[i].rec.width = sake.width;
        items[i].rec.height = sake.height;
        items[i].pos.x = windowWidth + itemDist;
        items[i].pos.y = (windowHeight - 200) - sake.height;
        items[i].frame = 0;
        items[i].runningTime = 0;
        items[i].updateTime = 0.2f;
        itemDist += 15000;
    }
    
    // Variables du jeu
    bool isJumping = false;
    int zoroVelocity = 0;
    const int jumpForce = 800;
    const int gravity = 2000;
    float gameSpeed = 300.0f;
    int score = 0;
    float scoreTimer = 0.0f;
    bool collision = false;
    
    // États du jeu
    enum GameState { STATE_INTRO, STATE_MENU, STATE_PLAYING, STATE_GAME_OVER, STATE_HIGHSCORES };
    GameState currentState = STATE_INTRO;
    bool musicOn = true;
    float introTimer = 0.0f;
    float introWalkTimer = 0.0f;
    
    // Variables pour le menu
    int menuSelection = 0;
    std::vector<HighScore> highScores = LoadHighScores();
    
    // Animation d'intro de Zoro
    AnimData introZoroData;
    introZoroData.rec.width = zoroWalk.width / 8.0f;
    introZoroData.rec.height = zoroWalk.height;
    introZoroData.pos.x = windowWidth / 2 - introZoroData.rec.width / 2;
    introZoroData.pos.y = windowHeight / 2;
    introZoroData.frame = 0;
    introZoroData.runningTime = 0;
    introZoroData.updateTime = 0.15f;
    
    SetTargetFPS(60);
    
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        
        switch (currentState)
        {
            case STATE_INTRO:
            {
                introTimer += dt;
                introWalkTimer += dt;
                
                // Animation de Zoro qui marche sur place
                if (introWalkTimer >= introZoroData.updateTime)
                {
                    introWalkTimer = 0;
                    introZoroData.frame = (introZoroData.frame + 1) % 8;
                }
                
                // Après 4 secondes ou si on appuie sur ENTER, aller au menu
                if (introTimer > 4.0f || IsKeyPressed(KEY_ENTER))
                {
                    currentState = STATE_MENU;
                    if (musicOn) PlayMusicStream(bgMusic);
                }
                break;
            }
            
            case STATE_MENU:
            {
                UpdateMusicStream(bgMusic);
                
                // Navigation du menu
                if (IsKeyPressed(KEY_DOWN)) menuSelection = (menuSelection + 1) % 4;
                if (IsKeyPressed(KEY_UP)) menuSelection = (menuSelection + 3) % 4;
                
                if (IsKeyPressed(KEY_ENTER))
                {
                    switch (menuSelection)
                    {
                        case 0: // Jouer
                            score = 0;
                            gameSpeed = 300.0f;
                            collision = false;
                            zoroData.pos.y = (windowHeight - 80) - zoroData.rec.height;
                            isJumping = false;
                            zoroVelocity = 0;
                            isAttacking = false;
                            attackCooldown = 0.0f;
                            
                            // Réinitialiser les obstacles
                            kunaiDist = 100;
                            for (int i = 0; i < NumOfKunais; i++)
                            {
                                kunais[i].pos.x = windowWidth + kunaiDist;
                                kunais[i].frame = 0;
                                kunaiDist += 10000;
                            }
                            
                            shurikenDist = 300;
                            for (int i = 0; i < NumOfShurikens; i++)
                            {
                                shurikens[i].pos.x = windowWidth + shurikenDist;
                                shurikens[i].frame = 0;
                                shurikenDist += 15000;
                            }
                            
                            itemDist = 1000;
                            for (int i = 0; i < NumOfItems; i++)
                            {
                                items[i].pos.x = windowWidth + itemDist;
                                itemDist += 15000;
                            }
                            
                            currentState = STATE_PLAYING;
                            break;
                            
                        case 1: // Meilleurs scores
                            currentState = STATE_HIGHSCORES;
                            break;
                            
                        case 2: // Musique ON/OFF
                            musicOn = !musicOn;
                            if (musicOn) PlayMusicStream(bgMusic);
                            else PauseMusicStream(bgMusic);
                            break;
                            
                        case 3: // Quitter
                            CloseWindow();
                            return 0;
                    }
                }
                break;
            }
            
            case STATE_PLAYING:
            {
                if (!collision)
                {
                    UpdateMusicStream(bgMusic);
                    
                    // Gestion du cooldown de l'attaque
                    if (attackCooldown > 0)
                    {
                        attackCooldown -= dt;
                    }
                    
                    // Contrôles
                    if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W)) && !isJumping)
                    {
                        zoroVelocity = -jumpForce;
                        isJumping = true;
                        PlaySound(jumpSound);
                    }
                    
                    // Attaque avec la touche A ou clic gauche
                    if ((IsKeyPressed(KEY_A) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) && !isAttacking && attackCooldown <= 0)
                    {
                        isAttacking = true;
                        slashData.frame = 0;
                        slashData.runningTime = 0;
                        slashData.pos.x = zoroData.pos.x + zoroData.rec.width - 50;
                        slashData.pos.y = zoroData.pos.y;
                        PlaySound(slashSound);
                    }
                    
                    // Mise à jour de l'animation d'attaque
                    if (isAttacking)
                    {
                        slashData = updateAnimData(slashData, dt, 5); // 6 frames (0-5)
                        
                        // Fin de l'animation d'attaque
                        if (slashData.frame == 0 && slashData.runningTime == 0)
                        {
                            isAttacking = false;
                            attackCooldown = attackCooldownTime;
                        }
                    }
                    
                    // Physique du saut
                    if (isJumping)
                    {
                        zoroVelocity += gravity * dt;
                        zoroData.pos.y += zoroVelocity * dt;
                        
                        if (zoroData.pos.y >= (windowHeight - 80) - zoroData.rec.height)
                        {
                            zoroData.pos.y = (windowHeight - 80) - zoroData.rec.height;
                            isJumping = false;
                            zoroVelocity = 0;
                        }
                    }
                    
                    // Mise à jour du score
                    scoreTimer += dt;
                    if (scoreTimer >= 1.0f)
                    {
                        scoreTimer = 0.0f;
                        score += 10;
                        
                        // Augmenter la difficulté
                        if (gameSpeed < 700.0f)
                        {
                            gameSpeed += 5.0f;
                        }
                    }
                    
                    // Animation de Zoro
                    if (isJumping)
                    {
                        // Utiliser l'animation de saut
                        zoroData = updateAnimData(zoroData, dt, 6);
                    }
                    else if (isAttacking)
                    {
                        // Animation d'attaque (frame fixe ou spéciale)
                        zoroData.frame = 0; // Frame d'attente
                    }
                    else
                    {
                        // Animation de marche
                        zoroData = updateAnimData(zoroData, dt, 7);
                    }
                    
                    // Mise à jour des obstacles
                    for (int i = 0; i < NumOfKunais; i++)
                    {
                        kunais[i] = updateAnimData(kunais[i], dt, 3);
                        kunais[i].pos.x -= gameSpeed * dt;
                        
                        if (kunais[i].pos.x <= -200)
                        {
                            kunais[i].pos.x = windowWidth + GetRandomValue(1000, 5000);
                        }
                        
                        // Collision avec les kunais (bas)
                        Rectangle kunaiRect = { kunais[i].pos.x + 10, kunais[i].pos.y + 10, 
                                               kunais[i].rec.width - 20, kunais[i].rec.height - 20 };
                        Rectangle zoroRect = { zoroData.pos.x + 20, zoroData.pos.y + 20, 
                                              zoroData.rec.width - 40, zoroData.rec.height - 40 };
                        
                        // Hitbox du slash pour détruire les projectiles
                        Rectangle slashRect = { slashData.pos.x, slashData.pos.y + 20,
                                              slashData.rec.width - 20, slashData.rec.height - 40 };
                        
                        // Vérifier si le kunai est détruit par le slash
                        if (isAttacking && CheckCollisionRecs(slashRect, kunaiRect))
                        {
                            PlaySound(collectSound); // Son de destruction
                            score += 50; // Points bonus pour la destruction
                            kunais[i].pos.x = windowWidth + GetRandomValue(1000, 5000);
                        }
                        // Sinon, vérifier la collision avec Zoro
                        else if (CheckCollisionRecs(kunaiRect, zoroRect) && !isJumping)
                        {
                            collision = true;
                            PlaySound(hitSound);
                            StopMusicStream(bgMusic);
                        }
                    }
                    
                    for (int i = 0; i < NumOfShurikens; i++)
                    {
                        shurikens[i] = updateAnimData(shurikens[i], dt, 3);
                        shurikens[i].pos.x -= gameSpeed * dt;
                        
                        if (shurikens[i].pos.x <= -200)
                        {
                            shurikens[i].pos.x = windowWidth + GetRandomValue(1500, 6000);
                        }
                        
                        // Collision avec les shurikens (haut)
                        Rectangle shurikenRect = { shurikens[i].pos.x + 10, shurikens[i].pos.y + 10, 
                                                  shurikens[i].rec.width - 20, shurikens[i].rec.height - 20 };
                        Rectangle zoroRect = { zoroData.pos.x + 20, zoroData.pos.y + 20, 
                                              zoroData.rec.width - 40, zoroData.rec.height - 40 };
                        
                        // Hitbox du slash pour détruire les projectiles
                        Rectangle slashRect = { slashData.pos.x, slashData.pos.y + 20,
                                              slashData.rec.width - 20, slashData.rec.height - 40 };
                        
                        // Vérifier si le shuriken est détruit par le slash
                        if (isAttacking && CheckCollisionRecs(slashRect, shurikenRect))
                        {
                            PlaySound(collectSound); // Son de destruction
                            score += 50; // Points bonus pour la destruction
                            shurikens[i].pos.x = windowWidth + GetRandomValue(1500, 6000);
                        }
                        // Sinon, vérifier la collision avec Zoro
                        else if (CheckCollisionRecs(shurikenRect, zoroRect) && isJumping)
                        {
                            collision = true;
                            PlaySound(hitSound);
                            StopMusicStream(bgMusic);
                        }
                    }
                    
                    // Mise à jour des items
                    for (int i = 0; i < NumOfItems; i++)
                    {
                        items[i].pos.x -= gameSpeed * dt;
                        
                        if (items[i].pos.x <= -200)
                        {
                            items[i].pos.x = windowWidth + GetRandomValue(2000, 8000);
                        }
                        
                        // Collecter les items
                        Rectangle itemRect = { items[i].pos.x, items[i].pos.y, 
                                             items[i].rec.width, items[i].rec.height };
                        Rectangle zoroRect = { zoroData.pos.x + 20, zoroData.pos.y + 20, 
                                              zoroData.rec.width - 40, zoroData.rec.height - 40 };
                        
                        if (CheckCollisionRecs(itemRect, zoroRect))
                        {
                            PlaySound(collectSound);
                            score += 100;
                            items[i].pos.x = windowWidth + GetRandomValue(2000, 8000);
                        }
                    }
                    
                    // Retour au menu avec ECHAP
                    if (IsKeyPressed(KEY_ESCAPE))
                    {
                        currentState = STATE_MENU;
                    }
                }
                else
                {
                    // Écran Game Over
                    if (IsKeyPressed(KEY_ENTER))
                    {
                        // Ajouter le score aux high scores si c'est un bon score
                        if (highScores.size() < 5 || score > highScores.back().score)
                        {
                            AddHighScore(highScores, score);
                        }
                        
                        currentState = STATE_MENU;
                        menuSelection = 0;
                    }
                }
                break;
            }
            
            case STATE_HIGHSCORES:
            {
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
                {
                    currentState = STATE_MENU;
                }
                break;
            }
        }
        
        // Dessin
        BeginDrawing();
        ClearBackground(SKYBLUE);
        
        switch (currentState)
        {
            case STATE_INTRO:
            {
                // Background intro
                DrawTexture(bgIntro, 0, 0, WHITE);
                
                // Titre avec effet
                float pulse = sinf(GetTime() * 3.0f) * 0.5f + 0.5f;
                DrawText("ZORO RUNNER", windowWidth/2 - 250, 100, 80, 
                         Color{255, (unsigned char)(50 * pulse), 50, 255});
                
                // Zoro qui marche sur place
                Rectangle sourceRect = { introZoroData.frame * introZoroData.rec.width, 0, 
                                        introZoroData.rec.width, introZoroData.rec.height };
                DrawTextureRec(zoroWalk, sourceRect, introZoroData.pos, WHITE);
                
                // Texte d'introduction
                DrawText("Le sabreur perdu dans le temps...", windowWidth/2 - 300, 400, 40, WHITE);
                DrawText("Appuyez sur ENTRER pour commencer", windowWidth/2 - 300, 500, 30, 
                         Color{255, 255, 255, (unsigned char)(pulse * 255)});
                
                break;
            }
            
            case STATE_MENU:
            {
                // Background menu
                DrawTexture(bgIntro, 0, 0, WHITE);
                
                // Titre
                DrawText("ZORO RUNNER", windowWidth/2 - 250, 50, 80, RED);
                
                // Options du menu
                const char* menuItems[] = { "JOUER", "MEILLEURS SCORES", "MUSIQUE: ", "QUITTER" };
                
                for (int i = 0; i < 4; i++)
                {
                    Color color = (i == menuSelection) ? YELLOW : WHITE;
                    int yPos = 200 + i * 80;
                    
                    if (i == menuSelection)
                    {
                        DrawText(">", windowWidth/2 - 200, yPos, 40, color);
                        DrawText("<", windowWidth/2 + 180, yPos, 40, color);
                    }
                    
                    if (i == 2) // Option musique
                    {
                        std::string musicText = std::string(menuItems[i]) + (musicOn ? "ON" : "OFF");
                        DrawText(musicText.c_str(), windowWidth/2 - 100, yPos, 40, color);
                    }
                    else
                    {
                        DrawText(menuItems[i], windowWidth/2 - 100, yPos, 40, color);
                    }
                }
                
                // Instructions
                DrawText("Utilisez les fleches HAUT/BAS pour naviguer", windowWidth/2 - 300, 550, 25, GRAY);
                DrawText("ENTRER pour selectionner", windowWidth/2 - 200, 600, 25, GRAY);
                DrawText("ESPACE pour sauter en jeu", windowWidth/2 - 200, 630, 25, GRAY);
                
                break;
            }
            
            case STATE_PLAYING:
            {
                if (!collision)
                {
                    // Background jeu
                    DrawTexture(bgGame, 0, 0, WHITE);
                    
                    // Items
                    for (int i = 0; i < NumOfItems; i++)
                    {
                        DrawTextureRec(sake, items[i].rec, items[i].pos, WHITE);
                    }
                    
                    // Obstacles
                    for (int i = 0; i < NumOfKunais; i++)
                    {
                        DrawTextureRec(kunai, kunais[i].rec, kunais[i].pos, WHITE);
                    }
                    
                    for (int i = 0; i < NumOfShurikens; i++)
                    {
                        DrawTextureRec(shuriken, shurikens[i].rec, shurikens[i].pos, WHITE);
                    }
                    
                    // Zoro
                    if (isJumping)
                    {
                        // Animation de saut
                        Rectangle sourceRect = { zoroData.frame * zoroData.rec.width, 0, 
                                                zoroData.rec.width, zoroData.rec.height };
                        DrawTextureRec(zoroJump, sourceRect, zoroData.pos, WHITE);
                    }
                    else
                    {
                        // Animation de marche
                        Rectangle sourceRect = { zoroData.frame * zoroData.rec.width, 0, 
                                                zoroData.rec.width, zoroData.rec.height };
                        DrawTextureRec(zoroWalk, sourceRect, zoroData.pos, WHITE);
                    }
                    
                    // Afficher l'attaque slash si active
                    if (isAttacking)
                    {
                        Rectangle slashSourceRect = { slashData.frame * slashData.rec.width, 0,
                                                     slashData.rec.width, slashData.rec.height };
                        DrawTextureRec(zoroSlash, slashSourceRect, slashData.pos, WHITE);
                    }
                    
                    // UI
                    DrawText(TextFormat("SCORE: %d", score), 20, 20, 40, YELLOW);
                    DrawText(TextFormat("VITESSE: %.0f", gameSpeed), 20, 70, 30, GREEN);
                    
                    // Instructions en jeu
                    DrawText("ESPACE: SAUTER", windowWidth - 250, 20, 25, WHITE);
                    DrawText("A ou CLIC: ATTAQUER", windowWidth - 250, 50, 25, WHITE);
                    DrawText("ECHAP: MENU", windowWidth - 250, 80, 25, WHITE);
                    
                    // Indicateur de cooldown d'attaque
                    if (attackCooldown > 0)
                    {
                        float cooldownPercent = attackCooldown / attackCooldownTime;
                        int barWidth = 100;
                        DrawRectangle(windowWidth - 250, 110, barWidth, 10, RED);
                        DrawRectangle(windowWidth - 250, 110, (int)(barWidth * (1.0f - cooldownPercent)), 10, GREEN);
                        DrawText("ATTAQUE", windowWidth - 250, 125, 15, WHITE);
                    }
                }
                else
                {
                    // Écran Game Over
                    DrawRectangle(0, 0, windowWidth, windowHeight, Color{0, 0, 0, 200});
                    
                    DrawText("GAME OVER", windowWidth/2 - 200, 100, 80, RED);
                    DrawText(TextFormat("SCORE FINAL: %d", score), windowWidth/2 - 150, 200, 50, YELLOW);
                    
                    // Vérifier si c'est un nouveau record
                    bool isNewHighScore = false;
                    if (highScores.size() < 5 || score > highScores.back().score)
                    {
                        isNewHighScore = true;
                        DrawText("NOUVEAU RECORD!", windowWidth/2 - 200, 250, 40, GOLD);
                    }
                    
                    DrawText("Appuyez sur ENTRER pour", windowWidth/2 - 250, 350, 40, WHITE);
                    DrawText("retourner au menu", windowWidth/2 - 200, 400, 40, WHITE);
                    
                    if (isNewHighScore)
                    {
                        DrawText("Votre score sera enregistre!", windowWidth/2 - 300, 470, 30, GREEN);
                    }
                }
                break;
            }
            
            case STATE_HIGHSCORES:
            {
                DrawRectangle(0, 0, windowWidth, windowHeight, Color{30, 30, 50, 255});
                
                DrawText("MEILLEURS SCORES", windowWidth/2 - 250, 50, 60, GOLD);
                
                if (highScores.empty())
                {
                    DrawText("AUCUN SCORE ENREGISTRE", windowWidth/2 - 200, 200, 40, GRAY);
                }
                else
                {
                    // Dessiner le tableau des scores
                    DrawText("RANG", windowWidth/2 - 350, 150, 30, YELLOW);
                    DrawText("SCORE", windowWidth/2 - 150, 150, 30, YELLOW);
                    DrawText("DATE", windowWidth/2 + 100, 150, 30, YELLOW);
                    
                    for (size_t i = 0; i < highScores.size() && i < 5; i++)
                    {
                        int yPos = 200 + (int)i * 60;
                        Color color = (i % 2 == 0) ? WHITE : LIGHTGRAY;
                        
                        // Rang
                        DrawText(TextFormat("%d.", i + 1), windowWidth/2 - 350, yPos, 30, color);
                        
                        // Score
                        DrawText(TextFormat("%d", highScores[i].score), windowWidth/2 - 150, yPos, 30, color);
                        
                        // Date
                        DrawText(highScores[i].date, windowWidth/2 + 100, yPos, 30, color);
                    }
                }
                
                DrawText("Appuyez sur ENTRER pour retourner au menu", windowWidth/2 - 350, 550, 30, GRAY);
                break;
            }
        }
        
        EndDrawing();
    }
    
    // Sauvegarder les scores avant de quitter
    SaveHighScores(highScores);
    
    // Nettoyage
    UnloadTexture(zoroWalk);
    UnloadTexture(zoroJump);
    UnloadTexture(zoroIdle);
    UnloadTexture(zoroVictory);
    UnloadTexture(zoroSlash);
    
    UnloadTexture(kunai);
    UnloadTexture(shuriken);
    UnloadTexture(sake);
    UnloadTexture(coin);
    
    UnloadTexture(bgIntro);
    UnloadTexture(bgGame);
    
    UnloadSound(jumpSound);
    UnloadSound(collectSound);
    UnloadSound(hitSound);
    UnloadSound(slashSound);
    UnloadMusicStream(bgMusic);
    
    CloseAudioDevice();
    CloseWindow();
    
    return 0;
}