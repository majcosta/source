#include <cstdio>
#include <cstring>
#include <type_traits>

#include <vfs/Core/vfs.h>

namespace Wrappers {
template <typename T, typename = std::enable_if_t<std::is_same_v<T, char> ||
                                                  std::is_same_v<T, wchar_t>>>
class StringWrapper {
private:
  T *m_data; // STR8 was char*

public:
  // Constructor
  StringWrapper(const T *str = nullptr) : m_data(nullptr) {
    if (str) {
      size_t len = std::char_traits<T>::length(str);
      m_data = new T[len + 1];
      std::char_traits<T>::copy(m_data, str, len + 1);
    }
  }

  // Copy constructor
  StringWrapper(const StringWrapper &other) : m_data(nullptr) {
    if (other.m_data) {
      size_t len = std::char_traits<T>::length(other.m_data);
      m_data = new T[len + 1];
      std::char_traits<T>::copy(m_data, other.m_data, len + 1);
    }
  }

  StringWrapper(size_t len) : m_data(new T[len + 1]) {}

  // Implicitly convert int to a string
  StringWrapper(int string) : m_data(new T[12]) {
    if constexpr (std::is_same_v<T, char>) {
      std::sprintf(m_data, "%d", string);
    } else if constexpr (std::is_same_v<T, wchar_t>) {
      std::swprintf(m_data, 12, L"%d", string);
    }
  }

  // Move constructor
  StringWrapper(StringWrapper &&other) noexcept : m_data(other.m_data) {
    other.m_data = nullptr;
  }

  T* operator++(int) { return m_data++; }
  T& operator*() { return *m_data; }
  bool operator!() { return !m_data; }

  // Destructor
  ~StringWrapper() { delete[] m_data; }

  // Copy assignment operator
  StringWrapper &operator=(const StringWrapper &other) {
    if (this != &other) {
      delete[] m_data;
      if (other.m_data) {
        size_t len = std::char_traits<T>::length(other.m_data);
        m_data = new T[len + 1];
        std::char_traits<T>::copy(m_data, other.m_data, len + 1);
      } else {
        m_data = nullptr;
      }
    }
    return *this;
  }

  // Move assignment operator
  StringWrapper &operator=(StringWrapper &&other) noexcept {
    if (this != &other) {
      delete[] m_data;
      m_data = other.m_data;
      other.m_data = nullptr;
    }
    return *this;
  }

  // Conversion to const T*
  operator T *() const { return m_data; }
  operator const T *() const { return m_data; }
  explicit operator vfs::String() const { return vfs::String(m_data); }
  operator vfs::Path() const { return vfs::Path(m_data); }
  explicit operator bool() const { return m_data != nullptr; }
  

  // Friend or static function to overload sprintf within the namespace
  // Variadic template function to handle sprintf with all arguments variadic
  template <typename... Args>
  friend int sprintf(T *buffer, const T *format, Args... args) {
    if constexpr (std::is_same_v<T, char>) {
      return std::sprintf(buffer, format, static_cast<const char *>(args)...);
    } else if constexpr (std::is_same_v<T, wchar_t>) {
      return std::swprintf(buffer, 1024, format,
                           static_cast<const wchar_t *>(args)...);
    }
  }
  
  
  // Inequality comparison with signed char
  bool operator!=(const T *c) const {
    return !m_data || std::char_traits<T>::compare(
                          m_data, c, std::char_traits<T>::length(c));
  }
  bool operator==(const T *c) const { return m_data != c; }  
};

} // namespace Wrappers
