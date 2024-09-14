#include <cstdint>

class OtherClass {

};

/**
 * General notes:
 * DO THE FOLLOWING:
 *      - Indentation is 4 spaces
 *      - Opening curley braces same line
 *      - Use c style strings
 *      - Make your class names, variable names, etc descriptive without acronyms
 *      - Use dependency injection when designing classes
 *      - Try to keep function short, to within one screen height when possible
 *      - Code should be easy to read and understand
 * AVOID THE FOLLOWING:
 *      - Using "auto" when declaring variables
 *      - Using static variables in functions
 *      - Using dynamic memory on embedded systems
 *      - Using reference variables that persist beyond a function
 *      - Minimize complex object inheritance
 */

/**
 * @brief A brief description
 * @details A detailed description (optional)
 * @tparam TTemplateType Template argument description
 */
template<typename TTemplateType>
class ClassName {
public:
    /**
     * @brief A brief description
     * @details A detailed description (optional)
     * @param dependency Argument description
     */
    explicit ClassName(OtherClass* dependency);

    /**
     * @brief A brief description
     * @details A detailed description (optional)
     * @param localVarName Argument description
     * @return Description of return value
     */
    int32_t methodName(double localVarName);

    // Variables go after the methods in each public/protected/private section
    // Public variables are sometimes nice, but are often best hidden behind an API
    int32_t m_publicMemberVar = 0;                              ///< Description

protected:
    // protected methods
    // protected variables

private:
    OtherClass* m_dependency;                                   ///< Description
    double m_memberCamelCase = 0;                               ///< Description

};

// Use .h files for declaring and .cpp files for definitions:
template<typename TTemplateType>
ClassName<TTemplateType>::ClassName(OtherClass* dependency) {
    m_dependency = dependency;
}

template<typename TTemplateType>
int32_t ClassName<TTemplateType>::methodName(double localVarName) {
    return 0;
}


/**
 * @brief A brief description
 * @details A detailed description (optional)
 */
struct StructName_s {
    double camelCase;                                           ///< Description
    int32_t otherVar;                                           ///< Description
};

/**
 * @enum TypeNameNoPlural_e
 * @brief A brief description
 * @details A detailed description (optional)
 */
enum TypeNameNoPlural_e : uint8_t { // Explicitly control the size
    NAME,                                                       ///< Description
    OTHER_NAME = 5,                                             ///< Description
};

typedef ClassName<StructName_s> TypeName_t;                     ///< Description

// Constants
const char* const CONSTANT_NAME = "constantValue";              ///< Description



void functionName() {
    int32_t useFixedWithIntegers;   // Local variables don't need descriptions, but should have descriptive names
    int32_t* pointerDeclaration;
}
