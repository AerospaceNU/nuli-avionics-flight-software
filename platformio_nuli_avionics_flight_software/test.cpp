#include <iostream>
#include <tuple>
#include <cstdint>

// Enum for field identifiers
enum class ConfigField {
    Altitude,
    StartTime,
    SomethingHappened
};

// Template for a configuration entry
template <ConfigField Field, typename T>
struct ConfigEntry {
    using Type = T;
    static constexpr ConfigField field = Field;
    T value;
};

// ConfigStruct holding multiple entries
template <typename... Entries>
struct ConfigStruct {
    std::tuple<Entries...> entries;

    template <ConfigField Field, std::size_t Index = 0>
    auto& get() {
        if constexpr (Index >= sizeof...(Entries))
            static_assert(Index < sizeof...(Entries), "Field not found!");

        if constexpr (std::tuple_element_t<Index, std::tuple<Entries...>>::field == Field)
            return std::get<Index>(entries).value;
        else
            return get<Field, Index + 1>();
    }

    template <ConfigField Field, typename T, std::size_t Index = 0>
    void set(const T& newValue) {
        if constexpr (Index >= sizeof...(Entries))
            static_assert(Index < sizeof...(Entries), "Field not found!");

        if constexpr (std::tuple_element_t<Index, std::tuple<Entries...>>::field == Field)
            std::get<Index>(entries).value = newValue;
        else
            set<Field, T, Index + 1>(newValue);
    }

    void print() const {
        std::apply([](const auto&... entry) {
            ((std::cout << static_cast<int>(entry.field) << ": " << entry.value << '\n'), ...);
        }, entries);
    }
};

int main() {
    using MyConfig = ConfigStruct<
        ConfigEntry<ConfigField::Altitude, float>,
        ConfigEntry<ConfigField::StartTime, uint32_t>,
        ConfigEntry<ConfigField::SomethingHappened, bool>
    >;

    MyConfig config;

    // Access values
    std::cout << "Altitude: " << config.get<ConfigField::Altitude>() << '\n';

    // Modify values using .set()
    config.set<ConfigField::Altitude>(456.7f);
    config.set<ConfigField::SomethingHappened>(false);

    // Print modified values
    config.print();
    config.set<ConfigField::SomethingHappened>(true);

    // Print modified values
    config.print();

    return 0;
}