/*
 *------------------------------------------------------------------------
 * vim: ts=8 sw=8
 *------------------------------------------------------------------------
 * Author:   reynolds (Tommy Reynolds)
 * Filename: mmapps.c
 * Created:  2007-01-03 19:07:51
 *------------------------------------------------------------------------
 */

#define	_GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include <gcc-compat.h>

#define USE_MADVISE	1
#define USE_MSYNC	1

#define	IS_DEBUG(l)	( (l) <= debugLevel )

#define	BYTE_QTY	( ((off_t) 4096) * 1024 * 1024 );

typedef	enum	methods_e	{
	Method_pio,		/* Copy using normal I/O		*/
	Method_mmap		/* Copy using mmap(2) style		*/
} Method;

typedef	enum	roles_e	{
	Role_read,
	Role_write
} Role;

static	char *		me = "backbencher";
static	unsigned	nonfatal;
static	unsigned	debugLevel;
static	off_t		byte_qty;
static	Method		method = Method_pio;
static	Role		role = Role_write;
static	struct timeval	start_time;
static	struct timeval	stop_time;
static	off_t		chunksize;

static	void
vsay(
	int		e,
	char const *	fmt,
	va_list		ap
)
{
	fprintf( stderr, "%s: ", me );
	vfprintf( stderr, fmt, ap );
	if( e )	{
		fprintf( stderr, "; errno=%d (%s)", e, strerror( e ) );
	}
	fprintf( stderr, ".\n" );
}

static	void		_printf(2,3)
say(
	int		e,
	char const *	fmt,
	...
)
{
	va_list		ap;

	va_start( ap, fmt );
	vsay( e, fmt, ap );
	va_end( ap );
}

static	void		_printf(3,4)
debug(
	unsigned	theLevel,
	int		e,
	char const *	fmt,
	...
)
{
	if( IS_DEBUG( theLevel ) )	{
		va_list	ap;

		va_start( ap, fmt );
		vsay( e, fmt, ap );
		va_end( ap );
	}
}

static	void
handler(
	int	signo
)
{
	char	buf[ BUFSIZ ];

	signal( signo, handler );
	snprintf(
		buf,
		sizeof( buf ),
		"Signal %d (%s) seen.\n",
		signo,
		strsignal( signo )
	);
	write( fileno( stderr ), buf, strlen( buf ) );
}

static	void
cleanup(
	void
)
{
}

static	double
tv2double(
	struct timeval const *	tv
)
{
	double		result;

	result = 0.0;
	do	{
		result += (double) tv->tv_sec;
		result += ((double) tv->tv_usec * 1.0e-6);
	} while( 0 );
	return( result );
}
	

static	void
report(
	void
)
{
	static char const	fmt_f[] = "%7s: %f\n";
	static char const	fmt_g[] = "%7s: %g\n";
	static char const	fmt_s[] = "%7s: %s\n";
	static char const	fmt_llu[] = "%7s: %llu\n";
	double const	started = tv2double( &start_time );
	double const	ended   = tv2double( &stop_time );
	double const	seconds = ended - started;
	double const	bytes = byte_qty;
	double const	rate = bytes / seconds;

	switch( role )	{
	default:
		say( 0, "bogus role is %d", role );
		exit( 1 );
	case Role_read:
		printf( fmt_s, "Role", "Reading" );
		break;
	case Role_write:
		printf( fmt_s, "Role", "Writing" );
		break;
	}
	switch( method )	{
	default:
		say( 0, "honking bad method %d", method );
		exit( 1 );
	case Method_pio:
		printf( fmt_s, "Method", "read(2) / write(2)" );
		break;
	case Method_mmap:
		printf( fmt_s, "Method", "mmap(2) / munmap(2)" );
		break;
	}
	printf( fmt_f, "End",		ended );
	printf( fmt_f, "Start",		started );
	printf( fmt_f, "Seconds",	seconds );
	printf( fmt_f, "Bytes",		bytes );
	printf( fmt_g, "Rate",		rate );
	printf( fmt_llu, "BlkSiz",	(unsigned long long) chunksize );
}

/*
 *------------------------------------------------------------------------
 * main: central control logic
 *------------------------------------------------------------------------
 */

