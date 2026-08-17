#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Sick::Handling
{
    enum class Group : std::uint8_t
    {
        General,
        Transmission,
        Brakes,
        Steering,
        Traction,
        Suspension,
        AntiRoll,
        RollCentre,
        Other,
    };

    enum class Field : std::uint8_t
    {
        Mass,
        InitialDragCoeff,
        PercentSubmerged,
        CentreOfMassX,
        CentreOfMassY,
        CentreOfMassZ,
        InertiaX,
        InertiaY,
        InertiaZ,
        DriveBiasFront,
        InitialDriveGears,
        InitialDriveForce,
        DriveInertia,
        ClutchChangeRateScaleUpShift,
        ClutchChangeRateScaleDownShift,
        InitialDriveMaxFlatVelocity,
        BrakeForce,
        BrakeBiasFront,
        HandBrakeForce,
        SteeringLock,
        TractionCurveMax,
        TractionCurveMin,
        TractionCurveLateral,
        TractionSpringDeltaMax,
        LowSpeedTractionLossMult,
        CamberStiffness,
        TractionBiasFront,
        TractionLossMult,
        SuspensionForce,
        SuspensionCompDamp,
        SuspensionReboundDamp,
        SuspensionUpperLimit,
        SuspensionLowerLimit,
        SuspensionRaise,
        SuspensionBiasFront,
        AntiRollBarForce,
        AntiRollBarBiasFront,
        RollCentreHeightFront,
        RollCentreHeightRear,
        CollisionDamageMult,
        WeaponDamageMult,
        DeformationDamageMult,
        EngineDamageMult,
        Count,
    };

    inline constexpr std::size_t FieldCount = static_cast<std::size_t>(Field::Count);
    static_assert(FieldCount <= 64, "Handling dirty mask requires at most 64 fields");

    struct FieldSpec
    {
        Field field{};
        Group group{};
        const char* key{};
        const char* label{};
        float minimum{};
        float maximum{};
        float step{};
        std::uint8_t precision{};
        bool integral{};
        const char* description{};
    };

    inline constexpr std::array<FieldSpec, FieldCount> FieldSpecs{{
        {Field::Mass, Group::General, "mass", "Mass", 1.0F, 10000.0F, 50.0F, 0, false, "Vehicle mass used by the handling model."},
        {Field::InitialDragCoeff, Group::General, "initial_drag_coeff", "Initial Drag Coeff", 0.0F, 100.0F, 0.1F, 2, false, "Aerodynamic drag coefficient."},
        {Field::PercentSubmerged, Group::General, "percent_submerged", "Percent Submerged", 0.0F, 200.0F, 0.5F, 1, false, "Submersion percentage used for buoyancy behavior."},
        {Field::CentreOfMassX, Group::General, "centre_of_mass_x", "Centre Of Mass X", -5.0F, 5.0F, 0.05F, 2, false, "Moves the handling centre of mass left or right."},
        {Field::CentreOfMassY, Group::General, "centre_of_mass_y", "Centre Of Mass Y", -5.0F, 5.0F, 0.05F, 2, false, "Moves the handling centre of mass forward or backward."},
        {Field::CentreOfMassZ, Group::General, "centre_of_mass_z", "Centre Of Mass Z", -5.0F, 5.0F, 0.05F, 2, false, "Moves the handling centre of mass vertically."},
        {Field::InertiaX, Group::General, "inertia_x", "Inertia X", 0.05F, 10.0F, 0.05F, 2, false, "X-axis inertia multiplier."},
        {Field::InertiaY, Group::General, "inertia_y", "Inertia Y", 0.05F, 10.0F, 0.05F, 2, false, "Y-axis inertia multiplier."},
        {Field::InertiaZ, Group::General, "inertia_z", "Inertia Z", 0.05F, 10.0F, 0.05F, 2, false, "Z-axis inertia multiplier."},

        {Field::DriveBiasFront, Group::Transmission, "drive_bias_front", "Drive Bias Front", 0.0F, 1.0F, 0.01F, 2, false, "Front-wheel drive bias: 0 rear, 0.5 AWD, 1 front."},
        {Field::InitialDriveGears, Group::Transmission, "initial_drive_gears", "Initial Drive Gears", 1.0F, 10.0F, 1.0F, 0, true, "Number of forward transmission gears."},
        {Field::InitialDriveForce, Group::Transmission, "initial_drive_force", "Initial Drive Force", 0.0F, 5.0F, 0.01F, 2, false, "Primary acceleration / drive-force value."},
        {Field::DriveInertia, Group::Transmission, "drive_inertia", "Drive Inertia", 0.0F, 10.0F, 0.05F, 2, false, "Controls how quickly the drivetrain changes speed."},
        {Field::ClutchChangeRateScaleUpShift, Group::Transmission, "clutch_upshift", "Clutch Upshift Rate", 0.0F, 20.0F, 0.1F, 2, false, "Clutch change-rate multiplier while shifting up."},
        {Field::ClutchChangeRateScaleDownShift, Group::Transmission, "clutch_downshift", "Clutch Downshift Rate", 0.0F, 20.0F, 0.1F, 2, false, "Clutch change-rate multiplier while shifting down."},
        {Field::InitialDriveMaxFlatVelocity, Group::Transmission, "initial_drive_max_flat_velocity", "Max Flat Velocity", 0.0F, 500.0F, 1.0F, 1, false, "Handling top-speed value exposed in vehicle handling data."},

        {Field::BrakeForce, Group::Brakes, "brake_force", "Brake Force", 0.0F, 10.0F, 0.05F, 2, false, "Overall braking force."},
        {Field::BrakeBiasFront, Group::Brakes, "brake_bias_front", "Brake Bias Front", 0.0F, 1.0F, 0.01F, 2, false, "Front-to-rear service-brake bias."},
        {Field::HandBrakeForce, Group::Brakes, "handbrake_force", "Hand Brake Force", 0.0F, 10.0F, 0.05F, 2, false, "Parking / hand-brake force."},

        {Field::SteeringLock, Group::Steering, "steering_lock", "Steering Lock", 1.0F, 90.0F, 0.5F, 1, false, "Maximum steering-lock angle in degrees."},

        {Field::TractionCurveMax, Group::Traction, "traction_curve_max", "Traction Curve Max", 0.0F, 10.0F, 0.01F, 2, false, "Peak tire traction."},
        {Field::TractionCurveMin, Group::Traction, "traction_curve_min", "Traction Curve Min", 0.0F, 10.0F, 0.01F, 2, false, "Minimum tire traction after the peak."},
        {Field::TractionCurveLateral, Group::Traction, "traction_curve_lateral", "Traction Curve Lateral", 0.0F, 90.0F, 0.5F, 1, false, "Lateral traction angle."},
        {Field::TractionSpringDeltaMax, Group::Traction, "traction_spring_delta_max", "Traction Spring Delta Max", 0.01F, 5.0F, 0.01F, 2, false, "Maximum traction spring delta."},
        {Field::LowSpeedTractionLossMult, Group::Traction, "low_speed_traction_loss_mult", "Low Speed Traction Loss", 0.0F, 10.0F, 0.05F, 2, false, "Traction-loss multiplier at low speed."},
        {Field::CamberStiffness, Group::Traction, "camber_stiffness", "Camber Stiffness", 0.0F, 10.0F, 0.05F, 2, false, "Camber stiffness handling value."},
        {Field::TractionBiasFront, Group::Traction, "traction_bias_front", "Traction Bias Front", 0.0F, 1.0F, 0.01F, 2, false, "Front-to-rear traction bias."},
        {Field::TractionLossMult, Group::Traction, "traction_loss_mult", "Traction Loss Mult", 0.0F, 10.0F, 0.05F, 2, false, "General traction-loss multiplier."},

        {Field::SuspensionForce, Group::Suspension, "suspension_force", "Suspension Force", 0.0F, 10.0F, 0.05F, 2, false, "Suspension spring force."},
        {Field::SuspensionCompDamp, Group::Suspension, "suspension_comp_damp", "Compression Damp", 0.0F, 20.0F, 0.05F, 2, false, "Suspension compression damping."},
        {Field::SuspensionReboundDamp, Group::Suspension, "suspension_rebound_damp", "Rebound Damp", 0.0F, 20.0F, 0.05F, 2, false, "Suspension rebound damping."},
        {Field::SuspensionUpperLimit, Group::Suspension, "suspension_upper_limit", "Upper Limit", -1.0F, 1.0F, 0.01F, 2, false, "Upper suspension travel limit."},
        {Field::SuspensionLowerLimit, Group::Suspension, "suspension_lower_limit", "Lower Limit", -1.0F, 1.0F, 0.01F, 2, false, "Lower suspension travel limit."},
        {Field::SuspensionRaise, Group::Suspension, "suspension_raise", "Suspension Raise", -1.0F, 1.0F, 0.01F, 2, false, "Raises or lowers the vehicle handling suspension."},
        {Field::SuspensionBiasFront, Group::Suspension, "suspension_bias_front", "Suspension Bias Front", 0.0F, 1.0F, 0.01F, 2, false, "Front-to-rear suspension force bias."},

        {Field::AntiRollBarForce, Group::AntiRoll, "anti_roll_bar_force", "Anti-Roll Bar Force", 0.0F, 10.0F, 0.05F, 2, false, "Anti-roll bar force."},
        {Field::AntiRollBarBiasFront, Group::AntiRoll, "anti_roll_bar_bias_front", "Anti-Roll Bias Front", 0.0F, 1.0F, 0.01F, 2, false, "Front-to-rear anti-roll bar bias."},

        {Field::RollCentreHeightFront, Group::RollCentre, "roll_centre_height_front", "Roll Centre Front", -2.0F, 2.0F, 0.01F, 2, false, "Front roll-centre height."},
        {Field::RollCentreHeightRear, Group::RollCentre, "roll_centre_height_rear", "Roll Centre Rear", -2.0F, 2.0F, 0.01F, 2, false, "Rear roll-centre height."},

        {Field::CollisionDamageMult, Group::Other, "collision_damage_mult", "Collision Damage Mult", 0.0F, 10.0F, 0.05F, 2, false, "Handling collision-damage multiplier."},
        {Field::WeaponDamageMult, Group::Other, "weapon_damage_mult", "Weapon Damage Mult", 0.0F, 10.0F, 0.05F, 2, false, "Handling weapon-damage multiplier."},
        {Field::DeformationDamageMult, Group::Other, "deformation_damage_mult", "Deformation Damage Mult", 0.0F, 10.0F, 0.05F, 2, false, "Handling deformation-damage multiplier."},
        {Field::EngineDamageMult, Group::Other, "engine_damage_mult", "Engine Damage Mult", 0.0F, 10.0F, 0.05F, 2, false, "Handling engine-damage multiplier."},
    }};

    using Values = std::array<float, FieldCount>;

    [[nodiscard]] constexpr std::size_t ToIndex(Field field) noexcept
    {
        return static_cast<std::size_t>(field);
    }

    [[nodiscard]] constexpr const FieldSpec& Spec(Field field) noexcept
    {
        return FieldSpecs[ToIndex(field)];
    }

    inline constexpr std::uint64_t AllFieldMask =
        FieldCount == 64 ? ~std::uint64_t{} : ((std::uint64_t{1} << FieldCount) - 1U);
}
