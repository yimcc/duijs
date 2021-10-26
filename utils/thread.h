#pragma once
#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>
#include <string>
#include <array>
#include "waitable_event.h"


using Task = std::function<void()>;

class Thread {
public:
	Thread(const char* name);
	~Thread();

	void PostTask(Task task);
	void Start();
	void Stop();
	void Join();
private:
	Task PopTask();

	AutoResetWaitableEvent event_;

	std::string name_;
	std::atomic<bool> quit_;
	std::queue<Task> tasks_;
	std::mutex task_lock_;

	std::unique_ptr<std::thread> thread_;
};

//�̹߳�����
class ThreadManager {
public:
	enum TID{
		kIO,		//�ļ�io
		kStorage,	//�洢�߳�
		kImage,     //ͼƬ���ؽ����߳�
	};

	static ThreadManager* Instance();
	void PostTask(TID tid,Task task);

protected:
	ThreadManager();
	~ThreadManager();

	static ThreadManager* s_instance_;
	std::array<Thread*, 3> threads_;
};
