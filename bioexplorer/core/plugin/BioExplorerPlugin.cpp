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

#include "BioExplorerPlugin.h"

#include <plugin/bioexplorer/Assembly.h>
#include <plugin/common/CommonTypes.h>
#include <plugin/common/Logs.h>
#include <plugin/common/Utils.h>
#include <plugin/io/CacheLoader.h>

#include <brayns/common/ActionInterface.h>
#include <brayns/engineapi/Camera.h>
#include <brayns/engineapi/Engine.h>
#include <brayns/engineapi/FrameBuffer.h>
#include <brayns/engineapi/Material.h>
#include <brayns/engineapi/Model.h>
#include <brayns/parameters/ParametersManager.h>
#include <brayns/pluginapi/Plugin.h>

#include <fstream>

namespace bioexplorer
{
const std::string PLUGIN_API_PREFIX = "be_";

#define CATCH_STD_EXCEPTION()                  \
    catch (const std::runtime_error &e)        \
    {                                          \
        response.status = false;               \
        response.contents = e.what();          \
        PLUGIN_ERROR << e.what() << std::endl; \
    }

void _addBioExplorerRenderer(brayns::Engine &engine)
{
    PLUGIN_INFO << "Registering 'bio_explorer' renderer" << std::endl;
    brayns::PropertyMap properties;
    properties.setProperty(
        {"giDistance", 10000., {"Global illumination distance"}});
    properties.setProperty(
        {"giWeight", 0., 1., 1., {"Global illumination weight"}});
    properties.setProperty(
        {"giSamples", 0, 0, 64, {"Global illumination samples"}});
    properties.setProperty({"shadows", 0., 0., 1., {"Shadow intensity"}});
    properties.setProperty({"softShadows", 0., 0., 1., {"Shadow softness"}});
    properties.setProperty(
        {"softShadowsSamples", 1, 1, 64, {"Soft shadow samples"}});
    properties.setProperty({"exposure", 1., 0.01, 10., {"Exposure"}});
    properties.setProperty({"fogStart", 0., 0., 1e6, {"Fog start"}});
    properties.setProperty({"fogThickness", 1e6, 1e6, 1e6, {"Fog thickness"}});
    properties.setProperty(
        {"maxBounces", 3, 1, 100, {"Maximum number of ray bounces"}});
    properties.setProperty({"useHardwareRandomizer",
                            false,
                            {"Use hardware accelerated randomizer"}});
    properties.setProperty({"showBackground", false, {"Show background"}});
    engine.addRendererType("bio_explorer", properties);
}

void _addBioExplorerFieldsRenderer(brayns::Engine &engine)
{
    PLUGIN_INFO << "Registering 'bio_explorer_fields' renderer" << std::endl;
    brayns::PropertyMap properties;
    properties.setProperty({"exposure", 1., 1., 10., {"Exposure"}});
    properties.setProperty({"useHardwareRandomizer",
                            false,
                            {"Use hardware accelerated randomizer"}});
    properties.setProperty(
        {"minRayStep", 0.00001, 0.00001, 1.0, {"Smallest ray step"}});
    properties.setProperty(
        {"nbRaySteps", 8, 2, 2048, {"Number of ray marching steps"}});
    properties.setProperty({"nbRayRefinementSteps",
                            8,
                            1,
                            128,
                            {"Number of ray marching refinement steps"}});
    properties.setProperty({"cutoff", 2000.0, 0.0, 1e5, {"cutoff"}});
    properties.setProperty(
        {"alphaCorrection", 1.0, 0.001, 1.0, {"Alpha correction"}});
    engine.addRendererType("bio_explorer_fields", properties);
}

void _addBioExplorerPerspectiveCamera(brayns::Engine &engine)
{
    PLUGIN_INFO << "Registering 'bio_explorer_perspective' camera" << std::endl;

    brayns::PropertyMap properties;
    properties.setProperty({"fovy", 45., .1, 360., {"Field of view"}});
    properties.setProperty({"aspect", 1., {"Aspect ratio"}});
    properties.setProperty({"apertureRadius", 0., {"Aperture radius"}});
    properties.setProperty({"focusDistance", 1., {"Focus Distance"}});
    properties.setProperty({"enableClippingPlanes", true, {"Clipping"}});
    properties.setProperty({"stereo", false, {"Stereo"}});
    properties.setProperty(
        {"interpupillaryDistance", 0.0635, 0.0, 10.0, {"Eye separation"}});
    engine.addCameraType("bio_explorer_perspective", properties);
}

BioExplorerPlugin::BioExplorerPlugin()
    : ExtensionPlugin()
{
}

void BioExplorerPlugin::init()
{
    auto actionInterface = _api->getActionInterface();
    auto &scene = _api->getScene();
    auto &registry = scene.getLoaderRegistry();

    registry.registerLoader(
        std::make_unique<CacheLoader>(scene, CacheLoader::getCLIProperties()));

    if (actionInterface)
    {
        std::string entryPoint = PLUGIN_API_PREFIX + "version";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<Response>(entryPoint, [&]() {
            return _version();
        });

        entryPoint = PLUGIN_API_PREFIX + "remove-assembly";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<AssemblyDescriptor, Response>(
            entryPoint, [&](const AssemblyDescriptor &payload) {
                return _removeAssembly(payload);
            });

        entryPoint = PLUGIN_API_PREFIX + "add-assembly";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<AssemblyDescriptor, Response>(
            entryPoint, [&](const AssemblyDescriptor &payload) {
                return _addAssembly(payload);
            });

        entryPoint = PLUGIN_API_PREFIX + "set-protein-color-scheme";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<ColorSchemeDescriptor, Response>(
            entryPoint, [&](const ColorSchemeDescriptor &payload) {
                return _setColorScheme(payload);
            });

        entryPoint =
            PLUGIN_API_PREFIX + "set-protein-amino-acid-sequence-as-string";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface
            ->registerRequest<AminoAcidSequenceAsStringDescriptor, Response>(
                entryPoint,
                [&](const AminoAcidSequenceAsStringDescriptor &payload) {
                    return _setAminoAcidSequenceAsString(payload);
                });

        entryPoint =
            PLUGIN_API_PREFIX + "set-protein-amino-acid-sequence-as-ranges";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface
            ->registerRequest<AminoAcidSequenceAsRangesDescriptor, Response>(
                entryPoint,
                [&](const AminoAcidSequenceAsRangesDescriptor &payload) {
                    return _setAminoAcidSequenceAsRanges(payload);
                });

        entryPoint = PLUGIN_API_PREFIX + "get-protein-amino-acid-information";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface
            ->registerRequest<AminoAcidInformationDescriptor, Response>(
                entryPoint, [&](const AminoAcidInformationDescriptor &payload) {
                    return _getAminoAcidInformation(payload);
                });

        entryPoint = PLUGIN_API_PREFIX + "set-protein-amino-acid";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<SetAminoAcid, Response>(
            entryPoint, [&](const SetAminoAcid &payload) {
                return _setAminoAcid(payload);
            });

        entryPoint = PLUGIN_API_PREFIX + "add-rna-sequence";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<RNASequenceDescriptor, Response>(
            entryPoint, [&](const RNASequenceDescriptor &payload) {
                return _addRNASequence(payload);
            });

        entryPoint = PLUGIN_API_PREFIX + "add-membrane";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<MembraneDescriptor, Response>(
            entryPoint, [&](const MembraneDescriptor &payload) {
                return _addMembrane(payload);
            });

        entryPoint = PLUGIN_API_PREFIX + "add-protein";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<ProteinDescriptor, Response>(
            entryPoint, [&](const ProteinDescriptor &payload) {
                return _addProtein(payload);
            });

        entryPoint = PLUGIN_API_PREFIX + "add-mesh";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<MeshDescriptor, Response>(
            entryPoint,
            [&](const MeshDescriptor &payload) { return _addMesh(payload); });

        entryPoint = PLUGIN_API_PREFIX + "add-glycans";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<SugarsDescriptor, Response>(
            entryPoint, [&](const SugarsDescriptor &payload) {
                return _addGlycans(payload);
            });

        entryPoint = PLUGIN_API_PREFIX + "add-sugars";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<SugarsDescriptor, Response>(
            entryPoint, [&](const SugarsDescriptor &payload) {
                return _addSugars(payload);
            });

        entryPoint = PLUGIN_API_PREFIX + "export-to-cache";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<FileAccess, Response>(
            entryPoint,
            [&](const FileAccess &payload) { return _exportToCache(payload); });

        entryPoint = PLUGIN_API_PREFIX + "export-to-xyz";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<FileAccess, Response>(
            entryPoint,
            [&](const FileAccess &payload) { return _exportToXYZ(payload); });

        entryPoint = PLUGIN_API_PREFIX + "add-grid";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerNotification<AddGrid>(
            entryPoint, [&](const AddGrid &payload) { _addGrid(payload); });

        entryPoint = PLUGIN_API_PREFIX + "set-materials";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerNotification<MaterialsDescriptor>(
            entryPoint,
            [&](const MaterialsDescriptor &param) { _setMaterials(param); });

        entryPoint = PLUGIN_API_PREFIX + "get-material-ids";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<ModelId, MaterialIds>(
            entryPoint, [&](const ModelId &modelId) -> MaterialIds {
                return _getMaterialIds(modelId);
            });

        entryPoint = PLUGIN_API_PREFIX + "build-fields";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<BuildFields, Response>(
            entryPoint, [&](const BuildFields &s) { return _buildFields(s); });

        entryPoint = PLUGIN_API_PREFIX + "export-fields-to-file";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<ModelIdFileAccess, Response>(
            entryPoint,
            [&](const ModelIdFileAccess &s) { return _exportFieldsToFile(s); });

        entryPoint = PLUGIN_API_PREFIX + "import-fields-from-file";
        PLUGIN_INFO << "Registering '" + entryPoint + "' endpoint" << std::endl;
        actionInterface->registerRequest<FileAccess, Response>(
            entryPoint,
            [&](const FileAccess &s) { return _importFieldsFromFile(s); });
    }

    auto &engine = _api->getEngine();
    _addBioExplorerPerspectiveCamera(engine);
    _addBioExplorerRenderer(engine);
    _addBioExplorerFieldsRenderer(engine);
}

void BioExplorerPlugin::preRender()
{
    if (_dirty)
        _api->getScene().markModified();
    _dirty = false;
}

Response BioExplorerPlugin::_version() const
{
    Response response;
    response.contents = BIOEXPLORER_VERSION;
    return response;
}

Response BioExplorerPlugin::_removeAssembly(const AssemblyDescriptor &payload)
{
    auto assembly = _assemblies.find(payload.name);
    if (assembly != _assemblies.end())
        _assemblies.erase(assembly);

    return Response();
}

Response BioExplorerPlugin::_addAssembly(const AssemblyDescriptor &payload)
{
    Response response;
    try
    {
        auto &scene = _api->getScene();
        AssemblyPtr assembly = AssemblyPtr(new Assembly(scene, payload));
        _assemblies[payload.name] = std::move(assembly);
    }
    CATCH_STD_EXCEPTION()
    return response;
}

Response BioExplorerPlugin::_setColorScheme(
    const ColorSchemeDescriptor &payload) const
{
    Response response;
    try
    {
        auto it = _assemblies.find(payload.assemblyName);
        if (it != _assemblies.end())
            (*it).second->setColorScheme(payload);
        else
        {
            std::stringstream msg;
            msg << "Assembly not found: " << payload.assemblyName;
            PLUGIN_ERROR << msg.str() << std::endl;
            response.status = false;
            response.contents = msg.str();
        }
    }
    CATCH_STD_EXCEPTION()
    return response;
}

Response BioExplorerPlugin::_setAminoAcidSequenceAsString(
    const AminoAcidSequenceAsStringDescriptor &payload) const
{
    Response response;
    try
    {
        if (payload.sequence.empty())
            throw std::runtime_error("A valid sequence must be specified");

        auto it = _assemblies.find(payload.assemblyName);
        if (it != _assemblies.end())
            (*it).second->setAminoAcidSequenceAsString(payload);
        else
        {
            std::stringstream msg;
            msg << "Assembly not found: " << payload.assemblyName;
            PLUGIN_ERROR << msg.str() << std::endl;
            response.status = false;
            response.contents = msg.str();
        }
    }
    CATCH_STD_EXCEPTION()
    return response;
}

Response BioExplorerPlugin::_setAminoAcidSequenceAsRanges(
    const AminoAcidSequenceAsRangesDescriptor &payload) const
{
    Response response;
    try
    {
        if (payload.ranges.size() % 2 != 0 || payload.ranges.size() < 2)
            throw std::runtime_error("A valid range must be specified");

        auto it = _assemblies.find(payload.assemblyName);
        if (it != _assemblies.end())
            (*it).second->setAminoAcidSequenceAsRange(payload);
        else
        {
            std::stringstream msg;
            msg << "Assembly not found: " << payload.assemblyName;
            PLUGIN_ERROR << msg.str() << std::endl;
            response.status = false;
            response.contents = msg.str();
        }
    }
    CATCH_STD_EXCEPTION()
    return response;
}

Response BioExplorerPlugin::_getAminoAcidInformation(
    const AminoAcidInformationDescriptor &payload) const
{
    Response response;
    try
    {
        auto it = _assemblies.find(payload.assemblyName);
        if (it != _assemblies.end())
            response.contents = (*it).second->getAminoAcidInformation(payload);
        else
        {
            std::stringstream msg;
            msg << "Assembly not found: " << payload.assemblyName;
            PLUGIN_ERROR << msg.str() << std::endl;
            response.status = false;
            response.contents = msg.str();
        }
    }
    CATCH_STD_EXCEPTION()
    return response;
}

Response BioExplorerPlugin::_addRNASequence(
    const RNASequenceDescriptor &payload) const
{
    Response response;
    try
    {
        auto it = _assemblies.find(payload.assemblyName);
        if (it != _assemblies.end())
            (*it).second->addRNASequence(payload);
        else
        {
            std::stringstream msg;
            msg << "Assembly not found: " << payload.assemblyName;
            PLUGIN_ERROR << msg.str() << std::endl;
            response.status = false;
            response.contents = msg.str();
        }
    }
    CATCH_STD_EXCEPTION()
    return response;
}

Response BioExplorerPlugin::_addMembrane(
    const MembraneDescriptor &payload) const
{
    Response response;
    try
    {
        auto it = _assemblies.find(payload.assemblyName);
        if (it != _assemblies.end())
            (*it).second->addMembrane(payload);
        else
        {
            std::stringstream msg;
            msg << "Assembly not found: " << payload.assemblyName;
            PLUGIN_ERROR << msg.str() << std::endl;
            response.status = false;
            response.contents = msg.str();
        }
    }
    CATCH_STD_EXCEPTION()
    return response;
}

Response BioExplorerPlugin::_addProtein(const ProteinDescriptor &payload) const
{
    Response response;
    try
    {
        auto it = _assemblies.find(payload.assemblyName);
        if (it != _assemblies.end())
            (*it).second->addProtein(payload);
        else
        {
            std::stringstream msg;
            msg << "Assembly not found: " << payload.assemblyName;
            PLUGIN_ERROR << msg.str() << std::endl;
            response.status = false;
            response.contents = msg.str();
        }
    }
    CATCH_STD_EXCEPTION()
    return response;
}

Response BioExplorerPlugin::_addGlycans(const SugarsDescriptor &payload) const
{
    Response response;
    try
    {
        auto it = _assemblies.find(payload.assemblyName);
        if (it != _assemblies.end())
            (*it).second->addGlycans(payload);
        else
        {
            std::stringstream msg;
            msg << "Assembly not found: " << payload.assemblyName;
            PLUGIN_ERROR << msg.str() << std::endl;
            response.status = false;
            response.contents = msg.str();
        }
    }
    CATCH_STD_EXCEPTION()
    return response;
}

Response BioExplorerPlugin::_addSugars(const SugarsDescriptor &payload) const
{
    Response response;
    try
    {
        auto it = _assemblies.find(payload.assemblyName);
        if (it != _assemblies.end())
            (*it).second->addSugars(payload);
        else
        {
            std::stringstream msg;
            msg << "Assembly not found: " << payload.assemblyName;
            PLUGIN_ERROR << msg.str() << std::endl;
            response.status = false;
            response.contents = msg.str();
        }
    }
    catch (const std::runtime_error &e)
    {
        response.status = false;
        response.contents = e.what();
    }
    return response;
}

Response BioExplorerPlugin::_setAminoAcid(const SetAminoAcid &payload) const
{
    Response response;
    try
    {
        auto it = _assemblies.find(payload.assemblyName);
        if (it != _assemblies.end())
            (*it).second->setAminoAcid(payload);
        else
        {
            std::stringstream msg;
            msg << "Assembly not found: " << payload.assemblyName;
            PLUGIN_ERROR << msg.str() << std::endl;
            response.status = false;
            response.contents = msg.str();
        }
    }
    catch (const std::runtime_error &e)
    {
        response.status = false;
        response.contents = e.what();
    }
    return response;
}

Response BioExplorerPlugin::_exportToCache(const FileAccess &payload)
{
    Response response;
    try
    {
        auto &scene = _api->getScene();
        CacheLoader loader(scene);
        loader.exportToCache(payload.filename);
    }
    CATCH_STD_EXCEPTION()
    return response;
}

Response BioExplorerPlugin::_exportToXYZ(const FileAccess &payload)
{
    Response response;
    try
    {
        auto &scene = _api->getScene();
        CacheLoader loader(scene);
        loader.exportToXYZ(payload.filename, payload.fileFormat);
    }
    CATCH_STD_EXCEPTION()
    return response;
}

Response BioExplorerPlugin::_addMesh(const MeshDescriptor &payload) const
{
    Response response;
    try
    {
        auto it = _assemblies.find(payload.assemblyName);
        if (it != _assemblies.end())
            (*it).second->addMesh(payload);
        else
        {
            std::stringstream msg;
            msg << "Assembly not found: " << payload.assemblyName;
            PLUGIN_ERROR << msg.str() << std::endl;
            response.status = false;
            response.contents = msg.str();
        }
    }
    CATCH_STD_EXCEPTION()
    return response;
}

Response BioExplorerPlugin::_addGrid(const AddGrid &payload)
{
    Response response;
    try
    {
        BRAYNS_INFO << "Building Grid scene" << std::endl;

        auto &scene = _api->getScene();
        auto model = scene.createModel();

        const brayns::Vector3f red = {1, 0, 0};
        const brayns::Vector3f green = {0, 1, 0};
        const brayns::Vector3f blue = {0, 0, 1};
        const brayns::Vector3f grey = {0.5, 0.5, 0.5};

        brayns::PropertyMap props;
        props.setProperty({MATERIAL_PROPERTY_SHADING_MODE,
                           static_cast<int>(MaterialShadingMode::basic)});
        props.setProperty({MATERIAL_PROPERTY_USER_PARAMETER, 1.0});
        props.setProperty(
            {MATERIAL_PROPERTY_CHAMELEON_MODE,
             static_cast<int>(
                 MaterialChameleonMode::undefined_chameleon_mode)});

        auto material = model->createMaterial(0, "x");
        material->setDiffuseColor(grey);
        material->setProperties(props);
        const auto &position =
            brayns::Vector3f(payload.position[0], payload.position[1],
                             payload.position[2]);

        const float m = payload.minValue;
        const float M = payload.maxValue;
        const float s = payload.steps;
        const float r = payload.radius;
        for (float x = m; x <= M; x += s)
            for (float y = m; y <= M; y += s)
                if (fabs(x) < 0.001f || fabs(y) < 0.001f)
                {
                    model->addCylinder(0, {position + brayns::Vector3f(x, y, m),
                                           position + brayns::Vector3f(x, y, M),
                                           r});
                    model->addCylinder(0, {position + brayns::Vector3f(m, x, y),
                                           position + brayns::Vector3f(M, x, y),
                                           r});
                    model->addCylinder(0, {position + brayns::Vector3f(x, m, y),
                                           position + brayns::Vector3f(x, M, y),
                                           r});
                }

        material = model->createMaterial(1, "plane_x");
        material->setDiffuseColor(payload.useColors ? red : grey);
        material->setOpacity(payload.planeOpacity);
        material->setProperties(props);
        auto &tmx = model->getTriangleMeshes()[1];
        tmx.vertices.push_back(position + brayns::Vector3f(m, 0, m));
        tmx.vertices.push_back(position + brayns::Vector3f(M, 0, m));
        tmx.vertices.push_back(position + brayns::Vector3f(M, 0, M));
        tmx.vertices.push_back(position + brayns::Vector3f(m, 0, M));
        tmx.indices.push_back(brayns::Vector3ui(0, 1, 2));
        tmx.indices.push_back(brayns::Vector3ui(2, 3, 0));

        material = model->createMaterial(2, "plane_y");
        material->setDiffuseColor(payload.useColors ? green : grey);
        material->setOpacity(payload.planeOpacity);
        material->setProperties(props);
        auto &tmy = model->getTriangleMeshes()[2];
        tmy.vertices.push_back(position + brayns::Vector3f(m, m, 0));
        tmy.vertices.push_back(position + brayns::Vector3f(M, m, 0));
        tmy.vertices.push_back(position + brayns::Vector3f(M, M, 0));
        tmy.vertices.push_back(position + brayns::Vector3f(m, M, 0));
        tmy.indices.push_back(brayns::Vector3ui(0, 1, 2));
        tmy.indices.push_back(brayns::Vector3ui(2, 3, 0));

        material = model->createMaterial(3, "plane_z");
        material->setDiffuseColor(payload.useColors ? blue : grey);
        material->setOpacity(payload.planeOpacity);
        material->setProperties(props);
        auto &tmz = model->getTriangleMeshes()[3];
        tmz.vertices.push_back(position + brayns::Vector3f(0, m, m));
        tmz.vertices.push_back(position + brayns::Vector3f(0, m, M));
        tmz.vertices.push_back(position + brayns::Vector3f(0, M, M));
        tmz.vertices.push_back(position + brayns::Vector3f(0, M, m));
        tmz.indices.push_back(brayns::Vector3ui(0, 1, 2));
        tmz.indices.push_back(brayns::Vector3ui(2, 3, 0));

        if (payload.showAxis)
        {
            const float l = M;
            const float smallRadius = payload.radius * 25.0;
            const float largeRadius = payload.radius * 50.0;
            const float l1 = l * 0.89;
            const float l2 = l * 0.90;

            brayns::PropertyMap props;
            props.setProperty({MATERIAL_PROPERTY_USER_PARAMETER, 1.0});
            props.setProperty({MATERIAL_PROPERTY_SHADING_MODE,
                               static_cast<int>(MaterialShadingMode::basic)});
            props.setProperty(
                {MATERIAL_PROPERTY_CHAMELEON_MODE,
                 static_cast<int>(
                     MaterialChameleonMode::undefined_chameleon_mode)});

            // X
            material = model->createMaterial(4, "x_axis");
            material->setDiffuseColor({1, 0, 0});
            material->setProperties(props);

            model->addCylinder(4,
                               {position, position + brayns::Vector3f(l1, 0, 0),
                                smallRadius});
            model->addCone(4, {position + brayns::Vector3f(l1, 0, 0),
                               position + brayns::Vector3f(l2, 0, 0),
                               smallRadius, largeRadius});
            model->addCone(4, {position + brayns::Vector3f(l2, 0, 0),
                               position + brayns::Vector3f(M, 0, 0),
                               largeRadius, 0});

            // Y
            material = model->createMaterial(5, "y_axis");
            material->setDiffuseColor({0, 1, 0});
            material->setProperties(props);

            model->addCylinder(5,
                               {position, position + brayns::Vector3f(0, l1, 0),
                                smallRadius});
            model->addCone(5, {position + brayns::Vector3f(0, l1, 0),
                               position + brayns::Vector3f(0, l2, 0),
                               smallRadius, largeRadius});
            model->addCone(5, {position + brayns::Vector3f(0, l2, 0),
                               position + brayns::Vector3f(0, M, 0),
                               largeRadius, 0});

            // Z
            material = model->createMaterial(6, "z_axis");
            material->setDiffuseColor({0, 0, 1});
            material->setProperties(props);

            model->addCylinder(6,
                               {position, position + brayns::Vector3f(0, 0, l1),
                                smallRadius});
            model->addCone(6, {position + brayns::Vector3f(0, 0, l1),
                               position + brayns::Vector3f(0, 0, l2),
                               smallRadius, largeRadius});
            model->addCone(6, {position + brayns::Vector3f(0, 0, l2),
                               position + brayns::Vector3f(0, 0, M),
                               largeRadius, 0});

            // Origin
            model->addSphere(0, {position, smallRadius});
        }

        scene.addModel(
            std::make_shared<brayns::ModelDescriptor>(std::move(model),
                                                      "Grid"));
    }
    CATCH_STD_EXCEPTION()
    return response;
}

MaterialIds BioExplorerPlugin::_getMaterialIds(const ModelId &modelId)
{
    MaterialIds materialIds;
    auto modelDescriptor = _api->getScene().getModel(modelId.modelId);
    if (modelDescriptor)
    {
        for (const auto &material : modelDescriptor->getModel().getMaterials())
            if (material.first != brayns::BOUNDINGBOX_MATERIAL_ID &&
                material.first != brayns::SECONDARY_MODEL_MATERIAL_ID)
                materialIds.ids.push_back(material.first);
    }
    else
        PLUGIN_THROW(std::runtime_error("Invalid model ID"));
    return materialIds;
}

Response BioExplorerPlugin::_setMaterials(const MaterialsDescriptor &payload)
{
    Response response;
    try
    {
        auto &scene = _api->getScene();
        for (const auto modelId : payload.modelIds)
        {
            PLUGIN_INFO << "Modifying materials on model " << modelId
                        << std::endl;
            auto modelDescriptor = scene.getModel(modelId);
            if (modelDescriptor)
            {
                size_t id = 0;
                for (const auto materialId : payload.materialIds)
                {
                    try
                    {
                        auto material =
                            modelDescriptor->getModel().getMaterial(materialId);
                        if (material)
                        {
                            if (!payload.diffuseColors.empty())
                            {
                                const size_t index = id * 3;
                                material->setDiffuseColor(
                                    {payload.diffuseColors[index],
                                     payload.diffuseColors[index + 1],
                                     payload.diffuseColors[index + 2]});
                                material->setSpecularColor(
                                    {payload.specularColors[index],
                                     payload.specularColors[index + 1],
                                     payload.specularColors[index + 2]});
                            }

                            if (!payload.specularExponents.empty())
                                material->setSpecularExponent(
                                    payload.specularExponents[id]);
                            if (!payload.reflectionIndices.empty())
                                material->setReflectionIndex(
                                    payload.reflectionIndices[id]);
                            if (!payload.opacities.empty())
                                material->setOpacity(payload.opacities[id]);
                            if (!payload.refractionIndices.empty())
                                material->setRefractionIndex(
                                    payload.refractionIndices[id]);
                            if (!payload.emissions.empty())
                                material->setEmission(payload.emissions[id]);
                            if (!payload.glossinesses.empty())
                                material->setGlossiness(
                                    payload.glossinesses[id]);
                            if (!payload.shadingModes.empty())
                                material->updateProperty(
                                    MATERIAL_PROPERTY_SHADING_MODE,
                                    payload.shadingModes[id]);
                            if (!payload.userParameters.empty())
                                material->updateProperty(
                                    MATERIAL_PROPERTY_USER_PARAMETER,
                                    static_cast<double>(
                                        payload.userParameters[id]));
                            if (!payload.chameleonModes.empty())
                                material->updateProperty(
                                    MATERIAL_PROPERTY_CHAMELEON_MODE,
                                    payload.chameleonModes[id]);

                            // This is needed to apply modifications. Changes to
                            // the material will be committed after the
                            // rendering of the current frame is completed
                            material->markModified();
                        }
                    }
                    catch (const std::runtime_error &e)
                    {
                        PLUGIN_INFO << e.what() << std::endl;
                    }
                    ++id;
                }
                _dirty = true;
            }
            else
                PLUGIN_INFO << "Model " << modelId << " is not registered"
                            << std::endl;
        }
    }
    CATCH_STD_EXCEPTION()
    return response;
}

void BioExplorerPlugin::_attachFieldsHandler(FieldsHandlerPtr handler)
{
    auto &scene = _api->getScene();
    auto model = scene.createModel();
    const auto &spacing = Vector3f(handler->getSpacing());
    const auto &size = Vector3f(handler->getDimensions()) * spacing;
    const auto &offset = Vector3f(handler->getOffset());
    const brayns::Vector3f center{(offset + size / 2.f)};

    const size_t materialId = 0;
    auto material = model->createMaterial(materialId, "default");

    brayns::TriangleMesh box = brayns::createBox(offset, offset + size);
    model->getTriangleMeshes()[materialId] = box;
    ModelMetadata metadata;
    metadata["Center"] = std::to_string(center.x) + "," +
                         std::to_string(center.y) + "," +
                         std::to_string(center.z);
    metadata["Size"] = std::to_string(size.x) + "," + std::to_string(size.y) +
                       "," + std::to_string(size.z);
    metadata["Spacing"] = std::to_string(spacing.x) + "," +
                          std::to_string(spacing.y) + "," +
                          std::to_string(spacing.z);

    model->setSimulationHandler(handler);
    setTransferFunction(model->getTransferFunction());

    auto modelDescriptor =
        std::make_shared<ModelDescriptor>(std::move(model), "Fields", metadata);
    scene.addModel(modelDescriptor);

    PLUGIN_INFO << "Fields model " << modelDescriptor->getModelID()
                << " was successfully created" << std::endl;
}

Response BioExplorerPlugin::_buildFields(const BuildFields &payload)
{
    Response response;
    try
    {
        PLUGIN_INFO << "Building Fields from scene" << std::endl;
        auto &scene = _api->getScene();
        FieldsHandlerPtr handler =
            std::make_shared<FieldsHandler>(scene, payload.voxelSize);
        _attachFieldsHandler(handler);
    }
    CATCH_STD_EXCEPTION()
    return response;
}

Response BioExplorerPlugin::_importFieldsFromFile(const FileAccess &payload)
{
    Response response;
    try
    {
        PLUGIN_INFO << "Importing Fields from " << payload.filename
                    << std::endl;
        FieldsHandlerPtr handler =
            std::make_shared<FieldsHandler>(payload.filename);
        _attachFieldsHandler(handler);
    }
    CATCH_STD_EXCEPTION()
    return response;
}

Response BioExplorerPlugin::_exportFieldsToFile(
    const ModelIdFileAccess &payload)
{
    Response response;
    try
    {
        PLUGIN_INFO << "Exporting fields to " << payload.filename << std::endl;
        const auto &scene = _api->getScene();
        auto modelDescriptor = scene.getModel(payload.modelId);
        if (modelDescriptor)
        {
            auto handler = modelDescriptor->getModel().getSimulationHandler();
            if (handler)
            {
                FieldsHandler *fieldsHandler =
                    dynamic_cast<FieldsHandler *>(handler.get());
                if (!fieldsHandler)
                    PLUGIN_THROW(
                        std::runtime_error("Model has no fields handler"));

                fieldsHandler->exportToFile(payload.filename);
            }
            else
                PLUGIN_THROW(std::runtime_error("Model has no handler"));
        }
        else
            PLUGIN_THROW(std::runtime_error("Unknown model ID"));
    }
    CATCH_STD_EXCEPTION()
    return response;
}

extern "C" ExtensionPlugin *brayns_plugin_create(int /*argc*/, char ** /*argv*/)
{
    PLUGIN_INFO << " _|_|_|    _|            _|_|_|_|                      _|  "
                   "                                      "
                << std::endl;
    PLUGIN_INFO << " _|    _|        _|_|    _|        _|    _|  _|_|_|    _|  "
                   "  _|_|    _|  _|_|    _|_|    _|  _|_|"
                << std::endl;
    PLUGIN_INFO << " _|_|_|    _|  _|    _|  _|_|_|      _|_|    _|    _|  _|  "
                   "_|    _|  _|_|      _|_|_|_|  _|_|    "
                << std::endl;
    PLUGIN_INFO << " _|    _|  _|  _|    _|  _|        _|    _|  _|    _|  _|  "
                   "_|    _|  _|        _|        _|      "
                << std::endl;
    PLUGIN_INFO << " _|_|_|    _|    _|_|    _|_|_|_|  _|    _|  _|_|_|    _|  "
                   "  _|_|    _|          _|_|_|  _|      "
                << std::endl;
    PLUGIN_INFO << "                                             _|            "
                   "                                      "
                << std::endl;
    PLUGIN_INFO << "                                             _|            "
                   "                                      "
                << std::endl;
    PLUGIN_INFO << "Initializing BioExplorer plug-in (version "
                << BIOEXPLORER_VERSION << ")" << std::endl;
    PLUGIN_INFO << std::endl;
    return new BioExplorerPlugin();
}

} // namespace bioexplorer