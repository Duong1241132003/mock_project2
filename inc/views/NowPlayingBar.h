#ifndef NOW_PLAYING_BAR_H
#define NOW_PLAYING_BAR_H

// System includes
#include <memory>
#include <string>

// Project includes
#include "IView.h"
#include "models/PlaybackStateModel.h"
#include "controllers/PlaybackController.h"

namespace media_player 
{
namespace views 
{

class NowPlayingBar : public IView 
{
public:
    NowPlayingBar(
        std::shared_ptr<models::PlaybackStateModel> playbackStateModel,
        std::shared_ptr<controllers::PlaybackController> playbackController
    );
    
    ~NowPlayingBar();
    
    // IView interface
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    
    // User interactions
    void onPlayPauseClicked();
    void onStopClicked();
    void onNextClicked();
    void onPreviousClicked();
    void onVolumeChanged(int volume);
    void onSeekBarDragged(float percentage);
    
private:
    void updateDisplayInfo();
    void updateProgressBar();
    void updatePlayPauseButton();
    
    std::shared_ptr<models::PlaybackStateModel> m_playbackStateModel;
    std::shared_ptr<controllers::PlaybackController> m_playbackController;
    
    bool m_isVisible;
};

} // namespace views
} // namespace media_player

#endif // NOW_PLAYING_BAR_H