int
main(
	int		argc,
	char * *	argv
)
{
	int		c;
	char *		bp;
	int		fd;
	char *		fn;
	char *		zeros;

	/* Get ready							*/
	signal( SIGINT, handler );
	atexit( cleanup );
	/* Figure out our process name					*/
	me = argv[ 0 ];
	if( (bp = strrchr( me, '/' )) != NULL )	{
		me = bp + 1;
	}
	/* Process the command line					*/
	opterr = 0;
	while( (c = getopt( argc, argv, "Dc:mn:r" )) != EOF )	{
		switch( c )	{
		default:
			say( 0, "switch -%c not implemented yet", c );
			++nonfatal;
			break;
		case '?':
			say( 0, "unknown switch -%c", optopt );
			++nonfatal;
			break;
		case 'D':
			++debugLevel;
			debug( debugLevel, 0, "debug level is %u", debugLevel );
			break;
		case 'c':
			chunksize = strtoull( optarg, NULL, 0 );
			break;
		case 'm':
			method = Method_mmap;
			break;
		case 'n':
			byte_qty = strtoull( optarg, NULL, 0 );
			break;
		case 'r':
			role = Role_read;
			break;
		}
	}
	if( nonfatal )	{
		say( 0, "illegal switch(es)" );
		exit( 1 );
	}
	/*								*/
	if( !chunksize )	{
		chunksize = getpagesize();
	}
	zeros = calloc( chunksize, 1 );
	/* Attach to output file					*/
	if( optind >= argc )	{
		say( 0, "output filename must be specified" );
		exit( 1 );
	}
	fn = argv[ optind++ ];
	debug( 1, 0, "Using file '%s'", fn );
	switch( role )	{
	default:
		say( 0, "where did role %d come from", role );
		exit( 1 );
	case Role_read:
		{
			fd = open( fn, O_RDONLY );
			/* Limit reading to actual size of the file	*/
			if( fd != -1 )	{
				struct stat	st;

				if( fstat( fd, &st ) )	{
					say( errno, "could not stat '%s'", fn );
					exit( 1 );
				}
				if( !byte_qty )	{
					byte_qty = st.st_size;
				} else	{
					byte_qty = min( byte_qty, st.st_size );
				}
			}
		}
		break;
	case Role_write:
		if( !byte_qty )	{
			say( 0, "writing requires '-n #' switch" );
			exit( 1 );
		}
		fd = open( fn, (O_RDWR | O_CREAT | O_TRUNC), 0660 );
		if( fd != -1 )	{
			if( ftruncate( fd, byte_qty ) )	{
				say(
					errno,
					"could not set filesize to %llu",
					(unsigned long long) byte_qty
				);
				exit( 1 );
			}
		}
		break;
	}
	if( fd == -1 )	{
		say( errno, "cannot open file '%s'", fn );
		exit( 1 );
	}
	/* That should be all						*/
	if( optind < argc )	{
		say( 0, "too many arguments" );
		exit( 1 );
	}
	/* Copy data using either read(2)/write(2) or mmap(2)		*/
	if( gettimeofday( &start_time, NULL ) )	{
		say( errno, "could not get start time" );
		exit( 1 );
	}
	switch( method )	{
	default:
		say( 0, "where did you come up with method %u", method );
		exit( 1 );
	case Method_pio:
		switch( role )	{
		default:
			say( 0, "bad role %d", role );
			exit( 1 );
		case Role_read:
			{
				off_t		remain;

				for( remain = byte_qty; remain; )	{
					size_t const	gulp = min(
						chunksize,
						(off_t) remain
					);
					if( read(
						fd,
						zeros,
						gulp
					) != (ssize_t) gulp ) {
						say( errno, "short read" );
						exit( 1 );
					}
					remain -= gulp;
				}
			}
			break;
		case Role_write:
			{
				off_t		remain;

				for( remain = byte_qty; remain; )	{
					size_t const	gulp = min(
						chunksize,
						(off_t) remain
					);

					if( write(
						fd,
						zeros,
						gulp
					) != (ssize_t) gulp )	{
						say( errno, "output truc" );
						exit( 1 );
					}
					remain -= gulp;
				}
			}
			break;
		}
		break;
	case Method_mmap:
		{
			off_t		offset;
			off_t		remain;

			for( offset = 0, remain = byte_qty; remain; )	{
				size_t	wlen;
				off_t	align;
				void *	dst_map;

				/* How big should our window be?	*/
				wlen = min( (off_t) remain, chunksize );
				debug(
					2,
					0,
					"window is %llu bytes",
					(unsigned long long) wlen
				);
				align = offset % (off_t) getpagesize();
				wlen += align;
				offset -= align;
				/* Map in the dst window		*/
				debug(
					1,
					0,
					"Mapping dst %llu@%llu",
					(unsigned long long) wlen,
					(unsigned long long) offset
				);
				switch( role )	{
				default:
					say( 0, "what's role %d", role );
					exit( 1 );
				case Role_read:
					dst_map = mmap(
						NULL,
						wlen,
						PROT_READ,
						MAP_SHARED,
						fd,
						offset
					);
					break;
				case Role_write:
					dst_map = mmap(
						NULL,
						wlen,
						PROT_WRITE,
						MAP_SHARED,
						fd,
						offset
					);
					break;
				}
				if( dst_map == MAP_FAILED )	{
					say( 
						errno,
						"dst mmap(2) failed"
						"; offset=%llu, wlen=%llu",
						(unsigned long long) offset,
						(unsigned long long) wlen
					);
					exit( 1 );
				}
#if	USE_MADVISE
				if( madvise(
					dst_map,
					wlen,
					( MADV_SEQUENTIAL| MADV_WILLNEED )
				) )	{
					perror("madvise()");
				}
#endif	/* USE_MADVISE */
				/* Copy data into destination file	*/
				switch( role )	{
				default:
					say( 0, "bogus role %d", role );
					exit( 1 );
				case Role_read:
					memcpy(
						zeros,
						(char *) dst_map + align,
						wlen - align
					);
					break;
				case Role_write:
					memcpy(
						(char *) dst_map + align,
						zeros,
						wlen - align
					);
#if	USE_MSYNC
					if( msync(
						dst_map,
						wlen,
						( MS_ASYNC )
					) )	{
						perror("madvise()");
					}
#endif	/* USE_MSYNC */
					break;
				}
				/* Release the maps			*/
				if( munmap( dst_map, wlen ) )	{
					say( errno, "dst unmap failed" );
					exit( 1 );
				}
				/* Update for next loop			*/
				offset += wlen;
				remain -= (wlen - align);
			}
		}
		break;
	}
	if( gettimeofday( &stop_time, NULL ) )	{
		say( errno, "could not get end time" );
	}
	close( fd );
	/*								*/
	report();
	/*								*/
	return( nonfatal ? 1 : 0 );
}
