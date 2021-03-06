/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/RTTI/RTTI.h>

// For attributes
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/typetraits/decay.h>
#include <AzCore/std/typetraits/function_traits.h>
#include <AzCore/std/typetraits/is_function.h>
#include <AzCore/std/typetraits/is_member_function_pointer.h>
#include <AzCore/std/typetraits/is_same.h>

namespace AZ
{
    class ReflectContext;

    /// Function type to called for on demand reflection within methods, properties, etc.
    using OnDemandReflectionFunctionPtr = void(*)(ReflectContext* context);
    // #MSVC2013 does not allow using of a function signature.
    typedef void OnDemandReflectionFunctionRef(ReflectContext* context);

    /**
     * Base classes for structures that store references to OnDemandReflection instantiations.
     * ReflectContext will own weak pointers to the function, so that we may look up already registered types.
     */
    class OnDemandReflectionOwner
    {
    public:
        virtual ~OnDemandReflectionOwner() = default;

        // Disallow all copy-move operations, as well as default construct
        OnDemandReflectionOwner() = delete;
        OnDemandReflectionOwner(const OnDemandReflectionOwner&) = delete;
        OnDemandReflectionOwner& operator=(const OnDemandReflectionOwner&) = delete;
        OnDemandReflectionOwner(OnDemandReflectionOwner&&) = delete;
        OnDemandReflectionOwner& operator=(OnDemandReflectionOwner&&) = delete;

        /// Register an OnDemandReflection function
        void AddReflectFunction(AZ::Uuid typeId, OnDemandReflectionFunctionPtr reflectFunction);

    protected:
        /// Constructor to be called by child class
        explicit OnDemandReflectionOwner(ReflectContext& context);

    private:
        /// Reflection functions this instance owns references to
        AZStd::vector<AZStd::shared_ptr<OnDemandReflectionFunctionRef>> m_reflectFunctions;
        ReflectContext& m_reflectContext;
    };

    /**
     *  This is the default implementation for OnDemandReflection (no reflection). You can specialize this template class
     * for the type you want to reflect on demand.
     * \note Currently only \ref BehaviorContext support on demand reflection.
     */
    template<class T>
    struct OnDemandReflection
    {
        void NoSpecializationFunction();
    };

    AZ_HAS_MEMBER(NoOnDemandReflection, NoSpecializationFunction, void, ())

    /**
     * Base class for all reflection contexts.
     * Currently we recommend to follow the following declarative format for all context.
     * (Keep in mind this if only for direct C++ interface, code generators will generate code
     * on the top of this interface)
     * context->Class<TYPE>("Other descriptive parameters")
     *     ->Field/Propety("field name",&TYPE::m_field)
     *     ->Method("method name),&TYPE::Method)
     *     ...
     * We recommend that all ReflectContext implement a path to remove reflection when a reflected class/descriptor is deleted.
     * To do so make sure that when \ref ReflectContext::m_isRemoveReflection is set to true any calls to Class<...>() actualy remove
     * reflection. We recommend this approach so the user can write only one reflection function (not Reflect and "Unreflect")
     */
    class ReflectContext
    {
    public:
        AZ_RTTI(ReflectContext, "{B18D903B-7FAD-4A53-918A-3967B3198224}")

        ReflectContext();
        virtual ~ReflectContext() = default;

        void EnableRemoveReflection();
        void DisableRemoveReflection();
        bool IsRemovingReflection() const;

        /// Check if an OnDemandReflection type's typeid is already reflected
        bool IsOnDemandTypeReflected(AZ::Uuid typeId);

        /// Execute all queued OnDemandReflection calls
        void ExecuteQueuedOnDemandReflections();

    protected:
        /// True if all calls in the context should be considered to remove not to add reflection.
        bool m_isRemoveReflection;

        /// Store the on demand reflect functions so we can avoid double-reflecting something
        AZStd::unordered_map<AZ::Uuid, AZStd::weak_ptr<OnDemandReflectionFunctionRef>> m_onDemandReflection;
        /// OnDemandReflection functions that need to be called
        AZStd::vector<AZStd::pair<AZ::Uuid, OnDemandReflectionFunctionPtr>> m_toProcessOnDemandReflection;
        /// The type ids of the currently reflecting type. Used to prevent circular references. Is a set to prevent recursive circular references.
        AZStd::deque<AZ::Uuid> m_currentlyProcessingTypeIds;

        friend OnDemandReflectionOwner;
    };

    // Attributes to be used by reflection contexts

