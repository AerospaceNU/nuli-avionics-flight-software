# NULI Avionics Flight Software
## Setup:
- Install clion
- Install platformio extension
- Install doxygen
- Add doxygen and g++ to windows path

## Formating Guide
### DO THE FOLLOWING:
- Indentation is 4 spaces
- Opening curley braces same line
- Use c style strings
- Make your class names, variable names, etc descriptive without acronyms
- Use dependency injection when designing classes
- Try to keep function short, to within one screen height when possible
- Code should be easy to read and understand
### AVOID THE FOLLOWING:
- Using "auto" when declaring variables
- Using static variables in functions
- Using dynamic memory on embedded systems
- Using reference variables that persist beyond a function
- Minimize complex object inheritance
### Doxygen
Doxygen is a code documentation generator. By formatting comments specifically, 
documentation can be easily generated. The following code block should be placed
before declarations of classes, enums, struts, etc (only the relevant lines).
```cpp
    /**
     * @brief A brief description
     * @details A detailed description (optional)
     * @param localVarName Argument description
     * @tparam TTemplateType Template argument description
     * @return Description of return value
     */
```
The following should be placed to the right of the declaration of member variables,
enumerated constants, constants, etc
```cpp
const int NAME = "";    ///< Description
```
### Variables 
- Everything should have descriptive names, without uncommon acronyms or words
- Used sized integer variables like int32_t when possible
```cpp
// Pointers (* on the type)
int32_t* pointerDeclaration;

// Class/struct member variables
int32_t m_classMemberVar = 0;    ///< Description
int32_t structMemberVar = 0;   ///< Description

// Constants
const char* const CONSTANT_NAME = "constantValue";  ///< Description
ENUM_CONSTANT_NAME,     ///< Description

// Local variables (don't require doxygen descriptions)
int32_t useFixedWithIntegers;
```
### Classes
- Class names are UpperCamel, methods are lowerCamel and members are m_lowerCamel
- Everything gets documented with doxygen as described above
- Dependency injection
  - Classes should take member objects as constructor arguments instead of constructing them internally
- Templated classes are OK, but don't get too fancy (no variadic or recursive templates)
- Order is always public/protected/private. Within each the order is data types/constructors/methods/member variables
- All member variables begin with m_
- Separate into .h and .cpp files
```cpp
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
```
### Structs, Enums, Typedefs and Functions
- Use postfix: _s, _e, _t
- Functions are camelCase()
```cpp
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
```