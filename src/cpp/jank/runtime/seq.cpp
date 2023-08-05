#include <iostream>
#include <sstream>

#include <fmt/core.h>

#include <jank/runtime/seq.hpp>
#include <jank/runtime/util.hpp>
#include <jank/runtime/hash.hpp>
#include <jank/runtime/obj/native_function_wrapper.hpp>
#include <jank/runtime/obj/vector.hpp>
#include <jank/runtime/obj/map.hpp>
#include <jank/runtime/behavior/seqable.hpp>
#include <jank/runtime/behavior/callable.hpp>
#include <jank/runtime/behavior/countable.hpp>
#include <jank/runtime/behavior/consable.hpp>
#include <jank/runtime/behavior/associatively_readable.hpp>
#include <jank/runtime/behavior/associatively_writable.hpp>

namespace jank::runtime
{
  namespace detail
  {
    size_t sequence_length(object_ptr const s)
    { return sequence_length(s, std::numeric_limits<size_t>::max()); }
    size_t sequence_length(object_ptr const s, size_t const max)
    {
      if(s == nullptr)
      { return 0; }

      return visit_object
      (
        [&](auto const typed_s) -> size_t
        {
          using T = typename decltype(typed_s)::value_type;

          if constexpr(std::same_as<T, obj::nil>)
          { return 0; }
          else if constexpr(behavior::countable<T>)
          { return typed_s->count(); }
          else if constexpr(behavior::seqable<T>)
          {
            size_t length{ 1 };
            for(auto i(typed_s->fresh_seq()); i != nullptr && length < max; i = i->next_in_place())
            { ++length; }
            return length;
          }
          else
          { throw std::runtime_error{ fmt::format("not seqable: {}", typed_s->to_string()) }; }
        },
        s
      );
    }
  }

  native_bool is_nil(object_ptr const o)
  { return (o == obj::nil::nil_const()); }
  native_bool is_some(object_ptr const o)
  { return (o != obj::nil::nil_const()); }

  object_ptr seq(object_ptr const s)
  {
    return visit_object
    (
      [](auto const typed_s) -> object_ptr
      {
        using T = typename decltype(typed_s)::value_type;

        if constexpr(std::same_as<T, obj::nil>)
        { return typed_s; }
        else if constexpr(behavior::seqable<T>)
        {
          auto const ret(typed_s->seq());
          if(!ret)
          { return obj::nil::nil_const(); }

          return ret;
        }
        else
        { throw std::runtime_error{ fmt::format("not seqable: {}", typed_s->to_string()) }; }
      },
      s
    );
  }

  object_ptr fresh_seq(object_ptr const s)
  {
    return visit_object
    (
      [](auto const typed_s) -> object_ptr
      {
        using T = typename decltype(typed_s)::value_type;

        if constexpr(std::same_as<T, obj::nil>)
        { return typed_s; }
        else if constexpr(behavior::seqable<T>)
        {
          auto const ret(typed_s->fresh_seq());
          if(!ret)
          { return obj::nil::nil_const(); }

          return ret;
        }
        else
        { throw std::runtime_error{ fmt::format("not seqable: {}", typed_s->to_string()) }; }
      },
      s
    );
  }

  object_ptr first(object_ptr const s)
  {
    return visit_object
    (
      [](auto const typed_s) -> object_ptr
      {
        using T = typename decltype(typed_s)::value_type;

        if constexpr(std::same_as<T, obj::nil>)
        { return typed_s; }
        else if constexpr(behavior::sequenceable<T>)
        { return typed_s->first(); }
        else if constexpr(behavior::seqable<T>)
        {
          auto const ret(typed_s->seq());
          if(!ret)
          { return obj::nil::nil_const(); }

          return ret->first();
        }
        else
        { throw std::runtime_error{ fmt::format("not seqable: {}", typed_s->to_string()) }; }
      },
      s
    );
  }

  object_ptr next(object_ptr const s)
  {
    return visit_object
    (
      [](auto const typed_s) -> object_ptr
      {
        using T = typename decltype(typed_s)::value_type;

        if constexpr(std::same_as<T, obj::nil>)
        { return typed_s; }
        else if constexpr(behavior::sequenceable<T>)
        {
          auto const ret(typed_s->next());
          if(!ret)
          { return obj::nil::nil_const(); }

          return ret;
        }
        else if constexpr(behavior::seqable<T>)
        {
          auto const s(typed_s->seq());
          if(!s)
          { return obj::nil::nil_const(); }

          auto const ret(s->next());
          if(!ret)
          { return obj::nil::nil_const(); }

          return ret;
        }
        else
        { throw std::runtime_error{ fmt::format("not seqable: {}", typed_s->to_string()) }; }
      },
      s
    );
  }

