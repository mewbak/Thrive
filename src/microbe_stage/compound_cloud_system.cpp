#include "microbe_stage/compound_cloud_system.h"
#include "microbe_stage/simulation_parameters.h"

#include "ThriveGame.h"

#include "engine/player_data.h"
#include "generated/cell_stage_world.h"

#include <Rendering/GeometryHelpers.h>
#include <Rendering/Graphics.h>
#include <bsfCore/Components/BsCRenderable.h>
#include <bsfCore/Image/BsTexture.h>
#include <bsfCore/Material/BsMaterial.h>

#include <atomic>

using namespace thrive;

constexpr auto CLOUD_TEXTURE_BYTES_PER_ELEMENT = 4;
constexpr auto BS_PIXEL_FORMAT = bs::PF_RGBA8;

////////////////////////////////////////////////////////////////////////////////
// CompoundCloudComponent
////////////////////////////////////////////////////////////////////////////////
CompoundCloudComponent::CompoundCloudComponent(CompoundCloudSystem& owner,
    Compound* first,
    Compound* second,
    Compound* third,
    Compound* fourth) :
    Leviathan::Component(TYPE),
    m_owner(owner)
{
    if(!first)
        throw std::runtime_error(
            "CompoundCloudComponent needs at least one Compound type");

    // Read data
    m_color1 = first->colour;
    m_compoundId1 = first->id;

    if(second) {

        m_compoundId2 = second->id;
        m_color2 = second->colour;
    }

    if(third) {

        m_compoundId3 = third->id;
        m_color3 = third->colour;
    }

    if(fourth) {

        m_compoundId4 = fourth->id;
        m_color4 = fourth->colour;
    }
}

CompoundCloudComponent::~CompoundCloudComponent()
{
    LEVIATHAN_ASSERT(
        !m_sceneNode && !m_renderable, "CompoundCloudComponent not Released");

    m_owner.cloudReportDestroyed(this);
}

void
    CompoundCloudComponent::Release(bs::Scene* scene)
{
    if(m_sceneNode && !m_sceneNode.isDestroyed()) {

        m_sceneNode->destroy();
        m_sceneNode = nullptr;
        m_renderable = nullptr;
    }

    m_initialized = false;

    // Other resources are held by smart pointers
}

// ------------------------------------ //
CompoundCloudComponent::SLOT
    CompoundCloudComponent::getSlotForCompound(CompoundId compound)
{
    if(compound == m_compoundId1)
        return SLOT::FIRST;
    if(compound == m_compoundId2)
        return SLOT::SECOND;
    if(compound == m_compoundId3)
        return SLOT::THIRD;
    if(compound == m_compoundId4)
        return SLOT::FOURTH;

    throw std::runtime_error("This cloud doesn't contain the used CompoundId");
}

bool
    CompoundCloudComponent::handlesCompound(CompoundId compound)
{
    if(compound == m_compoundId1)
        return true;
    if(compound == m_compoundId2)
        return true;
    if(compound == m_compoundId3)
        return true;
    if(compound == m_compoundId4)
        return true;
    return false;
}
// ------------------------------------ //
void
    CompoundCloudComponent::addCloud(CompoundId compound,
        float dens,
        size_t x,
        size_t y)
{
    switch(getSlotForCompound(compound)) {
    case SLOT::FIRST: m_density1[x][y] += dens; break;
    case SLOT::SECOND: m_density2[x][y] += dens; break;
    case SLOT::THIRD: m_density3[x][y] += dens; break;
    case SLOT::FOURTH: m_density4[x][y] += dens; break;
    }
}

int
    CompoundCloudComponent::takeCompound(CompoundId compound,
        size_t x,
        size_t y,
        float rate)
{
    switch(getSlotForCompound(compound)) {
    case SLOT::FIRST: {
        int amountToGive = static_cast<int>(m_density1[x][y] * rate);
        m_density1[x][y] -= amountToGive;
        if(m_density1[x][y] < 1)
            m_density1[x][y] = 0;

        return amountToGive;
    }
    case SLOT::SECOND: {
        int amountToGive = static_cast<int>(m_density2[x][y] * rate);
        m_density2[x][y] -= amountToGive;
        if(m_density2[x][y] < 1)
            m_density2[x][y] = 0;

        return amountToGive;
    }
    case SLOT::THIRD: {
        int amountToGive = static_cast<int>(m_density3[x][y] * rate);
        m_density3[x][y] -= amountToGive;
        if(m_density3[x][y] < 1)
            m_density3[x][y] = 0;

        return amountToGive;
    }
    case SLOT::FOURTH: {
        int amountToGive = static_cast<int>(m_density4[x][y] * rate);
        m_density4[x][y] -= amountToGive;
        if(m_density4[x][y] < 1)
            m_density4[x][y] = 0;

        return amountToGive;
    }
    }

    LEVIATHAN_ASSERT(false, "Shouldn't get here");
    return -1;
}

