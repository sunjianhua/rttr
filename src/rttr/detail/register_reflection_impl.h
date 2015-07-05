/************************************************************************************
*                                                                                   *
*   Copyright (c) 2014, 2015 Axel Menzel <info@axelmenzel.de>                       *
*                                                                                   *
*   This file is part of RTTR (Run Time Type Reflection)                            *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining           *
*   a copy of this software and associated documentation files (the "Software"),    *
*   to deal in the Software without restriction, including without limitation       *
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,        *
*   and/or sell copies of the Software, and to permit persons to whom the           *
*   Software is furnished to do so, subject to the following conditions:            *
*                                                                                   *
*   The above copyright notice and this permission notice shall be included in      *
*   all copies or substantial portions of the Software.                             *
*                                                                                   *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*   SOFTWARE.                                                                       *
*                                                                                   *
*************************************************************************************/

#ifndef RTTR_REGISTER_REFLECTION_IMPL_H_
#define RTTR_REGISTER_REFLECTION_IMPL_H_

#include "rttr/detail/constructor/constructor_container.h"
#include "rttr/detail/destructor/destructor_container.h"
#include "rttr/detail/enumeration/enumeration_container.h"
#include "rttr/detail/method/method_container.h"
#include "rttr/detail/property/property_container.h"
#include "rttr/detail/type/accessor_type.h"
#include "rttr/detail/misc/misc_type_traits.h"
#include "rttr/detail/misc/utility.h"
#include "rttr/detail/type/type_register.h"

#include <type_traits>
#include <memory>

