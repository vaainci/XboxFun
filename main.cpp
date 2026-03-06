#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <string>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

// --- CODES ANSI ---
#define RESET       "\033[0m"
#define BOLD        "\033[1m"
#define INV         "\033[7m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define CYAN        "\033[36m"
#define CLEAR_ALL   "\033[2J\033[H"
#define HIDE_CURSOR "\033[?25l"

enum ModeVibration { TRIGGER, PULSE, WAVE, HEARTBEAT, ABS, STROBO, GEIGER };

ModeVibration currentMode = TRIGGER;
float forceGlobale = 0.8f;
float vitesseBase = 1.0f;
bool controleClavier = false;
bool entreePressee = false;

float joyX = 0.0f;
float joyY = 0.0f;

// Lecture clavier
int get_key() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    int ch = getchar();
    if (ch == 27) {
        int n1 = getchar(); int n2 = getchar();
        if (n1 == 91) {
            if (n2 == 65) ch = 1000; // HAUT
            else if (n2 == 66) ch = 1001; // BAS
            else if (n2 == 67) ch = 1002; // DROITE
            else if (n2 == 68) ch = 1003; // GAUCHE
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    return ch;
}

std::string dessinerVumetre(float intensite) {
    int maxSegments = 20;
    int segments = static_cast<int>(intensite * maxSegments);
    std::string barre = "[";
    for (int i = 0; i < maxSegments; i++) {
        if (i < segments) {
            if (i < 10) barre += GREEN "█" RESET;
            else if (i < 16) barre += YELLOW "█" RESET;
            else barre += RED "█" RESET;
        } else barre += " ";
    }
    return barre + "]";
}

void afficherInterface(float intensiteReelle, float vEff) {
    // ON EFFACE TOUT PROPREMENT AU DÉBUT DU RENDU
    std::cout << CLEAR_ALL << HIDE_CURSOR;

    std::cout << (controleClavier ? YELLOW : CYAN) << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << RESET << "\n";
    std::cout << "  " << INV << (controleClavier ? " MODE ACTIF : ORDINATEUR (CLAVIER) " : " MODE ACTIF : MANETTE (XBOX ONE) ") << RESET << "\n";
    std::cout << (controleClavier ? YELLOW : CYAN) << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << RESET << "\n\n";

    std::cout << BOLD << "CHOIX DU MODE DE VIBRATION :" << RESET << "\n";
    std::string m[] = {"GÂCHETTE SIMPLE (Basé sur RT)", "PULSATIONS (ON/OFF)", "VAGUES CONTINUES (Sinus)", "BATTEMENT DE COEUR", "SYSTÈME ABS (Freinage)", "STROBOSCOPE (Rotation)", "COMPTEUR GEIGER (Aléatoire)"};
    for(int i=0; i<7; i++) {
        if((int)currentMode == i) {
            std::cout << " " << INV << " > " << m[i] << " " << RESET << "\n";
        } else {
            std::cout << "     " << m[i] << "\n";
        }
    }

    std::cout << "\n" << BOLD << "POSITION DE LA VIBRATION DANS LA MANETTE :" << RESET << "\n";
    int radarX = (int)((joyX + 1.0f) * 5);
    int radarY = (int)((joyY + 1.0f) * 2);
    for(int y=0; y<5; y++) {
        std::cout << "    ";
        for(int x=0; x<11; x++) {
            if (y == 0 && (x==0 || x==10)) std::cout << RED "∩" RESET;
            else if (y == 4 && (x==0 || x==10)) std::cout << CYAN "U" RESET;
            else if (y == radarY && x == radarX) std::cout << YELLOW "●" RESET;
            else std::cout << ".";
        }
        if (y == 0) std::cout << "  (Haut : Moteurs des Gâchettes)";
        if (y == 4) std::cout << "  (Bas : Moteurs des Poignées)";
        std::cout << "\n";
    }

    std::cout << "\n" << BOLD << "COMMANDES ACTIVES :" << RESET << "\n";
    if (controleClavier) {
        std::cout << " • TOUCHES [1] à [7] : Changer de mode\n";
        std::cout << " • FLÈCHES [HAUT/BAS] : Puissance | [GAUCHE/DROITE] : Vitesse\n";
        std::cout << " • TOUCHE [ENTRÉE] : Bloquer la gâchette à 100%\n";
        std::cout << " • TOUCHE [ESPACE] : Repasser en mode MANETTE\n";
    } else {
        std::cout << " • BOUTONS [A, B, X, Y, START, BACK] : Changer de mode\n";
        std::cout << " • CROIX DIRECTIONNELLE : Régler Puissance et Vitesse\n";
        std::cout << " • JOYSTICK DROIT : Déplacer la vibration dans la manette\n";
        std::cout << " • ESPACE (Clavier) : Passer en mode ORDINATEUR\n";
    }

    std::cout << "\n" << (entreePressee ? RED BOLD "  [!] GÂCHETTE BLOQUÉE AU MAXIMUM PAR LE CLAVIER [!]" : "") << RESET << "\n";
    std::cout << "  INTENSITÉ REELLE : " << dessinerVumetre(intensiteReelle) << " " << (int)(intensiteReelle * 100) << "%\n";
    std::cout << "  FORCE MAX : " << (int)(forceGlobale*100) << "%  |  VITESSE : " << std::fixed << std::setprecision(1) << vEff << "x" << std::endl;
}

int main() {
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) return 1;
    SDL_GameController* gc = nullptr;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) { gc = SDL_GameControllerOpen(i); break; }
    }
    if (!gc) { std::cerr << "Aucune manette détectée !" << std::endl; return 1; }

    SDL_Event e;
    while (true) {
        int key = get_key();
        if (key == ' ') { controleClavier = !controleClavier; }

        if (controleClavier) {
            if (key == 10 || key == 13) entreePressee = !entreePressee;
            if (key >= '1' && key <= '7') currentMode = (ModeVibration)(key - '1');
            if (key == 1000) forceGlobale = std::min(1.0f, forceGlobale + 0.05f);
            if (key == 1001) forceGlobale = std::max(0.0f, forceGlobale - 0.05f);
            if (key == 1002) vitesseBase += 0.2f;
            if (key == 1003) vitesseBase = std::max(0.2f, vitesseBase - 0.2f);
        }

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return 0;
            if (!controleClavier && e.type == SDL_CONTROLLERBUTTONDOWN) {
                switch(e.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_A: currentMode = TRIGGER; break;
                case SDL_CONTROLLER_BUTTON_X: currentMode = PULSE; break;
                case SDL_CONTROLLER_BUTTON_Y: currentMode = WAVE; break;
                case SDL_CONTROLLER_BUTTON_B: currentMode = HEARTBEAT; break;
                case SDL_CONTROLLER_BUTTON_BACK: currentMode = ABS; break;
                case SDL_CONTROLLER_BUTTON_START: currentMode = STROBO; break;
                case SDL_CONTROLLER_BUTTON_LEFTSTICK: currentMode = GEIGER; break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP: forceGlobale = std::min(1.0f, forceGlobale + 0.05f); break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN: forceGlobale = std::max(0.0f, forceGlobale - 0.05f); break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: vitesseBase += 0.2f; break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT: vitesseBase = std::max(0.2f, vitesseBase - 0.2f); break;
                }
            }
        }

        joyX = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTX) / 32767.0f;
        joyY = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTY) / 32767.0f;

        float rt = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 32767.0f;
        if (controleClavier && entreePressee) rt = 1.0f;

        float vEff = vitesseBase * (1.0f + rt * 4.0f);
        float intensite = 0.0f; int forcedMotor = -1; Uint32 ms = SDL_GetTicks();

        switch(currentMode) {
        case TRIGGER:   intensite = std::pow(rt, 2.5f); break;
        case PULSE:     intensite = ((Uint32)(ms * vEff / 500) % 2) ? 1.0f : 0.0f; break;
        case WAVE:      intensite = (std::sin((ms/1000.0f)*vEff*M_PI*2)+1)/2; break;
        case HEARTBEAT: { int c=1500/vEff; int p=ms%c; intensite=((p>0&&p<150)||(p>300&&p<450))?1:0; break; }
        case ABS:       { int r=std::max(10,(int)(100-(rt*95))); intensite=((ms/r)%2==0&&rt>0.05f)?1:0; break; }
        case STROBO:    { forcedMotor=(int)(ms*vEff/150)%4; intensite=1; break; }
        case GEIGER:    { if((rand()%100)<(rt*60)) intensite=1; break; }
        }

        // --- Application Vibration Spatialisée ---
        float gG = std::min(1.0f, (1.0f - joyX)) * std::min(1.0f, (1.0f - joyY));
        float gD = std::min(1.0f, (1.0f + joyX)) * std::min(1.0f, (1.0f - joyY));
        float hG = std::min(1.0f, (1.0f - joyX)) * std::min(1.0f, (1.0f + joyY));
        float hD = std::min(1.0f, (1.0f + joyX)) * std::min(1.0f, (1.0f + joyY));

        if (forcedMotor != -1) {
            if (forcedMotor == 0) SDL_GameControllerRumble(gc, (Uint16)(intensite * forceGlobale * hG * 0xFFFF), 0, 40);
            else if (forcedMotor == 1) SDL_GameControllerRumbleTriggers(gc, (Uint16)(intensite * forceGlobale * gG * 0xFFFF), 0, 40);
            else if (forcedMotor == 2) SDL_GameControllerRumbleTriggers(gc, 0, (Uint16)(intensite * forceGlobale * gD * 0xFFFF), 40);
            else if (forcedMotor == 3) SDL_GameControllerRumble(gc, 0, (Uint16)(intensite * forceGlobale * hD * 0xFFFF), 40);
        } else {
            SDL_GameControllerRumble(gc, (Uint16)(intensite * forceGlobale * hG * 0xFFFF), (Uint16)(intensite * forceGlobale * hD * 0xFFFF), 40);
            SDL_GameControllerRumbleTriggers(gc, (Uint16)(intensite * forceGlobale * gG * 0xFFFF), (Uint16)(intensite * forceGlobale * gD * 0xFFFF), 40);
        }

        // AFFICHAGE FINAL
        afficherInterface(intensite * forceGlobale, vEff);
        SDL_Delay(20); // délai pour stabiliser l'affichage
    }
    return 0;
}
