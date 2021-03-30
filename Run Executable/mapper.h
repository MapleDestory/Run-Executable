#ifndef MAPPER
#define MAPPER

#ifndef MAPPER_API
#define MAPPER_API
#endif

#ifndef MAPPER_IMPL_API
#define MAPPER_IMPL_API
#endif

#if defined(_WIN64)
typedef unsigned long long	maplr;
#else
typedef unsigned long		maplr;
#endif

namespace Mapper
{
	MAPPER_API	bool	LoadRaw(unsigned char* lpData);
	MAPPER_API	bool	Builder(void);
	MAPPER_API	bool	Execute(void);
}

#endif