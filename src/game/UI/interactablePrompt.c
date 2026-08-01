#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "interactablePrompt.h"

#include "globals/globals.h"
#include "globals/localization.h"
#include "game/rendering/textures.h"
#include "game/settings/settings.h"

static const StringKey INTERACTABLE_PROMPT_STRING_KEYS[INTERACTABLE_PROMPT_COUNT] = {
    [INTERACTABLE_PROMPT_NEXT_FLOOR] = STR_PROMPT_NEXT_FLOOR,
};

static LCDBitmap *interactablePromptBitmaps[INTERACTABLE_PROMPT_COUNT][LANGUAGE_COUNT];

static LCDBitmap *renderInteractablePrompt(InteractablePrompt prompt, Language language) {
    const char *text = getLocStringFor(INTERACTABLE_PROMPT_STRING_KEYS[prompt], language);
    LCDFont *font = (language == LANGUAGE_JP) ? fontTitleJP : fontTitleEN;
    int len = (int)strlen(text);

    int tracking = (language == LANGUAGE_JP) ? -1 : 0;
    int width = pd->graphics->getTextWidth(font, text, len, kUTF8Encoding, tracking);
    int height = pd->graphics->getFontHeight(font);

    LCDBitmap *bitmap = pd->graphics->newBitmap(width, height, kColorClear);
    pd->graphics->pushContext(bitmap);
        pd->graphics->setFont(font);
        pd->graphics->setTextTracking(tracking);
        pd->graphics->drawText(text, len, kUTF8Encoding, 0, 0);
    pd->graphics->popContext();
    return bitmap;
}

int getInteractablePromptAssetCount(void) {
    return INTERACTABLE_PROMPT_COUNT;
}

void buildInteractablePrompt(int step) {
    InteractablePrompt prompt = (InteractablePrompt)step;
    for (Language language = 0; language < LANGUAGE_COUNT; language++) {
        interactablePromptBitmaps[prompt][language] = renderInteractablePrompt(prompt, language);
    }
}

LCDBitmap *getInteractablePromptBitmap(InteractablePrompt prompt) {
    return interactablePromptBitmaps[prompt][SETTINGS.language];
}
