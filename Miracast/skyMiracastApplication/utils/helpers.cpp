#include "helpers.hpp"

/**
 * @brief This method used to Get the value of the given key from the device.properties file.
 *
 * @param: key and value reference pointer.
 * @return Returns the true or false.
 */
bool GetValueFromDeviceProperties(const char *key, std::string &value)
{
    std::ifstream fs(MIRACAST_DEVICE_PROPERTIES_FILE, std::ifstream::in);
    std::string::size_type delimpos;
    std::string line;
    if (!fs.fail())
    {
        while (std::getline(fs, line))
        {
            if (!line.empty() && ((delimpos = line.find('=')) > 0))
            {
                std::string itemKey = line.substr(0, delimpos);
                if (itemKey == key)
                {
                    value = line.substr(delimpos + 1, std::string::npos);
                    return true;
                }
            }
        }
    }
    return false;
}