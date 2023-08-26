#pragma once
#include<Core/type.h>
#include<typeinfo>  /* std::typeinfo */
#include<cstring>   /* for memory move and copy*/
#include<exception> /* for std exception */
#include<type_traits>
#include<stdlib.h>
#include<assert.h>

//pointer releative type / helpful fn
namespace ptr
{
#if A3D_COMPILER_CLANG
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wunused-value"
#endif
	template<typename R> 
	RS_FORCEINLINE R* safe_new_copy_array(R const* array, size_t length =1 ) { 
		R* newArray = nullptr;
		ptr::size_t size =0;
		if(array && length>0){
			if constexpr(std::is_void_v<R>){
				size = length;
			}else{
				size = sizeof(R)*length;
			}
			newArray = reinterpret_cast<R*>(malloc(size));
			rs_assert(newArray!=nullptr);
			if constexpr(is_raw_string_v<R>){
				// raw-string array
				for(uint32 i=0; i<length; ++i){
					newArray[i] = array[i] ? strdup(array[i]) : nullptr;
				}
			}
			else if constexpr(std::is_pod_v<R>|| std::is_void_v<R>) {
				// raw bytes or pod array
				memcpy(newArray, array, size);
			}
			// need constructor
			else{
				for(size_t i=0; i<length; ++i){
					// copy constructor
					new(newArray+i) R{array[i]};
				}
			}
		} 
		return newArray;
	}

	template<typename R, typename RawR = std::remove_const_t<R> > 
	RS_FORCEINLINE R* safe_new_default_array(size_t length=1) { 
		RawR* newArray = nullptr;
		if(length>0){ 
			size_t size = 0;
			if constexpr(std::is_void_v<R>){
				size = length;
			}else{
				size = sizeof(R)*length;
			}
			newArray = reinterpret_cast<RawR*>( malloc(size));
			if(!newArray){ assert(false); return nullptr;}
			if constexpr(std::is_pod_v<R> || std::is_void_v<R> ) {
				memset( newArray, 0, size);
			}
			else{
				// not pod and not void
				for(ptr::size_t i=0; i<length; ++i){
					// default constructor
					new(newArray+i) R{};
				}
			}
		}
		return newArray;
	}

	template<typename R> 
	RS_FORCEINLINE R* safe_new_copy(R const* object ) { 
		if constexpr(is_char_v<R>){
			// raw-string
			return object ? strdup(object) : nullptr;
		}else{
			return safe_new_copy_array(object, 1);
		}
	}

	template<typename T, typename NoConstT = std::remove_const_t<T>> 
	void safe_delete(T*& t) { 
		NoConstT* addr = const_cast<NoConstT*>(t);

		if constexpr(std::is_void_v<T> || std::is_pod_v<T> || is_char_v<T>){
			free( addr );
			t = nullptr;
		}else{
			sizeof(*t); 
			if (t) { t->~NoConstT(); free(t);  t = nullptr; } 
		}
	}
	template<typename T > 
	void safe_delete_array(T*& t, u32 length ) { 
		if(!t || length==0 ) return;
		if constexpr(is_raw_string_v<T>){
			rs_assert(length>0);
			using NoConstPointerT = std::remove_const_t< std::remove_pointer_t<T>>;
			for(uint32 i=0; i<length; ++i){
				NoConstPointerT* addr = const_cast<NoConstPointerT*>(t[i]);
				free(addr);
				addr=nullptr;
			}
			free( const_cast<NoConstPointerT**>( const_cast<NoConstPointerT* const*>(t)) );
			t=nullptr;
		}
		else if constexpr(std::is_void_v<T> || std::is_pod_v<T>){
			using NoConstT = std::remove_const_t< T>;
			NoConstT* addr = const_cast<NoConstT*>(t);
			free(addr);
			t = nullptr;
		}else{
			sizeof(*t); 
			//using NoConstT = std::remove_const_t< T>;
			for(uint32 i=0; i<length; ++i){
				t[i].~T();
			}
			free(t);
			t = nullptr;
		}
	}

	//like void* / declared T*, not check type
	template<typename T> void imcomp_delete(T*& t) { if (t) { delete t; t = nullptr; } }

} // end ptr

#if A3D_COMPILER_CLANG
	#pragma clang diagnostic pop
#endif
