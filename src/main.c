#include <gba.h>
#include <string.h>

#define SCREEN_W 240
#define SCREEN_H 160
#undef VRAM
#undef SRAM
#define VRAM ((volatile u16 *)0x06000000)
#define SRAM ((volatile u8 *)0x0E000000)

#define COL_BLACK RGB5(1, 1, 2)
#define COL_RED   RGB5(31, 3, 5)
#define COL_BLUE  RGB5(5, 13, 31)
#define COL_WHITE RGB5(31, 31, 29)
#define COL_GREY  RGB5(14, 14, 16)
#define COL_DARK  RGB5(4, 4, 7)
#define COL_GOLD  RGB5(31, 22, 5)
#define COL_NEON  RGB5(7, 31, 20)

typedef unsigned char u8;

enum Screen {
    SCREEN_MENU,
    SCREEN_OPTIONS,
    SCREEN_CREDITS,
    SCREEN_SESSION,
    SCREEN_BEDROOM,
    SCREEN_WARDROBE
};

static enum Screen screen = SCREEN_MENU;
static int selected = 0;
static int music_on = 1;
static int continued = 0;

static int player_x = 116;
static int player_y = 108;
static int player_facing = 0;
static int wardrobe_choice = 0;
static int appearance_skin = 0;
static int appearance_hair = 0;
static int appearance_top = 0;
static int appearance_trousers = 0;

static const u8 font[36][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
    {15,16,16,16,16,16,15}, {30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {15,16,16,23,17,17,15}, {17,17,17,31,17,17,17},
    {14,4,4,4,4,4,14},      {7,2,2,2,18,18,12},
    {17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30},  {31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},
    {17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4},   {31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},
    {14,17,1,2,4,8,31},    {30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2},    {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14},{14,17,17,15,1,1,14}
};

static u16 rgb(int r, int g, int b)
{
    return RGB5(r, g, b);
}

static void pixel(int x, int y, u16 color)
{
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H)
        VRAM[y * SCREEN_W + x] = color;
}

static void rect(int x, int y, int w, int h, u16 color)
{
    for (int py = y; py < y + h; py++)
        for (int px = x; px < x + w; px++)
            pixel(px, py, color);
}

static void line(int x1, int y1, int x2, int y2, u16 color)
{
    int dx = x2 > x1 ? x2 - x1 : x1 - x2;
    int dy = y2 > y1 ? y2 - y1 : y1 - y2;
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;

    for (;;) {
        pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}

static int glyph_index(char c)
{
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= '0' && c <= '9') return 26 + c - '0';
    return -1;
}

static void text(const char *s, int x, int y, int scale, u16 color)
{
    int start_x = x;

    while (*s) {
        if (*s == '\n') {
            x = start_x;
            y += 9 * scale;
            s++;
            continue;
        }

        int index = glyph_index(*s);
        if (index >= 0) {
            for (int row = 0; row < 7; row++) {
                for (int col = 0; col < 5; col++) {
                    if (font[index][row] & (1 << (4 - col)))
                        rect(x + col * scale, y + row * scale,
                             scale, scale, color);
                }
            }
        }
        x += 6 * scale;
        s++;
    }
}

