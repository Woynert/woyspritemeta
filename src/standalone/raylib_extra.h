#ifndef RAYLIB_EXTRA
#define RAYLIB_EXTRA

#include "portable_utils.h"
#include "raylib.h"
#include "raymath.h"
#include "strview.h"
#include "stdio.h"
#include "strbuf_extra.h"
#include "la.h"


typedef union Rect2 {
    struct {
        float x;
        float y;
        float width;
        float height;
    };
    struct {
        V2f pos;
        V2f size;
    };
    Rectangle rect;
} Rect2;

typedef union Rect2i {
    struct {
        int x;
        int y;
        int width;
        int height;
    };
    struct {
        V2i pos;
        V2i size;
    };
} Rect2i;

Rect2 Rect2i_to_Rect2(Rect2i r)
{ return (Rect2) {{ (float)r.x, (float)r.y, (float)r.width, (float)r.height }}; }

Rect2i Rect2_to_Rect2i(Rect2 r)
{ return (Rect2i) {{ (int)r.x, (int)r.y, (int)r.width, (int)r.height }}; }

Vector2 v2i_to_Vector2(V2i v) { return (Vector2) { (float)v.x, (float)v.y }; }

V2i Vector2_to_v2i(Vector2 v) { return (V2i) {{ (int)v.x, (int)v.y }}; }

/* For drawing vertically invertex textures (i.e. BeginTextureMode) */
void DrawTextureRec_flipped (Texture2D texture, Rect2i source, V2i position, Color tint) {
    DrawTextureRec(
        texture,
        (Rectangle){
            (float)source.x,
            (float)texture.height - (float)source.height - (float)source.y,
            (float)source.width,
            -(float)source.height
        },
        v2i_to_Vector2(position), tint
    );
}

void DrawTexture_flipped(Texture2D texture, int posX, int posY, Color tint)
{
    DrawTextureRec(texture, (Rectangle) {
            (float)posX, (float)posY,
            (float)texture.width, (float)-texture.height }, (Vector2) { 0, 0 }, tint);
}

/*
 * Extracted from rtext.c
 * Get index position for a unicode character on font, fallbacks to '�'
 */
int GetGlyphIndex_woy(Font font, int codepoint) {
    /* Defaults to '�' (0xFFFD). */

    int index = 0;
    if (!IsFontValid(font)) return index;
    int fallbackIndex = 0;
    for (int i = 0; i < font.glyphCount; i++)
    {
        if (font.glyphs[i].value == 0xFFFD) fallbackIndex = i;

        if (font.glyphs[i].value == codepoint)
        {
            index = i;
            break;
        }
    }
    if ((index == 0) && (font.glyphs[0].value != codepoint)) index = fallbackIndex;
    return index;
}

/* @returns byte cursor. */
int utf8_visually_nearest(const char *str, int size, int visual_char_count_target) {
    int count = 0;
    int codepoint_size = 0;
    for (int i = 0; i < size;) {
        if (count >= visual_char_count_target) {
            return i;
        }
        GetCodepointNext_woy(&str[i], &codepoint_size, size-i);
        i += codepoint_size;
        ++count;
    }
    return size;
}

/*
 * Extracted from rtext.c
 */
void DrawTextCodepoint_woy(Font font, int codepoint, Vector2 position, float fontSize, Color tint) {
    int index = GetGlyphIndex_woy(font, codepoint);
    float scaleFactor = fontSize/(float)font.baseSize;
    Rectangle dstRec = {
        position.x + (float)font.glyphs[index].offsetX*scaleFactor - (float)font.glyphPadding*scaleFactor,
        position.y + (float)font.glyphs[index].offsetY*scaleFactor - (float)font.glyphPadding*scaleFactor,
        (font.recs[index].width + 2.0f*(float)font.glyphPadding)*scaleFactor,
        (font.recs[index].height + 2.0f*(float)font.glyphPadding)*scaleFactor };
    Rectangle srcRec = {
        font.recs[index].x - (float)font.glyphPadding,
        font.recs[index].y - (float)font.glyphPadding,
        font.recs[index].width + 2.0f*(float)font.glyphPadding,
        font.recs[index].height + 2.0f*(float)font.glyphPadding };
    DrawTexturePro(font.texture, srcRec, dstRec, (Vector2){ 0, 0 }, 0.0f, tint);
}

