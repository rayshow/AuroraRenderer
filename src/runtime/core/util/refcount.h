#pragma once

#include"../type.h"

PROJECT_NAMESPACE_BEGIN

/*
struct T 
{
	AddRef(){}
	Release(){}
	int GetRefCount(){}
};
*/
template<typename T>
class TRefCountPtr
{
private:
	T* _ptr;
public:
	typedef T* ReferenceType;

	template <typename OtherType>
	friend class TRefCountPtr;


	AR_FORCEINLINE TRefCountPtr() :
		_ptr(nullptr)
	{ }

	TRefCountPtr(T* inPtr, bool bAddRef = true)
	{
		_ptr = inPtr;
		if (_ptr && bAddRef) {
			_ptr->AddRef();
		}
	}

	TRefCountPtr(const TRefCountPtr& other)
	{
		_ptr = other._ptr;
		if (_ptr){
			_ptr->AddRef();
		}
	}

	template<typename T2>
	explicit TRefCountPtr(const TRefCountPtr<T2>& other)
	{
		_ptr = static_cast<T*>(other.getReference());
		if (_ptr){
			_ptr->AddRef();
		}
	}

	AR_FORCEINLINE TRefCountPtr(TRefCountPtr&& other)
		: _ptr{ other._ptr }
	{
		other._ptr = nullptr;
	}

	template<typename T2>
	explicit TRefCountPtr(TRefCountPtr<T2>&& other)
		: _ptr{ static_cast<T*>(other.getReference()) }
	{
		other._ptr = nullptr;
	}

	~TRefCountPtr()
	{
		if (_ptr) {
			_ptr->Release();
		}
	}

	TRefCountPtr& operator=(T* raw)
	{
		if (_ptr != raw)
		{
			// Call AddRef before Release, in case the new reference is the same as the old reference.
			T* oldRef = _ptr;
			_ptr = raw;
			if (_ptr) {
				_ptr->AddRef();
			}
			if (oldRef) {
				oldRef->Release();
			}
		}
		return *this;
	}

	AR_FORCEINLINE TRefCountPtr& operator=(const TRefCountPtr& InPtr)
	{
		return *this = InPtr._ptr;
	}

	template<typename T2>
	AR_FORCEINLINE TRefCountPtr& operator=(const TRefCountPtr<T2>& other)
	{
		return *this = other.getReference();
	}

	TRefCountPtr& operator=(TRefCountPtr&& other)
	{
		if (this != std::addressof(other)) {
			T* old = _ptr;
			_ptr = other._ptr;
			other._ptr = nullptr;
			if (old) {
				old->Release();
			}
		}
		return *this;
	}

	template<typename T2>
	TRefCountPtr& operator=(TRefCountPtr<T2>&& other)
	{
		T* oldPtr = _ptr;
		_ptr = other._ptr;
		other._ptr = nullptr;
		if (oldPtr) {
			oldPtr->Release();
		}
		return *this;
	}

	AR_FORCEINLINE T* operator->() const
	{
		return _ptr;
	}

	AR_FORCEINLINE operator T() const
	{
		return _ptr;
	}

	AR_FORCEINLINE T** getInitAddress()
	{
		*this = nullptr;
		return &_ptr;
	}

	AR_FORCEINLINE T* getReference() const
	{
		return _ptr;
	}


	AR_FORCEINLINE bool isValid() const
	{
		return _ptr != nullptr;
	}

	AR_FORCEINLINE void safeRelease()
	{
		*this = nullptr;
	}

	u32 getRefCount()
	{
		u32 count = 0;
		if (_ptr) {
			count = _ptr->GetRefCount();
			// you should never have a zero ref count if there is a live ref counted pointer (*this is live)
			ARAssert(count > 0);
		}
		return count;
	}

	// this does not change the reference count, and so is faster
	AR_FORCEINLINE void swap(TRefCountPtr& other) 
	{
		if (_ptr != other._ptr) {
			T* old = _ptr;
			_ptr = other._ptr;
			other._ptr = old;
		}
	}

	AR_FORCEINLINE bool operator==(const TRefCountPtr& B) const
	{
		return _ptr == B.getReference();
	}

	AR_FORCEINLINE bool operator==(T* B) const
	{
		return _ptr == B;
	}

	AR_FORCEINLINE friend bool IsValidRef(const TRefCountPtr& refCount)
	{
		return refCount._ptr != nullptr;
	}
};

PROJECT_NAMESPACE_END