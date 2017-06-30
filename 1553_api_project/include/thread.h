/**
 * \file thread.h
 * 任务及同步管理.
 *
 * 提供Windows/VxWorks/Linux/UNIX(posix)全兼容的线程/任务管理类、
 * 线程同步/互斥所需的各种机制和一些工具，包括互斥锁、条件锁、同步事件、
 * 以及退出作用域自动释放的锁。
 *
 *    \date 2009-8-22
 *  \author anhu.xie
 */

#ifndef __THREAD_SYNC_UTIL_H_
#define __THREAD_SYNC_UTIL_H_

#if defined(WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#define  _CRT_SECURE_NO_WARNINGS
#endif

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <errno.h>
#include <stdio.h>

// 兼容性设置
#ifdef __vxworks
#include <sysLib.h>
#include <taskLib.h>
#include <semLib.h>
#define OSAPI
typedef int thread_return_t;
typedef int thread_id_t;
const int DEFAULT_THREAD_PRIORITY = 100;
int usleep(unsigned long usec);
#elif defined(WIN32)
#include <Windows.h>
#define OSAPI WINAPI
const int DEFAULT_THREAD_PRIORITY = THREAD_PRIORITY_NORMAL;
typedef DWORD thread_return_t;
typedef HANDLE thread_id_t;
inline void usleep(unsigned long usec) {
	return Sleep((usec + 999) / 1000);
}
#else
#include <pthread.h>
#include <string.h>
#define OSAPI
const int DEFAULT_THREAD_PRIORITY = 100;
typedef void *thread_return_t;
typedef pthread_t thread_id_t;
#endif

/**
 * 操作系统异常的包装
 */
class os_error : public std::exception {
	int error_code;
	/*
	 * 因为what()要求返回指针，需要一个存放指针所指内容的对象！而不能返回一个指向临时/自动对象的指针。
	 */
protected:
	std::string msg;
public:
	/**
	 * 构造函数，用操作系统的错误码来初始化。
	 * \param eno 操作系统错误码
	 */
	os_error(int eno) : error_code(eno) {
		std::stringstream ss;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif
		ss << "system error: \"" << strerror(error_code) << "\"(" << error_code << ").";
#ifdef _MSC_VER
#pragma warning(pop)
#endif
		msg = ss.str();
	}
	/// 析构函数
	virtual ~os_error() throw () {
	}
	/// 异常的错误信息，重实现
	virtual const char* what() const throw () {
		return msg.c_str();
	}
// 	/// 异常的错误信息，重实现
// 	virtual const char* what() const {
// 		return msg.c_str();
// 	}
	/// 返回操作系统错误码的方法
	virtual int reason() const {
		return error_code;
	}
};

/**
 * 调用成员函数的线程函数.
 * 被CreateThreadWithMember()用到。
 * \see CreateThreadWithMember
 * \tparam T 类名称
 * \tparam proc T类的成员函数，这是T类对象作为线程启动的工作函数。它没有参数，返回操作系统要求的thread_return_t类型的值。
 * \param o T类的对象，本函数调用此对象的指定函数作为实际的线程函数
 */
template<class T, thread_return_t(T::*proc)()>
thread_return_t OSAPI thread_proc(void *o) {
	return ((reinterpret_cast<T*>(o))->*proc)();
}
/**
 * 成员函数作为线程函数的工具.
 * \tparam T 类名称
 * \tparam proc T类的成员函数，这是T类对象作为线程启动的工作函数。它没有参数，返回操作系统要求的thread_return_t类型的值。
 * \param o 要启动线程的T类对象
 * \param tid 存放新任务的任务id，输出
 * \param priority 启动任务的优先级
 * \param t_name 任务的名字，调试和排故时非常有用
 * \param size WIN32下使用，创建线程时分配的堆栈大小，若为0，则为默认大小
 */
