#ifndef UI_MANAGER_H
#define UI_MANAGER_H

// System includes
#include <memory>
#include <string>
#include <vector>
#include <functional>

// SDL includes
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// Project includes
#include "models/MediaFileModel.h"

namespace media_player 
{
namespace ui 
{

// Color definitions
struct Color 
{
    Uint8 r, g, b, a;
    
    static Color background() { return {30, 30, 40, 255}; }
    static Color primary() { return {100, 149, 237, 255}; }
    static Color secondary() { return {50, 50, 70, 255}; }
    static Color text() { return {255, 255, 255, 255}; }
    static Color textDim() { return {180, 180, 180, 255}; }
    static Color highlight() { return {70, 130, 180, 255}; }
    static Color success() { return {50, 205, 50, 255}; }
    static Color warning() { return {255, 165, 0, 255}; }
    static Color error() { return {220, 50, 50, 255}; }
};

class UIManager 
{
public:
    UIManager();
    ~UIManager();
    
    bool initialize(const std::string& title, int width, int height);
    void shutdown();
    
    // Rendering
    void beginFrame();
    void endFrame();
    void clear();
    
    // Drawing primitives
    void drawRect(int x, int y, int w, int h, const Color& color, bool filled = true);
    void drawText(const std::string& text, int x, int y, const Color& color, int fontSize = 16);
    void drawButton(const std::string& text, int x, int y, int w, int h, bool hover = false);
    void drawProgressBar(int x, int y, int w, int h, float progress, const Color& fg, const Color& bg);
    
    // Media list rendering
    void drawMediaItem(const models::MediaFileModel& media, int x, int y, int w, int h, 
                       bool selected = false, bool playing = false);
    void drawHeader(const std::string& title, int itemCount);
    void drawStatusBar(const std::string& status);
    void drawScanProgress(const std::string& path, int current, int total);
    
    // Getters
    SDL_Window* getWindow() const { return m_window; }
    SDL_Renderer* getRenderer() const { return m_renderer; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    
    // Font loading
    bool loadFont(const std::string& path, int size);
    
private:
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    TTF_Font* m_font;
    TTF_Font* m_fontLarge;
    TTF_Font* m_fontSmall;
    
    int m_width;
    int m_height;
};

} // namespace ui
} // namespace media_player

#endif // UI_MANAGER_H
