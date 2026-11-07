#pragma once
#include <cstdint>

namespace platform {

// Deterministic battery model for emulator experiments.
// Capacity/current defaults are provisional simulation parameters, not a
// verified GT-C3350 battery specification.
class Battery {
public:
    explicit Battery(std::uint32_t capacity_mah = 1000,
                     std::uint8_t initial_percent = 100);
    void set_charging(bool enabled) noexcept;
    void advance_seconds(std::uint32_t seconds) noexcept;
    [[nodiscard]] std::uint8_t percentage() const noexcept;
    [[nodiscard]] std::uint16_t voltage_mv() const noexcept;
    [[nodiscard]] std::uint32_t capacity_mah() const noexcept;
    [[nodiscard]] std::uint32_t remaining_mah() const noexcept;
    [[nodiscard]] bool charging() const noexcept;
    [[nodiscard]] bool full() const noexcept;
private:
    std::uint32_t capacity_mah_;
    std::uint64_t charge_milliamp_seconds_;
    std::uint32_t fractional_milliamp_seconds_ = 0;
    bool charging_ = false;
};

} // namespace platform
