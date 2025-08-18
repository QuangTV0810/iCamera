#include "aiotek_video.hpp"
#include "aiotek_logger.hpp"
#include <chrono>

namespace AIOTEK {

class DummyVideoDevice : public VideoDevice {
private:
    bool initialized;
    bool capturing;
    VideoConfig config;
    uint64_t frameCounter;

public:
    DummyVideoDevice() : initialized(false), capturing(false), frameCounter(0) {}
    
    bool initialize(const VideoConfig& cfg) override {
        AIOTEK_LOG_INFO("DummyVideoDevice: Initializing with " + 
                       std::to_string(cfg.width) + "x" + std::to_string(cfg.height));
        config = cfg;
        initialized = true;
        return true;
    }
    
    void shutdown() override {
        AIOTEK_LOG_INFO("DummyVideoDevice: Shutting down");
        initialized = false;
        capturing = false;
    }
    
    bool isInitialized() const override {
        return initialized;
    }
    
    bool startCapture() override {
        if (!initialized) return false;
        AIOTEK_LOG_INFO("DummyVideoDevice: Starting capture");
        capturing = true;
        return true;
    }
    
    void stopCapture() override {
        AIOTEK_LOG_INFO("DummyVideoDevice: Stopping capture");
        capturing = false;
    }
    
    bool isCapturing() const override {
        return capturing;
    }
    
    VideoFrame getFrame() override {
        VideoFrame frame;
        if (!capturing) return frame;
        
        int frameSize = config.width * config.height * 2;
        frame.data.resize(frameSize, 0);
        
        for (size_t i = 0; i < frame.data.size(); i += 2) {
            frame.data[i] = static_cast<uint8_t>(frameCounter % 256);
            frame.data[i + 1] = static_cast<uint8_t>((frameCounter + i) % 256);
        }
        
        frame.width = config.width;
        frame.height = config.height;
        frame.channels = 2;
        frame.format = config.format;
        frame.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        frameCounter++;
        return frame;
    }
    
    bool hasFrame() const override {
        return capturing;
    }
    
    VideoConfig getConfig() const override {
        return config;
    }
    
    bool setConfig(const VideoConfig& cfg) override {
        if (capturing) return false;
        config = cfg;
        return true;
    }
};

VideoManager::VideoManager() : initialized(false) {
    device = std::make_unique<DummyVideoDevice>();
}

VideoManager::~VideoManager() {
    shutdown();
}

bool VideoManager::initialize(const VideoConfig& cfg) {
    if (initialized) return true;
    
    config = cfg;
    if (device->initialize(config)) {
        initialized = true;
        AIOTEK_LOG_INFO("VideoManager: Initialized successfully");
        return true;
    }
    
    AIOTEK_LOG_ERROR("VideoManager: Failed to initialize");
    return false;
}

void VideoManager::shutdown() {
    if (!initialized) return;
    
    device->shutdown();
    initialized = false;
    AIOTEK_LOG_INFO("VideoManager: Shutdown completed");
}

bool VideoManager::isInitialized() const {
    return initialized;
}

bool VideoManager::startCapture() {
    if (!initialized) return false;
    return device->startCapture();
}

void VideoManager::stopCapture() {
    if (initialized) {
        device->stopCapture();
    }
}

bool VideoManager::isCapturing() const {
    return initialized && device->isCapturing();
}

VideoFrame VideoManager::getFrame() {
    if (!initialized) return VideoFrame{};
    return device->getFrame();
}

bool VideoManager::hasFrame() const {
    return initialized && device->hasFrame();
}

void VideoManager::setFrameCallback(std::function<void(const VideoFrame&)> callback) {
    frameCallback = callback;
}

VideoConfig VideoManager::getConfig() const {
    return config;
}

bool VideoManager::setConfig(const VideoConfig& cfg) {
    if (!initialized) {
        config = cfg;
        return true;
    }
    
    if (device->setConfig(cfg)) {
        config = cfg;
        AIOTEK_LOG_INFO("VideoManager: Configuration updated");
        return true;
    }
    
    return false;
}

bool VideoManager::processFrame(const VideoFrame& frame) {
    if (!initialized) return false;
    
    AIOTEK_LOG_DEBUG("VideoManager: Processing frame " + 
                    std::to_string(frame.width) + "x" + std::to_string(frame.height));
    
    if (frameCallback) {
        frameCallback(frame);
    }
    
    return true;
}

bool VideoManager::saveFrame(const VideoFrame& frame, const std::string& filename) {
    if (!initialized) return false;
    (void)frame;
    AIOTEK_LOG_INFO("VideoManager: Saving frame to " + filename);
    return true;
}

} // namespace AIOTEK
