/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_SCOPED_POINTER_HPP_INCLUDED
#define DISTRHO_SCOPED_POINTER_HPP_INCLUDED

#include "../DistrhoUtils.hpp"

#include <algorithm>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// The following code was based from juce-core ScopedPointer class
// Copyright (C) 2013 Raw Material Software Ltd.

//==============================================================================
/**
    This class holds a pointer which is automatically deleted when this object goes
    out of scope.

    Once a pointer has been passed to a ScopedPointer, it will make sure that the pointer
    gets deleted when the ScopedPointer is deleted. Using the ScopedPointer on the stack or
    as member variables is a good way to use RAII to avoid accidentally leaking dynamically
    created objects.

    A ScopedPointer can be used in pretty much the same way that you'd use a normal pointer
    to an object. If you use the assignment operator to assign a different object to a
    ScopedPointer, the old one will be automatically deleted.

    A const ScopedPointer is guaranteed not to lose ownership of its object or change the
    object to which it points during its lifetime. This means that making a copy of a const
    ScopedPointer is impossible, as that would involve the new copy taking ownership from the
    old one.

    If you need to get a pointer out of a ScopedPointer without it being deleted, you
    can use the release() method.

    Something to note is the main difference between this class and the std::auto_ptr class,
    which is that ScopedPointer provides a cast-to-object operator, wheras std::auto_ptr
    requires that you always call get() to retrieve the pointer. The advantages of providing
    the cast is that you don't need to call get(), so can use the ScopedPointer in pretty much
    exactly the same way as a raw pointer. The disadvantage is that the compiler is free to
    use the cast in unexpected and sometimes dangerous ways - in particular, it becomes difficult
    to return a ScopedPointer as the result of a function. To avoid this causing errors,
    ScopedPointer contains an overloaded constructor that should cause a syntax error in these
    circumstances, but it does mean that instead of returning a ScopedPointer from a function,
    you'd need to return a raw pointer (or use a std::auto_ptr instead).
*/
template<class ObjectType>
class ScopedPointer
{
public:
    //==============================================================================
    /** Creates a ScopedPointer containing a null pointer. */
    ScopedPointer() noexcept
        : object(nullptr) {}

    /** Creates a ScopedPointer that owns the specified object. */
    ScopedPointer(ObjectType* const objectToTakePossessionOf) noexcept
        : object(objectToTakePossessionOf) {}

    /** Creates a ScopedPointer that takes its pointer from another ScopedPointer.

        Because a pointer can only belong to one ScopedPointer, this transfers
        the pointer from the other object to this one, and the other object is reset to
        be a null pointer.
    */
    ScopedPointer(ScopedPointer& objectToTransferFrom) noexcept
        : object(objectToTransferFrom.object)
    {
        objectToTransferFrom.object = nullptr;
    }

    /** Destructor.
        This will delete the object that this ScopedPointer currently refers to.
    */
    ~ScopedPointer()
    {
        delete object;
    }

    /** Changes this ScopedPointer to point to a new object.

        Because a pointer can only belong to one ScopedPointer, this transfers
        the pointer from the other object to this one, and the other object is reset to
        be a null pointer.

        If this ScopedPointer already points to an object, that object
        will first be deleted.
    */
    ScopedPointer& operator=(ScopedPointer& objectToTransferFrom)
    {
        if (this != objectToTransferFrom.getAddress())
        {
            // Two ScopedPointers should never be able to refer to the same object - if
            // this happens, you must have done something dodgy!
            DISTRHO_SAFE_ASSERT_RETURN(object == nullptr || object != objectToTransferFrom.object, *this);

            ObjectType* const oldObject = object;
            object = objectToTransferFrom.object;
            objectToTransferFrom.object = nullptr;
            delete oldObject;
        }

        return *this;
    }

    /** Changes this ScopedPointer to point to a new object.

        If this ScopedPointer already points to an object, that object
        will first be deleted.

        The pointer that you pass in may be a nullptr.
    */
    ScopedPointer& operator=(ObjectType* const newObjectToTakePossessionOf)
    {
        if (object != newObjectToTakePossessionOf)
        {
            ObjectType* const oldObject = object;
            object = newObjectToTakePossessionOf;
            delete oldObject;
        }

        return *this;
    }

    //==============================================================================
    /** Returns the object that this ScopedPointer refers to. */
    operator ObjectType*() const noexcept   { return object; }

    /** Returns the object that this ScopedPointer refers to. */
    ObjectType* get() const noexcept        { return object; }

    /** Returns the object that this ScopedPointer refers to. */
    ObjectType& getObject() const noexcept  { return *object; }

    /** Returns the object that this ScopedPointer refers to. */
    ObjectType& operator*() const noexcept  { return *object; }

    /** Lets you access methods and properties of the object that this ScopedPointer refers to. */
    ObjectType* operator->() const noexcept { return object; }

    //==============================================================================
    /** Removes the current object from this ScopedPointer without deleting it.
        This will return the current object, and set the ScopedPointer to a null pointer.
    */
    ObjectType* release() noexcept          { ObjectType* const o = object; object = nullptr; return o; }

    //==============================================================================
    /** Swaps this object with that of another ScopedPointer.
        The two objects simply exchange their pointers.
    */
    void swapWith(ScopedPointer<ObjectType>& other) noexcept
    {
        // Two ScopedPointers should never be able to refer to the same object - if
        // this happens, you must have done something dodgy!
        DISTRHO_SAFE_ASSERT_RETURN(object != other.object || this == other.getAddress() || object == nullptr,);

        std::swap(object, other.object);
    }

private:
    //==============================================================================
    ObjectType* object;

    // (Required as an alternative to the overloaded & operator).
    const ScopedPointer* getAddress() const noexcept { return this; }

#ifndef _MSC_VER  // (MSVC can't deal with multiple copy constructors)
    /* The copy constructors are private to stop people accidentally copying a const ScopedPointer
       (the compiler would let you do so by implicitly casting the source to its raw object pointer).

       A side effect of this is that in a compiler that doesn't support C++11, you may hit an
       error when you write something like this:

          ScopedPointer<MyClass> m = new MyClass();  // Compile error: copy constructor is private.

       Even though the compiler would normally ignore the assignment here, it can't do so when the
       copy constructor is private. It's very easy to fix though - just write it like this:

          ScopedPointer<MyClass> m (new MyClass());  // Compiles OK

       It's probably best to use the latter form when writing your object declarations anyway, as
       this is a better representation of the code that you actually want the compiler to produce.
    */
# ifdef DISTRHO_PROPER_CPP11_SUPPORT
    ScopedPointer(const ScopedPointer&) = delete;
    ScopedPointer& operator=(const ScopedPointer&) = delete;
# else
    ScopedPointer(const ScopedPointer&);
    ScopedPointer& operator=(const ScopedPointer&);
# endif
#endif
};

//==============================================================================
/** Compares a ScopedPointer with another pointer.
    This can be handy for checking whether this is a null pointer.
*/
template<class ObjectType>
bool operator==(const ScopedPointer<ObjectType>& pointer1, ObjectType* const pointer2) noexcept
{
    return static_cast<ObjectType*>(pointer1) == pointer2;
}

/** Compares a ScopedPointer with another pointer.
    This can be handy for checking whether this is a null pointer.
*/
template<class ObjectType>
bool operator!=(const ScopedPointer<ObjectType>& pointer1, ObjectType* const pointer2) noexcept
{
    return static_cast<ObjectType*>(pointer1) != pointer2;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_SCOPED_POINTER_HPP_INCLUDED