template<class T, thread_return_t(T::*proc)()>
void CreateThreadWithMember(T *o, thread_id_t &tid, int priority = DEFAULT_THREAD_PRIORITY, const char *t_name = NULL,int size = 0) {
	thread_return_t(OSAPI *thproc)(void *) = &thread_proc<T,proc>;
#ifdef __vxworks
	int stackSize = 1024 * 1024 * 1;
	if (size != 0)
	{
		stackSize = size;
	}
	tid = taskSpawn(
	         const_cast<char*>(t_name),           /* name of new task (stored at pStackBase) */
	         priority,       /* priority of new task */
	         VX_FP_TASK,     /* task option word */
	         stackSize,      /* size (bytes) of stack needed plus name */
	         reinterpret_cast<int (*)(...)>(thproc),   /* entry point of new task */
	         reinterpret_cast<int>(o),          /* 1st of 10 req'd task args to pass to func */
	         0, 0, 0, 0, 0, 0, 0, 0, 0 /* we only use 1 arg -- the object itself */
	         );
	if ( tid == ERROR )
		throw os_error(errno);
#elif defined(WIN32)
	if (size != 0)
	{
		tid = CreateThread(NULL, size, thproc, (LPVOID)o,STACK_SIZE_PARAM_IS_A_RESERVATION | CREATE_SUSPENDED, 0);
	}else	
	{
		tid = CreateThread(NULL, 0, thproc, (LPVOID)o, CREATE_SUSPENDED, 0);
	}
	//tid = CreateThread(NULL, 1*1024*1024, thproc, (LPVOID)o,STACK_SIZE_PARAM_IS_A_RESERVATION, 0);
	if ( tid == NULL )
		throw os_error(GetLastError());
	if ( !SetThreadPriority(tid, priority) )
		throw os_error(GetLastError());
	if ( !ResumeThread(tid) )
		throw os_error(GetLastError());
	
#else
	int e = pthread_create(&tid, NULL, thproc, o);
	if ( e != 0 )
		throw os_error(e);
#endif
}

/**
 * 任务/线程的抽象.
 * 提供任务启动、停止和同步控制的机制。
 */
