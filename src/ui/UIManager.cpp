// Project includes
#include "ui/UIManager.h"

// System includes
#include <sstream>

namespace media_player 
{
namespace ui 
{

UIManager::UIManager()
    : m_window(nullptr)
    , m_renderer(nullptr)
    , m_font(nullptr)
    , m_fontLarge(nullptr)
    , m_fontSmall(nullptr)
    , m_width(0)
    , m_height(0)
{
}

UIManager::~UIManager() 
{
    shutdown();
}

bool UIManager::initialize(const std::string& title, int width, int height) 
{
    m_width = width;
    m_height = height;
    
    // Initialize SDL_ttf
    if (TTF_Init() < 0) 
    {
        return false;
    }
    
    // Create window
    m_window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!m_window) 
    {
        return false;
    }
    
    // Create renderer
    m_renderer = SDL_CreateRenderer(
        m_window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!m_renderer) 
    {
        return false;
    }
    
    // Enable alpha blending
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    
    // Load built-in font fallback (using SDL primitives if no font)
    // Try to load system font
    const char* fontPaths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf"
    };
    
    for (const char* path : fontPaths) 
    {
        m_font = TTF_OpenFont(path, 16);
        if (m_font) 
        {
            m_fontLarge = TTF_OpenFont(path, 24);
            m_fontSmall = TTF_OpenFont(path, 12);
            break;
        }
    }
    
    if (!m_font) 
    {
    }
    return true;
}

