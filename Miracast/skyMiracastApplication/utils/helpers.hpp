#ifndef _HELPERS_H_
#define _HELPERS_H_

#include <fstream>
#include <cstdint>
#include <string>

#define MIRACAST_DEVICE_PROPERTIES_FILE     "/etc/device.properties"

typedef struct videoRectSt
{
    int startX;
    int startY;
    int width;
    int height;
}
VideoRectangleInfo;

/**
 * @brief This method used to Get the value of the given key from the device.properties file.
 *
 * @param: key and value reference pointer.
 * @return Returns the true or false.
 */
bool GetValueFromDeviceProperties(const char *key, std::string &value);

#endif // _HELPERS_H_