    /**
    * Base abstract class for all attributes. Use azrtti to get the
    * appropriate version. Of course if NULL there is a data mismatch of attributes.
    */
    class Attribute
    {
    public:
        using ContextDeleter = void(*)(void* contextData);

        AZ_RTTI(AZ::Attribute, "{2C656E00-12B0-476E-9225-5835B92209CC}");
        Attribute()
            : m_contextData(nullptr, &DefaultDelete)
        { }
        virtual ~Attribute() = default;

        void SetContextData(void* contextData, ContextDeleter destroyer)
        {
            m_contextData = AZStd::unique_ptr<void, ContextDeleter>(contextData, destroyer);
        }

        void* GetContextData() const
        {
            return m_contextData.get();
        }

        bool m_describesChildren = false;

    private:
        AZStd::unique_ptr<void, ContextDeleter> m_contextData; ///< a generic value you can use to store extra data associated with the attribute

        static void DefaultDelete(void*) { }
    };

    typedef AZ::u32 AttributeId;
    typedef AZStd::pair<AttributeId, Attribute*> AttributePair;
    typedef AZStd::vector<AttributePair> AttributeArray;

    inline Attribute* FindAttribute(AttributeId id, const AttributeArray& attrArray)
    {
        for (const AttributePair& attrPair : attrArray)
        {
            if (attrPair.first == id)
            {
                return attrPair.second;
            }
        }
        return nullptr;
    }


    /**
    * Generic attribute by value data container. This is the most common attribute.
    */
    template<class T>
    class AttributeData
        : public Attribute
    {
    public:
        AZ_RTTI((AttributeData<T>, "{24248937-86FB-406C-8DD5-023B10BD0B60}", T), Attribute);
        AZ_CLASS_ALLOCATOR(AttributeData<T>, SystemAllocator, 0);
        template<class U>
        explicit AttributeData(U&& data)
            : m_data(AZStd::forward<U>(data)) {}
        virtual const T& Get(void* instance) const { (void)instance; return m_data; }
        T& operator = (T& data) { m_data = data; return m_data; }
        T& operator = (const T& data) { m_data = data; return m_data; }
    private:
        T   m_data;
    };

    /**
    * Generic attribute for class member data, we use the object instance to access member data.
    * In general since the AttributeData::Get function already takes the instance there is no reason to cast (azrtti)
    * to this class (unless you really want to check/know).
    */
    template<class T>
    class AttributeMemberData;

    template<class T, class C>
    class AttributeMemberData<T C::*>
        : public AttributeData<T>
    {
    public:
        AZ_RTTI((AZ::AttributeMemberData<T C::*>, "{00E5F991-6B96-43CC-9869-F371548581D9}", T, C), AttributeData<T>);
        AZ_CLASS_ALLOCATOR(AttributeMemberData<T C::*>, SystemAllocator, 0);
        typedef T C::* DataPtr;
        explicit AttributeMemberData(DataPtr p)
            : AttributeData<T>(T())
            , m_dataPtr(p) {}
        const T& Get(void* instance) const override { return (reinterpret_cast<C*>(instance)->*m_dataPtr); }
    private:
        DataPtr m_dataPtr;
    };

    /**
    * Generic attribute global function pointer container. All function must return non void result (we can implement this)
    * as this is attribute and assumed to return something.
    */
    template<class F>
    class AttributeFunction;

    template<class R, class... Args>
    class AttributeFunction<R(Args...)> : public Attribute
    {
    public:
        AZ_RTTI((AZ::AttributeFunction<R(Args...)>, "{EE535A42-940C-42DE-848D-9C6CE57D8A62}", R, Args...), Attribute);
        AZ_CLASS_ALLOCATOR(AttributeFunction<R(Args...)>, SystemAllocator, 0);
        typedef R(*FunctionPtr)(Args...);
        explicit AttributeFunction(FunctionPtr f)
                : m_function(f)
        {}

        virtual R Invoke(void* instance, Args&&... args)
        {
            (void)instance;
            return m_function(AZStd::forward<Args>(args) ...);
        }

        virtual AZ::Uuid GetInstanceType() const
        {
            return AZ::Uuid::CreateNull();
        }

        FunctionPtr m_function;
    };

    /**
    * Generic attribute member function pointer container. All function must return non void result (we can implement this)
    * as this is attribute and assumed to return something. The instance to the class will provided vie the invoke function.
    * In general (unless you care) you should use this class vie the AttributeFunction class.
    */
    template<class T>
    class AttributeMemberFunction;