static void draw_city(void)
{
    rect(0, 0, 240, 160, rgb(3, 4, 10));

    /* Night sky and distant skyline. */
    rect(0, 18, 240, 76, rgb(5, 7, 17));
    for (int i = 0; i < 20; i++)
        pixel((i * 37 + 11) % 240, (i * 13 + 7) % 47,
              RGB5(20, 20, 24));

    /* Buildings. */
    rect(0, 42, 43, 67, rgb(8, 9, 15));
    rect(45, 30, 48, 79, rgb(11, 10, 16));
    rect(95, 48, 39, 62, rgb(7, 9, 15));
    rect(136, 24, 48, 86, rgb(10, 9, 16));
    rect(186, 40, 54, 70, rgb(7, 8, 14));

    /* Neon roof edges. */
    rect(45, 30, 48, 2, COL_BLUE);
    rect(136, 24, 48, 2, COL_RED);
    rect(0, 42, 43, 2, COL_RED);
    rect(186, 40, 54, 2, COL_BLUE);

    /* Lit windows. */
    for (int bx = 5; bx < 40; bx += 11)
        for (int by = 51; by < 100; by += 13)
            rect(bx, by, 5, 7, rgb(20, 15, 8));

    for (int bx = 51; bx < 88; bx += 12)
        for (int by = 40; by < 101; by += 14)
            rect(bx, by, 5, 7, rgb(8, 15, 24));

    for (int bx = 143; bx < 178; bx += 12)
        for (int by = 35; by < 100; by += 14)
            rect(bx, by, 5, 7, rgb(23, 8, 10));

    for (int bx = 194; bx < 233; bx += 12)
        for (int by = 49; by < 100; by += 14)
            rect(bx, by, 5, 7, rgb(8, 14, 25));

    /* Alley, pavement and road. */
    rect(0, 108, 240, 14, rgb(13, 12, 17));
    rect(0, 122, 240, 38, rgb(5, 5, 8));
    line(0, 124, 240, 124, rgb(22, 19, 21));
    for (int x = 8; x < 240; x += 44)
        rect(x, 143, 23, 2, rgb(20, 18, 15));

    /* Graffiti tags on the lower wall. */
    line(12, 115, 25, 110, COL_RED);
    line(25, 110, 33, 118, COL_RED);
    line(33, 118, 42, 110, COL_RED);
    line(194, 110, 205, 118, COL_BLUE);
    line(205, 118, 215, 110, COL_BLUE);
    line(215, 110, 228, 116, COL_BLUE);

    /* Street lamps. */
    rect(4, 79, 2, 31, rgb(18, 18, 20));
    rect(2, 77, 8, 3, COL_GOLD);
    rect(230, 76, 2, 34, rgb(18, 18, 20));
    rect(226, 74, 8, 3, COL_BLUE);
}

static void draw_title(void)
{
    /* Layered lettering gives the logo a graffiti-like outline. */
    text("STREET", 50, 13, 3, COL_BLACK);
    text("STREET", 47, 10, 3, COL_BLUE);
    text("STREET", 49, 12, 3, COL_WHITE);

    text("LEGENDS", 24, 39, 3, COL_BLACK);
    text("LEGENDS", 21, 36, 3, COL_RED);
    text("LEGENDS", 23, 38, 3, COL_WHITE);

    line(20, 62, 220, 62, COL_RED);
    line(35, 65, 205, 65, COL_BLUE);
}

static int save_valid(void)
{
    const u8 magic[4] = {'S','L','G','1'};
    u8 check = 0xA5;

    for (int i = 0; i < 4; i++)
        if (SRAM[i] != magic[i]) return 0;

    if (SRAM[4] != 1) return 0;

    /* Match the writer. Byte zero is excluded. */
    for (int i = 1; i <= 4; i++)
        check ^= SRAM[i];
    for (int i = 6; i <= 9; i++)
        check ^= SRAM[i];

    return SRAM[5] == check;
}

static void save_test_marker(void)
{
    const u8 magic[4] = {'S','L','G','1'};
    u8 check = 0xA5;

    /* Invalidate the record before updating it. */
    SRAM[0] = 0;

    for (int i = 1; i < 4; i++)
        SRAM[i] = magic[i];

    SRAM[4] = 1;
    SRAM[6] = (u8)appearance_skin;
    SRAM[7] = (u8)appearance_hair;
    SRAM[8] = (u8)appearance_top;
    SRAM[9] = (u8)appearance_trousers;

    for (int i = 1; i <= 4; i++)
        check ^= SRAM[i];
    for (int i = 6; i <= 9; i++)
        check ^= SRAM[i];

    SRAM[5] = check;

    /* Commit the record by writing the first magic byte last. */
    SRAM[0] = magic[0];
}

static void load_appearance(void)
{
    appearance_skin = SRAM[6] & 3;
    appearance_hair = SRAM[7] & 3;
    appearance_top = SRAM[8] & 3;
    appearance_trousers = SRAM[9] & 3;
}

