#pragma once

namespace utils {

/*
Hints:
0. only make sure to accept copy-constructable objects only (see 'constructor' page on the cppreference.com)
1. aligned_storage<sizeof(_M_ptr), alignof(void*)>::type _M_buffer; - this is used for small object optimization
2. 
	union _Storage
    {
      constexpr _Storage() : _M_ptr{nullptr} {}

      // Prevent trivial copies of this type, buffer might hold a non-POD.
      _Storage(const _Storage&) = delete;
      _Storage& operator=(const _Storage&) = delete;

      void* _M_ptr;
      aligned_storage<sizeof(_M_ptr), alignof(void*)>::type _M_buffer;
    };

	// fields
	 enum _Op {
	_Op_access, _Op_get_type_info, _Op_clone, _Op_destroy, _Op_xfer
    };

    union _Arg
    {
	void* _M_obj;
	const std::type_info* _M_typeinfo;
	any* _M_any;
    };

    void (*_M_manager)(_Op, const any*, _Arg*);
    _Storage _M_storage;



	// _S_manage func impl
	
  template<typename _Tp>
    void
    any::_Manager_external<_Tp>::
    _S_manage(_Op __which, const any* __any, _Arg* __arg)
    {
      // The contained object is *_M_storage._M_ptr
      auto __ptr = static_cast<const _Tp*>(__any->_M_storage._M_ptr);
      switch (__which)
      {
      case _Op_access:
	__arg->_M_obj = const_cast<_Tp*>(__ptr);
	break;
      case _Op_get_type_info:
#if __cpp_rtti
	__arg->_M_typeinfo = &typeid(_Tp);
#endif
	break;
      case _Op_clone:
	__arg->_M_any->_M_storage._M_ptr = new _Tp(*__ptr);
	__arg->_M_any->_M_manager = __any->_M_manager;
	break;
      case _Op_destroy:
	delete __ptr;
	break;
      case _Op_xfer:
	__arg->_M_any->_M_storage._M_ptr = __any->_M_storage._M_ptr;
	__arg->_M_any->_M_manager = __any->_M_manager;
	const_cast<any*>(__any)->_M_manager = nullptr;
	break;
      }
    }


    // Manage external contained object.
    template<typename _Tp>
      struct _Manager_external
      {
	static void
	_S_manage(_Op __which, const any* __anyp, _Arg* __arg);

	template<typename _Up>
	  static void
	  _S_create(_Storage& __storage, _Up&& __value)
	  {
	    __storage._M_ptr = new _Tp(std::forward<_Up>(__value));
	  }
	template<typename... _Args>
	  static void
	  _S_create(_Storage& __storage, _Args&&... __args)
	  {
	    __storage._M_ptr = new _Tp(std::forward<_Args>(__args)...);
	  }
	static _Tp*
	_S_access(const _Storage& __storage)
	{
	  // The contained object is in *__storage._M_ptr
	  return static_cast<_Tp*>(__storage._M_ptr);
	}
      };



	any impl
	/// Default constructor, creates an empty object.
    constexpr any() noexcept : _M_manager(nullptr) { }

    /// Copy constructor, copies the state of @p __other
    any(const any& __other)
    {
      if (!__other.has_value())
	_M_manager = nullptr;
      else
	{
	  _Arg __arg;
	  __arg._M_any = this;
	  __other._M_manager(_Op_clone, &__other, &__arg);
	}
    }

	/// Move constructor, transfer the state from @p __other
    any(any&& __other) noexcept
    {
      if (!__other.has_value())
	_M_manager = nullptr;
      else
	{
	  _Arg __arg;
	  __arg._M_any = this;
	  __other._M_manager(_Op_xfer, &__other, &__arg);
	}
    }

	/// Construct with a copy of @p __value as the contained object.
    template <typename _Tp, typename _VTp = _Decay_if_not_any<_Tp>,
	      typename _Mgr = _Manager<_VTp>,
	      typename = _Require<__not_<__is_in_place_type<_VTp>>,
				  is_copy_constructible<_VTp>>>
      any(_Tp&& __value)
      : _M_manager(&_Mgr::_S_manage)
      {
	_Mgr::_S_create(_M_storage, std::forward<_Tp>(__value));
      }

	  /// Destructor, calls @c reset()
    ~any() { reset(); }
	/// If not empty, destroy the contained object.
    void reset() noexcept
    {
      if (has_value())
      {
	_M_manager(_Op_destroy, this, nullptr);
	_M_manager = nullptr;
      }
    }

	
    // assignments
    /// Copy the state of another object.
    any&
    operator=(const any& __rhs) {
      *this = any(__rhs);
      return *this;
    }

	
    /// @brief Move assignment operator
    any& operator=(any&& __rhs) noexcept {
      if (!__rhs.has_value())
		reset();
      else if (this != &__rhs) {
		reset();
		_Arg __arg;
		__arg._M_any = this;
		__rhs._M_manager(_Op_xfer, &__rhs, &__arg);
	  }
      return *this;
    }

	/// Store a copy of @p __rhs as the contained object.
    template<typename _Tp>
      enable_if_t<is_copy_constructible<_Decay_if_not_any<_Tp>>::value, any&>
      operator=(_Tp&& __rhs)
      {
	*this = any(std::forward<_Tp>(__rhs));
	return *this;
      }

	/// Exchange state with another object.
    void swap(any& __rhs) noexcept
    {
      if (!has_value() && !__rhs.has_value())
	return;

      if (has_value() && __rhs.has_value())
	{
	  if (this == &__rhs)
	    return;

	  any __tmp;
	  _Arg __arg;
	  __arg._M_any = &__tmp;
	  __rhs._M_manager(_Op_xfer, &__rhs, &__arg);
	  __arg._M_any = &__rhs;
	  _M_manager(_Op_xfer, this, &__arg);
	  __arg._M_any = this;
	  __tmp._M_manager(_Op_xfer, &__tmp, &__arg);
	}
      else
	{
	  any* __empty = !has_value() ? this : &__rhs;
	  any* __full = !has_value() ? &__rhs : this;
	  _Arg __arg;
	  __arg._M_any = __empty;
	  __full->_M_manager(_Op_xfer, __full, &__arg);
	}
    }

	/// Reports whether there is a contained object or not.
    bool has_value() const noexcept { return _M_manager != nullptr; }


	#if __cpp_rtti
    /// The @c typeid of the contained object, or @c typeid(void) if empty.
    const type_info& type() const noexcept
    {
      if (!has_value())
	return typeid(void);
      _Arg __arg;
      _M_manager(_Op_get_type_info, this, &__arg);
      return *__arg._M_typeinfo;
    }
#endif

	// casting
	template<typename _Tp>
	friend void* __any_caster(const any* __any);


   **
   * @brief Access the contained object.
   *
   * @tparam  _ValueType  A const-reference or CopyConstructible type.
   * @param   __any       The object to access.
   * @return  The contained object.
   * @throw   bad_any_cast If <code>
   *          __any.type() != typeid(remove_reference_t<_ValueType>)
   *          </code>
   *
	template<typename _ValueType>
    inline _ValueType any_cast(const any& __any)
    {
      using _Up = __remove_cvref_t<_ValueType>;
      static_assert(any::__is_valid_cast<_ValueType>(),
	  "Template argument must be a reference or CopyConstructible type");
      static_assert(is_constructible_v<_ValueType, const _Up&>,
	  "Template argument must be constructible from a const value.");
      auto __p = any_cast<_Up>(&__any);
      if (__p)
	return static_cast<_ValueType>(*__p);
      __throw_bad_any_cast();
    }

	template<typename _ValueType>
    inline _ValueType any_cast(any& __any)
    {
      using _Up = __remove_cvref_t<_ValueType>;
      static_assert(any::__is_valid_cast<_ValueType>(),
	  "Template argument must be a reference or CopyConstructible type");
      static_assert(is_constructible_v<_ValueType, _Up&>,
	  "Template argument must be constructible from an lvalue.");
      auto __p = any_cast<_Up>(&__any);
      if (__p)
	return static_cast<_ValueType>(*__p);
      __throw_bad_any_cast();
    }

	 template<typename _ValueType>
    inline _ValueType any_cast(any&& __any)
    {
      using _Up = __remove_cvref_t<_ValueType>;
      static_assert(any::__is_valid_cast<_ValueType>(),
	  "Template argument must be a reference or CopyConstructible type");
      static_assert(is_constructible_v<_ValueType, _Up>,
	  "Template argument must be constructible from an rvalue.");
      auto __p = any_cast<_Up>(&__any);
      if (__p)
	return static_cast<_ValueType>(std::move(*__p));
      __throw_bad_any_cast();
    }

   **
   * @brief Access the contained object.
   *
   * @tparam  _ValueType  The type of the contained object.
   * @param   __any       A pointer to the object to access.
   * @return  The address of the contained object if <code>
   *          __any != nullptr && __any.type() == typeid(_ValueType)
   *          </code>, otherwise a null pointer.
   *
  template<typename _ValueType>
    inline const _ValueType* any_cast(const any* __any) noexcept
    {
      if constexpr (is_object_v<_ValueType>)
	if (__any)
	  return static_cast<_ValueType*>(__any_caster<_ValueType>(__any));
      return nullptr;
    }

  template<typename _ValueType>
    inline _ValueType* any_cast(any* __any) noexcept
    {
      if constexpr (is_object_v<_ValueType>)
	if (__any)
	  return static_cast<_ValueType*>(__any_caster<_ValueType>(__any));
      return nullptr;
    }


	template<typename _Tp>
    void* __any_caster(const any* __any)
    {
      // any_cast<T> returns non-null if __any->type() == typeid(T) and
      // typeid(T) ignores cv-qualifiers so remove them:
      using _Up = remove_cv_t<_Tp>;
      // The contained value has a decayed type, so if decay_t<U> is not U,
      // then it's not possible to have a contained value of type U:
      if constexpr (!is_same_v<decay_t<_Up>, _Up>)
	return nullptr;
      // Only copy constructible types can be used for contained values:
      else if constexpr (!is_copy_constructible_v<_Up>)
	return nullptr;
      // First try comparing function addresses, which works without RTTI
      else if (__any->_M_manager == &any::_Manager<_Up>::_S_manage
#if __cpp_rtti
	  || __any->type() == typeid(_Tp)
#endif
	  )
	{
	  return any::_Manager<_Up>::_S_access(__any->_M_storage);
	}
      return nullptr;
    }
*/

#include <typeinfo>
#include <type_traits>
#include <exception>

#include "has-rtti.h"

class any {
	// returns U if it's not `any` class 
	template<class T, class U = std::decay_t<T>>
	using decay_if_not_any_t = std::enable_if_t<!std::is_same_v<U, any>, U>;

public:
	friend void swap(any& a, any& b) {
		std::swap(a.m_data, b.m_data);
		std::swap(a.m_manager_func, b.m_manager_func);
	}

