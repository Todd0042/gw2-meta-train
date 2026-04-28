#pragma once
#include <cstdint>
#include "Nexus.h"

extern Texture_t* g_AddonIcon;

// Called once per frame from the Nexus RT_Render callback.
void UIRender();

// Called from the Nexus RT_OptionsRender callback (shown in Nexus addon settings tab).
void UIRenderOptions();

// Called by dllmain when EV_MUMBLE_IDENTITY_UPDATED fires with a new map ID.
// If autoAdvanceOnMap is enabled and newMapId matches the next event's map, advances the loop.
void UIOnMapChanged(uint32_t newMapId);
