#include "NativeRegistry.hpp"
#include "Natives.hpp"

#include <mutex>
#include <utility>

namespace Sick::Game::Natives
{
    NativeRegistry& NativeRegistry::Get() noexcept
    {
        static NativeRegistry registry;
        return registry;
    }

    void NativeRegistry::Register(NativeMetadata metadata)
    {
        std::unique_lock lock(m_Mutex);
        m_ByName[metadata.name] = metadata.hash;
        m_ByHash[metadata.hash] = std::move(metadata);
    }

    void NativeRegistry::RegisterDefaults()
    {
        Clear();

        Register({Hashes::PLAYER_PED_ID, "PLAYER_PED_ID", "PLAYER", 0, false});
        Register({Hashes::PLAYER_ID, "PLAYER_ID", "PLAYER", 0, false});
        Register({Hashes::DOES_ENTITY_EXIST, "DOES_ENTITY_EXIST", "ENTITY", 1, false});
        Register({Hashes::SET_ENTITY_INVINCIBLE, "SET_ENTITY_INVINCIBLE", "ENTITY", 3, false});
        Register({Hashes::GET_PLAYER_WANTED_LEVEL, "GET_PLAYER_WANTED_LEVEL", "PLAYER", 1, false});
        Register({Hashes::SET_PLAYER_WANTED_LEVEL, "SET_PLAYER_WANTED_LEVEL", "PLAYER", 3, false});
        Register({Hashes::SET_PLAYER_WANTED_LEVEL_NOW, "SET_PLAYER_WANTED_LEVEL_NOW", "PLAYER", 2, false});
        Register({Hashes::CLEAR_PLAYER_WANTED_LEVEL, "CLEAR_PLAYER_WANTED_LEVEL", "PLAYER", 1, false});
        Register({Hashes::SET_SUPER_JUMP_THIS_FRAME, "SET_SUPER_JUMP_THIS_FRAME", "PLAYER", 1, false});
        Register({Hashes::SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER, "SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER", "PLAYER", 2, false});
        Register({Hashes::SET_SWIM_MULTIPLIER_FOR_PLAYER, "SET_SWIM_MULTIPLIER_FOR_PLAYER", "PLAYER", 2, false});
        Register({Hashes::SET_PED_MAX_TIME_UNDERWATER, "SET_PED_MAX_TIME_UNDERWATER", "PED", 2, false});
        Register({Hashes::SET_PED_CAN_RAGDOLL, "SET_PED_CAN_RAGDOLL", "PED", 2, false});
        Register({Hashes::SET_PED_CONFIG_FLAG, "SET_PED_CONFIG_FLAG", "PED", 3, false});
        Register({Hashes::CLEAR_PED_ENV_DIRT, "CLEAR_PED_ENV_DIRT", "PED", 1, false});
        Register({Hashes::RESET_PED_VISIBLE_DAMAGE, "RESET_PED_VISIBLE_DAMAGE", "PED", 1, false});
        Register({Hashes::CLEAR_PED_BLOOD_DAMAGE, "CLEAR_PED_BLOOD_DAMAGE", "PED", 1, false});
        Register({Hashes::SET_ENABLE_SCUBA, "SET_ENABLE_SCUBA", "PED", 2, false});
        Register({Hashes::SET_PED_DIES_IN_WATER, "SET_PED_DIES_IN_WATER", "PED", 2, false});
        Register({Hashes::SET_ENTITY_HAS_GRAVITY, "SET_ENTITY_HAS_GRAVITY", "ENTITY", 2, false});
        Register({Hashes::GET_VEHICLE_PED_IS_IN, "GET_VEHICLE_PED_IS_IN", "PED", 2, false});
        Register({Hashes::SET_ENTITY_COLLISION, "SET_ENTITY_COLLISION", "ENTITY", 3, false});
        Register({Hashes::SET_VEHICLE_FIXED, "SET_VEHICLE_FIXED", "VEHICLE", 1, false});
        Register({Hashes::SET_VEHICLE_DEFORMATION_FIXED, "SET_VEHICLE_DEFORMATION_FIXED", "VEHICLE", 1, false});
        Register({Hashes::SET_VEHICLE_DIRT_LEVEL, "SET_VEHICLE_DIRT_LEVEL", "VEHICLE", 2, false});
        Register({Hashes::SET_VEHICLE_ENGINE_ON, "SET_VEHICLE_ENGINE_ON", "VEHICLE", 4, false});
        Register({Hashes::SET_VEHICLE_ON_GROUND_PROPERLY, "SET_VEHICLE_ON_GROUND_PROPERLY", "VEHICLE", 2, false});
        Register({Hashes::SET_VEHICLE_ENGINE_HEALTH, "SET_VEHICLE_ENGINE_HEALTH", "VEHICLE", 2, false});
        Register({Hashes::SET_VEHICLE_BODY_HEALTH, "SET_VEHICLE_BODY_HEALTH", "VEHICLE", 2, false});
        Register({Hashes::SET_VEHICLE_PETROL_TANK_HEALTH, "SET_VEHICLE_PETROL_TANK_HEALTH", "VEHICLE", 2, false});
        Register({Hashes::GET_ENTITY_COORDS, "GET_ENTITY_COORDS", "ENTITY", 2, false});
        Register({Hashes::GET_ENTITY_HEADING, "GET_ENTITY_HEADING", "ENTITY", 1, false});
        Register({Hashes::IS_MODEL_IN_CDIMAGE, "IS_MODEL_IN_CDIMAGE", "STREAMING", 1, false});
        Register({Hashes::IS_MODEL_A_VEHICLE, "IS_MODEL_A_VEHICLE", "STREAMING", 1, false});
        Register({Hashes::REQUEST_MODEL, "REQUEST_MODEL", "STREAMING", 1, false});
        Register({Hashes::HAS_MODEL_LOADED, "HAS_MODEL_LOADED", "STREAMING", 1, false});
        Register({Hashes::SET_MODEL_AS_NO_LONGER_NEEDED, "SET_MODEL_AS_NO_LONGER_NEEDED", "STREAMING", 1, false});
        Register({Hashes::CREATE_VEHICLE, "CREATE_VEHICLE", "VEHICLE", 7, false});
        Register({Hashes::SET_PED_INTO_VEHICLE, "SET_PED_INTO_VEHICLE", "PED", 3, false});
    }

    void NativeRegistry::Clear()
    {
        std::unique_lock lock(m_Mutex);
        m_ByHash.clear();
        m_ByName.clear();
    }

    std::optional<NativeMetadata> NativeRegistry::Find(NativeHash hash) const
    {
        std::shared_lock lock(m_Mutex);
        const auto it = m_ByHash.find(hash);
        if (it == m_ByHash.end())
            return std::nullopt;
        return it->second;
    }

    std::optional<NativeMetadata> NativeRegistry::Find(std::string_view name) const
    {
        std::shared_lock lock(m_Mutex);
        const auto nameIt = m_ByName.find(std::string{name});
        if (nameIt == m_ByName.end())
            return std::nullopt;
        const auto hashIt = m_ByHash.find(nameIt->second);
        if (hashIt == m_ByHash.end())
            return std::nullopt;
        return hashIt->second;
    }

    std::vector<NativeMetadata> NativeRegistry::All() const
    {
        std::shared_lock lock(m_Mutex);
        std::vector<NativeMetadata> result;
        result.reserve(m_ByHash.size());
        for (const auto& [_, metadata] : m_ByHash)
            result.push_back(metadata);
        return result;
    }

    std::size_t NativeRegistry::Size() const noexcept
    {
        std::shared_lock lock(m_Mutex);
        return m_ByHash.size();
    }
}