    any() = default;

    any(const any& other) {
		if (other.has_value()) {
			other.m_manager_func(CLONE, &other, this, nullptr);
		}
	}

    any(any&& other) {
		swap(*this, other);
	}

    any& operator=(any other) {
		swap(*this, other);
        return *this;
    }

	template<class T, class V = decay_if_not_any_t<T>>
	any& operator=(T&& value) {
		if (has_value()) {
			reset();
		}

		Manager<V>::create(this, std::forward<T>(value));
		m_manager_func = &Manager<V>::manage;
		return *this;
	}

	template<class T, class V = decay_if_not_any_t<T>>
    any(T&& value) {
		*this = value;
	}

	~any() {
		if (has_value()) {
			reset();
		}
	}

    bool empty() const {
		return !has_value();
    }

#ifdef RTTI_ENABLED
	const std::type_info& get_type_info() const {		
		const std::type_info* info = &typeid(void);

		if (has_value()) {
			m_manager_func(GET_TYPE_INFO, this, nullptr, info);
		}

		return *info;
	};
#endif

	template<class T>
	static void* cast(const any* obj) {
		if (obj->m_manager_func == &any::Manager<T>::manage) {
			return any::Manager<T>::access(obj);
		}
		return nullptr;
	}

private:
	enum Operation {
		CLONE, DESTROY, GET_TYPE_INFO
	};