class Thread {
protected:
	/// 操作系统返回的线程标识
	thread_id_t thread_id;
	/// 线程标识，线程主动停止后，thread_id会被清除，这里记住它等待父线程JOIN
	thread_id_t join_id; // Linux must join, even when the thread has stopped!
	/// 线程优先级
	const int priority;
	/// 线程名称，便于排故时区分各个不同的线程
	std::string thread_name;
	/// 线程状态，注意它们是有序的！
	volatile enum {
		WS_INIT, ///< 对象刚刚创建时的初始状态
		WS_RUN, ///< 线程运行中/保持线程运行的状态
		WS_STOP, ///< 让线程停止时设置的状态
		WS_ERROR, ///< 线程异常
		WS_EXIT ///< 线程已退出
	} work_stat;

public:
	/**
	 * 构造函数
	 * \param pri 任务启动时使用的优先级
	 */
	Thread(int pri = DEFAULT_THREAD_PRIORITY) :
		thread_id(0), join_id(0), priority(pri), work_stat(WS_INIT) {
	}
	/**
	 * 析构函数
	 */
	virtual ~Thread() {
#ifdef __linux__
		sync();
#elif defined(WIN32)
		if ( thread_id )
			CloseHandle(thread_id);
#endif
	}
	/**
	 * 启动任务
	 * \param thread_name 操作系统中任务的名字
	 * \param size 创建线程分配的堆栈大小，若为0，则为默认大小
	 */
	void start(const char *display_name = NULL,int size = 0) {
#ifdef _TRACE
		std::cerr << "thread start [" << display_name << "]" << std::endl;
#endif
		if ( thread_id /*&& taskIdVerify(thread_id) == OK*/ ) // 线程已启动
			return;
		if ( display_name )
			thread_name = display_name;
		work_stat = WS_RUN;
		CreateThreadWithMember<Thread, &Thread::run0> (this, thread_id, priority, display_name,size);
		join_id = thread_id;
	}
	/**
	 * 任务同步.
	 * 等待任务完成。
	 * 注意：Linux系统，对每个启动了的线程，必须调用sync()。
	 * \param retval_ptr 存放任务返回值的指针。NULL表示不需要返回值。
	 * \return 操作系统返回值，操作是否成功。
	 */
	int sync(void **retval_ptr = NULL) {
		if ( !join_id )
			return 0;
		int r;
#ifdef __vxworks
		r = -1;
#elif defined(WIN32)
		r = WaitForSingleObject(join_id, INFINITE);
#else
		r = pthread_join(join_id, retval_ptr);
#endif
		join_id = 0;
		return r;
	}
	/**
	 * 停止任务.
	 * 给任务发送停止信号。
	 */
	void stop() {
		if ( work_stat == WS_RUN )
			work_stat = WS_STOP;
	}
	/**
	 * (强制)停止任务.
	 * 先给任务发送停止信号。如果100ms任务不主动停止，则强行停止任务。
	 */
	int exit() {
		if ( thread_id == 0 && join_id == 0 )
			return 0;
		stop();
		usleep(100000);
		if ( thread_id == 0 ) // 再次检查是否退出
			return 0;
#ifdef __vxworks
		int ret = taskIdVerify(thread_id) == OK ? taskDelete(thread_id) : OK;
#elif defined(WIN32)
		int ret = TerminateThread(thread_id, -1) ? 0 : GetLastError();
		CloseHandle(thread_id);
		join_id = 0;
#else
		int ret = pthread_cancel(thread_id);
#endif
		work_stat = WS_EXIT;
		thread_id = 0;
		return ret;
	}
private:
	/*
	 * 使run方法可被派生类重定义
	 */
	thread_return_t run0()
	{
		return run();
	}
protected:
	/**
	 * 任务的工作函数.
	 * 默认的工作函数反复调用do_job()，直到任务停止信号被设置。在循环过程中它处理并报告异常。
	 * 停止信号可能被停止任务的方法stop()、exit()设置，也可能根据业务逻辑有工作任务do_job()设置。
	 * \p不推荐派生类重写此方法。
	 * \return 操作系统指定的任务返回值。
	 */
	virtual thread_return_t run() {
		while ( work_stat == WS_RUN )
			try {
				do_job();
			}
			catch ( std::exception &x ) {
				deal_error(std::string("std::exception caught: ") + x.what());
				usleep(100000);
			}
			catch ( ... ) {
				deal_error("unknown exception caught");
				usleep(100000);
			}
#if defined(WIN32)
		CloseHandle(thread_id);
		join_id = 0;
#endif
		thread_id = 0;
		return 0;
	}
	/**
	 * 任务函数.
	 * 任务真正的工作在这里完成。由于它会被反复调用，所以重复的工作不应该在这里循环。
	 * \p默认的实现什么也不干。派生类应该重新实现此方法。
	 */
	virtual void do_job() {
		usleep(100000);
	}
	/**
	 * 异常处理函数.
	 * 任务工作中发生异常时调用此方法处理错误。
	 * 默认的处理是输出错误信息并停止任务。派生类往往需要重新实现此方法。
	 */
	virtual void deal_error(const std::string &msg) {
		std::cerr << "thread " << thread_name << " <" << thread_id << "> " << msg.c_str() << std::endl;
		work_stat = WS_ERROR;
	}
};

//__thread bool lock_locked;
/**
 * \class MXLock
 * 互斥锁定
 */

class MXLock {
#ifdef __vxworks
	SEM_ID mySem; // 锁定数据结构
#elif defined(WIN32)
	CRITICAL_SECTION sync_lock;
#else
	pthread_mutex_t mutex;
#endif
	bool lock_inited; // CRITICAL_SECTION需要延迟(我们等第一次上锁时)初始化，所以记录是否初始化
public:
	void lock(); ///< 锁定：保护多任务时数据结构的安全性
	void unlock(); ///< 解除锁定

	MXLock();
	~MXLock();
#if !defined( __vxworks ) && !defined(WIN32)
	pthread_mutex_t *get_mutex() { return &mutex; }
#endif
private:
	MXLock(const MXLock &s);
	MXLock &operator=(const MXLock &s);
};

inline MXLock::~MXLock() {
	if ( lock_inited ) {
#ifdef __vxworks
		semDelete (mySem);
#elif defined(WIN32)
		DeleteCriticalSection(&sync_lock);
#else /* __vxworks */
		pthread_mutex_destroy(&mutex);
#endif /* __vxworks */
		lock_inited = false;
	}
}

