#include <type_traits>
#include <cstdint>
#include "Avionics.h"
#include "core/cli/Parser.h"
#include "core/cli/SimpleFlag.h"
#include "core/cli/ArgumentFlag.h"

template <typename T>
struct is_configuration_string : std::false_type {};

template <unsigned N>
struct is_configuration_string<ConfigurationString<N>> : std::true_type {};

template <typename T>
struct config_flag_type {
    using type = T;
};

template <unsigned N>
struct config_flag_type<ConfigurationString<N>> {
    using type = const char*;
};

template <unsigned ConfigurationID>
class ConfigurationCliBinding {
public:
    ConfigurationCliBinding() : m_setValueFlag("-set", "", false, 255, []() {}),
                                m_configurationFlag(command(), "", true, 255, [this]() { this->callback(); }) {}

    void setup(Configuration* configuration, Parser* parser, DebugStream* debugStream) {
        m_data = configuration->getConfigurable<ConfigurationID>();
        parser->addFlagGroup(m_configurationGeneratorGroup);
        m_debug = debugStream;
    }

    void callback() {
        if (m_setValueFlag.isSet()) {
            m_data.set(m_setValueFlag.getValueDerived());
            printValue(name(), "has been set to", m_data.get());
        } else {
            printValue(name(), "is set to", m_data.get());
        }
    }

private:
    static constexpr const char* name() {
        return GetConfigurationType_s<ConfigurationID>::name;
    }

    static constexpr const char* command() {
        return GetConfigurationType_s<ConfigurationID>::command;
    }

    using config_type_t = typename GetConfigurationType_s<ConfigurationID>::type;

    template <typename T>
    typename std::enable_if<is_configuration_string<T>::value>::type
    printValue(const char* nameStr, const char* msg, const T& value) {
        m_debug->message("%s %s: %s", nameStr, msg, value.str);
    }

    // Overload for floating point types
    template <typename T>
    typename std::enable_if<std::is_floating_point<T>::value>::type
    printValue(const char* nameStr, const char* msg, const T& value) {
        m_debug->message("%s %s: %.8f", nameStr, msg, static_cast<double>(value));
    }

    // Overload for unsigned integral types
    template <typename T>
    typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type
    printValue(const char* nameStr, const char* msg, const T& value) {
        m_debug->message("%s %s: %llu", nameStr, msg,
                         static_cast<unsigned long long>(value));
    }

    // Overload for signed integral types
    template <typename T>
    typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type
    printValue(const char* nameStr, const char* msg, const T& value) {
        m_debug->message("%s %s: %lld", nameStr, msg,
                         static_cast<long long>(value));
    }

    // Fallback for unsupported types
    template <typename T>
    // typename std::enable_if<!std::is_integral<T>::value && !std::is_floating_point<T>::value>::type
    typename std::enable_if<!std::is_integral<T>::value && !std::is_floating_point<T>::value && !is_configuration_string<T>::value>::type
    printValue(const char* nameStr, const char* msg, const T&) {
        m_debug->message("%s %s: (unsupported type)", nameStr, msg);
    }

    DebugStream* m_debug = nullptr;
    ConfigurationData<config_type_t> m_data{};
    // ArgumentFlag<config_type_t> m_setValueFlag{};
    ArgumentFlag<typename config_flag_type<config_type_t>::type> m_setValueFlag{};
    SimpleFlag m_configurationFlag;
    BaseFlag* m_configurationGeneratorGroup[2] = {&m_configurationFlag, &m_setValueFlag};
};

template <std::size_t... Is>
struct index_sequence {};

template <std::size_t N, std::size_t... Is>
struct make_index_sequence : make_index_sequence<N - 1, N - 1, Is...> {};

template <std::size_t... Is>
struct make_index_sequence<0, Is...> : index_sequence<Is...> {};


// Variadic wrapper
template <unsigned... Configs>
struct ConfigurationCliBindings {
    std::tuple<ConfigurationCliBinding<Configs>...> bindings;

    void setupAll(Configuration* configuration, Parser* parser, DebugStream* debugStream) {
        setupAllImpl(make_index_sequence<sizeof...(Configs)>(), configuration, parser, debugStream);
    }

private:
    template <std::size_t... Is>
    void setupAllImpl(index_sequence<Is...>, Configuration* configuration, Parser* parser, DebugStream* debugStream) {
        // Expand parameter pack using initializer list trick
        int dummy[] = {(std::get<Is>(bindings).setup(configuration, parser, debugStream), 0)...};
        (void)dummy; // suppress unused warning
    }
};
