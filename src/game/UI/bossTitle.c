#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "bossTitle.h"

#include "globals/globals.h"
#include "globals/localization.h"
#include "game/rendering/textures.h"
#include "game/settings/settings.h"

static const StringKey BOSS_TITLE_STRING_KEYS[BOSS_TITLE_COUNT] = {
    [BOSS_TITLE_ELDER]  = STR_POPUP_ELDER_BOSS,
    [BOSS_TITLE_WRAITH] = STR_POPUP_WRAITH_BOSS,
    [BOSS_TITLE_DRAGON] = STR_POPUP_DRAGON_BOSS,
};

static LCDBitmap *bossTitleBitmaps[BOSS_TITLE_COUNT][LANGUAGE_COUNT];

static LCDBitmap *renderBossTitle(BossTitle title, Language language) {
    const char *text = getLocStringFor(BOSS_TITLE_STRING_KEYS[title], language);
    LCDFont *font = (language == LANGUAGE_JP) ? fontSubJPDoubleOutline : fontSubENDoubleOutline;
    int len = (int)strlen(text);

    int tracking = (language == LANGUAGE_JP) ? 0 : -3;
    int width = pd->graphics->getTextWidth(font, text, len, kUTF8Encoding, tracking);
    int height = pd->graphics->getFontHeight(font);

    LCDBitmap *bitmap = pd->graphics->newBitmap(width, height, kColorClear);
    pd->graphics->pushContext(bitmap);
        pd->graphics->setFont(font);
        pd->graphics->setTextTracking(tracking);
        pd->graphics->setDrawMode(kDrawModeCopy);
        pd->graphics->drawText(text, len, kUTF8Encoding, 0, 0);
    pd->graphics->popContext();
    return bitmap;
}

int getBossTitleAssetCount(void) {
    return BOSS_TITLE_COUNT;
}

void buildBossTitle(int step) {
    BossTitle title = (BossTitle)step;
    for (Language language = 0; language < LANGUAGE_COUNT; language++) {
        bossTitleBitmaps[title][language] = renderBossTitle(title, language);
    }
}

LCDBitmap *getBossTitleBitmap(BossTitle title) {
    return bossTitleBitmaps[title][SETTINGS.language];
}
