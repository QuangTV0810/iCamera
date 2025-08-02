#include <iostream>
#include <thread>
#include <chrono>
#include "utils/aiotek_log.hpp"
#include "common/aiotek_timer.hpp"
#include "module/audio/aiotek_audio.hpp"

namespace AIOTEK {

class AudioTask {
  private:
    bool running;
    std::thread taskThread;
    Timer timer;
    AudioManager audioManager;

  public:
    AudioTask() : running(false)
    {
    }

    ~AudioTask()
    {
        stop();
    }

    bool start()
    {
        if (running)
            return true;

        AIOTEK_LOG_INFO("AudioTask: Starting");

        if (!audioManager.initialize()) {
            AIOTEK_LOG_ERROR("AudioTask: Failed to initialize audio manager");
            return false;
        }

        running = true;
        taskThread = std::thread(&AudioTask::run, this);
        return true;
    }

    void stop()
    {
        if (!running)
            return;

        AIOTEK_LOG_INFO("AudioTask: Stopping");
        running = false;

        if (taskThread.joinable()) {
            taskThread.join();
        }

        audioManager.shutdown();
    }

    bool state() const
    {
        return running;
    }

  private:
    void run()
    {
        AIOTEK_LOG_INFO("AudioTask: Thread started");
        timer.start();

        if (!audioManager.startCapture()) {
            AIOTEK_LOG_ERROR("AudioTask: Failed to start audio capture");
            return;
        }

        while (running) {
            processAudio();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        audioManager.stopCapture();

        timer.stop();
        AIOTEK_LOG_INFO("AudioTask: Thread stopped after " + timer.getElapsedString());
    }

    void processAudio()
    {
        auto audioData = audioManager.getAudioData();
        if (!audioData.empty()) {
            audioManager.processAudio(audioData);

            static int counter = 0;
            if (++counter % 1000 == 0) {
                AIOTEK_LOG_DEBUG("AudioTask: Processed " + std::to_string(audioData.size()) + " bytes");
            }
        }
    }
};

} // namespace AIOTEK
