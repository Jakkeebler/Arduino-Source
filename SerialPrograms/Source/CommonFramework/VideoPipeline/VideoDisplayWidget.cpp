/*  Video Display
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <QResizeEvent>
#include "VideoDisplayWidget.h"

#include <iostream>
using std::cout;
using std::endl;

namespace PokemonAutomation{


VideoDisplayWidget::VideoDisplayWidget(QWidget& parent)
    : QWidget(&parent)
    , m_overlay(new VideoOverlayWidget(*this))
{}

void VideoDisplayWidget::close_video(){
    if (m_video != nullptr){
        delete m_video;
        m_video = nullptr;
    }
    update_size();
}
void VideoDisplayWidget::set_video(VideoWidget* video){
    if (m_video != nullptr){
        delete m_video;
        m_video = nullptr;
    }
    m_video = video;
    video->setParent(this);
    video->setVisible(true);
    m_overlay->setVisible(true);
    m_overlay->setHidden(false);
    m_overlay->raise();
    update_size();
}
void VideoDisplayWidget::set_video(std::function<VideoWidget*(QWidget& parent)> video_factory){
    if (m_video != nullptr){
        delete m_video;
        m_video = nullptr;
    }
    m_video = video_factory(*this);
    m_video->setVisible(true);
    m_overlay->setVisible(true);
    m_overlay->setHidden(false);
    m_overlay->raise();
    update_size();
}

Resolution VideoDisplayWidget::resolution() const{
    if (m_video == nullptr){
        return Resolution();
    }
    return m_video->camera().current_resolution();
}
std::vector<Resolution> VideoDisplayWidget::resolutions() const{
    if (m_video == nullptr){
        return {};
    }
    return m_video->camera().supported_resolutions();
}
void VideoDisplayWidget::set_resolution(const Resolution& resolution){
    if (m_video == nullptr){
        return;
    }
    m_video->camera().set_resolution(resolution);
    update_size(resolution);
}
VideoSnapshot VideoDisplayWidget::snapshot(){
    if (m_video == nullptr){
        return VideoSnapshot();
    }
    return m_video->camera().snapshot();
}



void VideoDisplayWidget::add_box(const ImageFloatBox& box, Color color){
    m_overlay->add_box(box, color);
}
void VideoDisplayWidget::remove_box(const ImageFloatBox& box){
    m_overlay->remove_box(box);
}



void VideoDisplayWidget::update_size(Resolution resolution){
    if (m_video == nullptr){
        this->setFixedHeight(45);
        return;
    }
    int width = this->width();
    double aspect_ratio = 16. / 9;
    if (!resolution){
        Camera* camera = &m_video->camera();
        resolution = camera->current_resolution();
    }
//    cout << "resolution: " << resolution.width() << " x " << resolution.height() << endl;
    if (resolution){
        aspect_ratio = (double)resolution.height / resolution.width;
        int height = (int)(width * aspect_ratio);
        this->setFixedHeight(height);
    }
    m_video->setFixedSize(this->size());
    m_overlay->setFixedSize(this->size());
    m_overlay->update_size(this->size(), this->size());
}
void VideoDisplayWidget::resizeEvent(QResizeEvent* event){
    QWidget::resizeEvent(event);
//    cout << "resizeEvent(): " << this->width() << " x " << this->height() << endl;

    int width = this->width();

    //  Safeguard against a resizing loop where the UI bounces between larger
    //  height with scroll bar and lower height with no scroll bar.
    auto iter = m_recent_widths.find(width);
    if (iter != m_recent_widths.end() && std::abs(width - event->oldSize().width()) < 50){
//        cout << "Supressing potential infinite resizing loop." << endl;
        return;
    }

    m_width_history.push_back(width);
    m_recent_widths.insert(width);
    if (m_width_history.size() > 10){
        m_recent_widths.erase(m_width_history[0]);
        m_width_history.pop_front();
    }

    update_size();
}




}
