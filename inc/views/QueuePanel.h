#ifndef QUEUE_PANEL_H
#define QUEUE_PANEL_H

// System includes
#include <memory>

// Project includes
#include "IView.h"
#include "controllers/QueueController.h"
#include "models/QueueModel.h"

namespace media_player 
{
namespace views 
{

class QueuePanel : public IView 
{
public:
    QueuePanel(
        std::shared_ptr<controllers::QueueController> queueController,
        std::shared_ptr<models::QueueModel> queueModel
    );
    
    ~QueuePanel();
    
    // IView interface
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    
    // Queue actions
    void removeSelectedItem();
    void clearQueue();
    void moveItemUp(size_t index);
    void moveItemDown(size_t index);
    void jumpToItem(size_t index);
    
    // Mode toggles
    void toggleShuffle();
    void toggleRepeat();
    
private:
    void displayQueue();
    
    std::shared_ptr<controllers::QueueController> m_queueController;
    std::shared_ptr<models::QueueModel> m_queueModel;
    
    bool m_isVisible;
};

} // namespace views
} // namespace media_player

#endif // QUEUE_PANEL_H