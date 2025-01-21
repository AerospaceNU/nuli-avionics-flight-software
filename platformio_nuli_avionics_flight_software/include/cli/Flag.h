//
// Created by chris on 1/6/2025.
//

#ifndef DESKTOP_FLAG_H
#define DESKTOP_FLAG_H

#include <string>
#include <cstdint>

/**
 * @class
 * @brief
 */
class BaseFlag {
public:
    virtual ~BaseFlag() = default;

    virtual const char* name() const = 0;

    virtual const char* help() const = 0;

    virtual void parse(const char* value) = 0;

    virtual bool isSet() const = 0;

    virtual bool isRequired() const = 0;

    template <typename T>
    T getValue();

protected:
private:
};



/**
 * @class
 * @brief
 */
class SimpleFlag : public BaseFlag {
public:
    SimpleFlag(const char* name, const char* helpText, bool m_required);

    const char* name() const override;

    const char* help() const override;

    void parse(const char* value) override;

    bool isSet() const override;

    bool isRequired() const override;

    bool getValueDerived() const {
        return isSet();
    }

protected:
private:
    bool m_set;
    bool m_required;
    const char* m_name;
    const char* m_helpText;
};


/**
 * @class
 * @brief
 */
template<typename T>
class ArgumentFlag : public BaseFlag {
public:
    ArgumentFlag(const char* name, T defaultValue, const char* helpText, bool m_required);

    const char* name() const override;

    const char* help() const override;

    void parse(const char* value) override;

    bool isSet() const override;

    bool isRequired() const override;

    T getValueDerived() const {
        return m_argument;
    }

protected:
private:
    bool m_set;
    bool m_required;
    const char* m_name;
    const char* m_helpText;
    T m_argument;
};

#include "Flag.tpp"
#endif //DESKTOP_FLAG_H
