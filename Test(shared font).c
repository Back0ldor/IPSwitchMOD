#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <switch.h>

#include "menu.h"

#include <ft2build.h>
#include FT_FREETYPE_H

//See also libnx pl.h.

// Define the desired framebuffer resolution (here we set it to 720p).
#define FB_WIDTH  1280
#define FB_HEIGHT 720

//This requires the switch-freetype package.
//Freetype code here is based on the example code from freetype docs.

static u32 framebuf_width=0;

//Note that this doesn't handle any blending.
void draw_glyph(FT_Bitmap* bitmap, u32* framebuf, u32 x, u32 y)
{
    u32 framex, framey;
    u32 tmpx, tmpy;
    u8* imageptr = bitmap->buffer;

    if (bitmap->pixel_mode!=FT_PIXEL_MODE_GRAY) return;

    for (tmpy=0; tmpy<bitmap->rows; tmpy++)
    {
        for (tmpx=0; tmpx<bitmap->width; tmpx++)
        {
            framex = x + tmpx;
            framey = y + tmpy;

            framebuf[framey * framebuf_width + framex] = RGBA8_MAXALPHA(imageptr[tmpx], imageptr[tmpx], imageptr[tmpx]);
        }

        imageptr+= bitmap->pitch;
    }
}

//Note that this doesn't handle {tmpx > width}, etc.
//str is UTF-8.
void draw_text(FT_Face face, u32* framebuf, u32 x, u32 y, const char* str)
{
    u32 tmpx = x;
    FT_Error ret=0;
    FT_UInt glyph_index;
    FT_GlyphSlot slot = face->glyph;

    u32 i;
    u32 str_size = strlen(str);
    uint32_t tmpchar;
    ssize_t unitcount=0;

    for (i = 0; i < str_size; )
    {
        unitcount = decode_utf8 (&tmpchar, (const uint8_t*)&str[i]);
        if (unitcount <= 0) break;
        i+= unitcount;

        if (tmpchar == '\n')
        {
            tmpx = x;
            y+= face->size->metrics.height / 64;
            continue;
        }

        glyph_index = FT_Get_Char_Index(face, tmpchar);
        //If using multiple fonts, you could check for glyph_index==0 here and attempt using the FT_Face for the other fonts with FT_Get_Char_Index.

        ret = FT_Load_Glyph(
                face,          /* handle to face object */
                glyph_index,   /* glyph index           */
                FT_LOAD_DEFAULT);

        if (ret==0)
        {
            ret = FT_Render_Glyph( face->glyph,   /* glyph slot  */
                                   FT_RENDER_MODE_NORMAL);  /* render mode */
        }

        if (ret) return;

        draw_glyph(&slot->bitmap, framebuf, tmpx + slot->bitmap_left, y - slot->bitmap_top);

        tmpx += slot->advance.x >> 6;
        y += slot->advance.y >> 6;
    }
}

__attribute__((format(printf, 1, 2)))
static int error_screen(const char* fmt, ...)
{
    consoleInit(NULL);
    va_list va;
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
    printf("Press PLUS to exit\n");
    while (appletMainLoop())
    {
        hidScanInput();
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_PLUS)
            break;
        consoleUpdate(NULL);
    }
    consoleExit(NULL);
    return EXIT_FAILURE;
}

static u64 getSystemLanguage(void)
{
    Result rc;
    u64 code = 0;

    rc = setInitialize();
    if (R_SUCCEEDED(rc)) {
        rc = setGetSystemLanguage(&code);
        setExit();
    }

    return R_SUCCEEDED(rc) ? code : 0;
}

void mainMenu() {
    StrList* main_menu_list = getStrList();
    addToStrList(main_menu_list, "パッチテキストによるIPSの生成");   //0
    addToStrList(main_menu_list, "パッチテキストコンテンツの切り替え");   //1
    //addToStrList(main_menu_list, "Toggle Existing IPS");        //2 (planned)
    //addToStrList(main_menu_list, "Create Patch Text with IPS"); //3 (planned)

    while (appletMainLoop()) {
        int selection = 0;
        printf(
            "\n%s\n",
            CONSOLE_ESC(
                1m) "操作を選択するか、+を押して終了します:" CONSOLE_ESC(m));
        u64 kDown = selectFromList(&selection, main_menu_list);

        if (kDown & KEY_A) {
            switch (selection) {
                case 0:
                    kDown = patchTextToIPSMenu();
                    break;
                case 1:
                    kDown = patchTextToggleMenu();
                    break;
                default:
                    break;
            }
        }

        if (kDown & KEY_PLUS) break;
        consoleUpdate(NULL);
    }

    freeStrList(main_menu_list);
}

