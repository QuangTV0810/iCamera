#include "aiotek_audio.hpp"
#include "aiotek_logger.hpp"

namespace AIOTEK {

class DummyAudioDevice : public AudioDevice {
private:
    bool initialized;
    bool capturing;
    bool playing;
    int sampleRate;
    int channels;
    int bitsPerSample;

public:
    DummyAudioDevice() : initialized(false), capturing(false), playing(false),
                        sampleRate(44100), channels(2), bitsPerSample(16) {}
    
    bool initialize() override {
        AIOTEK_LOG_INFO("DummyAudioDevice: Initializing");
        initialized = true;
        return true;
    }
    
    void shutdown() override {
        AIOTEK_LOG_INFO("DummyAudioDevice: Shutting down");
        initialized = false;
        capturing = false;
        playing = false;
    }
    
    bool isInitialized() const override {
        return initialized;
    }
    
    bool startCapture() override {
        if (!initialized) return false;
        AIOTEK_LOG_INFO("DummyAudioDevice: Starting capture");
        capturing = true;
        return true;
    }
    
    void stopCapture() override {
        AIOTEK_LOG_INFO("DummyAudioDevice: Stopping capture");
        capturing = false;
    }
    
    bool isCapturing() const override {
        return capturing;
    }
    
    bool startPlayback() override {
        if (!initialized) return false;
        AIOTEK_LOG_INFO("DummyAudioDevice: Starting playback");
        playing = true;
        return true;
    }
    
    void stopPlayback() override {
        AIOTEK_LOG_INFO("DummyAudioDevice: Stopping playback");
        playing = false;
    }
    
    bool isPlaying() const override {
        return playing;
    }
    
    int getSampleRate() const override {
        return sampleRate;
    }
    
    int getChannels() const override {
        return channels;
    }
    
    int getBitsPerSample() const override {
        return bitsPerSample;
    }
};

AudioManager::AudioManager() : initialized(false) {
    device = std::make_unique<DummyAudioDevice>();
}

AudioManager::~AudioManager() {
    shutdown();
}

bool AudioManager::initialize() {
    if (initialized) return true;
    
    if (device->initialize()) {
        initialized = true;
        AIOTEK_LOG_INFO("AudioManager: Initialized successfully");
        return true;
    }
    
    AIOTEK_LOG_ERROR("AudioManager: Failed to initialize");
    return false;
}

void AudioManager::shutdown() {
    if (!initialized) return;
    
    device->shutdown();
    initialized = false;
    AIOTEK_LOG_INFO("AudioManager: Shutdown completed");
}

bool AudioManager::isInitialized() const {
    return initialized;
}

bool AudioManager::startCapture() {
    if (!initialized) return false;
    return device->startCapture();
}

void AudioManager::stopCapture() {
    if (initialized) {
        device->stopCapture();
    }
}

bool AudioManager::isCapturing() const {
    return initialized && device->isCapturing();
}

bool AudioManager::startPlayback() {
    if (!initialized) return false;
    return device->startPlayback();
}

void AudioManager::stopPlayback() {
    if (initialized) {
        device->stopPlayback();
    }
}

bool AudioManager::isPlaying() const {
    return initialized && device->isPlaying();
}

bool AudioManager::processAudio(const std::vector<uint8_t>& data) {
    if (!initialized) return false;
    
    AIOTEK_LOG_DEBUG("AudioManager: Processing " + std::to_string(data.size()) + " bytes");
    return true;
}

std::vector<uint8_t> AudioManager::getAudioData() {

    std::vector<uint8_t> dummyData(1024, 0);
    return dummyData;
}

void AudioManager::setSampleRate(int sampleRate) {
    AIOTEK_LOG_INFO("AudioManager: Setting sample rate to " + std::to_string(sampleRate));
}

void AudioManager::setChannels(int channels) {
    AIOTEK_LOG_INFO("AudioManager: Setting channels to " + std::to_string(channels));
}

void AudioManager::setBitsPerSample(int bitsPerSample) {
    AIOTEK_LOG_INFO("AudioManager: Setting bits per sample to " + std::to_string(bitsPerSample));
}

} // namespace AIOTEK
