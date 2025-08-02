#include <iostream>
#include <thread>
#include <chrono>
#include "utils/aiotek_log.hpp"
#include "common/aiotek_timer.hpp"
#include "module/video/aiotek_video.hpp"

namespace AIOTEK {

class VideoTask {
private:
    bool running;
    std::thread taskThread;
    Timer timer;
    VideoManager videoManager;

public:
    VideoTask() : running(false) {}
    
    ~VideoTask() {
        stop();
    }
    
    bool start() {
        if (running) return true;
        
        AIOTEK_LOG_INFO("VideoTask: Starting");
        
        VideoConfig config;
        config.width = 640;
        config.height = 480;
        config.fps = 30;
        
        if (!videoManager.initialize(config)) {
            AIOTEK_LOG_ERROR("VideoTask: Failed to initialize video manager");
            return false;
        }
        
        videoManager.setFrameCallback([this](const VideoFrame& frame) {
            this->onFrameReceived(frame);
        });
        
        running = true;
        taskThread = std::thread(&VideoTask::run, this);
        return true;
    }
    
    void stop() {
        if (!running) return;
        
        AIOTEK_LOG_INFO("VideoTask: Stopping");
        running = false;
        
        if (taskThread.joinable()) {
            taskThread.join();
        }
        
        videoManager.shutdown();
    }
    
    bool state() const {
        return running;
    }

private:
    void run() {
        AIOTEK_LOG_INFO("VideoTask: Thread started");
        timer.start();

        if (!videoManager.startCapture()) {
            AIOTEK_LOG_ERROR("VideoTask: Failed to start video capture");
            return;
        }
        
        while (running) {

            processVideo();
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }
        
        videoManager.stopCapture();
        
        timer.stop();
        AIOTEK_LOG_INFO("VideoTask: Thread stopped after " + timer.getElapsedString());
    }
    
    void processVideo() {
        if (videoManager.hasFrame()) {
            VideoFrame frame = videoManager.getFrame();
            videoManager.processFrame(frame);
        }
    }
    
    void onFrameReceived(const VideoFrame& frame) {
        static int frameCounter = 0;
        if (++frameCounter % 30 == 0) {
            AIOTEK_LOG_DEBUG("VideoTask: Received frame " + 
                           std::to_string(frame.width) + "x" + std::to_string(frame.height));
        }
    }
};

} // namespace AIOTEK
