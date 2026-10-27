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
    SCREEN_SESSION
};

static enum Screen screen = SCREEN_MENU;
static int selected = 0;
static int music_on = 1;
static int continued = 0;

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

    for (int i = 0; i < 5; i++)
        check ^= SRAM[i];

    for (int i = 0; i < 4; i++)
        if (SRAM[i] != magic[i]) return 0;

    return SRAM[4] == 1 && SRAM[5] == check;
}

static void save_test_marker(void)
{
    const u8 magic[4] = {'S','L','G','1'};
    u8 check = 0xA5;

    /* Invalidate the marker before writing a new record. */
    SRAM[0] = 0;

    for (int i = 1; i < 4; i++) SRAM[i] = magic[i];
    SRAM[4] = 1;

    for (int i = 0; i < 5; i++)
        check ^= SRAM[i];

    SRAM[5] = check;

    /* Write the first magic byte last. */
    SRAM[0] = magic[0];
}

static void draw_menu_panel(void)
{
    const char *items[4] = {"NEW GAME", "CONTINUE", "OPTIONS", "CREDITS"};
    int has_save = save_valid();

    rect(40, 72, 160, 78, rgb(2, 3, 8));
    rect(42, 74, 156, 74, rgb(5, 6, 12));
    line(42, 74, 198, 74, COL_BLUE);
    line(42, 148, 198, 148, COL_RED);

    for (int i = 0; i < 4; i++) {
        int y = 82 + i * 16;

        if (i == selected) {
            rect(52, y - 2, 136, 12, rgb(14, 4, 8));
            text(">", 58, y, 1, COL_GOLD);
        }

        u16 color = (i == 1 && !has_save) ? COL_GREY : COL_WHITE;
        text(items[i], 72, y, 1, color);

        if (i == 1 && !has_save)
            text("NO SAVE", 132, y, 1, COL_GREY);
    }

    text("A SELECT", 48, 138, 1, COL_GOLD);
    text("B BACK", 142, 138, 1, COL_GOLD);
}

static void draw_menu(void)
{
    draw_city();
    draw_title();
    draw_menu_panel();
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
                    save_test_marker();
                    continued = 0;
                    screen = SCREEN_SESSION;
                } else if (selected == 1 && save_valid()) {
                    continued = 1;
                    screen = SCREEN_SESSION;
                } else if (selected == 2) {
                    screen = SCREEN_OPTIONS;
                } else if (selected == 3) {
                    screen = SCREEN_CREDITS;
                }
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
                (screen == SCREEN_OPTIONS && music_on != drawn_music_on)) {

                VBlankIntrWait();

                if (screen != drawn_screen) {
                    switch (screen) {
                        case SCREEN_MENU:    draw_menu(); break;
                        case SCREEN_OPTIONS: draw_options(); break;
                        case SCREEN_CREDITS: draw_credits(); break;
                        case SCREEN_SESSION: draw_session(); break;
                    }
                } else if (screen == SCREEN_MENU &&
                           selected != drawn_selected) {
                    draw_menu_panel();
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
