/*
 * The Blue Brain BioExplorer is a tool for scientists to extract and analyse
 * scientific data from visualization
 *
 * Copyright 2020-2021 Blue BrainProject / EPFL
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ShadowRenderer.h"

// ospray
#include <ospray/SDK/lights/Light.h>

// ispc exports
#include "ShadowRenderer_ispc.h"

using namespace ospray;

namespace bioexplorer
{
namespace mediamaker
{
namespace rendering
{
void ShadowRenderer::commit()
{
    Renderer::commit();

    _lightData = (ospray::Data*)getParamData("lights");
    _lightArray.clear();

    if (_lightData)
        for (size_t i = 0; i < _lightData->size(); ++i)
            _lightArray.push_back(
                ((ospray::Light**)_lightData->data)[i]->getIE());
    _lightPtr = _lightArray.empty() ? nullptr : &_lightArray[0];

    _softness = getParam1f("softness", 0.f);
    _samplesPerFrame = getParam1i("samplesPerFrame", 16);
    _rayLength = getParam1f("rayLength", 1e6f);

    ispc::ShadowRenderer_set(getIE(), spp, _lightPtr, _lightArray.size(),
                             _samplesPerFrame, _rayLength, _softness);
}

ShadowRenderer::ShadowRenderer()
{
    ispcEquivalent = ispc::ShadowRenderer_create(this);
}

OSP_REGISTER_RENDERER(ShadowRenderer, shadow);
} // namespace rendering
} // namespace mediamaker
} // namespace bioexplorer