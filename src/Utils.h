#pragma once
#include <cmath>
#include <iostream>
#include <ctime>
#include <iomanip>

#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#define INFO(msg_)  std::cout << " INFO    " << msg_ << " |" << __FILENAME__ << ":" << __LINE__ << '\n'
#define WARN(msg_)  std::cout << " WARN    " << msg_ << " |" << __FILENAME__ << ":" << __LINE__ << '\n'
#define ERROR(msg_) std::cout << " ERROR   " << msg_ << " |" << __FILENAME__ << ":" << __LINE__ << '\n'

#define LOG_VAR(var_) #var_ << "='" << var_ << "', "
#define LOG_NVP(name_, var_) name_ << "=" << var_ << " "

#define ENUM_MACRO_5(name, v1, v2, v3, v4, v5)                                                                                                       \
    enum class name { v1, v2, v3, v4, v5, Unknown  };                                                                                                          \
    inline std::string enum2str(name value) { const char *name##Strings[] = { #v1, #v2, #v3, #v4, #v5, "Unknown" }; return std::string(name##Strings[(int)value]); } \
    inline std::ostream& operator<<(std::ostream& stream_, name val_ ) { return stream_ << enum2str(val_);}                                          \
    template<>                                                                                                                                       \
    inline name str2enum(const char * value) { return (enum2str(name::v1) == value) ? name::v1 :                                                     \
                                                      (enum2str(name::v2) == value) ? name::v2 :                                                     \
                                                      (enum2str(name::v3) == value) ? name::v3 :                                                     \
                                                      (enum2str(name::v4) == value) ? name::v4 :                                                     \
                                                      (enum2str(name::v5) == value) ? name::v5 : name::Unknown; }

#define ENUM_MACRO_4(name, v1, v2, v3, v4 )                                                                                                      \
    enum class name { v1, v2, v3, v4, Unknown };                                                                                                          \
    inline std::string enum2str(name value) { const char *name##Strings[] = { #v1, #v2, #v3, #v4, "Unknown" }; return std::string(name##Strings[(int)value]); } \
    inline std::ostream& operator<<(std::ostream& stream_, name val_ ) { return stream_ << enum2str(val_);}                                      \
    template<>                                                                                                                                   \
    inline name str2enum(const char * value) { return (enum2str(name::v1) == value) ? name::v1 :                                                 \
                                                      (enum2str(name::v2) == value) ? name::v2 :                                                 \
                                                      (enum2str(name::v3) == value) ? name::v3 :                                                 \
                                                      (enum2str(name::v4) == value) ? name::v4 : name::Unknown; }

#define ENUM_MACRO_2(name, v1, v2)                                                                                                  \
    enum class name {v1,v2, Unknown };                                                                                               \
    inline std::string enum2str(name value) { const char *name##Strings[]={#v1, #v2, "Unknown" }; return std::string(name##Strings[(int)value]); } \
    inline std::ostream& operator<<(std::ostream& stream_, name val_ ) { return stream_ << enum2str(val_);}                         \
    template<>                                                                                                                      \
    inline name str2enum(const char * value) { return (enum2str(name::v1) == value) ? name::v1 :                                    \
                                                      (enum2str(name::v2) == value) ? name::v2 : name::Unknown; }

#define ENUM_MACRO_3(name, v1, v2, v3)                                                                                                \
    enum class name {v1,v2, v3, Unknown };                                                                                                      \
    inline std::string enum2str(name value) { const char *name##Strings[]={#v1, #v2, #v3, "Unknown" }; return std::string(name##Strings[(int)value]);} \
    inline std::ostream& operator<<(std::ostream& stream_, name val_ ) { return stream_ << enum2str(val_);}                           \
    template<>                                                                                                                        \
    inline name str2enum(const char * value) { return (enum2str(name::v1) == value) ? name::v1 :                                      \
                                                      (enum2str(name::v2) == value) ? name::v2 :                                      \
                                                      (enum2str(name::v3) == value) ? name::v3 : name::Unknown; }

#define ENUM_MACRO_6(name, v1, v2, v3, v4, v5, v6)                                                                                                         \
    enum class name { v1, v2, v3, v4, v5, v6, Unknown };                                                                                                            \
    inline std::string enum2str(name value) { const char *name##Strings[] = { #v1, #v2, #v3, #v4, #v5, #v6, "Unknown" }; return std::string(name##Strings[(int)value]); } \
    inline std::ostream& operator<<(std::ostream& stream_, name val_ ) { return stream_ << enum2str(val_);}                                                \
    template<>                                                                                                                                             \
    inline name str2enum(const char * value) { return (enum2str(name::v1) == value) ? name::v1 :                                                           \
                                                      (enum2str(name::v2) == value) ? name::v2 :                                                           \
                                                      (enum2str(name::v3) == value) ? name::v3 :                                                           \
                                                      (enum2str(name::v4) == value) ? name::v4:                                                            \
                                                      (enum2str(name::v5) == value) ? name::v5:                                                            \
                                                      (enum2str(name::v6) == value) ? name::v6 : name::Unknown; }
#define ENUM_MACRO_7(name, v1, v2, v3, v4, v5, v6, v7)                                                                                                         \
    enum class name { v1, v2, v3, v4, v5, v6, v7, Unknown };                                                                                                            \
    inline std::string enum2str(name value) { const char *name##Strings[] = { #v1, #v2, #v3, #v4, #v5, #v6, #v7, "Unknown" }; return std::string(name##Strings[(int)value]); } \
    inline std::ostream& operator<<(std::ostream& stream_, name val_ ) { return stream_ << enum2str(val_);}                                                \
    template<>                                                                                                                                             \
    inline name str2enum(const char * value) { return (enum2str(name::v1) == value) ? name::v1 :                                                           \
                                                      (enum2str(name::v2) == value) ? name::v2 :                                                           \
                                                      (enum2str(name::v3) == value) ? name::v3 :                                                           \
                                                      (enum2str(name::v4) == value) ? name::v4:                                                            \
                                                      (enum2str(name::v5) == value) ? name::v5:                                                            \
                                                      (enum2str(name::v6) == value) ? name::v6 :                                                               \
                                                      (enum2str(name::v7) == value) ? name::v7 : name::Unknown; }
template<typename T> inline const char* enum2str(T) {return "";}
template<typename T> inline T str2enum(const char* value) {return enum2str<T>(value);} 


template<typename T>
bool almost_equal(T num1, T num2)
{
    return std::abs(num1 - num2) < 0.000001;
}
template<typename T>
bool less_than(T num1, T num2)
{
    return !almost_equal(num1, num2) and num1 < num2;
}
template<typename T>
bool greater_than(T num1, T num2)
{
    return !almost_equal(num1, num2) and num1 > num2;
}

template<typename T>
bool greater_equal(T num1, T num2)
{
    return almost_equal(num1, num2) or num1 > num2;
}

template<typename T>
bool less_equal(T num1, T num2)
{
    return almost_equal(num1, num2) or num1 < num2;
}
