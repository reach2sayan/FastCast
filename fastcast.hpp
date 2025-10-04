//
// Created by sayan on 10/4/25.
//

#ifndef FASTCAST_FASTCAST_HPP
#define FASTCAST_FASTCAST_HPP
#ifndef FAST_DYNAMIC_CAST_H
#define FAST_DYNAMIC_CAST_H

#include <cstddef>
#include <limits>
#include <memory>
#include <type_traits>

namespace fast_dcast {
using v_table_ptr = const uintptr_t *;

constexpr std::ptrdiff_t NO_OFFSET = std::numeric_limits<std::ptrdiff_t>::max();

// Core implementation for pointer types
template <typename To, typename From> inline To cast_impl(From *ptr) {
  static_assert(std::is_polymorphic_v<From>, "Type is not polymorphic!");

  if (!ptr)
    return nullptr;

  thread_local static std::ptrdiff_t offset = NO_OFFSET;
  thread_local static v_table_ptr src_vtable_ptr = nullptr;

  // Get vtable pointer
  auto this_vtable = *reinterpret_cast<v_table_ptr *>(
      const_cast<std::remove_cv_t<From> *>(ptr));

  // Fast path: use cached offset if vtable matches
  if (offset != NO_OFFSET && src_vtable_ptr == this_vtable) {
    auto new_ptr = reinterpret_cast<std::byte *>(
                       const_cast<std::remove_cv_t<From> *>(ptr)) +
                   offset;
    return reinterpret_cast<To>(new_ptr);
  }

  // Slow path: perform dynamic_cast and cache result
  auto result = dynamic_cast<To>(ptr);
  if (result) {
    src_vtable_ptr = this_vtable;
    offset = reinterpret_cast<const std::byte *>(result) -
             reinterpret_cast<const std::byte *>(ptr);
  }
  return result;
}

// Pointer overload
template <typename To, typename From>
  requires std::is_pointer_v<To>
constexpr inline To fast_dynamic_cast(From *ptr) {
  using ToNonPtr = std::remove_pointer_t<To>;
  using ToPtr =
      std::conditional_t<std::is_const_v<From>, const ToNonPtr *, ToNonPtr *>;
  return cast_impl<ToPtr>(ptr);
}

// Reference overload
template <typename To, typename From>
  requires std::is_reference_v<To>
constexpr inline To fast_dynamic_cast(From &ref) {
  using ToPtr = std::add_pointer_t<std::remove_reference_t<To>>;
  auto casted_ptr = fast_dynamic_cast<ToPtr>(&ref);
  if (!casted_ptr)
    throw std::bad_cast{};
  return *casted_ptr;
}

// shared_ptr overload
template <typename To, typename From>
constexpr inline std::shared_ptr<To>
fast_dynamic_pointer_cast(const std::shared_ptr<From> &ptr) {
  return std::shared_ptr<To>(ptr, fast_dynamic_cast<To *>(ptr.get()));
}

// Identity cast (same type)
template <typename To, typename From>
  requires std::is_same_v<std::remove_cv_t<To>, std::remove_cv_t<From>>
constexpr inline To fast_dynamic_cast(To ptr) {
  return ptr;
}
} // namespace fast_dcast

using fast_dcast::fast_dynamic_cast;
using fast_dcast::fast_dynamic_pointer_cast;

#endif // FAST_DYNAMIC_CAST_H
#endif // FASTCAST_FASTCAST_HPP
