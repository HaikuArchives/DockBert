#ifndef LOCKING_LIST2_H
#define LOCKING_LIST2_H

// we just make 'lock' public to be able to use it in a BAutoLock

#include <Locker.h>
#include <ObjectList.h>

namespace BPrivate {

template <class T, bool Owning = false>
class LockingList2 : public BObjectList<T, Owning> {
public:
	LockingList2(int32 itemsPerBlock = 20);
	~LockingList2()
		{
		Lock();
		}

	bool Lock();
	void Unlock();
	bool IsLocked() const;

	BLocker lock;
};

template<class T, bool O>
LockingList2<T, O>::LockingList2(int32 itemsPerBlock)
	:	BObjectList<T, O>(itemsPerBlock)
{
}

template<class T, bool O>
bool 
LockingList2<T, O>::Lock()
{
	return lock.Lock();
}

template<class T, bool O>
void 
LockingList2<T, O>::Unlock()
{
	lock.Unlock();
}

template<class T, bool O>
bool 
LockingList2<T, O>::IsLocked() const
{
	return lock.IsLocked();
}


} // namespace BPrivate

using namespace BPrivate;

#endif