static void draw_menu_row(int i)
{
    const char *items[4] = {"NEW GAME", "CONTINUE", "OPTIONS", "CREDITS"};
    int y = 82 + i * 16;
    int has_save = save_valid();

    /* Clear only this row before drawing its current state. */
    rect(52, y - 2, 136, 12, rgb(5, 6, 12));

    if (i == selected) {
        rect(52, y - 2, 136, 12, rgb(14, 4, 8));
        text(">", 58, y, 1, COL_GOLD);
    }

    u16 color = (i == 1 && !has_save) ? COL_GREY : COL_WHITE;
    text(items[i], 72, y, 1, color);

    if (i == 1 && !has_save)
        text("NO SAVE", 132, y, 1, COL_GREY);
}

static void draw_menu_hints(void)
{
    text("A SELECT", 48, 138, 1, COL_GOLD);
    text("B BACK", 142, 138, 1, COL_GOLD);
}

static void draw_menu_panel(void)
{
    rect(40, 72, 160, 78, rgb(2, 3, 8));
    rect(42, 74, 156, 74, rgb(5, 6, 12));
    line(42, 74, 198, 74, COL_BLUE);
    line(42, 148, 198, 148, COL_RED);

    for (int i = 0; i < 4; i++)
        draw_menu_row(i);

    draw_menu_hints();
}

static void draw_menu(void)
{
    draw_city();
    draw_title();
    draw_menu_panel();
}

static int near_wardrobe(void)
{
    /* Stand in front of the wardrobe artwork. */
    return player_x >= 150 && player_x <= 208 &&
           player_y >= 94 && player_y <= 110;
}

static int bedroom_blocked(int x, int y)
{
    if (x >= 14 && x <= 93 && y >= 48 && y <= 134)
        return 1; /* Bed */

    if (x >= 85 && x <= 144 && y >= 9 && y <= 55)
        return 1; /* Desk */

    if (x >= 151 && x <= 211 && y >= 12 && y <= 93)
        return 1; /* Wardrobe */

    if (x >= 203 && x <= 230 && y >= 9 && y <= 74)
        return 1; /* Door artwork */

    return 0;
}

static void draw_player(void)
{
    u16 skin[] = {RGB5(24, 15, 9), RGB5(18, 10, 6),
                  RGB5(29, 21, 14), RGB5(10, 7, 5)};
    u16 tops[] = {RGB5(5, 13, 26), RGB5(23, 5, 9),
                  RGB5(5, 20, 12), RGB5(20, 13, 25)};
    u16 pants[] = {RGB5(7, 8, 15), RGB5(15, 12, 9),
                   RGB5(5, 12, 19), RGB5(17, 17, 17)};
    int x = player_x;
    int y = player_y;

    /* Shadow, shoes, trousers, torso, neck, face and hair. */
    rect(x - 5, y + 9, 11, 3, RGB5(3, 3, 5));
    rect(x - 4, y + 7, 3, 4, RGB5(2, 2, 3));
    rect(x + 2, y + 7, 3, 4, RGB5(2, 2, 3));
    rect(x - 4, y + 3, 3, 5, pants[appearance_trousers & 3]);
    rect(x + 1, y + 3, 3, 5, pants[appearance_trousers & 3]);
    rect(x - 5, y - 3, 10, 7, tops[appearance_top & 3]);
    rect(x - 2, y - 5, 4, 3, skin[appearance_skin & 3]);
    rect(x - 4, y - 10, 8, 6, skin[appearance_skin & 3]);

    u16 hair[] = {RGB5(4, 3, 3), RGB5(15, 8, 4),
                  RGB5(24, 19, 10), RGB5(5, 12, 13)};
    rect(x - 4, y - 11, 8, 3, hair[appearance_hair & 3]);
    rect(x - 5, y - 8, 2, 3, hair[appearance_hair & 3]);
}