/*
 * Extracted from rtext.c
 * @note: chars spacing is NOT proportional to fontSize.
 */
void DrawTextEx_strview(
    Font font, const strview_t string, Vector2 position, float fontSize,
    float spacing, float textLineSpacing, Color tint
) {
    float textOffsetY = 0;
    float textOffsetX = 0.0f;
    float scaleFactor = fontSize/(float)font.baseSize; 

    for (int i = 0; i < string.size;)
    {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext_woy(
                &string.data[i], &codepointByteCount, string.size-i);

        if (codepoint == '\n') {
            textOffsetY += (fontSize + textLineSpacing);
            textOffsetX = 0.0f;
        } else {
            if ((codepoint != ' ') && (codepoint != '\t')) {
                DrawTextCodepoint_woy(
                    font, codepoint,
                    (Vector2){
                        position.x + textOffsetX,
                        position.y + textOffsetY
                    },
                    fontSize, tint);
            }

            int index = GetGlyphIndex_woy(font, codepoint);
            if (font.glyphs[index].advanceX == 0) {
                textOffsetX += ((float)font.recs[index].width*scaleFactor + spacing);
            } else {
                textOffsetX += ((float)font.glyphs[index].advanceX*scaleFactor + spacing);
            }
        }
        i += codepointByteCount;   // Move text bytes counter to next codepoint
    }
}

/*
 * Extracted from rtext.c
 * @note: Uses strview_t instead of Cstrings.
 */
Vector2 MeasureTextEx_woy(Font font, strview_t string, float fontSize, float spacing, float textLineSpacing)
{
    Vector2 textSize = { 0 };

    if ((font.texture.id == 0) || (string.data == NULL) || (string.size == 0) || (string.data[0] == '\0')) return textSize; // Security check

    int tempByteCounter = 0;        // Used to count longer text line num chars
    int byteCounter = 0;

    float textWidth = 0.0f;
    float tempTextWidth = 0.0f;     // Used to count longer text line width

    float textHeight = fontSize;
    float scaleFactor = fontSize/(float)font.baseSize;

    int letter = 0;                 // Current character
    int index = 0;                  // Index position in sprite font

    for (int i = 0; i < string.size;)
    {
        byteCounter++;

        int codepointByteCount = 0;
        letter = GetCodepointNext_woy(&string.data[i], &codepointByteCount, string.size -i);
        index = GetGlyphIndex_woy(font, letter);

        i += codepointByteCount;

        if (letter != '\n')
        {
            if (font.glyphs[index].advanceX > 0) textWidth += (float)font.glyphs[index].advanceX;
            else textWidth += (font.recs[index].width + (float)font.glyphs[index].offsetX);
        }
        else
        {
            if (tempTextWidth < textWidth) tempTextWidth = textWidth;
            byteCounter = 0;
            textWidth = 0;
            textHeight += (fontSize + textLineSpacing);
        }

        if (tempByteCounter < byteCounter) tempByteCounter = byteCounter;
    }

    if (tempTextWidth < textWidth) tempTextWidth = textWidth;

    textSize.x = tempTextWidth*scaleFactor + ((float)(tempByteCounter - 1)*spacing);
    textSize.y = textHeight;

    return textSize;
}



void DrawTextEx_strview_i(
    Font font, const strview_t string, V2i position, int fontSize,
    int spacing, int textLineSpacing, Color tint
) {
    DrawTextEx_strview(font, string, v2i_to_Vector2(position), (float)fontSize, (float)spacing, (float)textLineSpacing, tint);
}

bool CheckCollisionPointReci(V2i point, Rect2i rec) {
    return ((point.x >= rec.x) && (point.x < (rec.x + rec.width)) && (point.y >= rec.y) && (point.y < (rec.y + rec.height)));
}

