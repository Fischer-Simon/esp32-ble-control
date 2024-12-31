#pragma once

#include "HslwColor.h"

#include <LightweightMap.h>
#include <NeoPixelBus.h>
#include <string>

namespace Led {
class ColorManager {
public:
    void loadColorsFromConfig(const std::string& namedColorPath);

    HslwColor parseColor(const std::string& colorString, const HslwColor& primaryColor = {{0, 0, 0}, 0, 0}) const;
private:
    LightweightMap<HslwColor> m_namedColors;
};
}