    template<class R, class C, class... Args>
    class AttributeMemberFunction<R(C::*)(Args...)>
        : public AttributeFunction<R(Args...)>
    {
    public:
        AZ_RTTI((AZ::AttributeMemberFunction<R(C::*)(Args...)>, "{F41F655D-87F7-4A87-9412-9AF4B528B142}", R, C, Args...), AttributeFunction<R(Args...)>);
        AZ_CLASS_ALLOCATOR(AttributeMemberFunction<R(C::*)(Args...)>, SystemAllocator, 0);
        typedef R(C::* FunctionPtr)(Args...);

        explicit AttributeMemberFunction(FunctionPtr f)
                : AttributeFunction<R(Args...)>(nullptr)
                , m_memFunction(f)
        {}

        R Invoke(void* instance, Args&&... args) override
        {
            return (reinterpret_cast<C*>(instance)->*m_memFunction)(AZStd::forward<Args>(args) ...);
        }

        AZ::Uuid GetInstanceType() const override
        {
            return AzTypeInfo<C>::Uuid();
        }

    private:
        FunctionPtr m_memFunction;
    };

    // const versions
    template<class R, class C, class... Args>
    class AttributeMemberFunction<R(C::*)(Args...) const>
        : public AttributeFunction<R(Args...)>
    {
    public:
        AZ_RTTI((AZ::AttributeMemberFunction<R(C::*)(Args...) const>, "{4E21155A-0FB0-4F11-999A-B946B5954A0A}", R, C, Args...), AttributeFunction<R(Args...)>);
        AZ_CLASS_ALLOCATOR(AttributeMemberFunction<R(C::*)(Args...) const>, SystemAllocator, 0);
        typedef R(C::* FunctionPtr)(Args...) const;

        explicit AttributeMemberFunction(FunctionPtr f)
            : AttributeFunction<R(Args...)>(nullptr)
            , m_memFunction(f)
        {}

        R Invoke(void* instance, Args&&... args) override
        {
            return (reinterpret_cast<const C*>(instance)->*m_memFunction)(AZStd::forward<Args>(args) ...);
        }

        AZ::Uuid GetInstanceType() const override
        {
            return AzTypeInfo<C>::Uuid();
        }

    private:
        FunctionPtr m_memFunction;
    };

    template <typename T>
    using AttributeContainerType = typename AZStd::Utils::if_c<AZStd::is_member_pointer<T>::value,
        typename AZStd::Utils::if_c<AZStd::is_member_function_pointer<T>::value, AttributeMemberFunction<T>, AttributeMemberData<T> >::type,
        typename AZStd::Utils::if_c<AZStd::is_function<typename AZStd::remove_pointer<T>::type>::value, AttributeFunction<typename AZStd::remove_pointer<T>::type>, AttributeData<T> >::type
    >::type;

    namespace Internal
    {
        template<class T>
        struct AttributeValueTypeClassChecker
        {
            // By default any value is OK
            static bool Check(const Uuid&, IRttiHelper*)      
            { 
                return true; 
            }  
        };

        template<class T, class C>
        struct AttributeValueTypeClassChecker<T C::*>
        {
            static bool Check(const Uuid& classId, IRttiHelper* rtti)
            { 
                if (classId == AzTypeInfo<C>::Uuid())
                {
                    return true;
                }
                return rtti ? rtti->IsTypeOf(AzTypeInfo<C>::Uuid()) : false;
            }
        };

        template<class T, class C>
        struct AttributeValueTypeClassChecker<T(C::*)()>
        {
            static bool Check(const Uuid& classId, IRttiHelper* rtti) 
            { 
                if (classId == AzTypeInfo<C>::Uuid())
                {
                    return true;
                }
                return rtti ? rtti->IsTypeOf(AzTypeInfo<C>::Uuid()) : false;
            }
        };

        template<class T, class C>
        struct AttributeValueTypeClassChecker<T(C::*)() const>
        {
            static bool Check(const Uuid& classId, IRttiHelper* rtti) 
            { 
                if (classId == AzTypeInfo<C>::Uuid())
                {
                    return true;
                }
                return rtti ? rtti->IsTypeOf(AzTypeInfo<C>::Uuid()) : false;
            }
        };

        template<class T, bool IsNoSpecialization = HasNoOnDemandReflection<OnDemandReflection<T>>::value>
        struct OnDemandReflectHook
        {
            static OnDemandReflectionFunctionPtr Get()
            {
                return nullptr;
            }
        };

        template<class T>
        struct OnDemandReflectHook<T, false>
        {
            static OnDemandReflectionFunctionPtr Get()
            {
                return &OnDemandReflection<T>::Reflect;
            }
        };
    }
}