static void draw_bedroom(void)
{
    /* Floor and walls: Neon Grit amber/blue palette. */
    rect(0, 0, SCREEN_W, SCREEN_H, RGB5(8, 7, 10));
    rect(8, 8, 224, 144, RGB5(13, 10, 12));
    rect(8, 8, 224, 8, RGB5(18, 13, 15));
    rect(8, 8, 4, 144, RGB5(22, 15, 14));
    rect(228, 8, 4, 144, RGB5(7, 12, 20));

    /* Floorboards. */
    for (int y = 20; y < 152; y += 12) {
        line(12, y, 227, y, RGB5(18, 13, 14));
        for (int x = 25 + ((y / 12) % 2) * 18; x < 225; x += 48)
            line(x, y, x, y + 11, RGB5(15, 11, 12));
    }

    /* Window with blue night light. */
    rect(24, 18, 52, 30, RGB5(3, 7, 16));
    rect(27, 21, 46, 24, RGB5(7, 14, 25));
    line(50, 21, 50, 45, RGB5(20, 17, 19));
    line(27, 33, 73, 33, RGB5(20, 17, 19));
    rect(28, 23, 5, 3, RGB5(17, 20, 27));
    rect(60, 37, 10, 3, RGB5(12, 20, 28));

    /* Bed and blanket. */
    rect(20, 62, 67, 61, RGB5(4, 4, 7));
    rect(19, 59, 66, 60, RGB5(21, 18, 18));
    rect(23, 63, 58, 51, RGB5(5, 9, 17));
    rect(25, 64, 24, 12, RGB5(22, 20, 19));
    rect(51, 64, 28, 12, RGB5(18, 19, 22));
    rect(25, 80, 54, 31, RGB5(7, 12, 22));
    line(25, 80, 78, 80, RGB5(14, 21, 29));
    line(25, 111, 78, 111, RGB5(14, 21, 29));

    /* Rug. */
    rect(93, 83, 53, 49, RGB5(5, 4, 9));
    rect(96, 86, 47, 43, RGB5(17, 9, 13));
    line(99, 90, 140, 90, RGB5(27, 11, 15));
    line(99, 125, 140, 125, RGB5(27, 11, 15));

    /* Desk, monitor and small warm lamp. */
    rect(91, 20, 47, 23, RGB5(4, 4, 6));
    rect(94, 20, 42, 18, RGB5(18, 12, 10));
    rect(99, 23, 20, 11, RGB5(4, 9, 17));
    rect(102, 26, 14, 5, RGB5(8, 17, 26));
    rect(124, 24, 7, 7, RGB5(27, 17, 6));
    rect(125, 25, 4, 4, RGB5(31, 23, 10));
    rect(96, 39, 38, 3, RGB5(8, 7, 8));

    /* Wardrobe with mirror. */
    rect(157, 24, 48, 57, RGB5(4, 4, 6));
    rect(160, 23, 42, 54, RGB5(21, 13, 11));
    rect(163, 27, 18, 45, RGB5(11, 9, 12));
    rect(184, 27, 14, 45, RGB5(4, 10, 18));
    rect(186, 29, 10, 41, RGB5(8, 16, 26));
    line(188, 31, 194, 31, RGB5(22, 23, 27));
    line(187, 33, 192, 38, RGB5(16, 22, 29));
    rect(179, 46, 2, 7, RGB5(29, 21, 10));
    rect(190, 46, 2, 7, RGB5(29, 21, 10));
    line(160, 77, 202, 77, RGB5(28, 17, 10));

    /* Exit door. */
    rect(209, 20, 15, 43, RGB5(4, 4, 6));
    rect(211, 22, 11, 39, RGB5(20, 12, 10));
    rect(219, 42, 2, 2, RGB5(30, 20, 7));

    /* Small posters. */
    rect(92, 48, 17, 23, RGB5(5, 5, 8));
    rect(95, 51, 11, 17, RGB5(17, 5, 11));
    line(96, 55, 104, 61, RGB5(30, 8, 14));
    line(104, 55, 96, 61, RGB5(30, 8, 14));

    draw_player();

    /* Compact interaction prompt. */
    if (near_wardrobe()) {
        rect(57, 136, 126, 16, RGB5(3, 4, 9));
        line(57, 136, 182, 136, RGB5(8, 19, 30));
        text("A WARDROBE", 69, 141, 1, COL_GOLD);
    } else {
        rect(57, 136, 126, 16, RGB5(3, 4, 9));
        text("D PAD MOVE", 72, 141, 1, COL_WHITE);
    }
}

