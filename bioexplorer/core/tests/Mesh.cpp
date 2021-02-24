/* Copyright (c) 2020-2021, EPFL/Blue Brain Project
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: cyrille.favreau@epfl.ch
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <plugin/bioexplorer/Mesh.h>
#include <plugin/bioexplorer/Protein.h>

#include <brayns/Brayns.h>
#include <brayns/engineapi/Engine.h>
#include <brayns/engineapi/Scene.h>

#define BOOST_TEST_MODULE mesh
#include <boost/test/unit_test.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

using namespace bioexplorer;

std::string getFileContents(const std::string& filename)
{
    std::ifstream file(filename);
    std::string str;
    if (file)
    {
        std::ostringstream ss;
        ss << file.rdbuf();
        str = ss.str();
    }
    else
        throw std::runtime_error("Failed to open " + filename);
    return str;
}

MeshDescriptor getDescriptor()
{
    MeshDescriptor descriptor;

    descriptor.assemblyName = "test";
    descriptor.name = "test";
    descriptor.meshContents =
        getFileContents("./python/tests/test_files/obj/capsule.obj");
    descriptor.proteinContents =
        getFileContents("./python/tests/test_files/pdb/membrane/popc.pdb");
    descriptor.recenter = true;
    descriptor.density = 100;
    descriptor.surfaceFixedOffset = 0.f;
    descriptor.surfaceVariableOffset = 0.f;
    descriptor.atomRadiusMultiplier = 1.f;
    descriptor.representation = ProteinRepresentation::atoms;
    descriptor.randomSeed = 0;
    descriptor.position = {0.f, 0.f, 0.f};
    descriptor.orientation = {0.f, 0.f, 0.f, 1.f};
    descriptor.scale = {0.f, 0.f, 0.f};
    return descriptor;
}

BOOST_AUTO_TEST_CASE(mesh)
{
    std::vector<const char*> argv{"brayns", "--http-server", "localhost:0",
                                  "--plugin", "BioExplorer"};
    brayns::Brayns brayns(argv.size(), argv.data());
    auto& scene = brayns.getEngine().getScene();
    Mesh mesh(scene, getDescriptor());

    BOOST_CHECK(mesh.getProtein()->getAtoms().size() == 426);
}