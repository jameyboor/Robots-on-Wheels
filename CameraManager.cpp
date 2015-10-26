#include "CameraManager.h"

void CameraManager::TakePicture(std::string filename, uint32_t delay)
{
    if (filename.empty())
        return;

    std::ostringstream ss;
    ss << "raspistill -t " << delay << " -o " << filename << ".jpg &";
    system(ss.str().c_str());
    Log(LOG_LEVEL_INFO, "Taking picture.. picture will be saved as : %s in %f seconds", (filename).c_str(), (float)delay / 1000.0f);
    Timer timer(delay, true, [] {
        Log(LOG_LEVEL_INFO, "Picture taken.");
    });
}

void CameraManager::RecordVideo(std::string filename, uint32_t duration, uint32_t framerate)
{
    if (!framerate)
        framerate = DEFAULT_FRAMERATE;

    std::ostringstream ss;
    ss << "raspivid -t " << duration << " -o " << filename << ".h264" << " -fps " << framerate << " &";
    system(ss.str().c_str());
    Log(LOG_LEVEL_INFO, "Recording video.. video will be saved as : %s, duration of recording will be %f", (filename).c_str(), (float)duration / 1000.0f);
    Timer timer(duration, true, [] {
        Log(LOG_LEVEL_INFO, "Video recorded.");
    });
}
