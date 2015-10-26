#pragma once
#include "Includes.h"

#define DEFAULT_FRAMERATE 30

class CameraManager
{
public:
    CameraManager() {}

    static CameraManager* instance()
    {
        static CameraManager ine;
        return &ine;
    }

    void TakePicture(std::string filename, uint32_t delay);
    void RecordVideo(std::string filename, uint32_t duration, uint32_t framerate = 0);
};