int
    CompoundCloudComponent::amountAvailable(CompoundId compound,
        size_t x,
        size_t y,
        float rate)
{
    switch(getSlotForCompound(compound)) {
    case SLOT::FIRST: {
        int amountToGive = static_cast<int>(m_density1[x][y] * rate);
        return amountToGive;
    }
    case SLOT::SECOND: {
        int amountToGive = static_cast<int>(m_density2[x][y] * rate);
        return amountToGive;
    }
    case SLOT::THIRD: {
        int amountToGive = static_cast<int>(m_density3[x][y] * rate);
        return amountToGive;
    }
    case SLOT::FOURTH: {
        int amountToGive = static_cast<int>(m_density4[x][y] * rate);
        return amountToGive;
    }
    }

    LEVIATHAN_ASSERT(false, "Shouldn't get here");
    return -1;
}

void
    CompoundCloudComponent::getCompoundsAt(size_t x,
        size_t y,
        std::vector<std::tuple<CompoundId, float>>& result)
{
    if(m_compoundId1 != NULL_COMPOUND) {
        const auto amount = m_density1[x][y];
        if(amount > 0)
            result.emplace_back(m_compoundId1, amount);
    }

    if(m_compoundId2 != NULL_COMPOUND) {
        const auto amount = m_density2[x][y];
        if(amount > 0)
            result.emplace_back(m_compoundId2, amount);
    }

    if(m_compoundId3 != NULL_COMPOUND) {
        const auto amount = m_density3[x][y];
        if(amount > 0)
            result.emplace_back(m_compoundId3, amount);
    }

    if(m_compoundId4 != NULL_COMPOUND) {
        const auto amount = m_density4[x][y];
        if(amount > 0)
            result.emplace_back(m_compoundId4, amount);
    }
}
// ------------------------------------ //
void
    CompoundCloudComponent::recycleToPosition(const Float3& newPosition)
{
    m_position = newPosition;

    // This check is for non-graphical mode
    if(m_sceneNode)
        m_sceneNode->setPosition(
            bs::Vector3(m_position.X, CLOUD_Y_COORDINATE, m_position.Z));

    clearContents();
}

