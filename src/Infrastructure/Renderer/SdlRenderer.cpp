#include "src/Infrastructure/Renderer/SdlRenderer.h"

using Infrastructure::SdlRenderer;

#include "src/Infrastructure/Renderer/RendererConfigFactory.h"

using Infrastructure::RendererConfigFactory;



bool SdlRenderer::Initialize() {
    renderer = SDL_CreateRenderer(
        (SDL_Window *)config->window->GetApiHandle(),
        -1,
        SDL_RENDERER_ACCELERATED
    );

    if (renderer == nullptr) {
        log->Info("SdlRenderer::Initialize: %s", SDL_GetError());
        return nullptr;
    }

    if (config->fullscreen) {
        config->window->SetFullscreen();
        config = RendererConfigFactory().Create(
            config->renderer,
            config->window,
            config->window->GetWidth(),
            config->window->GetHeight(),
            config->fullscreen
        );
    } else {
        config->window->SetWidth(config->width);
        config->window->SetHeight(config->height);
    }

    return true;
}

void SdlRenderer::ClearTarget(int r, int g, int b) {
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    unsigned short cw;
#ifdef _MSC_VER
    __asm { fnstcw cw }
    __asm { fnclex }
#else
    __asm__ __volatile__ ("fnstcw %0" : "=m" (cw) : : "memory");
    __asm__ __volatile__ ("fnclex" : : : "memory");
#endif
    unsigned short safe_cw = (cw & 0xFFC0) | 0x003F;
#ifdef _MSC_VER
    __asm { fldcw safe_cw }
    SDL_RenderClear(renderer);
    __asm { fldcw cw }
#else
    __asm__ __volatile__ ("fldcw %0" : : "m" (safe_cw) : "memory");
    SDL_RenderClear(renderer);
    __asm__ __volatile__ ("fldcw %0" : : "m" (cw) : "memory");
#endif
}

void SdlRenderer::Present() {
    SDL_RenderPresent(renderer);
}

void SdlRenderer::DrawImageCentered(int imageWidth, int imageHeight, const void *imagePixels) {
    int windowWidth = config->window->GetWidth();
    int windowHeight = config->window->GetHeight();
    int draw_x = windowWidth / 2 - imageWidth / 2;
    int draw_y = windowHeight / 2 - imageHeight / 2;

    auto surface = SDL_CreateRGBSurfaceFrom(
        (void *)imagePixels, imageWidth, imageHeight, 32, 4 * imageWidth,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
    );

    if (surface == nullptr) {
        log->Info("SdlRenderer::DrawImageCentered: Creating surface failed: %s", SDL_GetError());
    } else {
        auto texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        if (texture == nullptr) {
            log->Info("SdlRenderer::DrawImageCentered: Creating texture failed: %s", SDL_GetError());
        } else {
            SDL_Rect dstRect = { draw_x, draw_y, imageWidth, imageHeight };
            SDL_RenderCopy(renderer, texture, nullptr, &dstRect);

            SDL_DestroyTexture(texture);
        }
    }
}