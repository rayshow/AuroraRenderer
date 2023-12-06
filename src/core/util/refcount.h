#pragma once

#include"../type.h"

template<typename ReferencedType>
class TRefCountPtr
{
	typedef ReferencedType* ReferenceType;

public:

	AR_FORCEINLINE TRefCountPtr() :
		Reference(nullptr)
	{ }

	TRefCountPtr(ReferencedType* InReference, bool bAddRef = true)
	{
		Reference = InReference;
		if (Reference && bAddRef)
		{
			Reference->AddRef();
		}
	}

	TRefCountPtr(const TRefCountPtr& Copy)
	{
		Reference = Copy.Reference;
		if (Reference)
		{
			Reference->AddRef();
		}
	}

	template<typename CopyReferencedType>
	explicit TRefCountPtr(const TRefCountPtr<CopyReferencedType>& Copy)
	{
		Reference = static_cast<ReferencedType*>(Copy.GetReference());
		if (Reference)
		{
			Reference->AddRef();
		}
	}

	AR_FORCEINLINE TRefCountPtr(TRefCountPtr&& Move)
	{
		Reference = Move.Reference;
		Move.Reference = nullptr;
	}

	template<typename MoveReferencedType>
	explicit TRefCountPtr(TRefCountPtr<MoveReferencedType>&& Move)
	{
		Reference = static_cast<ReferencedType*>(Move.GetReference());
		Move.Reference = nullptr;
	}

	~TRefCountPtr()
	{
		if (Reference)
		{
			Reference->Release();
		}
	}

	TRefCountPtr& operator=(ReferencedType* InReference)
	{
		if (Reference != InReference)
		{
			// Call AddRef before Release, in case the new reference is the same as the old reference.
			ReferencedType* OldReference = Reference;
			Reference = InReference;
			if (Reference)
			{
				Reference->AddRef();
			}
			if (OldReference)
			{
				OldReference->Release();
			}
		}
		return *this;
	}

	AR_FORCEINLINE TRefCountPtr& operator=(const TRefCountPtr& InPtr)
	{
		return *this = InPtr.Reference;
	}

	template<typename CopyReferencedType>
	AR_FORCEINLINE TRefCountPtr& operator=(const TRefCountPtr<CopyReferencedType>& InPtr)
	{
		return *this = InPtr.GetReference();
	}

	TRefCountPtr& operator=(TRefCountPtr&& InPtr)
	{
		if (this != &InPtr)
		{
			ReferencedType* OldReference = Reference;
			Reference = InPtr.Reference;
			InPtr.Reference = nullptr;
			if (OldReference)
			{
				OldReference->Release();
			}
		}
		return *this;
	}

	template<typename MoveReferencedType>
	TRefCountPtr& operator=(TRefCountPtr<MoveReferencedType>&& InPtr)
	{
		// InPtr is a different type (or we would have called the other operator), so we need not test &InPtr != this
		ReferencedType* OldReference = Reference;
		Reference = InPtr.Reference;
		InPtr.Reference = nullptr;
		if (OldReference)
		{
			OldReference->Release();
		}
		return *this;
	}

	AR_FORCEINLINE ReferencedType* operator->() const
	{
		return Reference;
	}

	AR_FORCEINLINE operator ReferenceType() const
	{
		return Reference;
	}

	AR_FORCEINLINE ReferencedType** GetInitReference()
	{
		*this = nullptr;
		return &Reference;
	}

	AR_FORCEINLINE ReferencedType* GetReference() const
	{
		return Reference;
	}

	AR_FORCEINLINE friend bool IsValidRef(const TRefCountPtr& InReference)
	{
		return InReference.Reference != nullptr;
	}

	AR_FORCEINLINE bool IsValid() const
	{
		return Reference != nullptr;
	}

	AR_FORCEINLINE void SafeRelease()
	{
		*this = nullptr;
	}

	uint32 GetRefCount()
	{
		uint32 Result = 0;
		if (Reference)
		{
			Result = Reference->GetRefCount();
			check(Result > 0); // you should never have a zero ref count if there is a live ref counted pointer (*this is live)
		}
		return Result;
	}

	AR_FORCEINLINE void Swap(TRefCountPtr& InPtr) // this does not change the reference count, and so is faster
	{
		ReferencedType* OldReference = Reference;
		Reference = InPtr.Reference;
		InPtr.Reference = OldReference;
	}

private:

	ReferencedType* Reference;

	template <typename OtherType>
	friend class TRefCountPtr;

public:
	AR_FORCEINLINE bool operator==(const TRefCountPtr& B) const
	{
		return GetReference() == B.GetReference();
	}

	AR_FORCEINLINE bool operator==(ReferencedType* B) const
	{
		return GetReference() == B;
	}
};