void DrawRectangleReci(Rect2i rect, Color color) {
    DrawRectanglePro(
        (Rectangle){ (float)rect.x, (float)rect.y, (float)rect.width, (float)rect.height },
        (Vector2){ 0.0f, 0.0f }, 0.0f, color);
}

void DrawRectangleRecf(Rect2 rect, Color color) {
    DrawRectanglePro(
        (Rectangle){ rect.x, rect.y, rect.width, rect.height },
        (Vector2){ 0.0f, 0.0f }, 0.0f, color);
}

V2i GetMousePositioni(void) {
    Vector2 mouse = GetMousePosition();
    return (V2i) {{ (int)mouse.x, (int)mouse.y }};
}

void DrawTextureVi(Texture2D texture, V2i pos) {
    DrawTexture(texture, pos.x, pos.y, WHITE);
}

void DrawTextureScaled(Texture2D texture, Rect2i dest) {
    DrawTexturePro(
        texture,
        (Rectangle) {
            0, 0,
            (float)texture.width,
            (float)texture.height
        },
        (Rect2i_to_Rect2(dest)).rect,
        Vector2Zero(), 0, WHITE
    );
}

void DrawTextureScaled2(Texture2D texture, Rect2i dest, V2i source) {
    DrawTexturePro(
        texture,
        (Rectangle) {
            0, 0,
            (float)source.x,
            (float)source.y
        },
        (Rect2i_to_Rect2(dest)).rect,
        Vector2Zero(), 0, WHITE
    );
}

void DrawTextureScaled2_flipped(Texture2D texture, Rect2i dest, V2i source) {
    DrawTexturePro(
        texture,
        (Rectangle) {
            0,
            (float)(texture.height - source.y),
            (float)source.x,
            (float)(-source.y)
        },
        (Rect2i_to_Rect2(dest)).rect,
        Vector2Zero(), 0, WHITE
    );
}

/*
   * AVOID USING THIS. This works on my machine but supposedly this should
   break on other hardware.
   * Under OpenGL there is no concept of integer coordinates, so to get closest
   to pixel perfect you need to add 0.5.
   * See https://stackoverflow.com/questions/10040961
   And https://github.com/raysan5/raylib/issues/2744
*/
void DrawLineFixed(V2i from, V2i to, float thick, Color tint) {
    DrawLineEx(
        Vector2Add(v2i_to_Vector2(from), (Vector2){0.5,0.5}),
        Vector2Add(v2i_to_Vector2(to), (Vector2){0.5,0.5}),
        thick, tint);
}


Rect2i Rect2i_add_padding(Rect2i r, int top, int right, int bottom, int left) {
    return (Rect2i) {{ r.x + left, r.y + top, r.width - left - right, r.height - top - bottom }};
}

Rect2i Rect2i_add_padding_all(Rect2i r, int pad) {
    return (Rect2i) {{ r.x + pad, r.y + pad, r.width - pad*2, r.height - pad*2 }};
}

Rect2i Rect2i_from_two_positions(V2i a, V2i b) {
    int min_x = int_min(a.x, b.x);
    int min_y = int_min(a.y, b.y);
    int max_x = int_max(a.x, b.x);
    int max_y = int_max(a.y, b.y);
    return (Rect2i) {
        .x = min_x,
        .y = min_y,
        .width = max_x - min_x,
        .height = max_y - min_y
    };
}

bool Rect2i_collides_Rect2i (Rect2i lhs, Rect2i rhs) {
    int left1   = lhs.pos.x;
    int right1  = lhs.pos.x + lhs.size.x;
    int top1    = lhs.pos.y;
    int bottom1 = lhs.pos.y + lhs.size.y;
    int left2   = rhs.pos.x;
    int right2  = rhs.pos.x + rhs.size.x;
    int top2    = rhs.pos.y;
    int bottom2 = rhs.pos.y + rhs.size.y;
    return ! (
        left1 > right2 || left2 > right1 ||
        top1 > bottom2 || top2 > bottom1
    );
}


#endif // !RAYLIB_EXTRA
