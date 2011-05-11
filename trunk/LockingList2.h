#ifndef LOCKING_LIST2_H
#define LOCKING_LIST2_H

// we just make 'lock' public to be able to use it in a BAutoLock

#include <Locker.h>
#include "support/ObjectList.h"

namespace BPrivate {

template <class T>
class LockingList2 : public BObjectList<T> {
public:
	LockingList2(int32 itemsPerBlock = 20, bool owning = false);
	~LockingList2()
		{
		Lock();
		}

	bool Lock();
	void Unlock();
	bool IsLocked() const;

	BLocker lock;
};

template<class T>
LockingList2<T>::LockingList2(int32 itemsPerBlock, bool owning)
	:	BObjectList<T>(itemsPerBlock, owning)
{
}

template<class T>
bool 
LockingList2<T>::Lock()
{
	return lock.Lock();
}

template<class T>
void 
LockingList2<T>::Unlock()
{
	lock.Unlock();
}

template<class T>
bool 
LockingList2<T>::IsLocked() const
{
	return lock.IsLocked();
}


} // namespace BPrivate

using namespace BPrivate;

#endif
