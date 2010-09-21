/*
 *------------------------------------------------------------------------
 * vim: ts=8 sw=8
 *------------------------------------------------------------------------
 * Author:   reynolds
 * Filename: Fri Jan  5 07:56:09 PST 2007
 * Created:  Fri Jan  5 07:56:09 PST 2007
 *------------------------------------------------------------------------
 */

#ifndef	GCC_COMPAT_H
# define GCC_COMPAT_H
# define clamp(v,l,u)	min( (u), max( (l), (v) ) )
# define DIM(a)		( sizeof( (a) ) / sizeof( (a)[0] ) )

# ifdef __GNUC__
   /* This is a type-safe max() implementation				*/
#  define max(x,y)	( __extension__ ({				\
        __typeof__(x) x__ = (x);                                        \
        __typeof__(y) y__ = (y);                                        \
        (void) (&x__ == &y__);                                          \
        x__ > y__ ? x__ : y__;                                          \
  }))
   /* This is a type-safe min() implementation                          */
#  define min(x,y)	( __extension__ ({                     		\
        __typeof__(x) x__ = (x);                                        \
        __typeof__(y) y__ = (y);                                        \
        (void) (&x__ == &y__);                                          \
        x__ < y__ ? x__ : y__;                                          \
  }))
#  define barrier()	( __extension__ ({				\
        __asm__ __volatile__("" : : : "memory" );                       \
  }))
#  define _aligned(v)	__attribute__((aligned(v)))
#  define _const	__attribute__((__const__))
#  define _constructor	__attribute__((constructor))
#  define _deprecated	__attribute__((deprecated))
#  define _destructor	__attribute__((destructor))
#  define _inline	__attribute__((always_inline))
#  define likely(e)	__builtin_expect((e),1)
#  define _nonnull(l)	__attribute__((nonnull l))
#  define _noreturn	__attribute__((__noreturn__))
#  define _packed	__attribute__((packed))
#  define _pure		__attribute__((pure))
#  define _printf(f,v)	__attribute__((format(printf,f,v)))
#  define _regparm	__attribute__((__regparm__(3)))
#  define _scanf(f,v)	__attribute__((format(scanf,f,v)))
#  define _sentinel(n)	__attribute__((sentinel(n)))
#  define _sentinel0	_sentinel(0)
#  define _stdcall	__attribute__((stdcall))
#  define unlikely(e)	__builtin_expect((e),0)
#  define _unused	__attribute__((unused))
#  define _used		__attribute__((used))
#  define _warn_unused	__attribute__((warn_unused_result))
# else  /* !__GCC__ */
#  define max(x,y)	( (x) > (y) ? (x) : (y) )
#  define min(x,y)	( (x) < (y) ? (x) : (y) )
#  define barrier()	/* You must limp along using 'volatile'		*/
#  define _aligned(v)
#  define _const
#  define _constructor	"You've got a problem"
#  define _deprecated
#  define _destructor	"You've got a problem"
#  define _inline	/* You don't get to inline			*/
#  define likely(e)	(e)
#  define _nonnull(l)
#  define _noreturn
#  define _packed
#  define _printf(f,v)	/* You are on your own, so deskcheck carefully!	*/
#  define _pure
#  define _regparm
#  define _sentinel(n)
#  define _sentinel0	_sentinel(0)
#  define _scanf(f,v)	/* You are on your own, so deskcheck carefully!	*/
#  define _stdcall
#  define unlikely(e)	(e)
#  define _unused
#  define _used
#  define _warn_unused
#endif /* !__GCC__ */

#endif	/* GCC_COMPAT_H */