void UIManager::shutdown() 
{
    if (m_fontSmall) 
    {
        TTF_CloseFont(m_fontSmall);
        m_fontSmall = nullptr;
    }
    
    if (m_fontLarge) 
    {
        TTF_CloseFont(m_fontLarge);
        m_fontLarge = nullptr;
    }
    
    if (m_font) 
    {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }
    
    if (m_renderer) 
    {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    
    if (m_window) 
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    TTF_Quit();
}

void UIManager::beginFrame() 
{
    clear();
}

void UIManager::endFrame() 
{
    SDL_RenderPresent(m_renderer);
}

void UIManager::clear() 
{
    Color bg = Color::background();
    SDL_SetRenderDrawColor(m_renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderClear(m_renderer);
}

void UIManager::drawRect(int x, int y, int w, int h, const Color& color, bool filled) 
{
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {x, y, w, h};
    
    if (filled) 
    {
        SDL_RenderFillRect(m_renderer, &rect);
    }
    else 
    {
        SDL_RenderDrawRect(m_renderer, &rect);
    }
}

void UIManager::drawText(const std::string& text, int x, int y, const Color& color, int fontSize) 
{
    if (!m_font || text.empty()) 
    {
        return;
    }
    
    TTF_Font* font = m_font;
    if (fontSize >= 20 && m_fontLarge) 
    {
        font = m_fontLarge;
    }
    else if (fontSize <= 12 && m_fontSmall) 
    {
        font = m_fontSmall;
    }
    
    SDL_Color sdlColor = {color.r, color.g, color.b, color.a};
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), sdlColor);
    
    if (!surface) 
    {
        return;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    
    if (texture) 
    {
        SDL_Rect destRect = {x, y, surface->w, surface->h};
        SDL_RenderCopy(m_renderer, texture, nullptr, &destRect);
        SDL_DestroyTexture(texture);
    }
    
    SDL_FreeSurface(surface);
}

void UIManager::drawButton(const std::string& text, int x, int y, int w, int h, bool hover) 
{
    Color bg = hover ? Color::highlight() : Color::secondary();
    drawRect(x, y, w, h, bg, true);
    drawRect(x, y, w, h, Color::primary(), false);
    
    // Center text in button
    if (m_font) 
    {
        int textW, textH;
        TTF_SizeUTF8(m_font, text.c_str(), &textW, &textH);
        int textX = x + (w - textW) / 2;
        int textY = y + (h - textH) / 2;
        drawText(text, textX, textY, Color::text(), 16);
    }
}

void UIManager::drawProgressBar(int x, int y, int w, int h, float progress, const Color& fg, const Color& bg) 
{
    // Background
    drawRect(x, y, w, h, bg, true);
    
    // Progress
    int progressWidth = static_cast<int>(w * progress);
    if (progressWidth > 0) 
    {
        drawRect(x, y, progressWidth, h, fg, true);
    }
    
    // Border
    drawRect(x, y, w, h, Color::primary(), false);
}

void UIManager::drawMediaItem(const models::MediaFileModel& media, int x, int y, int w, int h, 
                              bool selected, bool playing) 
{
    // Background
    Color bg = selected ? Color::highlight() : Color::secondary();
    drawRect(x, y, w, h, bg, true);
    
    // Playing indicator
    if (playing) 
    {
        drawRect(x, y, 4, h, Color::success(), true);
    }
    
    // Media type icon area
    int iconX = x + 10;
    Color iconColor = media.isAudio() ? Color::primary() : Color::warning();
    drawRect(iconX, y + 8, 30, h - 16, iconColor, true);
    
    std::string typeLabel = media.isAudio() ? "A" : "V";
    drawText(typeLabel, iconX + 10, y + 12, Color::text(), 16);
    
    // Filename
    int textX = x + 50;
    drawText(media.getFileName(), textX, y + 10, Color::text(), 16);
    
    // File size
    std::stringstream ss;
    float sizeMB = static_cast<float>(media.getFileSize()) / (1024 * 1024);
    ss.precision(2);
    ss << std::fixed << sizeMB << " MB";
    drawText(ss.str(), x + w - 100, y + 10, Color::textDim(), 12);
}

void UIManager::drawHeader(const std::string& title, int itemCount) 
{
    // Header background
    drawRect(0, 0, m_width, 60, Color::secondary(), true);
    
    // Title
    drawText(title, 20, 15, Color::text(), 24);
    
    // Item count
    std::string countText = std::to_string(itemCount) + " items";
    drawText(countText, m_width - 150, 20, Color::textDim(), 16);
}

void UIManager::drawStatusBar(const std::string& status) 
{
    int y = m_height - 30;
    
    // Background
    drawRect(0, y, m_width, 30, Color::secondary(), true);
    
    // Status text
    drawText(status, 20, y + 5, Color::textDim(), 12);
}

void UIManager::drawScanProgress(const std::string& path, int current, int total) 
{
    int centerY = m_height / 2 - 50;
    
    // Box background
    int boxW = 500;
    int boxH = 120;
    int boxX = (m_width - boxW) / 2;
    int boxY = centerY;
    
    drawRect(boxX, boxY, boxW, boxH, Color::secondary(), true);
    drawRect(boxX, boxY, boxW, boxH, Color::primary(), false);
    
    // Title
    drawText("Scanning Media Files...", boxX + 20, boxY + 15, Color::text(), 20);
    
    // Path
    std::string shortPath = path;
    if (shortPath.length() > 50) 
    {
        shortPath = "..." + shortPath.substr(shortPath.length() - 47);
    }
    drawText(shortPath, boxX + 20, boxY + 45, Color::textDim(), 12);
    
    // Progress bar
    float progress = total > 0 ? static_cast<float>(current) / total : 0;
    drawProgressBar(boxX + 20, boxY + 70, boxW - 40, 20, progress, Color::primary(), Color::background());
    
    // Count
    std::string countText = std::to_string(current) + " / " + std::to_string(total) + " files";
    drawText(countText, boxX + 20, boxY + 95, Color::textDim(), 12);
}

bool UIManager::loadFont(const std::string& path, int size) 
{
    TTF_Font* font = TTF_OpenFont(path.c_str(), size);
    
    if (font) 
    {
        if (m_font) 
        {
            TTF_CloseFont(m_font);
        }
        m_font = font;
        return true;
    }
    
    return false;
}

} // namespace ui
} // namespace media_player