void
    CompoundCloudComponent::clearContents()
{
    // Clear data. Maybe there is a faster way
    if(m_compoundId1 != NULL_COMPOUND) {
        for(size_t x = 0; x < m_density1.size(); ++x) {
            for(size_t y = 0; y < m_density1[x].size(); ++y) {
                m_density1[x][y] = 0;
                m_oldDens1[x][y] = 0;
            }
        }
    }

    if(m_compoundId2 != NULL_COMPOUND) {
        for(size_t x = 0; x < m_density2.size(); ++x) {
            for(size_t y = 0; y < m_density2[x].size(); ++y) {
                m_density2[x][y] = 0;
                m_oldDens2[x][y] = 0;
            }
        }
    }

    if(m_compoundId3 != NULL_COMPOUND) {
        for(size_t x = 0; x < m_density3.size(); ++x) {
            for(size_t y = 0; y < m_density3[x].size(); ++y) {
                m_density3[x][y] = 0;
                m_oldDens3[x][y] = 0;
            }
        }
    }

    if(m_compoundId4 != NULL_COMPOUND) {
        for(size_t x = 0; x < m_density4.size(); ++x) {
            for(size_t y = 0; y < m_density4[x].size(); ++y) {
                m_density4[x][y] = 0;
                m_oldDens4[x][y] = 0;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// CompoundCloudSystem
////////////////////////////////////////////////////////////////////////////////

void
    CompoundCloudSystem::Init(CellStageWorld& world)
{
    // Use the curl of a Perlin noise field to create a turbulent velocity
    // field.
    // createVelocityField();

    // Skip if no graphics
    if(!Engine::Get()->IsInGraphicalMode())
        return;

    m_planeMesh = Leviathan::GeometryHelpers::CreateXZPlane(
        CLOUD_X_EXTENT, CLOUD_Y_EXTENT);

    m_perlinNoise =
        Engine::Get()->GetGraphics()->LoadTextureByName("PerlinNoise.jpg");

    LEVIATHAN_ASSERT(m_perlinNoise, "failed to load perlin noise texture");
}

void
    CompoundCloudSystem::Release(CellStageWorld& world)
{
    // Make sure all of our entities are destroyed //
    // Because their destruction callback unregisters them, we have to delete
    // them like this
    while(!m_managedClouds.empty()) {

        world.DestroyEntity(m_managedClouds.begin()->first);
    }

    m_planeMesh = nullptr;
    m_perlinNoise = nullptr;
}
// ------------------------------------ //
void
    CompoundCloudSystem::registerCloudTypes(CellStageWorld& world,
        const std::vector<Compound>& clouds)
{
    m_cloudTypes = clouds;

    // We do a spawn cycle immediately to make sure that even early code can
    // spawn clouds
    doSpawnCycle(world, Float3(0, 0, 0));
}

bool
    CompoundCloudSystem::addCloud(CompoundId compound,
        float density,
        const Float3& worldPosition)
{
    // Find the target cloud //
    for(auto& cloud : m_managedClouds) {

        const auto& pos = cloud.second->m_position;

        if(cloudContainsPosition(pos, worldPosition)) {
            // Within cloud

            // Skip wrong types
            if(!cloud.second->handlesCompound(compound))
                continue;

            try {
                auto [x, y] = convertWorldToCloudLocal(pos, worldPosition);
                cloud.second->addCloud(compound, density, x, y);

                return true;

            } catch(const Leviathan::InvalidArgument& e) {
                LOG_ERROR("CompoundCloudSystem: can't place cloud because the "
                          "cloud math is "
                          "wrong, exception:");
                e.PrintToLog();
                return false;
            }
        }
    }

    return false;
}

float
    CompoundCloudSystem::takeCompound(CompoundId compound,
        const Float3& worldPosition,
        float rate)
{
    for(auto& cloud : m_managedClouds) {

        const auto& pos = cloud.second->m_position;

        if(cloudContainsPosition(pos, worldPosition)) {
            // Within cloud

            // Skip wrong types
            if(!cloud.second->handlesCompound(compound))
                continue;

            try {
                auto [x, y] = convertWorldToCloudLocal(pos, worldPosition);
                return cloud.second->takeCompound(compound, x, y, rate);

            } catch(const Leviathan::InvalidArgument& e) {
                LOG_ERROR(
                    "CompoundCloudSystem: can't take from cloud because the "
                    "cloud math is "
                    "wrong, exception:");
                e.PrintToLog();
                return false;
            }
        }
    }

    return 0;
}

float
    CompoundCloudSystem::amountAvailable(CompoundId compound,
        const Float3& worldPosition,
        float rate)
{
    for(auto& cloud : m_managedClouds) {

        const auto& pos = cloud.second->m_position;

        if(cloudContainsPosition(pos, worldPosition)) {
            // Within cloud

            // Skip wrong types
            if(!cloud.second->handlesCompound(compound))
                continue;

            try {
                auto [x, y] = convertWorldToCloudLocal(pos, worldPosition);
                return cloud.second->amountAvailable(compound, x, y, rate);

            } catch(const Leviathan::InvalidArgument& e) {
                LOG_ERROR(
                    "CompoundCloudSystem: can't get available compounds "
                    "from cloud because the cloud math is wrong, exception:");
                e.PrintToLog();
                return false;
            }
        }
    }

    return 0;
}

std::vector<std::tuple<CompoundId, float>>
    CompoundCloudSystem::getAllAvailableAt(const Float3& worldPosition)
{
    std::vector<std::tuple<CompoundId, float>> result;

    for(auto& cloud : m_managedClouds) {

        const auto& pos = cloud.second->m_position;

        if(cloudContainsPosition(pos, worldPosition)) {
            // Within cloud

            try {
                auto [x, y] = convertWorldToCloudLocal(pos, worldPosition);
                cloud.second->getCompoundsAt(x, y, result);

            } catch(const Leviathan::InvalidArgument& e) {
                LOG_ERROR(
                    "CompoundCloudSystem: can't get available compounds "
                    "from cloud because the cloud math is wrong, exception:");
                e.PrintToLog();
            }
        }
    }

    return result;
}
// ------------------------------------ //
void
    CompoundCloudSystem::emptyAllClouds()
{
    for(auto& cloud : m_managedClouds) {
        cloud.second->clearContents();
    }
}
// ------------------------------------ //
bool
    CompoundCloudSystem::cloudContainsPosition(const Float3& cloudPosition,
        const Float3& worldPosition)
{
    if(worldPosition.X < cloudPosition.X - CLOUD_WIDTH ||
        worldPosition.X >= cloudPosition.X + CLOUD_WIDTH ||
        worldPosition.Z < cloudPosition.Z - CLOUD_HEIGHT ||
        worldPosition.Z >= cloudPosition.Z + CLOUD_HEIGHT)
        return false;
    return true;
}

bool
    CompoundCloudSystem::cloudContainsPositionWithRadius(
        const Float3& cloudPosition,
        const Float3& worldPosition,
        float radius)
{
    if(worldPosition.X + radius < cloudPosition.X - CLOUD_WIDTH ||
        worldPosition.X - radius >= cloudPosition.X + CLOUD_WIDTH ||
        worldPosition.Z + radius < cloudPosition.Z - CLOUD_HEIGHT ||
        worldPosition.Z - radius >= cloudPosition.Z + CLOUD_HEIGHT)
        return false;
    return true;
}

std::tuple<size_t, size_t>
    CompoundCloudSystem::convertWorldToCloudLocal(const Float3& cloudPosition,
        const Float3& worldPosition)
{
    const auto topLeftRelative =
        Float3(worldPosition.X - (cloudPosition.X - CLOUD_WIDTH), 0,
            worldPosition.Z - (cloudPosition.Z - CLOUD_HEIGHT));

    // Floor is used here because otherwise the last coordinate is wrong
    const auto localX =
        static_cast<size_t>(std::floor(topLeftRelative.X / CLOUD_RESOLUTION));
    const auto localY =
        static_cast<size_t>(std::floor(topLeftRelative.Z / CLOUD_RESOLUTION));

    // Safety check
    if(localX >= CLOUD_SIMULATION_WIDTH || localY >= CLOUD_SIMULATION_HEIGHT)
        throw Leviathan::InvalidArgument("position not within cloud");

    return std::make_tuple(localX, localY);
}

std::tuple<float, float>
    CompoundCloudSystem::convertWorldToCloudLocalForGrab(
        const Float3& cloudPosition,
        const Float3& worldPosition)
{
    const auto topLeftRelative =
        Float3(worldPosition.X - (cloudPosition.X - CLOUD_WIDTH), 0,
            worldPosition.Z - (cloudPosition.Z - CLOUD_HEIGHT));

    // Floor is used here because otherwise the last coordinate is wrong
    // and we don't want our caller to constantly have to call std::floor
    const auto localX = std::floor(topLeftRelative.X / CLOUD_RESOLUTION);
    const auto localY = std::floor(topLeftRelative.Z / CLOUD_RESOLUTION);

    return std::make_tuple(localX, localY);
}

Float3
    CompoundCloudSystem::calculateGridCenterForPlayerPos(const Float3& pos)
{
    // The gaps between the positions is used for calculations here. Otherwise
    // all clouds get moved when the player moves
    return Float3(
        static_cast<int>(std::round(pos.X / CLOUD_X_EXTENT)) * CLOUD_X_EXTENT,
        0,
        static_cast<int>(std::round(pos.Z / CLOUD_Y_EXTENT)) * CLOUD_Y_EXTENT);
}

// ------------------------------------ //
void
    CompoundCloudSystem::Run(CellStageWorld& world, float elapsed)
{
    if(!world.GetNetworkSettings().IsAuthoritative)
        return;

    Float3 position = Float3(0, 0, 0);

    // Hybrid client-server version
    if(ThriveGame::Get()) {

        auto playerEntity = ThriveGame::Get()->playerData().activeCreature();

        if(playerEntity == NULL_OBJECT) {

            LOG_WARNING(
                "CompoundCloudSystem: Run: playerData().activeCreature() "
                "is NULL_OBJECT. "
                "Using default position");
        } else {

            try {
                // Get the player's position.
                const Leviathan::Position& posEntity =
                    world.GetComponent_Position(playerEntity);
                position = posEntity.Members._Position;

            } catch(const Leviathan::NotFound&) {
                LOG_WARNING("CompoundCloudSystem: Run: playerEntity(" +
                            std::to_string(playerEntity) + ") has no position");
            }
        }
    }

    doSpawnCycle(world, position);

    for(auto& value : m_managedClouds) {

        if(!value.second->m_initialized) {
            LEVIATHAN_ASSERT(false, "CompoundCloudSystem spawned a cloud that "
                                    "it didn't initialize");
        }

        processCloud(*value.second, elapsed, world.GetFluidSystem());
    }
}

void
    CompoundCloudSystem::doSpawnCycle(CellStageWorld& world,
        const Float3& playerPos)
{
    // Initial spawning if everything is empty
    if(m_managedClouds.empty()) {

        m_cloudGridCenter = Float3(0, 0, 0);

        const auto requiredCloudPositions{
            calculateGridPositions(m_cloudGridCenter)};

        for(size_t i = 0; i < m_cloudTypes.size(); i += CLOUDS_IN_ONE) {

            // All positions
            for(const auto& pos : requiredCloudPositions) {
                _spawnCloud(world, pos, i);
            }
        }
    }
    // This rounds up to the nearest multiple of 4,
    // divides that by 4 and multiplies by 9 to get all the clouds we have
    // (if we have 5 compounds that are clouds, we need 18 clouds, if 4 we need
    // 9 etc)
    LEVIATHAN_ASSERT(m_managedClouds.size() ==
                         ((((m_cloudTypes.size() + 4 - 1) / 4 * 4) / 4) * 9),
        "A CompoundCloud entity has mysteriously been destroyed");

    // Calculate what our center should be
    const auto targetCenter = calculateGridCenterForPlayerPos(playerPos);

    // TODO: because we no longer check if the player has moved at least a bit
    // it is possible that this gets triggered very often if the player spins
    // around a cloud edge.

    // This needs an extra variable to track how much the player has moved
    // auto moved = playerPos - m_cloudGridCenter;

    if(m_cloudGridCenter != targetCenter) {

        m_cloudGridCenter = targetCenter;
        applyNewCloudPositioning();
    }
}

void
    CompoundCloudSystem::applyNewCloudPositioning()
{
    // Calculate the new positions
    const auto requiredCloudPositions{
        calculateGridPositions(m_cloudGridCenter)};

    // Reposition clouds according to the origin
    // The max amount of clouds is that all need to be moved
    const size_t MAX_FAR_CLOUDS = m_managedClouds.size();

    // According to spec this check is superfluous, but it makes me feel
    // better
    if(m_tooFarAwayClouds.size() != MAX_FAR_CLOUDS)
        m_tooFarAwayClouds.resize(MAX_FAR_CLOUDS);

    size_t farAwayIndex = 0;

    // All clouds that aren't at one of the requiredCloudPositions needs to
    // be moved. Also only one from each cloud group needs to be at each
    // position
    for(auto iter = m_managedClouds.begin(); iter != m_managedClouds.end();
        ++iter) {

        const auto pos = iter->second->m_position;

        bool matched = false;

        // Check if it is at any of the valid positions
        for(size_t i = 0; i < std::size(requiredCloudPositions); ++i) {

            const auto& requiredPos = requiredCloudPositions[i];

            // An exact check might work but just to be safe slight
            // inaccuracy is allowed here
            if((pos - requiredPos).HAddAbs() < Leviathan::EPSILON) {

                matched = true;
                break;
            }
        }

        if(!matched) {

            if(farAwayIndex >= MAX_FAR_CLOUDS) {

                LOG_FATAL("CompoundCloudSystem: Logic error in calculating "
                          "far away clouds that need to move");
                break;
            }

            m_tooFarAwayClouds[farAwayIndex++] = iter->second;
        }
    }

    // Move clouds that are too far away
    // We check through each position that should have a cloud and move one
    // where there isn't one. This also needs to take into account the cloud
    // groups

    // Loop through the cloud groups
    for(size_t c = 0; c < m_cloudTypes.size(); c += CLOUDS_IN_ONE) {

        const CompoundId groupType = m_cloudTypes[c].id;

        // Loop for moving clouds to all needed positions for each group
        for(size_t i = 0; i < std::size(requiredCloudPositions); ++i) {

            bool hasCloud = false;

            const auto& requiredPos = requiredCloudPositions[i];

            for(auto iter = m_managedClouds.begin();
                iter != m_managedClouds.end(); ++iter) {

                const auto pos = iter->second->m_position;
                // An exact check might work but just to be safe slight
                // inaccuracy is allowed here
                if(((pos - requiredPos).HAddAbs() < Leviathan::EPSILON)) {

                    // Check that the group of the cloud is correct
                    if(groupType == iter->second->getCompoundId1()) {
                        hasCloud = true;
                        break;
                    }
                }
            }

            if(hasCloud)
                continue;

            bool filled = false;

            // We need to find a cloud from the right group
            for(size_t checkReposition = 0; checkReposition < farAwayIndex;
                ++checkReposition) {

                if(m_tooFarAwayClouds[checkReposition] &&
                    m_tooFarAwayClouds[checkReposition]->getCompoundId1() ==
                        groupType) {

                    // Found a candidate
                    m_tooFarAwayClouds[checkReposition]->recycleToPosition(
                        requiredPos);

                    // Set to null to skip on next scan
                    m_tooFarAwayClouds[checkReposition] = nullptr;

                    filled = true;
                    break;
                }
            }

            if(!filled) {
                LOG_FATAL("CompoundCloudSystem: Logic error in moving far "
                          "clouds, didn't find any to use for needed pos");
                break;
            }
        }
    }

    // TODO: this can be removed once this has been fully confirmed to work fine
    // Errors about clouds that should have been moved but haven't been
    for(size_t checkReposition = 0; checkReposition < farAwayIndex;
        ++checkReposition) {
        if(m_tooFarAwayClouds[checkReposition]) {
            LOG_FATAL(
                "CompoundCloudSystem: Logic error in moving far "
                "clouds, a cloud that should have been moved wasn't moved");
        }
    }
}

void
    CompoundCloudSystem::_spawnCloud(CellStageWorld& world,
        const Float3& pos,
        size_t startIndex)
{
    auto entity = world.CreateEntity();
    Compound* first =
        startIndex < m_cloudTypes.size() ? &m_cloudTypes[startIndex] : nullptr;

    Compound* second = startIndex + 1 < m_cloudTypes.size() ?
                           &m_cloudTypes[startIndex + 1] :
                           nullptr;

    Compound* third = startIndex + 2 < m_cloudTypes.size() ?
                          &m_cloudTypes[startIndex + 2] :
                          nullptr;

    Compound* fourth = startIndex + 3 < m_cloudTypes.size() ?
                           &m_cloudTypes[startIndex + 3] :
                           nullptr;
    CompoundCloudComponent& cloud = world.Create_CompoundCloudComponent(
        entity, *this, first, second, third, fourth);
    m_managedClouds[entity] = &cloud;

    // Set correct position
    // TODO: this should probably be made a constructor parameter
    cloud.m_position = pos;

    initializeCloud(cloud, world.GetScene());
}


void
    CompoundCloudSystem::initializeCloud(CompoundCloudComponent& cloud,
        bs::Scene* scene)
{
    // All the densities
    if(cloud.m_compoundId1 != NULL_COMPOUND) {
        cloud.m_density1.resize(CLOUD_SIMULATION_WIDTH,
            std::vector<float>(CLOUD_SIMULATION_HEIGHT, 0));
        cloud.m_oldDens1.resize(CLOUD_SIMULATION_WIDTH,
            std::vector<float>(CLOUD_SIMULATION_HEIGHT, 0));
    }
    if(cloud.m_compoundId2 != NULL_COMPOUND) {
        cloud.m_density2.resize(CLOUD_SIMULATION_WIDTH,
            std::vector<float>(CLOUD_SIMULATION_HEIGHT, 0));
        cloud.m_oldDens2.resize(CLOUD_SIMULATION_WIDTH,
            std::vector<float>(CLOUD_SIMULATION_HEIGHT, 0));
    }
    if(cloud.m_compoundId3 != NULL_COMPOUND) {
        cloud.m_density3.resize(CLOUD_SIMULATION_WIDTH,
            std::vector<float>(CLOUD_SIMULATION_HEIGHT, 0));
        cloud.m_oldDens3.resize(CLOUD_SIMULATION_WIDTH,
            std::vector<float>(CLOUD_SIMULATION_HEIGHT, 0));
    }
    if(cloud.m_compoundId4 != NULL_COMPOUND) {
        cloud.m_density4.resize(CLOUD_SIMULATION_WIDTH,
            std::vector<float>(CLOUD_SIMULATION_HEIGHT, 0));
        cloud.m_oldDens4.resize(CLOUD_SIMULATION_WIDTH,
            std::vector<float>(CLOUD_SIMULATION_HEIGHT, 0));
    }

    cloud.m_initialized = true;

    // Skip if no graphics
    if(!Engine::Get()->IsInGraphicalMode())
        return;

    cloud.m_sceneNode = bs::SceneObject::create("cloud");

    cloud.m_renderable = cloud.m_sceneNode->addComponent<bs::CRenderable>();
    cloud.m_renderable->setLayer(1 << *scene);
    cloud.m_renderable->setMesh(m_planeMesh);


    // Set initial position
    cloud.m_sceneNode->setPosition(bs::Vector3(
        cloud.m_position.X, CLOUD_Y_COORDINATE, cloud.m_position.Z));

    cloud.m_textureData1 = bs::PixelData::create(
        CLOUD_SIMULATION_WIDTH, CLOUD_SIMULATION_HEIGHT, 1, BS_PIXEL_FORMAT);

    LEVIATHAN_ASSERT(bs::PixelUtil::getNumElemBytes(BS_PIXEL_FORMAT) ==
                         CLOUD_TEXTURE_BYTES_PER_ELEMENT,
        "Pixel format bytes has changed");

    // Fill with zeroes
    std::memset(static_cast<uint8_t*>(cloud.m_textureData1->getData()), 0,
        cloud.m_textureData1->getSize());

    // cloud.m_renderable->setCastShadows(false);

    // cloud.m_compoundCloudsPlane->setRenderQueueGroup(2);

    cloud.m_texture = bs::Texture::create(cloud.m_textureData1, bs::TU_DYNAMIC);

    // TODO: this should be loaded just once to be more efficient
    auto shader =
        Engine::Get()->GetGraphics()->LoadShaderByName("compound_cloud.bsl");

    bs::HMaterial material = bs::Material::create(shader);
    material->setTexture("gDensityTex", cloud.m_texture);

    // Set colour parameters //
    material->setVec4("gCloudColour1", cloud.m_color1);
    material->setVec4("gCloudColour2", cloud.m_color2);
    material->setVec4("gCloudColour3", cloud.m_color3);
    material->setVec4("gCloudColour4", cloud.m_color4);

    // The perlin noise texture needs to be tileable. We can't do tricks with
    // the cloud's position
    material->setTexture("gNoiseTex", m_perlinNoise);

    cloud.m_renderable->setMaterial(material);

    // cloud.m_planeMaterial->setReceiveShadows(false);
}
// ------------------------------------ //
void
    CompoundCloudSystem::cloudReportDestroyed(CompoundCloudComponent* cloud)
{
    for(auto iter = m_managedClouds.begin(); iter != m_managedClouds.end();
        ++iter) {

        if(iter->second == cloud) {
            m_managedClouds.erase(iter);
            return;
        }
    }

    LOG_WARNING("CompoundCloudSystem: non-registered CompoundCloudComponent "
                "reported that it was destroyed");
}
// ------------------------------------ //
void
    CompoundCloudSystem::processCloud(CompoundCloudComponent& cloud,
        float elapsed,
        FluidSystem& fluidSystem)
{
    elapsed *= 100.f;
    Float2 pos(cloud.m_position.X, cloud.m_position.Z);

    // The diffusion rate seems to have a bigger effect

    // Compound clouds move from area of high concentration to area of low.
    if(cloud.m_compoundId1 != NULL_COMPOUND) {
        diffuse(0.007f, cloud.m_oldDens1, cloud.m_density1, elapsed);
        // Move the compound clouds about the velocity field.
        advect(cloud.m_oldDens1, cloud.m_density1, elapsed, fluidSystem, pos);
    }
    if(cloud.m_compoundId2 != NULL_COMPOUND) {
        diffuse(0.007f, cloud.m_oldDens2, cloud.m_density2, elapsed);
        // Move the compound clouds about the velocity field.
        advect(cloud.m_oldDens2, cloud.m_density2, elapsed, fluidSystem, pos);
    }
    if(cloud.m_compoundId3 != NULL_COMPOUND) {
        diffuse(0.007f, cloud.m_oldDens3, cloud.m_density3, elapsed);
        // Move the compound clouds about the velocity field.
        advect(cloud.m_oldDens3, cloud.m_density3, elapsed, fluidSystem, pos);
    }
    if(cloud.m_compoundId4 != NULL_COMPOUND) {
        diffuse(0.007f, cloud.m_oldDens4, cloud.m_density4, elapsed);
        // Move the compound clouds about the velocity field.
        advect(cloud.m_oldDens4, cloud.m_density4, elapsed, fluidSystem, pos);
    }

    // No graphics check
    if(!cloud.m_texture)
        return;

    if(cloud.m_textureData1->isLocked()) {
        // Just skip for now. in the future we'll want two rotating buffers.
        // When the game lags and updates get queued is when this happens. Which
        // currently happens a lot so this is commented out
        // LOG_WARNING("CompoundCloud: texture data buffer is still locked, "
        //             "skipping writing new data");
        return;
    }

    const size_t rowBytes = cloud.m_textureData1->getRowPitch();
    uint8_t* const pDest = cloud.m_textureData1->getData();

    // Copy the density vector into the buffer.

    // This is probably branch predictor friendly to move each bunch of pixels
    // separately

    // Old Ogre info:
    // Even with that pixel format the actual channel indexes are:
    // PF_B8G8R8A8 for some reason
    // R - 2
    // G - 1
    // B - 0
    // A - 3
    // Channels should now be RGBA

    if(cloud.m_compoundId1 == NULL_COMPOUND)
        LEVIATHAN_ASSERT(false, "cloud with not even the first compound");

    // First density. R goes to channel 0 (see above for the mapping)
    fillCloudChannel(cloud.m_density1, 0, rowBytes, pDest);

    // Second. G - 1
    if(cloud.m_compoundId2 != NULL_COMPOUND)
        fillCloudChannel(cloud.m_density2, 1, rowBytes, pDest);
    // Third. B - 2
    if(cloud.m_compoundId3 != NULL_COMPOUND)
        fillCloudChannel(cloud.m_density3, 2, rowBytes, pDest);

    // Fourth. A - 3
    if(cloud.m_compoundId4 != NULL_COMPOUND)
        fillCloudChannel(cloud.m_density4, 3, rowBytes, pDest);

    // Submit the updated data
    cloud.m_texture->writeData(cloud.m_textureData1, 0, 0, true);
}

void
    CompoundCloudSystem::fillCloudChannel(
        const std::vector<std::vector<float>>& density,
        size_t index,
        size_t rowBytes,
        uint8_t* pDest)
{
    const auto height = density[0].size();
    for(size_t j = 0; j < height; j++) {
        for(size_t i = 0; i < density.size(); i++) {

            // This formula smoothens the cloud density so that we get gradients
            // of transparency.
            // TODO: move this to the shaders for better performance (we would
            // need to pass a float instead of a byte).
            int intensity =
                static_cast<int>(255 * 2 * std::atan(0.003f * density[i][j]));

            // This is the same clamping code as in the old version
            intensity = std::clamp(intensity, 0, 255);

            pDest[rowBytes * j + (i * CLOUD_TEXTURE_BYTES_PER_ELEMENT) +
                  index] = static_cast<uint8_t>(intensity);
        }
    }
}

void
    CompoundCloudSystem::diffuse(float diffRate,
        std::vector<std::vector<float>>& oldDens,
        const std::vector<std::vector<float>>& density,
        float dt)
{
    float a = dt * diffRate;
    for(int x = 1; x < CLOUD_SIMULATION_WIDTH - 1; x++) {
        for(int y = 1; y < CLOUD_SIMULATION_HEIGHT - 1; y++) {
            oldDens[x][y] = density[x][y] * (1 - a) +
                            (oldDens[x - 1][y] + oldDens[x + 1][y] +
                                oldDens[x][y - 1] + oldDens[x][y + 1]) *
                                a / 4;
        }
    }
}

void
    CompoundCloudSystem::advect(const std::vector<std::vector<float>>& oldDens,
        std::vector<std::vector<float>>& density,
        float dt,
        FluidSystem& fluidSystem,
        Float2 pos)
{
    for(int x = 0; x < CLOUD_SIMULATION_WIDTH; x++) {
        for(int y = 0; y < CLOUD_SIMULATION_HEIGHT; y++) {
            density[x][y] = 0;
        }
    }

    // TODO: this is probably the place to move the compounds on the edges into
    // the next cloud (instead of not handling them here)
    for(size_t x = 1; x < CLOUD_SIMULATION_WIDTH - 1; x++) {
        for(size_t y = 1; y < CLOUD_SIMULATION_HEIGHT - 1; y++) {
            if(oldDens[x][y] > 1) {
                constexpr float viscosity =
                    0.0525f; // TODO: give each cloud a viscosity value in the
                             // JSON file and use it instead.
                Float2 velocity = fluidSystem.getVelocityAt(
                                      pos + Float2(x, y) * CLOUD_RESOLUTION) *
                                  viscosity;

                float dx = x + dt * velocity.X;
                float dy = y + dt * velocity.Y;

                dx = std::clamp(dx, 0.5f, CLOUD_SIMULATION_WIDTH - 1.5f);
                dy = std::clamp(dy, 0.5f, CLOUD_SIMULATION_HEIGHT - 1.5f);

                const int x0 = static_cast<int>(dx);
                const int x1 = x0 + 1;
                const int y0 = static_cast<int>(dy);
                const int y1 = y0 + 1;

                float s1 = dx - x0;
                float s0 = 1.0f - s1;
                float t1 = dy - y0;
                float t0 = 1.0f - t1;

                density[x0][y0] += oldDens[x][y] * s0 * t0;
                density[x0][y1] += oldDens[x][y] * s0 * t1;
                density[x1][y0] += oldDens[x][y] * s1 * t0;
                density[x1][y1] += oldDens[x][y] * s1 * t1;
            }
        }
    }
}
