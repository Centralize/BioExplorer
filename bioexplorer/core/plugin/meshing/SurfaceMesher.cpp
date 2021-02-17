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

#include "SurfaceMesher.h"

#include <plugin/common/CommonTypes.h>
#include <plugin/common/Logs.h>

#include <brayns/engineapi/Material.h>
#include <brayns/engineapi/Model.h>
#include <brayns/engineapi/Scene.h>
#include <brayns/io/MeshLoader.h>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Skin_surface_3.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Union_of_balls_3.h>
#include <CGAL/mesh_skin_surface_3.h>
#include <CGAL/mesh_union_of_balls_3.h>
#include <CGAL/subdivide_skin_surface_mesh_3.h>
#include <CGAL/subdivide_union_of_balls_mesh_3.h>
#include <fstream>
#include <list>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Skin_surface_traits_3<K> Traits;
typedef K::Point_3 Point_3;
typedef K::Weighted_point_3 Weighted_point;
typedef CGAL::Polyhedron_3<K> Polyhedron;
typedef CGAL::Skin_surface_traits_3<K> Traits;
typedef CGAL::Skin_surface_3<Traits> Skin_surface_3;
typedef CGAL::Union_of_balls_3<Traits> Union_of_balls_3;
typedef K::Vector_3 Vector;
typedef CGAL::Surface_mesh<Point_3> Surface_mesh;

typedef boost::graph_traits<Surface_mesh>::vertex_descriptor vertex_descriptor;
typedef boost::graph_traits<Polyhedron>::face_descriptor face_descriptor;

typedef Polyhedron::Facet_iterator Facet_iterator;
typedef Polyhedron::Halfedge_around_facet_circulator Halfedge_facet_circulator;

namespace bioexplorer
{
SurfaceMesher::SurfaceMesher() {}

ModelDescriptorPtr SurfaceMesher::generateSurface(brayns::Scene& scene,
                                                  const std::string& title,
                                                  const Vector4fs& atoms,
                                                  const double shrinkfactor)
{
    ModelDescriptorPtr modelDescriptor{nullptr};
    MeshLoader meshLoader(scene);
    const std::string filename =
        "/tmp/" + title.substr(title.find("_") + 1) + ".off";
    try
    {
        PLUGIN_INFO << "Trying to load surface from cache " << filename
                    << std::endl;
        modelDescriptor =
            meshLoader.importFromFile(filename, LoaderProgress(), {});
        _setMaterialExtraAttributes(modelDescriptor);
        PLUGIN_INFO << "Surface loaded from cache " << filename << std::endl;
        return modelDescriptor;
    }
    catch (const std::runtime_error& e)
    {
        PLUGIN_INFO << "Failed to load surface from cache (" << e.what()
                    << "), constructing it..." << std::endl;
    }

    std::list<Weighted_point> l;
    for (const auto& atom : atoms)
        l.push_front(Weighted_point(Point_3(atom.x, atom.y, atom.z), atom.w));

    PLUGIN_INFO << "Constructing skin surface from " << l.size() << " atoms"
                << std::endl;

    Polyhedron polyhedron;
    Skin_surface_3 skinSurface(l.begin(), l.end(), shrinkfactor);

    PLUGIN_INFO << "Meshing skin surface..." << std::endl;
    CGAL::mesh_skin_surface_3(skinSurface, polyhedron);
    CGAL::Polygon_mesh_processing::triangulate_faces(polyhedron);

    PLUGIN_INFO << "Adding mesh to model" << std::endl;
    std::ofstream out(filename);
    out << polyhedron;
    modelDescriptor = meshLoader.importFromFile(filename, LoaderProgress(), {});
    _setMaterialExtraAttributes(modelDescriptor);
    return modelDescriptor;
}

ModelDescriptorPtr SurfaceMesher::generateUnionOfBalls(brayns::Scene& scene,
                                                       const std::string& title,
                                                       const Vector4fs& atoms)
{
    std::list<Weighted_point> l;
    for (const auto& atom : atoms)
        l.push_front(Weighted_point(Point_3(atom.x, atom.y, atom.z), atom.w));

    ModelDescriptorPtr modelDescriptor{nullptr};
    MeshLoader meshLoader(scene);
    const std::string filename =
        "/tmp/" + title.substr(title.find("_") + 1) + ".obj";
    try
    {
        PLUGIN_INFO << "Trying to load union of balls from cache " << filename
                    << std::endl;
        modelDescriptor =
            meshLoader.importFromFile(filename, LoaderProgress(), {});
        _setMaterialExtraAttributes(modelDescriptor);
        PLUGIN_INFO << "Surface loaded from cache " << filename << std::endl;
        return modelDescriptor;
    }
    catch (const std::runtime_error& e)
    {
        PLUGIN_INFO << "Failed to load union of balls from cache (" << e.what()
                    << "), constructing it..." << std::endl;
    }

    PLUGIN_INFO << "Constructing union of balls from " << l.size() << " atoms"
                << std::endl;

    Polyhedron polyhedron;
    Union_of_balls_3 union_of_balls(l.begin(), l.end());
    CGAL::mesh_union_of_balls_3(union_of_balls, polyhedron);

    PLUGIN_INFO << "Adding mesh to model" << std::endl;
    std::ofstream out(filename);
    out << polyhedron;
    modelDescriptor = meshLoader.importFromFile(filename, LoaderProgress(), {});
    _setMaterialExtraAttributes(modelDescriptor);
    return modelDescriptor;
}

void SurfaceMesher::_setMaterialExtraAttributes(
    ModelDescriptorPtr modelDescriptor)
{
    auto materials = modelDescriptor->getModel().getMaterials();
    for (auto& material : materials)
    {
        brayns::PropertyMap props;
        props.setProperty({MATERIAL_PROPERTY_SHADING_MODE,
                           static_cast<int>(MaterialShadingMode::basic)});
        props.setProperty({MATERIAL_PROPERTY_USER_PARAMETER, 1.0});
        props.setProperty({MATERIAL_PROPERTY_VISIBLE, true});
        props.setProperty({MATERIAL_PROPERTY_CHAMELEON, false});
        material.second->updateProperties(props);
    }
}
} // namespace bioexplorer
