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

#pragma once

#include <plugin/common/Types.h>

namespace bioexplorer
{
using namespace brayns;

class OOCManager
{
public:
    OOCManager(Scene& scene, Camera& camera,
               const OutOfCoreDescriptor& descriptor);
    ~OOCManager() {}

    void loadBricks();

private:
    void _loadBricks();

    OutOfCoreDescriptor _descriptor;
    Scene& _scene;
    Camera& _camera;
    Vector3f _sceneSize;
    Vector3f _brickSize;
    std::set<int32_t> _loadedBricks;
    std::set<int32_t> _bricksToLoad;
    int32_t _currentBoxId{std::numeric_limits<int32_t>::max()};
};
} // namespace bioexplorer
