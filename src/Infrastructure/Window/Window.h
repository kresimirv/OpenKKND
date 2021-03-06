#pragma once

#include <memory>

#include "src/Infrastructure/Window/MouseObserver.h"
#include "src/Infrastructure/Window/KeyboardObserver.h"
#include "src/Infrastructure/Window/WindowConfig.h"
#include "src/Infrastructure/Window/WindowObserver.h"

namespace Infrastructure {
    class Window {
    public:
        inline Window(std::shared_ptr<const WindowConfig> config) {}
        virtual ~Window() {}

        // 
        virtual bool Initialize() = 0;
        virtual void AddObserver(std::shared_ptr<WindowObserver> observer) = 0;
        virtual void AddObserver(std::shared_ptr<MouseObserver> observer) = 0;
        virtual void AddObserver(std::shared_ptr<KeyboardObserver> observer) = 0;

        // dimensions
        virtual void SetWidth(int width) = 0;
        virtual int GetWidth() const = 0;
        virtual void SetHeight(int height) = 0;
        virtual int GetHeight() const = 0;

        // appearance
        virtual void SetFullscreen() = 0;
        virtual void SetWindowed() = 0;

        // behaviour
        virtual bool GetIsActive() const = 0;
        
        // messaging
        virtual void PeekMessageSingle() = 0;
        virtual void PeekMessageAll() = 0;
        virtual void WaitMessage() = 0;

        // input
        virtual int GetMouseX() const = 0;
        virtual int GetMouseY() const = 0;
        virtual void SetMousePos(int x, int y) = 0;
        virtual bool GetMousePressed(int button) const = 0;
        virtual bool GetIsKKNDKeyPressed(int kknd_key_mask) const = 0;
        virtual bool GetIsCharKeyPressed(char character) const = 0;

        // misc
        virtual void *GetApiHandle() const = 0;
        virtual void *GetHwnd() const = 0; // tmp windows hack
    };
};
