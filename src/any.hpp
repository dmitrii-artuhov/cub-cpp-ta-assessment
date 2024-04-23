#pragma once

namespace utils {
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