inline void MXLock::lock() {
	if ( !lock_inited ) {
#ifdef __vxworks
		mySem = semMCreate (SEM_Q_PRIORITY);
#elif defined(WIN32)
		InitializeCriticalSection(&sync_lock);
#else /* __vxworks */
		class MutexAutoRecursiveAttr {
		public:
			pthread_mutexattr_t mutex_attr;
			MutexAutoRecursiveAttr() {
				pthread_mutexattr_init(&mutex_attr);
				pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
			}
			~MutexAutoRecursiveAttr() {
				pthread_mutexattr_destroy(&mutex_attr);
			}
		};
		static MutexAutoRecursiveAttr recursive;
		pthread_mutex_init(&mutex, &recursive.mutex_attr);
#endif /* __vxworks */
		lock_inited = true;
	}
#if _TRACE > 8
	std::cout << this << "\tMXLock::lock() begin..." << std::endl;
#endif
#ifdef __vxworks
	int err = S_objLib_OBJ_TIMEOUT;
	while ( err == S_objLib_OBJ_TIMEOUT ) {
		int s = semTake (mySem, 100);
		if ( s == OK )
			return;
		err = errno;
	}
	throw os_error(err);
#elif defined(WIN32)
	EnterCriticalSection(&sync_lock);
#else /* __vxworks */
	pthread_mutex_lock(&mutex);
#endif /* __vxworks */
#if _TRACE > 8
	std::cout << this << "\tMXLock::lock() ...success" << std::endl;
#endif
}

inline void MXLock::unlock() {
#ifdef __vxworks
	semGive (mySem);
#elif defined(WIN32)
	LeaveCriticalSection(&sync_lock);
#else /* __vxworks */
	pthread_mutex_unlock(&mutex);
#endif /* __vxworks */
#if _TRACE > 8
	std::cout << this << "\tMXLock::unlock() ... released!" << std::endl;
#endif
}

inline MXLock::MXLock() : lock_inited(false) {
}

inline MXLock::MXLock(const MXLock &) : lock_inited(false) { // 同步对象不能拷贝！
}
inline MXLock& MXLock::operator=(const MXLock &) {
	return *this; // 保留自己的同步对象，不要拷贝！
}

/// 自动释放的锁！
/// \note 注意使用AutoLock的变量要精确控制作用域，
/// 尤其注意不要用作临时(匿名)变量；因为临时变量可能提前析构(解锁)，往往起不到你希望的作用。
class AutoLock {
	MXLock &lock_obj; // 注意我们是“引用”，并没有创建新的锁对象！
public:
	/**
	 * 构造函数，锁定指定的锁
	 * \param lock 指定锁
	 */
	AutoLock(MXLock &lock) :
		lock_obj(lock) {
		lock_obj.lock();
	}
	/**
	 * 析构函数，释放我们锁定的内容
	 */
	~AutoLock() {
		lock_obj.unlock();
	}
};

/// 条件锁：满足条件时锁定
class CondEvent {
#ifdef __vxworks
	SEM_ID ev;
#elif defined(WIN32)
	HANDLE ev;
#else /* __vxworks */
	pthread_cond_t ev;
#endif
public:
	/// 构造函数
	/// \param initial_signal 条件的初始值
	CondEvent(bool initial_signal = false) {
#ifdef __vxworks
		ev = semBCreate (SEM_Q_PRIORITY, initial_signal ? SEM_FULL : SEM_EMPTY);
#elif defined(WIN32)
		ev = CreateEvent(NULL, false, initial_signal, NULL);
#else /* __vxworks */
		pthread_cond_init(&ev, NULL);
		if ( initial_signal )
			pthread_cond_signal(&ev);
#endif
	}

	~CondEvent() {
#ifdef __vxworks
		semDelete (ev);
#elif defined(WIN32)
		CloseHandle(ev);
#else /* __vxworks */
		pthread_cond_destroy(&ev);
#endif /* __vxworks */
	}

