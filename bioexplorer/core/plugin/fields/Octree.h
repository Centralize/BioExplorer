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

#include <brayns/common/types.h>

#include "OctreeNode.h"

namespace bioexplorer
{
namespace fields
{
/**
 * @brief The Octree class implements the Octree acceleration structure used by
 * the FieldsRenderer class to render magnetic fields
 *
 */
class Octree
{
public:
    /**
     * @brief Construct a new Octree object
     *
     * @param events Events used to build the tree. Events contain x, y, z
     * coordinates, as well as a radius, and a value
     * @param voxelSize Voxel size
     * @param minAABB Lower bound of the scene bounding box
     * @param maxAABB Upper bound of the scene bounding box
     */
    Octree(const floats &events, float voxelSize, const glm::vec3 &minAABB,
           const glm::vec3 &maxAABB);

    /**
     * @brief Destroy the Octree object
     *
     */
    ~Octree();

    /**
     * @brief Get the volume dimentions defined by the scene and the voxel sizes
     *
     * @return The dimensions of the volume
     */
    const glm::uvec3 &getVolumeDim() const;

    /**
     * @brief Get the size of the volume
     *
     * @return The size of the volume
     */
    const uint64_t getVolumeSize() const;

    /**
     * @brief Get the size of the Octree
     *
     * @return The size of the Octree
     */
    const uint32_t getOctreeSize() const;

    /**
     * @brief Get a flattened representation of the Octree indices
     *
     * @return A flattened representation of the Octree indices
     */
    const uint32_ts &getFlatIndexes() const;

    /**
     * @brief Get a flattened representation of the Octree data (node values)
     *
     * @return A flattened representation of the Octree data (node values)
     */
    const floats &getFlatData() const;

private:
    void _flattenChildren(const OctreeNode *node, uint32_t level);

    inline uint32_t _pow2roundup(uint32_t x)
    {
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return x + 1;
    }

    glm::uvec3 _volumeDim;
    uint64_t _volumeSize;
    uint32_t _octreeSize;

    uint32_t *_offsetPerLevel;

    uint32_ts _flatIndexes;
    floats _flatData;
};
} // namespace fields
} // namespace bioexplorer