  object_ptr next_in_place(object_ptr const s)
  {
    return visit_object
    (
      [](auto const typed_s) -> object_ptr
      {
        using T = typename decltype(typed_s)::value_type;

        if constexpr(std::same_as<T, obj::nil>)
        { return typed_s; }
        else if constexpr(behavior::sequenceable<T>)
        { return typed_s->next_in_place(); }
        else if constexpr(behavior::seqable<T>)
        {
          auto const ret(typed_s->seq());
          if(!ret)
          { return obj::nil::nil_const(); }

          return ret->next_in_place();
        }
        else
        { throw std::runtime_error{ fmt::format("not seqable: {}", typed_s->to_string()) }; }
      },
      s
    );
  }

  object_ptr conj(object_ptr const s, object_ptr const o)
  {
    return visit_object
    (
      [&](auto const typed_s) -> object_ptr
      {
        using T = typename decltype(typed_s)::value_type;

        if constexpr(std::same_as<T, obj::nil>)
        { return typed_s; }
        else if constexpr(behavior::consable<T>)
        { return typed_s->cons(o); }
        else if constexpr(behavior::seqable<T>)
        { return typed_s->seq()->cons(o); }
        else
        { throw std::runtime_error{ fmt::format("not seqable: {}", typed_s->to_string()) }; }
      },
      s
    );
  }

  object_ptr assoc(object_ptr const m, object_ptr const k, object_ptr const v)
  {
    return visit_object
    (
      [&](auto const typed_m) -> object_ptr
      {
        using T = typename decltype(typed_m)::value_type;

        if constexpr(behavior::associatively_writable<T>)
        { return typed_m->assoc(k, v); }
        else
        { throw std::runtime_error{ fmt::format("not associatively writable: {}", typed_m->to_string()) }; }
      },
      m
    );
  }

  object_ptr get(object_ptr const m, object_ptr const key)
  {
    return visit_object
    (
      [&](auto const typed_m) -> object_ptr
      {
        using T = typename decltype(typed_m)::value_type;

        if constexpr(behavior::associatively_readable<T>)
        { return typed_m->get(key); }
        else
        { return obj::nil::nil_const(); }
      },
      m
    );
  }

  object_ptr get(object_ptr const m, object_ptr const key, object_ptr const fallback)
  {
    return visit_object
    (
      [&](auto const typed_m) -> object_ptr
      {
        using T = typename decltype(typed_m)::value_type;

        if constexpr(behavior::associatively_readable<T>)
        { return typed_m->get(key, fallback); }
        else
        { return obj::nil::nil_const(); }
      },
      m
    );
  }

  object_ptr get_in(object_ptr m, object_ptr keys)
  {
    return visit_object
    (
      [&](auto const typed_m) -> object_ptr
      {
        using T = typename decltype(typed_m)::value_type;

        if constexpr(behavior::associatively_readable<T>)
        {
          return visit_object
          (
            [&](auto const typed_keys) -> object_ptr
            {
              using T = typename decltype(typed_keys)::value_type;

              if constexpr(behavior::seqable<T>)
              {
                object_ptr ret{ typed_m };
                for(auto seq(typed_keys->fresh_seq()); seq != nullptr; seq = seq->next_in_place())
                { ret = get(ret, seq->first()); }
                return ret;
              }
              else
              { throw std::runtime_error{ fmt::format("not seqable: {}", typed_keys->to_string()) }; }
            },
            keys
          );
        }
        else
        { return obj::nil::nil_const(); }
      },
      m
    );
  }

  object_ptr get_in(object_ptr m, object_ptr keys, object_ptr fallback)
  {
    return visit_object
    (
      [&](auto const typed_m) -> object_ptr
      {
        using T = typename decltype(typed_m)::value_type;

        if constexpr(behavior::associatively_readable<T>)
        {
          return visit_object
          (
            [&](auto const typed_keys) -> object_ptr
            {
              using T = typename decltype(typed_keys)::value_type;

              if constexpr(behavior::seqable<T>)
              {
                object_ptr ret{ typed_m };
                for(auto seq(typed_keys->fresh_seq()); seq != nullptr; seq = seq->next_in_place())
                { ret = get(ret, seq->first()); }

                if(ret == obj::nil::nil_const())
                { return fallback; }
                return ret;
              }
              else
              { throw std::runtime_error{ fmt::format("not seqable: {}", typed_keys->to_string()) }; }
            },
            keys
          );
        }
        else
        { return obj::nil::nil_const(); }
      },
      m
    );
  }
}