namespace rttr
{
namespace detail
{

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

/*!
 * These helper functions are the only one who instantiate the actual container classes.
 * This has to be done here, otherwise a lot of class templates would be instantiated.
 */
class register_helper
{

public:
    template<typename T>
    static void store_metadata(T& obj, std::vector<rttr::metadata> data)
    {
        for (auto& item : data)
        {
            auto key    = item.get_key();
            auto value  = item.get_value();
            if (key.is_type<int>())
                obj->set_metadata(key.get_value<int>(), std::move(value));
            else if (key.is_type<std::string>())
                obj->set_metadata(std::move(key.get_value<std::string>()), std::move(value));
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////

    template<typename ClassType, typename... Args>
    static void constructor(std::vector< rttr::metadata > data)
    {
        auto ctor = detail::make_unique<constructor_container<ClassType, class_ctor, default_invoke, Args...>>();
        // register the type with the following call:
        ctor->get_instanciated_type();
        ctor->get_parameter_types();

        store_metadata(ctor, std::move(data));
        const type t = type::get<ClassType>();
        type_register::constructor(t, std::move(ctor));
        type_register::destructor(t, detail::make_unique<destructor_container<ClassType>>());
    }

    /////////////////////////////////////////////////////////////////////////////////////////

    template<typename ClassType, typename F>
    static void constructor(F function, std::vector< rttr::metadata > data)
    {
        static_assert(std::is_same<return_func, typename method_type<F>::type>::value, "For creating this 'class type', please provide a function pointer or std::function with a return value.");
        auto ctor = detail::make_unique<constructor_container<ClassType, return_func, default_invoke, F>>(function);
        ctor->get_instanciated_type();
        ctor->get_parameter_types();

        store_metadata(ctor, std::move(data));
        const type t = type::get<ClassType>();
        type_register::constructor(t, std::move(ctor));
    }

    /////////////////////////////////////////////////////////////////////////////////////////

    template<typename ClassType, typename A, typename Policy>
    static void property(const std::string& name, A accessor, std::vector<rttr::metadata> data, const Policy&)
    {
        using namespace std;
        using getter_policy = typename get_getter_policy<Policy>::type;
        using setter_policy = typename get_setter_policy<Policy>::type;
        using acc_type      = typename property_type<A>::type;

        auto prop = detail::make_unique<property_container<acc_type, A, void, getter_policy, setter_policy>>(name, type::get<ClassType>(), accessor);
        // register the type with the following call:
        prop->get_type();
        store_metadata(prop, std::move(data));
        type_register::property(type::get<ClassType>(), move(prop));
    }

    /////////////////////////////////////////////////////////////////////////////////////////

    template<typename ClassType, typename A1, typename A2, typename Policy>
    static void property(const std::string& name, A1 getter, A2 setter, std::vector<rttr::metadata> data, const Policy&)
    {
        using namespace std;
        using getter_policy = typename get_getter_policy<Policy>::type;
        using setter_policy = typename get_setter_policy<Policy>::type;
        using acc_type      = typename property_type<A1>::type;

        auto prop = detail::make_unique<property_container<acc_type, A1, A2, getter_policy, setter_policy>>(name, type::get<ClassType>(), getter, setter);
        // register the type with the following call:
        prop->get_type();
        store_metadata(prop, std::move(data));
        type_register::property(type::get<ClassType>(), move(prop));
    }

    /////////////////////////////////////////////////////////////////////////////////////////

    template<typename ClassType, typename A, typename Policy>
    static void property_readonly(const std::string& name, A accessor, std::vector<rttr::metadata> data, const Policy&)
    {
        using namespace std;
        using getter_policy = typename get_getter_policy<Policy>::type;
        using setter_policy = read_only;
        using acc_type      = typename property_type<A>::type;

        auto prop = detail::make_unique<property_container<acc_type, A, void, getter_policy, setter_policy>>(name, type::get<ClassType>(), accessor);
        // register the type with the following call:
        prop->get_type();
        store_metadata(prop, std::move(data));
        type_register::property(type::get<ClassType>(), move(prop));
    }

    /////////////////////////////////////////////////////////////////////////////////////////

    template<typename ClassType, typename F, typename Policy>
    static void method(const std::string& name, F function, std::vector< rttr::metadata > data, const Policy&)
    {
        using namespace std;
        using method_policy = typename get_method_policy<Policy>::type;

        auto meth = detail::make_unique<method_container<F, method_policy>>(name, type::get<ClassType>(), function);
        // register the underlying type with the following call:
        meth->get_return_type();
        meth->get_parameter_types();
        store_metadata(meth, std::move(data));
        type_register::method(type::get<ClassType>(), move(meth));
    }

    /////////////////////////////////////////////////////////////////////////////////////////

    template<typename ClassType, typename EnumType>
    static void enumeration(std::string name, std::vector< std::pair< std::string, EnumType> > enum_data, std::vector<rttr::metadata> data)
    {
        using namespace std;
        static_assert(is_enum<EnumType>::value, "No enum type provided, please call this method with an enum type!");

        type enum_type = type::get<EnumType>();
        if (!name.empty())
            type_register::custom_name(enum_type, std::move(name));

        type declar_type = get_invalid_type();
        if (!std::is_same<ClassType, void>::value)
            declar_type = type::get<ClassType>();

        auto enum_item = detail::make_unique<enumeration_container<EnumType>>(declar_type, move(enum_data));
        // register the underlying type with the following call:
        enum_item->get_type();

        store_metadata(enum_item, std::move(data));
        type_register::enumeration(enum_type, move(enum_item));
    }

};
} //  end namespace detail


/////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
void constructor_(std::vector< rttr::metadata > data)
{
    detail::register_helper::constructor<T>(std::move(data));
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A>
void property_(const std::string& name, A acc)
{
    using namespace std;
    static_assert(is_pointer<A>::value, "No valid property accessor provided!");
    detail::register_helper::property<void, A>(name, acc, std::vector< rttr::metadata >(), detail::default_property_policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A>
void property_(const std::string& name, A acc, std::vector< rttr::metadata > data)
{
    using namespace std;
    static_assert(is_pointer<A>::value, "No valid property accessor provided!");
    detail::register_helper::property<void, A>(name, acc, std::move(data), detail::default_property_policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A, typename Policy>
void property_(const std::string& name, A acc, 
               const Policy& policy, typename std::enable_if<detail::contains<Policy, detail::policy_list>::value>::type*)
{
    using namespace std;
    static_assert(is_pointer<A>::value, "No valid property accessor provided!");
    detail::register_helper::property<void, A>(name, acc, std::vector< rttr::metadata >(), policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A, typename Policy>
void property_(const std::string& name, A acc, std::vector< rttr::metadata > data,
               const Policy& policy)
{
    using namespace std;
    static_assert(is_pointer<A>::value, "No valid property accessor provided!");
    detail::register_helper::property<void, A>(name, acc, std::move(data), policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A1, typename A2>
void property_(const std::string& name, A1 getter, A2 setter, 
               typename std::enable_if<!detail::contains<A2, detail::policy_list>::value>::type*)
{
    using namespace std;
    static_assert(is_pointer<A1>::value || is_pointer<A2>::value ||
                  detail::is_function_ptr<A1>::value || detail::is_function_ptr<A2>::value ||
                  detail::is_std_function<A1>::value || detail::is_std_function<A2>::value, 
                  "No valid property accessor provided!");
    detail::register_helper::property<void, A1, A2>(name, getter, setter, std::vector< rttr::metadata >(), detail::default_property_policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A1, typename A2>
void property_(const std::string& name, A1 getter, A2 setter, std::vector< rttr::metadata > data)
{
    using namespace std;
    static_assert(is_pointer<A1>::value || is_pointer<A2>::value ||
                  detail::is_function_ptr<A1>::value || detail::is_function_ptr<A2>::value ||
                  detail::is_std_function<A1>::value || detail::is_std_function<A2>::value, 
                  "No valid property accessor provided!");
    detail::register_helper::property<void, A1, A2>(name, getter, setter, move(data), detail::default_property_policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A1, typename A2, typename Policy>
void property_(const std::string& name, A1 getter, A2 setter,
               const Policy& policy)
{
    using namespace std;
    static_assert(is_pointer<A1>::value || is_pointer<A2>::value ||
                  detail::is_function_ptr<A1>::value || detail::is_function_ptr<A2>::value ||
                  detail::is_std_function<A1>::value || detail::is_std_function<A2>::value, 
                  "No valid property accessor provided!");
    detail::register_helper::property<void, A1, A2>(name, getter, setter, std::vector< rttr::metadata >(), policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A1, typename A2, typename Policy>
void property_(const std::string& name, A1 getter, A2 setter, std::vector< rttr::metadata > data,
               const Policy& policy)
{
    using namespace std;
    static_assert(is_pointer<A1>::value || is_pointer<A2>::value ||
                  detail::is_function_ptr<A1>::value || detail::is_function_ptr<A2>::value ||
                  detail::is_std_function<A1>::value || detail::is_std_function<A2>::value, 
                  "No valid property accessor provided!");
    detail::register_helper::property<void, A1, A2>(name, getter, setter, std::move(data), policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A>
void property_readonly_(const std::string& name, A acc)
{
    using namespace std;
    static_assert(is_pointer<A>::value || detail::is_function_ptr<A>::value || detail::is_std_function<A>::value,
                  "No valid property accessor provided!");
    detail::register_helper::property_readonly<void, A>(name, acc, std::vector< rttr::metadata >(), detail::default_property_policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A>
void property_readonly_(const std::string& name, A acc, std::vector< rttr::metadata > data)
{
    using namespace std;
    static_assert(is_pointer<A>::value || detail::is_function_ptr<A>::value || detail::is_std_function<A>::value,
                  "No valid property accessor provided!");
    detail::register_helper::property_readonly<void, A>(name, acc, std::move(data), detail::default_property_policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A, typename Policy>
void property_readonly_(const std::string& name, A acc, const Policy& policy)
{
    using namespace std;
    static_assert(is_pointer<A>::value || detail::is_function_ptr<A>::value || detail::is_std_function<A>::value,
                  "No valid property accessor provided!");
    detail::register_helper::property_readonly<void, A>(name, acc, std::vector< rttr::metadata >(), policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A, typename Policy>
void property_readonly_(const std::string& name, A acc, std::vector< rttr::metadata > data, const Policy& policy)
{
    using namespace std;
    static_assert(is_pointer<A>::value || detail::is_function_ptr<A>::value || detail::is_std_function<A>::value,
                  "No valid property accessor provided!");
    detail::register_helper::property_readonly<void, A>(name, acc, std::move(data), policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename F>
void method_(const std::string& name, F function)
{
    detail::register_helper::method<void>(name, function, std::vector< rttr::metadata >(), detail::default_invoke());
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename F>
void method_(const std::string& name, F function, std::vector< rttr::metadata > data)
{
    detail::register_helper::method<void>(name, function, std::move(data), detail::default_invoke());
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename F, typename Policy>
void method_(const std::string& name, F function, const Policy& policy)
{
    detail::register_helper::method<void>(name, function, std::vector< rttr::metadata >(), policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename F, typename Policy>
void method_(const std::string& name, F function, std::vector< rttr::metadata > data, const Policy& policy)
{
    detail::register_helper::method<void>(name, function, std::move(data), policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename EnumType>
void enumeration_(std::string name, std::vector< std::pair< std::string, EnumType> > enum_data, std::vector<rttr::metadata> data)
{
    detail::register_helper::enumeration<void, EnumType>(std::move(name), std::move(enum_data), std::move(data));
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
class_<ClassType>::class_(std::string name, std::vector< rttr::metadata > data)
{
    auto t = type::get<ClassType>();

    if (!name.empty())
        detail::type_register::custom_name(t, std::move(name));

    detail::type_register::metadata(t, std::move(data));

    static_assert(std::is_class<ClassType>::value, "Reflected type is not a class or struct.");
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
class_<ClassType>::~class_()
{
    // make sure that all base classes are registered
    detail::base_classes<ClassType>::get_types();
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename... Args>
class_<ClassType>& class_<ClassType>::constructor(std::vector<rttr::metadata> data)
{
    detail::register_helper::constructor<ClassType, Args...>(std::move(data));
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename F>
class_<ClassType>& class_<ClassType>::constructor(F function, std::vector< rttr::metadata > data)
{
    detail::register_helper::constructor<ClassType, F>(function, std::move(data));
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A acc)
{
    static_assert(std::is_member_object_pointer<A>::value ||
                  std::is_pointer<A>::value,
                  "No valid property accessor provided!");

    detail::register_helper::property<ClassType, A>(name, acc, std::vector<rttr::metadata>(),
                                                  detail::default_property_policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A acc, std::vector<rttr::metadata> data)
{
    static_assert(std::is_member_object_pointer<A>::value ||
                  std::is_pointer<A>::value,
                  "No valid property accessor provided!");
    detail::register_helper::property<ClassType, A>(name, acc, std::move(data), 
                                                  detail::default_property_policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A, typename Policy>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A acc, const Policy& policy, 
                                               typename std::enable_if<detail::contains<Policy, detail::policy_list>::value>::type*)
{
    static_assert(std::is_member_object_pointer<A>::value ||
                  std::is_pointer<A>::value,
                  "No valid property accessor provided!");
    detail::register_helper::property<ClassType, A>(name, acc, std::vector<rttr::metadata>(), 
                                                  policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A, typename Policy>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A acc, std::vector<rttr::metadata> data,
                                               const Policy& policy)
{
    static_assert(std::is_member_object_pointer<A>::value ||
                  std::is_pointer<A>::value,
                  "No valid property accessor provided!");
    detail::register_helper::property<ClassType, A>(name, acc, std::move(data),
                                                  policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A1, typename A2>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A1 getter, A2 setter,
                                               typename std::enable_if<!detail::contains<A2, detail::policy_list>::value>::type*)
{
    detail::register_helper::property<ClassType, A1, A2>(name, getter, setter, std::vector<rttr::metadata>(),
                                                       detail::default_property_policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A1, typename A2>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A1 getter, A2 setter, std::vector<rttr::metadata> data)
{
    detail::register_helper::property<ClassType, A1, A2>(name, getter, setter, std::move(data),
                                                       detail::default_property_policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A1, typename A2, typename Policy>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A1 getter, A2 setter, const Policy& policy)
{
    detail::register_helper::property<ClassType, A1, A2>(name, getter, setter, std::vector<rttr::metadata>(), 
                                                       policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A1, typename A2, typename Policy>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A1 getter, A2 setter, 
                                               std::vector<rttr::metadata> data, const Policy& policy)
{
    detail::register_helper::property<ClassType, A1, A2>(name, getter, setter, std::move(data),
                                                       policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A>
class_<ClassType>& class_<ClassType>::property_readonly(const std::string& name, A acc)
{
    detail::register_helper::property_readonly<ClassType, A>(name, acc, std::vector<rttr::metadata>(),
                                                           detail::default_property_policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A>
class_<ClassType>& class_<ClassType>::property_readonly(const std::string& name, A acc, std::vector<rttr::metadata> data)
{
    detail::register_helper::property_readonly<ClassType, A>(name, acc, std::move(data),
                                                           detail::default_property_policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A, typename Policy>
class_<ClassType>& class_<ClassType>::property_readonly(const std::string& name, A acc, const Policy& policy)
{
    detail::register_helper::property_readonly<ClassType, A>(name, acc, std::vector<rttr::metadata>(),
                                                           policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A, typename Policy>
class_<ClassType>& class_<ClassType>::property_readonly(const std::string& name, A acc, std::vector<rttr::metadata> data,
                                                        const Policy& policy)
{
    detail::register_helper::property_readonly<ClassType, A>(name, acc, std::move(data),
                                                           policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename F>
class_<ClassType>& class_<ClassType>::method(const std::string& name, F function)
{
    detail::register_helper::method<ClassType>(name, function, std::vector< rttr::metadata >(), detail::default_invoke());
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename F>
class_<ClassType>& class_<ClassType>::method(const std::string& name, F function, std::vector< rttr::metadata > data)
{
    detail::register_helper::method<ClassType>(name, function, std::move(data), detail::default_invoke());
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename F, typename Policy>
class_<ClassType>& class_<ClassType>::method(const std::string& name, F function, const Policy& policy)
{
    detail::register_helper::method<ClassType>(name, function, std::vector< rttr::metadata >(), policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename F, typename Policy>
class_<ClassType>& class_<ClassType>::method(const std::string& name, F function, std::vector< rttr::metadata > data, const Policy& policy)
{
    detail::register_helper::method<ClassType>(name, function, std::move(data), policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename EnumType>
class_<ClassType>& class_<ClassType>::enumeration(std::string name, std::vector< std::pair< std::string, EnumType> > enum_data, std::vector<rttr::metadata> data)
{
    detail::register_helper::enumeration<ClassType, EnumType>(std::move(name), std::move(enum_data), std::move(data));
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// register_global

/////////////////////////////////////////////////////////////////////////////////////////


} // end namespace rttr


#define RTTR_REGISTER                                                   \
static void rttr__auto_register_reflection_function__();                \
namespace                                                               \
{                                                                       \
    struct rttr__auto__register__                                       \
    {                                                                   \
        rttr__auto__register__()                                        \
        {                                                               \
            rttr__auto_register_reflection_function__();                \
        }                                                               \
    };                                                                  \
}                                                                       \
static const rttr__auto__register__ RTTR_CAT(auto_register__,__LINE__); \
static void rttr__auto_register_reflection_function__()

#endif // RTTR_REGISTER_REFLECTION_IMPL_H_