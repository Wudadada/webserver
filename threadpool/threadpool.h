#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "..\lock\locker.h"
#include "..\CGImysql\sql_connection_pool.h"

template <typename T>
class threadpool
{
public:
	//thread_number线程池中线程数量，max_requests请求队列最多允许的、等待的请求的数量 
	threadpool(int actor_model, connection_pool* connPool, int thread_number = 8, int max_request = 10000);
	~threadpool();
	bool append(T *request, int state); //像请求队列中插入任务请求
	bool append_p(T *request);

private:
	//工作线程运行的函数，它不断从工作队列中取出任务并执行
	static void* worker(void* arg);//每个实例化线程都会改变某些数据，如果没有静态，每个实例改变数据后都要再去更新数据.函数原型中的第三个参数，为函数指针，指向处理线程函数的地址。该函数，要求为静态函数。如果处理线程函数为类成员函数时，需要将其设置为静态成员函数。
	void run();

private:
	int m_thread_number;			//线程池中的线程数
	int m_max_requests;				//请求队列中允许的最大请求数
	pthread_t* m_threads;			//描述线程池的数组,大小为m_thread_number
	std::list<T*> m_workqueue;		//请求队列
	locker m_queuelocker;			//保护请求队列的互斥锁
	sem m_quequestat;				//是否有任务需要处理
	connection_pool* m_connPool;	//数据库
	int m_actor_model;				//模型切换
};
template <typename T>
threadpool<T>::threadpool(int actor_model, connection_pool* connPool, int thread_number, int max_requests)
	:m_actor_model(actor_model),
	m_htread_number(thread_number),
	m_max_requests(max_requests),
	m_threads(NULL),
	m_connPool(connPool) 
{
	if (thread_number <= 0 || max_requests <= 0)
		throw std::exception();
	m_threads = new phread_t[m_thread_number];
	if (!m_threads)
		throw std::exception();
	for (int i = 0; i < thread_number; ++i)
	{
		if (pthread_create(m_threads + i, NULL, worker, this) != 0)//为什么this是void*
		{
			delete[] m_threads;
			throw std::exception();
		}
		if (pthread_detach(m_threads[i]))
		{
			deltte[] m_threads;
			throw std::exception;
		}
	}
}

template <typename T>
threadpool<T>::~threadpool()
{
	delele[] m_threads;
}

template <typename T>
bool threadpool<T>::append(T* request, int state)
{
	m_queuelocker.lock();
	if (m_workqueue.size() >= m_max_requests)
	{
		m_queuelocker.unlock();
		return false;
	}
	request->m_state = state;
	m_workqueue.push_back(request);
	m_queuelocker.unlock();
	m_queuestat.post();
	return true;
}

template<typename T>
bool threadpool<T>::append_p(T* request)
{
	m_queuelocker.lock();
	if (m_workqueue.size() >= m_max_requests)
	{
		m_queuelocker.unlock();
		return false;
	}
	m_workqueue.push_back(request);
	m_queuelocker.unlock();
	m_queuestat.post();
	return true;
}

//线程处理函数:内部访问私有成员函数run，完成线程处理要求。
template<typename T>
void* threadpool<T>::worker(void* arg)
{
	//将参数强转为线程池类，调用成员方法
	threadpool* pool = (threadpool*)arg;
	pool->run();
	return pool;
}

//主要实现，工作线程从请求队列中取出某个任务进行处理，注意线程同步。
template <typename T>
void threadpool<T>::run()
{
	while (!m_stop)
	{
		//信号量等待
		m_queuestat.wait();

		//被唤醒后先加互斥锁
		m_queuelocker.lock();
		if (m_workqueue.empty())
		{
			m_queuelocker.unlock();
			continue;
		}

		//从请求队列中取出第一个任务
		//将任务从请求队列删除
		T* request = m_workqueue.front();
		m_workqueue.pop_front();
		m_queuelocker.unlock();
		if (!request)
			continue;

		if (1 == m_actor_model)//整段看不太懂 
		{
			if (0 == request->m_state)
			{
				if (request->read_once())
				{
					request->improv = 1;
					connectionRAII mysqlcon(&request->mysql, m_connPool);
					request->process();
				}
				else
				{
					request->improv = 1;
					request->timer_flag = 1;
				}
			}
			else
			{
				if (request->write())
				{
					request->improv = 1;//improv是什么
				}
				else
				{
					request->improv = 1;
					request->timer_flag = 1;
				}
			}
		}
		else
		{
			connectionRAII mysqlcon(&request->mysql, m_connPool);
			request->process();
		}
	}
}

#endif 