	/**
	 * 条件锁定.
	 *
	 * 挂起任务，直到条件满足。
	 * 如果调用时条件为真，则直接锁定指定的锁；否则，挂起调用任务，
	 * 等待条件变为真(等待时解除锁定)。为了避免无限等待，可是设置超时值，
	 * 如果在规定的时间内条件变为真，锁定指定的锁，否则，直接返回。
	 *
	 * \param mutex 指定的锁，返回时如果条件为真，此锁被锁定，调用者可直接操作锁保护的数据
	 * \param time 等到的最长时间，单位微秒
	 * \return 等待的条件是否为真。条件为真时会锁定指定的锁，为假时不锁定。
	 * \exception os_error 底层操作系统错误抛出此异常，包含了错误原因。
	 */
	bool wait(MXLock *mutex, unsigned int time_out) {
#ifdef __vxworks
		if ( mutex )
			mutex->unlock();
		// time_out is in microseconds, so...
		int ticks = static_cast<int>(static_cast<double>(time_out) * sysClkRateGet() / 1000000);
		int status = semTake(ev, time_out ? ticks < 1 ? 1 : ticks : WAIT_FOREVER);
		if ( mutex )
			mutex->lock();
		return status != ERROR;
#elif defined(WIN32)
		if ( mutex )
			mutex->unlock();
		DWORD wait_ret = WaitForSingleObject(ev, (time_out + 999) / 1000);
		if ( mutex )
			mutex->lock();
		return wait_ret == WAIT_OBJECT_0;
#elif defined(__linux__)
		if ( time_out > 0 ) {
			timespec ts;
			::clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += time_out / 1000000;
			unsigned long nsec = ts.tv_nsec + (time_out % 1000000) * 1000;
			ts.tv_nsec = nsec % 1000000000;
			ts.tv_sec += nsec / 1000000000;
			if ( pthread_cond_timedwait(&ev, mutex->get_mutex(), &ts) )
				return errno != ETIMEDOUT;
			return true;
		} else {
			pthread_cond_wait(&ev, mutex->get_mutex());
			return true;
		}

#else /* __vxworks */
		if ( time_out > 0 ) {
			timespec ts = {0, time_out * 1000};
			if ( pthread_cond_timedwait(&ev, mutex->get_mutex(), &ts) )
				return errno != ETIMEDOUT;
			return true;
		} else {
			pthread_cond_wait(&ev, mutex->get_mutex());
			return true;
		}
#endif /* __vxworks */
	}

	/**
	 * \fn signal
	 * 设置条件为真，并释放正在等待条件的任务。
	 */
#ifdef __vxworks
	void signal() {
		semGive(ev);
	}
#elif defined(WIN32)
	void signal() {
		SetEvent(ev);
	}
#else /* __vxworks */
	void signal() {
		pthread_cond_signal(&ev);
	}

	/// 设置条件为真，并释放所有正在等待条件的任务。
	void broadcast() {
		pthread_cond_broadcast(&ev);
	}
#endif /* __vxworks */
};

/**
 * \class SyncEvent
 * 同步事件
 *
 * \fn SyncEvent::wait(unsigned int)
 * 等待同步.
 * 等待同步事件完成。
 * \param time_out 等待的最长时间，单位：毫秒。0表示永远等待。
 * \return 是否同步。如果超时指定时间，则不同步返回。
 *
 * \fn SyncEvent::signal
 * 同步.
 * 同步任务完成，通知要求同步的其它任务继续。
 */
#ifdef __vxworks
class SyncEvent {
	SEM_ID ev;
public:
	SyncEvent() : ev(semBCreate(SEM_Q_PRIORITY, SEM_EMPTY)) {}
	bool wait(unsigned int time_out/*microsecond*/ = 0) {
		if ( time_out == 0 )
			return semTake(ev, WAIT_FOREVER) != ERROR;
		int ticks = static_cast<int>(static_cast<double>(time_out) * sysClkRateGet() / 1000000);
		return semTake(ev, ticks < 1 ? 1 : ticks) != ERROR;
	}

	void signal() {
		semGive(ev);
	}

	~SyncEvent() {
		semDelete (ev);
	}
};
#elif defined(WIN32)
class SyncEvent {
	HANDLE ev;
public:
	SyncEvent() {
		ev = CreateEvent(NULL, false, false, NULL);
	}

	bool wait(unsigned int time_out/*microsecond*/ = 0) {
		return WaitForSingleObject(ev, time_out == 0 ? INFINITE : (time_out + 999) / 1000) == WAIT_OBJECT_0;
	}

	void signal() {
		SetEvent(ev);
	}

	~SyncEvent() {
		CloseHandle(ev);
	}
};
#else
class SyncEvent : protected CondEvent {
	MXLock mutex;
public:
	SyncEvent() : CondEvent(false) {}

	bool wait(unsigned int time_out/*microsecond*/ = 0) {
		AutoLock al(mutex);
		return CondEvent::wait(&mutex, time_out);
	}

	void signal() {
		return CondEvent::signal();
	}
};
#endif

#endif // __THREAD_SYNC_UTIL_H_
