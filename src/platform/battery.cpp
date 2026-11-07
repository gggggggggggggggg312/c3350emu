#include "platform/battery.hpp"
#include <algorithm>
#include <stdexcept>

namespace platform {
Battery::Battery(std::uint32_t capacity, std::uint8_t percent)
    : capacity_mah_(capacity),
      charge_milliamp_seconds_(static_cast<std::uint64_t>(capacity) *
                               std::min<unsigned>(percent, 100u) * 36u) {
    if (capacity == 0) throw std::invalid_argument("Battery capacity must be non-zero.");
}
void Battery::set_charging(bool enabled) noexcept {
    charging_ = enabled && !full();
}
void Battery::advance_seconds(std::uint32_t seconds) noexcept {
    if (seconds == 0) return;
    const std::uint64_t capacity_units = static_cast<std::uint64_t>(capacity_mah_) * 3600u;
    const std::uint64_t current = charging_ ? 300u : 180u;
    const std::uint64_t delta = current * seconds;
    if (charging_) charge_milliamp_seconds_ = std::min(capacity_units, charge_milliamp_seconds_ + delta);
    else charge_milliamp_seconds_ = delta >= charge_milliamp_seconds_ ? 0u : charge_milliamp_seconds_ - delta;
    if (charge_milliamp_seconds_ == capacity_units) charging_ = false;
}
std::uint8_t Battery::percentage() const noexcept {
    const auto capacity_units = static_cast<std::uint64_t>(capacity_mah_) * 3600u;
    return static_cast<std::uint8_t>((charge_milliamp_seconds_ * 100u) / capacity_units);
}
std::uint16_t Battery::voltage_mv() const noexcept {
    // Simple open-circuit approximation: 3.30 V empty to 4.20 V full.
    return static_cast<std::uint16_t>(3300u + (static_cast<unsigned>(percentage()) * 9u));
}
std::uint32_t Battery::capacity_mah() const noexcept { return capacity_mah_; }
std::uint32_t Battery::remaining_mah() const noexcept {
    return static_cast<std::uint32_t>(charge_milliamp_seconds_ / 3600u);
}
bool Battery::charging() const noexcept { return charging_; }
bool Battery::full() const noexcept {
    return charge_milliamp_seconds_ >= static_cast<std::uint64_t>(capacity_mah_) * 3600u;
}
} // namespace platform