static void draw_wardrobe(void)
{
    draw_bedroom();
    rect(20, 15, 200, 127, RGB5(2, 3, 8));
    rect(22, 17, 196, 123, RGB5(7, 8, 14));
    line(22, 17, 217, 17, COL_BLUE);
    line(22, 139, 217, 139, COL_RED);

    text("WARDROBE", 68, 24, 2, COL_WHITE);
    text("SKIN TONE", 34, 51, 1,
         wardrobe_choice == 0 ? COL_GOLD : COL_WHITE);
    text("HAIR STYLE", 34, 68, 1,
         wardrobe_choice == 1 ? COL_GOLD : COL_WHITE);
    text("TOP", 34, 85, 1,
         wardrobe_choice == 2 ? COL_GOLD : COL_WHITE);
    text("TROUSERS", 34, 102, 1,
         wardrobe_choice == 3 ? COL_GOLD : COL_WHITE);

    text("<", 142, 51 + wardrobe_choice * 17, 1, COL_BLUE);
    text(">", 190, 51 + wardrobe_choice * 17, 1, COL_BLUE);

    /* Preview character at right. */
    rect(151, 43, 43, 78, RGB5(4, 5, 10));
    rect(154, 46, 37, 72, RGB5(10, 12, 19));
    int old_x = player_x, old_y = player_y;
    player_x = 173;
    player_y = 82;
    draw_player();
    player_x = old_x;
    player_y = old_y;

    text("LEFT RIGHT", 135, 126, 1, COL_WHITE);
    text("A SAVE B BACK", 111, 132, 1, COL_GOLD);
}

static void draw_options(void)
{
    draw_city();
    text("OPTIONS", 70, 25, 2, COL_RED);
    text("MUSIC", 44, 70, 1, COL_WHITE);
    text(music_on ? "ON" : "OFF", 135, 70, 1, COL_GOLD);
    text("A TOGGLE", 55, 104, 1, COL_BLUE);
    text("B BACK", 55, 119, 1, COL_WHITE);
}

static void draw_credits(void)
{
    draw_city();
    text("CREDITS", 68, 22, 2, COL_BLUE);
    text("STREET LEGENDS", 39, 62, 1, COL_WHITE);
    text("PHASE 1 DEMO", 48, 78, 1, COL_RED);
    text("GBA HOMEBREW", 42, 94, 1, COL_GOLD);
    text("B BACK", 79, 126, 1, COL_WHITE);
}

static void draw_session(void)
{
    draw_city();
    text(continued ? "WELCOME BACK" : "NEW SESSION", 34, 22, 2, COL_RED);
    text("TEST SAVE MARKER", 36, 68, 1, COL_WHITE);
    text("GAMEPLAY COMES", 36, 86, 1, COL_BLUE);
    text("IN PHASE 2", 55, 98, 1, COL_BLUE);
    text("B BACK TO MENU", 42, 128, 1, COL_GOLD);
}