	void *m_data = nullptr;
	void (*m_manager_func) (Operation, const any*, any*, const std::type_info*) = nullptr;


	// the trick of utilizing templated helper object is used in `std::any`, it allows to compile solution with `-no-rtti` flag (you can try doing that in CMakeLists.txt)
	template<class T>
	struct Manager {
		static void manage(Operation op, const any* self, [[ maybe_unused ]] any* other, [[ maybe_unused ]] const std::type_info* type_data) {
			auto data = static_cast<const T*> (self->m_data);

			switch (op) {
				case GET_TYPE_INFO: {
#ifdef RTTI_ENABLED
					type_data = &typeid(T);
#endif
					break;
				}
				case CLONE: {
					other->m_data = new T(*data);
					other->m_manager_func = self->m_manager_func;
					break;
				}
				case DESTROY: {
					delete data;
					break;
				}
			}
		}

		template<class V>
		static void create(any* obj, V&& value) {
			obj->m_data = new T(std::forward<V>(value));
		}

		static T* access(const any* obj) {
			return static_cast<T*>(obj->m_data);
		}
	};

	bool has_value() const {
		return m_manager_func != nullptr;
	}

	void reset() {
		m_manager_func(DESTROY, this, nullptr, nullptr);
	}
};

struct bad_any_cast : std::runtime_error {
	bad_any_cast(std::string msg): std::runtime_error(std::move(msg)) {}
};

template<class T>
T any_cast(const any& obj) {
	auto ptr = any_cast<std::decay_t<T>>(&obj);
	if (ptr) {
		return static_cast<T>(*ptr);
	}

#ifdef RTTI_ENABLED
	auto expected_type = obj.get_type_info().name();
	auto got_type = typeid(T).name();
	throw bad_any_cast("bad_any_cast: expected type: '" + std::string(expected_type) + "'" + ", but got: '" + std::string(got_type) + "'");
#else
	throw bad_any_cast("bad_any_cast");
#endif
}

template<class T>
T any_cast(any& obj) {
	auto ptr = any_cast<std::decay_t<T>>(&obj);
	if (ptr) {
		return static_cast<T>(*ptr);
	}

#ifdef RTTI_ENABLED
	auto expected_type = obj.get_type_info().name();
	auto got_type = typeid(T).name();
	throw bad_any_cast("bad_any_cast: expected type: '" + std::string(expected_type) + "'" + ", but got: '" + std::string(got_type) + "'");
#else
	throw bad_any_cast("bad_any_cast");
#endif
}

template<class T>
const T* any_cast(const any* obj) {
	return static_cast<T*>(any::cast<T>(obj));
};

template<class T>
T* any_cast(any* obj) {
	return const_cast<T*>(any_cast<T>(const_cast<const any*>(obj)));
}

}