u64 patchTextSelect(PatchTextTarget* pchtxt_target) {
    printf("\n読み取り %s\n", IPSWITCH_DIR);

    StrList* pchtxt_list = getStrList();

    char tid_str[17];
    tid_str[16] = '\0';

    // read IPSWITCH_DIR folders
    DIR* dir;
    struct dirent* dir_ent;
    dir = opendir(IPSWITCH_DIR);
    if (dir == NULL) {
        printf(CONSOLE_ESC(31m) "開けませんでした %s.\n" CONSOLE_ESC(m),
               IPSWITCH_DIR);
    } else {
        while ((dir_ent = readdir(dir))) {
            if (dir_ent->d_type != DT_DIR) {
                continue;
            }

            strcpysize(tid_str, dir_ent->d_name, 16);

            if (strlen(tid_str) != 16 || !isValidHexStr(tid_str)) {
                tid_str[0] = '\0';
            }

            char subdir_path[0x100] = {0};
            strcpy(subdir_path, IPSWITCH_DIR);
            strcat(subdir_path, dir_ent->d_name);
            strcat(subdir_path, "/");

            // search patch text in folder
            DIR* subdir;
            struct dirent* subdir_ent;
            subdir = opendir(subdir_path);
            if (subdir == NULL) {
                printf(CONSOLE_ESC(31m) "開けませんでした %s.\n" CONSOLE_ESC(m),
                       subdir_path);
            } else {
                while ((subdir_ent = readdir(subdir))) {
                    if (subdir_ent->d_type != DT_REG) {
                        continue;
                    }

                    char* ent_extention = strrchr(subdir_ent->d_name, '.');
                    if (strcmp(ent_extention, PATCH_TEXT_FORMAT) == 0) {
                        char pchtxt_path_buf[0x100] = {0};
                        strcpy(pchtxt_path_buf,
                               &subdir_path[strlen(IPSWITCH_DIR)]);
                        strcat(pchtxt_path_buf, subdir_ent->d_name);
                        addToStrList(pchtxt_list, pchtxt_path_buf);
                    }
                }
                closedir(subdir);
            }
        }
        closedir(dir);
    }

    u64 kDown = 0;  // returns this later

    while (appletMainLoop()) {
        int selection = 0;
        printf("\n%s\n", CONSOLE_ESC(1m)
            "Aキーを押して、パッチテキストを選択 戻るB | PLUS（+）で終了："
            CONSOLE_ESC(m));
        kDown = selectFromList(&selection, pchtxt_list);

        if (kDown & KEY_PLUS || kDown & KEY_B) {
            break;
        }

        if (kDown & KEY_A) {
            // patch_txt_path
            strcpy(pchtxt_target->patch_txt_path, IPSWITCH_DIR);
            strcat(pchtxt_target->patch_txt_path,
                   pchtxt_list->str_list[selection]);
            // tid_str
            strcpysize(pchtxt_target->tid_str,
                       pchtxt_list->str_list[selection], 16);
            // name
            getPatchTextName(pchtxt_target->name,
                             pchtxt_target->patch_txt_path);
            // folder_name
            strcpy(pchtxt_target->folder_name,
                   pchtxt_list->str_list[selection]);
            *strchr(pchtxt_target->folder_name, '/') = '\0';
            break;
        }
        consoleUpdate(NULL);
    }

    freeStrList(pchtxt_list);
    return kDown;
}

u64 patchTextToIPSMenu() {
    u64 kDown = 0;
    while (appletMainLoop()) {
        PatchTextTarget pchtxt_target;
        kDown = patchTextSelect(&pchtxt_target);

        if (kDown & KEY_PLUS || kDown & KEY_B) break;

        if (kDown & KEY_A) {
            int rc = 0;

            rc = patchTextToIPS(&pchtxt_target);

            if (R_SUCCEEDED(rc))
                printf(CONSOLE_ESC(32m) "\n完了\n" CONSOLE_ESC(m));
            else
                printf(CONSOLE_ESC(31m) "\n失敗: %d\n" CONSOLE_ESC(m), rc);
        }
        consoleUpdate(NULL);
    }
    return kDown;
}

u64 patchTextToggleMenu() {
    u64 kDown = 0;
    PatchTextTarget pchtxt_target;
    kDown = patchTextSelect(&pchtxt_target);

    if (kDown & KEY_A) {
        printf(CONSOLE_ESC(1m) 
            "Aを押してパッチを切り替える | 保存して戻るにはB\n"
            "       変更を中止して破棄するX | "
            "変更を保存してIPSに適用するにはY\n"
            CONSOLE_ESC(0;32m) "\n\n緑 = 有効 "
            CONSOLE_ESC(31m) "赤 = 無効\n" CONSOLE_ESC(m));
        consoleUpdate(NULL);

        StrList* pchtxt = getStrList();
        StrList* patch_str_list = getStrList();

        Result rc = 0;
        rc = readPchtxtIntoStrList(&pchtxt_target, pchtxt, patch_str_list);
        if (!R_SUCCEEDED(rc)) {
            printf(CONSOLE_ESC(
                       31m) "\nパッチテキストを読み取れませんでした: %d\n" CONSOLE_ESC(m),
                   rc);
            goto end;
        }

        int selection = 0;  // dummy here
        kDown = selectFromList(&selection, patch_str_list);

        if (kDown & KEY_B || kDown & KEY_Y) {
            rc =
                writePchtxtFromStrList(&pchtxt_target, pchtxt, patch_str_list);

            if (R_SUCCEEDED(rc)) {
                printf(CONSOLE_ESC(32m) "適用された変更.\n" CONSOLE_ESC(m));
            } else {
                printf(CONSOLE_ESC(31m) "\n失敗: %d\n" CONSOLE_ESC(m), rc);
                goto end;
            }

            if (kDown & KEY_Y) {
                rc = patchTextToIPS(&pchtxt_target);

                if (R_SUCCEEDED(rc))
                    printf(CONSOLE_ESC(32m) "\n完了\n" CONSOLE_ESC(m));
                else
                    printf(CONSOLE_ESC(31m) "\n失敗: %d\n" CONSOLE_ESC(m),
                           rc);
            }
        } else {
            printf(CONSOLE_ESC(33m) "変更を破棄しました.\n" CONSOLE_ESC(m));
        }
    end:
        freeStrList(pchtxt);
        freeStrList(patch_str_list);
    }
    return kDown;
}