int main(void)
{
    irqInit();
    irqEnable(IRQ_VBLANK);
    SetMode(MODE_3 | BG2_ENABLE);

    int needs_redraw = 1;
    enum Screen drawn_screen = (enum Screen)-1;
    int drawn_selected = -1;
    int drawn_music_on = -1;

    while (1) {
        scanKeys();
        u16 down = keysDown();
        u16 held = keysHeld();
        int player_moved = 0;

        if (down)
            needs_redraw = 1;

        if (screen == SCREEN_MENU) {
            if (down & KEY_UP) {
                do {
                    selected = (selected + 3) % 4;
                } while (selected == 1 && !save_valid());
            }

            if (down & KEY_DOWN) {
                do {
                    selected = (selected + 1) % 4;
                } while (selected == 1 && !save_valid());
            }

            if (down & KEY_A) {
                if (selected == 0) {
                    continued = 0;
                    player_x = 116;
                    player_y = 108;
                    screen = SCREEN_BEDROOM;
                } else if (selected == 1 && save_valid()) {
                    continued = 1;
                    load_appearance();
                    screen = SCREEN_SESSION;
                } else if (selected == 2) {
                    screen = SCREEN_OPTIONS;
                } else if (selected == 3) {
                    screen = SCREEN_CREDITS;
                }
            }
        } else if (screen == SCREEN_BEDROOM) {
            int old_x = player_x;
            int old_y = player_y;
            int nx = player_x;
            int ny = player_y;

            if (held & KEY_LEFT)  { nx -= 2; player_facing = 1; }
            if (held & KEY_RIGHT) { nx += 2; player_facing = 2; }
            if (held & KEY_UP)    { ny -= 2; player_facing = 3; }
            if (held & KEY_DOWN)  { ny += 2; player_facing = 0; }

            if (nx < 20) nx = 20;
            if (nx > 220) nx = 220;
            if (ny < 38) ny = 38;
            if (ny > 132) ny = 132;

            /* Test each axis independently to avoid corner trapping. */
            if (!bedroom_blocked(nx, player_y))
                player_x = nx;
            if (!bedroom_blocked(player_x, ny))
                player_y = ny;

            player_moved = (player_x != old_x || player_y != old_y);
            if (player_moved)
                needs_redraw = 1;

            if ((down & KEY_A) &&
                player_x >= 208 && player_y >= 76 && player_y <= 88) {
                screen = SCREEN_MENU;
            } else if ((down & KEY_A) && near_wardrobe()) {
                wardrobe_choice = 0;
                screen = SCREEN_WARDROBE;
            }

        } else if (screen == SCREEN_WARDROBE) {
            if (down & KEY_UP)
                wardrobe_choice = (wardrobe_choice + 3) % 4;
            if (down & KEY_DOWN)
                wardrobe_choice = (wardrobe_choice + 1) % 4;

            if (down & KEY_LEFT) {
                if (wardrobe_choice == 0)
                    appearance_skin = (appearance_skin + 3) % 4;
                else if (wardrobe_choice == 1)
                    appearance_hair = (appearance_hair + 3) % 4;
                else if (wardrobe_choice == 2)
                    appearance_top = (appearance_top + 3) % 4;
                else
                    appearance_trousers = (appearance_trousers + 3) % 4;
            }

            if (down & KEY_RIGHT) {
                if (wardrobe_choice == 0)
                    appearance_skin = (appearance_skin + 1) % 4;
                else if (wardrobe_choice == 1)
                    appearance_hair = (appearance_hair + 1) % 4;
                else if (wardrobe_choice == 2)
                    appearance_top = (appearance_top + 1) % 4;
                else
                    appearance_trousers = (appearance_trousers + 1) % 4;
            }

            if (down & KEY_B)
                screen = SCREEN_BEDROOM;

            /* Confirm and persist the selected character appearance. */
            if (down & KEY_A) {
                save_test_marker();
                screen = SCREEN_BEDROOM;
            }

        } else if (screen == SCREEN_OPTIONS) {
            if (down & KEY_A)
                music_on = !music_on;
            if (down & KEY_B)
                screen = SCREEN_MENU;
        } else if (screen == SCREEN_CREDITS) {
            if (down & KEY_B)
                screen = SCREEN_MENU;
        } else if (screen == SCREEN_SESSION) {
            if (down & KEY_B)
                screen = SCREEN_MENU;
        }

        if (needs_redraw) {
            if (screen != drawn_screen ||
                (screen == SCREEN_MENU && selected != drawn_selected) ||
                (screen == SCREEN_OPTIONS && music_on != drawn_music_on) ||
                (screen == SCREEN_BEDROOM && player_moved) ||
                (screen == SCREEN_WARDROBE && down)) {

                VBlankIntrWait();

                if (screen != drawn_screen) {
                    switch (screen) {
                        case SCREEN_MENU:    draw_menu(); break;
                        case SCREEN_OPTIONS: draw_options(); break;
                        case SCREEN_CREDITS: draw_credits(); break;
                        case SCREEN_SESSION: draw_session(); break;
                        case SCREEN_BEDROOM: draw_bedroom(); break;
                        case SCREEN_WARDROBE: draw_wardrobe(); break;
                    }
                } else if (screen == SCREEN_BEDROOM && player_moved) {
                    draw_bedroom();
                } else if (screen == SCREEN_WARDROBE && down) {
                    draw_wardrobe();
                } else if (screen == SCREEN_MENU &&
                           selected != drawn_selected) {
                    if (drawn_selected >= 0 && drawn_selected < 4)
                        draw_menu_row(drawn_selected);

                    draw_menu_row(selected);
                    draw_menu_hints();
                } else if (screen == SCREEN_OPTIONS &&
                           music_on != drawn_music_on) {
                    draw_options();
                }

                drawn_screen = screen;
                drawn_selected = selected;
                drawn_music_on = music_on;
            }

            needs_redraw = 0;
        }

        VBlankIntrWait();
    }

    return 0;
}
