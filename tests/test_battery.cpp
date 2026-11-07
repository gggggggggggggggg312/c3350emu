#include "platform/battery.hpp"
#include <cassert>
int main() {
    platform::Battery battery(1000, 50);
    assert(battery.percentage() == 50);
    assert(battery.voltage_mv() == 3750);
    assert(battery.capacity_mah() == 1000);
    battery.set_charging(true);
    battery.advance_seconds(3600);
    assert(battery.percentage() == 80);
    assert(battery.charging());
    battery.set_charging(false);
    battery.advance_seconds(3600);
    assert(battery.percentage() == 62);
    platform::Battery full(800, 100);
    full.set_charging(true);
    assert(!full.charging());
    full.advance_seconds(100000);
    assert(full.percentage() == 100);
    platform::Battery empty(800, 0);
    empty.advance_seconds(100000);
    assert(empty.percentage() == 0);
    return 0;
}
